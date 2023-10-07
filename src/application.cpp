#include "application.h"
#include "log.h"
#include "config.h"
#include "env.h"
#include "daemon.h"
#include "http_server.h"
#include "ws_server.h"
#include "module.h"
#include "worker.h"

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static ConfigVar<std::string>::ptr g_server_work_path = 
                    Config::Lookup("server.work_path"
                    , std::string("/home/wwt/server_work")
                    , "server work path");

static ConfigVar<std::string>::ptr g_server_pid_file = 
                    Config::Lookup("server.pid_file"
                    , std::string("wwt.pid")
                    , "server pid file");

static ConfigVar<std::vector<TcpServerConf> >::ptr g_server_conf = 
                    Config::Lookup("servers"
                    , std::vector<TcpServerConf>()
                    , "tcp server config");

// 公用函数，其它文件也可调用
std::string GetServerWorkPath() {
    return g_server_work_path->getValue();
}

// 初始化静态成员变量
Application* Application::s_instance = nullptr;

// 将静态成员变量指向最新实例化对象
Application::Application() {
    s_instance = this;
}

bool Application::init(int argc, char** argv) {
    m_argc = argc;
    m_argv = argv;

    // 添加启动参数说明
    EnvMgr::GetInstance()->addHelp("p", "print help infomation");
    EnvMgr::GetInstance()->addHelp("t", "run as terminal");
    EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    EnvMgr::GetInstance()->addHelp("c", "config path, default: ../conf");

    // 添加启动参数，并确定工作路径
    if(!EnvMgr::GetInstance()->init(argc, argv)) {
        EnvMgr::GetInstance()->printHelps();
        return false;
    }

    if(EnvMgr::GetInstance()->hasArg("p")) {
        EnvMgr::GetInstance()->printHelps();
        return false;
    }

    // 加载配置文件
    std::string conf_path = EnvMgr::GetInstance()->getConfPath();
    Config::LoadFromConfDir(conf_path);
    
    // 创建出多态的Module，并保存下来
    ModuleMgr::GetInstance()->init();
    std::vector<Module::ptr> modules;
    ModuleMgr::GetInstance()->listAllModules(modules);
    // 解析启动参数前执行函数
    for(auto& i : modules) {
        i->onBeforeArgsParse(argc, argv);
    }
    
    int run_status = 0;
    if(EnvMgr::GetInstance()->hasArg("d")) {
        run_status = 1;
    }
    else if(EnvMgr::GetInstance()->hasArg("t")) {
        run_status = 2;
    }
    if(run_status == 0) {
        EnvMgr::GetInstance()->printHelps();
        return false;
    }

    // 解析启动参数后执行函数
    for(auto& i : modules) {
        i->onAfterArgsParse(argc, argv);
    }
    modules.clear();
    
    // 验证workpath目录下pidfile文件是否存在，并且文件中是否存在进程ID，如果是则说明Server已经启动
    std::string pidfile = g_server_work_path->getValue() + "/" 
                            + g_server_pid_file->getValue();
    if(FilesUtil::IsRunningPidfile(pidfile)) {
        SYLAR_LOG_ERROR(g_logger) << "server is running, " << pidfile;
        return false;
    }

    // 创建工作路径
    if(!FilesUtil::Mkdir(g_server_work_path->getValue())) {
        SYLAR_LOG_ERROR(g_logger) << "mkdir work path [" << g_server_work_path->getValue()
                            << "] fail, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    
    return true;
}

// 选择是否用守护进程的方式来启动程序
int Application::run() {
    bool is_daemon = EnvMgr::GetInstance()->hasArg("d");
    return start_daemon(m_argc, m_argv, std::bind(&Application::main, this
            , std::placeholders::_1, std::placeholders::_2), is_daemon);
}

// 这个main函数才是本服务器框架真正执行的函数
int Application::main(int argc, char** argv) {
    SYLAR_LOG_DEBUG(g_logger) << "main";

    // 加载配置文件
    std::string conf_path = EnvMgr::GetInstance()->getConfPath();
    Config::LoadFromConfDir(conf_path, true);

    // 把真正执行main函数的进程ID写进去 
    std::string pidfile = g_server_work_path->getValue() + "/" 
                            + g_server_pid_file->getValue();
    std::ofstream ofs(pidfile);
    if(!ofs) {
        SYLAR_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
        return -1;
    }
    ofs << getpid();
    ofs.close();

    // 线程跑到这里，开始用协程
    // 前面的初始化可以不用在协程里面做，Server的初始化需要用到协程
    m_mainIOManager = std::make_shared<IOManager>(1, true, "main");
    m_mainIOManager->scheduler(std::bind(&Application::run_fiber, this, argc, argv));
    m_mainIOManager->addTimer(2000, [](){
        // SYLAR_LOG_INFO(g_logger) << "hello";
    }, true);
    m_mainIOManager->stop();
    return 0;
}

int Application::run_fiber(int argc, char** argv) {
    std::vector<Module::ptr> modules;
    ModuleMgr::GetInstance()->listAllModules(modules);
    bool has_error = false;
    for(auto& i : modules) {
        if(!i->onLoad()) {
            SYLAR_LOG_ERROR(g_logger) << "module name=" << i->getName() 
                                    << " version=" << i->getVersion()
                                    << " filename=" << i->getFilename();
            has_error = true;
        }
    }
    if(has_error) {
        _exit(0);
    }

    // 初始化worker的YAML配置
    WorkerMgr::GetInstance()->init();

    std::vector<TcpServer::ptr> servers;
    auto confs = g_server_conf->getValue();
    for(auto& conf : confs) {
        // 拿出YAML配置中的地址端口号，绑定服务器地址
        std::vector<Address::ptr> address;
        for(auto& addr : conf.address) {
            // 分离地址和端口号
            size_t pos = addr.find(":");
            // std::string::npos是一个常数，它等于size_t类型可以表示的最大值
            if(pos == std::string::npos) {
                address.push_back(std::make_shared<UnixAddress>(addr));
                continue;
            }
            auto port = atoi(addr.substr(pos + 1).c_str());

            // 通过域名,IP,服务器名解析的地址
            auto addr_lookup = Address::LookupAny(addr);
            if(addr_lookup) {
                address.push_back(addr_lookup);
                continue;
            }

            // 通过网卡设备来解析ip地址
            std::vector<std::pair<Address::ptr, uint32_t> > results;
            if(Address::GetInterfaceAddress(results, addr.substr(0, pos))) {
                for(auto& i : results) {
                    IPAddress::ptr ipaddr = std::dynamic_pointer_cast<IPAddress>(i.first);
                    if(ipaddr) {
                        ipaddr->setPort(port);
                        address.push_back(ipaddr);
                    }
                }
                continue;
            }

            SYLAR_LOG_ERROR(g_logger) << "invalid address: " << addr;
            _exit(0);
        }

        // 测试
        // for(auto& i : address) {
        //     SYLAR_LOG_DEBUG(g_logger) << i->toString();
        // }

        IOManager* accept_worker = sylar::IOManager::GetThis();
        IOManager* process_worker = sylar::IOManager::GetThis();
        // 根据 conf.accept_worker 和 conf.process_worker 来选择不同线程池
        if(!conf.accept_worker.empty()) {
            accept_worker = WorkerMgr::GetInstance()->getAsIOManager(conf.accept_worker).get();
            if(!accept_worker) {
                SYLAR_LOG_ERROR(g_logger) << "accept_worker: " << conf.accept_worker
                                        << " not exists";
                _exit(0);
            }
        }
        if(!conf.process_worker.empty()) {
            process_worker = WorkerMgr::GetInstance()->getAsIOManager(conf.process_worker).get();
            if(!process_worker) {
                SYLAR_LOG_ERROR(g_logger) << "process_worker: " << conf.process_worker
                                        << " not exists";
                _exit(0);
            }
        }

        // 根据不同类型创建不同服务器
        TcpServer::ptr server = nullptr;
        if(conf.type == "http") {
            server = std::make_shared<http::HttpServer>(!!conf.keepalive
                                        , process_worker, accept_worker);        
        }
        else if(conf.type == "http2") {

        }
        else if(conf.type == "ws") {
            server = std::make_shared<http::WSServer>(process_worker, accept_worker);
        }
        else {
            SYLAR_LOG_ERROR(g_logger) << "invalid server type=" << conf.type;
            _exit(0);
        }
        // if(!conf.name.empty()) {
        //     server->setName(conf.name);
        // }
        
        std::vector<Address::ptr> fails;
        if(!server->bind(address, fails)) {
            for(auto i : fails) {
                SYLAR_LOG_ERROR(g_logger) << "invalid addr=" << i->toString();
            }
            _exit(0);   // 绑定失败直接退出
        }
        server->setConf(conf);
        // server->start();
        servers.push_back(server);
        m_servers[conf.type].push_back(server);
    }

    for(auto& i : modules) {
        i->onServerReady();
    }

    for(auto& i : servers) {
        i->start();
    }

    for(auto& i : modules) {
        i->onServerUp();
    }
    return 0;
}

bool Application::getServer(const std::string& type, std::vector<TcpServer::ptr>& servers) {
    auto it = m_servers.find(type);
    if(it == m_servers.end()) {
        return false;
    }
    servers = it->second;
    return true;
}

void Application::listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers) {
    servers = m_servers;
}

}