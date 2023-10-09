#include "tcp_server.h"
#include "config.h"


namespace sylar {


static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// TCP服务器接收超时时间
static ConfigVar<uint64_t>::ptr g_tcp_server_recv_timeout = Config::Lookup(
        "tcp_server.recv_timeout", (uint64_t)(2 * 60 * 1000), "tcp server recv timeout");

// 匿名空间：将命名空间的名字符号去掉，让其他文件找不到，效果类似于static
namespace {
// 添加监听函数，配置改变时执行回调函数
struct TcpServerRecvTimeoutInit {
    TcpServerRecvTimeoutInit() {
        g_tcp_server_recv_timeout->addListener([](const uint64_t& oldVal, const uint64_t& newVal) {
            g_tcp_server_recv_timeout->setValue(newVal);
            SYLAR_LOG_INFO(g_logger) << "tcp server recv timeout" << " old_value=" << oldVal
                                                                  << " new_value=" << newVal;
        });
    }
};
static TcpServerRecvTimeoutInit _Init;
}

// 构造函数
TcpServer::TcpServer(IOManager* worker, IOManager* acceptWorker) 
    :m_worker(worker)
    ,m_acceptWorker(acceptWorker)
    ,m_name("wwt/1.0.0")
    ,m_recvTimeout(g_tcp_server_recv_timeout->getValue())
    ,m_isStop(true) {
}

// 析构函数
TcpServer::~TcpServer() {
    for(auto& sock : m_socket) {
        sock->close();
    }
    m_socket.clear();
}

// 服务器绑定地址(单个地址)
bool TcpServer::bind(const Address::ptr addr) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

// 服务器绑定地址(多个地址)
// 完成所有socket的bind和listen的操作
bool TcpServer::bind(const std::vector<Address::ptr> addrs, std::vector<Address::ptr>& fails) {
    for(const auto& addr : addrs) {
        Socket::ptr sock = Socket::CreatTcpSocket(addr);
        if(!sock->bind(addr)) {
            SYLAR_LOG_ERROR(g_logger) << "tcp server bind address error";
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()) {
            SYLAR_LOG_ERROR(g_logger) << "tcp server listen error";
            fails.push_back(addr);
            continue;
        }
        m_socket.push_back(sock);
    }

    if(!fails.empty()) {
        m_socket.clear();
        return false;
    }

    for(const auto& sock : m_socket) {
        SYLAR_LOG_INFO(g_logger) << "tcp server bind success, "  << sock->toString();
    }
    return true;
}

// 完成服务器单个Socket的accept的操作
void TcpServer::startAccept(Socket::ptr sock) {
    // sock->setRecvTimeout(3*1000);
    // 用while的原因，可以让服务器一直等待客户端connect，不关闭
    while(!m_isStop) {
        // 这里sock的accept()并没有设置超时时间，所以hook中accept()没有设置定时器
        // 那么会一直阻塞到有客户端connect时，才会继续执行
        // 如果给其设置超时时间sock->setRecvTimeout(3*1000); 
        // 那么则不会阻塞，即使没有客户端connect也会继续执行，直到有connect或者定时器超时再回调
        Socket::ptr client = sock->accept();

        if(client) {
            // 设置新Socket的接收超时时间
            client->setRecvTimeout(m_recvTimeout);
            // accept()成功，触发回调，处理新连接的Socket类
            // m_worker负责调度 新连接的Socket需要做的工作
            m_worker->scheduler(std::bind(&TcpServer::handleClient, shared_from_this(), client));
        }
        else {
            SYLAR_LOG_ERROR(g_logger) << "tcp server accept error";
        }
    }
    
}

// 完成服务器所有Socket的accept的操作
bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(const auto& sock : m_socket) {
        // m_acceptWorker负责调度 服务器accept新的socket
        // 用shared_from_this()的原因是，防止在startAccept()函数执行时，TcpServer析构了
        m_acceptWorker->scheduler(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }
    return true;
}

// 停止服务器
void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    // 用shared_from_this()的原因是，防止在lambda表达式执行时，TcpServer析构了
    m_acceptWorker->scheduler([self]() {
        for(auto& sock : self->m_socket) {
            // 因为sock当前可能在accept()，需要把事件取消，不然不会唤醒
            sock->cancelAll();
            sock->close();
        }
        self->m_socket.clear();
    });
}

void TcpServer::setConf(const TcpServerConf& conf) {
    m_conf.reset(new TcpServerConf(conf));
}

// 每accept到一个socket，就会触发回调执行一次
void TcpServer::handleClient(Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient: " << client->toString();
}

}