#ifndef __SYLAR_APPLICATION_H__
#define __SYLAR_APPLICATION_H__

#include "iomanager.h"
#include "tcp_server.h"
#include <map>
#include <vector>

namespace sylar {

class Application {
public:
    Application();

    bool init(int argc, char** argv);
    int run();

    bool getServer(const std::string& type, std::vector<TcpServer::ptr>& servers);
    void listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers);

private:
    int main(int argc, char** argv);
    int run_fiber(int argc, char** argv);

private:
    int m_argc;
    char** m_argv;

    IOManager::ptr m_mainIOManager;
    std::map<std::string, std::vector<TcpServer::ptr> > m_servers;  // 存放不同类型的server

    static Application* s_instance;
};

std::string GetServerWorkPath();

}

#endif