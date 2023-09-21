#include "env.h"
#include <fstream>

// 1.读写环境变量
// 2.获取程序的绝对路径，基于绝对路径设置cwd
// 3.可以通过cmdline，在进入main函数之前，解析好参数


// 可以实现在main函数之前得到输入命令行，继而把执行的相关参数解析出来
struct CmdlineInit {
    CmdlineInit() {
        std::ifstream ifs("/proc/" + std::to_string(getpid()) + "/cmdline", std::ios::binary);
        std::string content;
        content.resize(4096);
        ifs.read(&content[0], content.size());
        content.resize(ifs.gcount());

        std::string real_content;
        std::cout << content << std::endl;
        for(int i = 0; i < (int)content.size(); i++) {
            if((int)content[i] == 0) {
                real_content += " ";
                continue;
            }
            real_content += content[i];
            // std::cout << content[i] << " - " << (int)content[i] << std::endl;
        }
        std::cout << real_content << std::endl;
    }
};

CmdlineInit _cmdlineInit;

int main(int argc, char** argv) {
    sylar::EnvMgr::GetInstance()->addHelp("t", "run as terminal");
    sylar::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    sylar::EnvMgr::GetInstance()->addHelp("p", "print help");
    sylar::EnvMgr::GetInstance()->addArg("p", "");
    if(sylar::EnvMgr::GetInstance()->init(argc, argv)) {
        sylar::EnvMgr::GetInstance()->printArgs();
    }
    if(sylar::EnvMgr::GetInstance()->hasArg("p")) {
        sylar::EnvMgr::GetInstance()->printHelps();
    }

    std::cout << "m_cwd=" << sylar::EnvMgr::GetInstance()->getCwd() << std::endl;
    std::cout << "m_exe=" << sylar::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "m_workPath=" << sylar::EnvMgr::GetInstance()->getWorkPath() << std::endl;

    std::cout << "PATH=" << sylar::EnvMgr::GetInstance()->getEnv("PATH") << std::endl;
    std::cout << "set env TEST " << sylar::EnvMgr::GetInstance()->setEnv("TEST", "XXX") << std::endl;
    std::cout << "TEST=" << sylar::EnvMgr::GetInstance()->getEnv("TEST") << std::endl;

    return 0;
}