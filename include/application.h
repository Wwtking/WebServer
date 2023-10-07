#ifndef __SYLAR_APPLICATION_H__
#define __SYLAR_APPLICATION_H__

#include "iomanager.h"
#include "tcp_server.h"
#include <map>
#include <vector>

namespace sylar {

/**
 * @brief 应用封装
*/
class Application {
public:
    /**
     * @brief 构造函数
    */
    Application();

    static Application* Getinstance() { return s_instance; }

    /**
     * @brief 初始化函数
    */
    bool init(int argc, char** argv);

    /**
     * @brief 启动程序，可选择是否以守护进程方式启动
    */
    int run();

    /**
     * @brief 寻找指定类型的服务器
     * @param[in] type 服务器类型
     * @param[out] servers 存放服务器
    */
    bool getServer(const std::string& type, std::vector<TcpServer::ptr>& servers);

    /**
     * @brief 列出所有的服务器
     * @param[out] servers 存放服务器
    */
    void listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers);

private:
    /**
     * @brief 本服务器框架真正执行的函数
    */
    int main(int argc, char** argv);

    /**
     * @brief 协程运行函数
    */
    int run_fiber(int argc, char** argv);

private:
    /// 启动参数个数
    int m_argc;
    /// 启动参数 
    char** m_argv;
    /// 主调度器
    IOManager::ptr m_mainIOManager;
    /// 存放不同类型的server
    std::map<std::string, std::vector<TcpServer::ptr> > m_servers;

    static Application* s_instance;
};

std::string GetServerWorkPath();

}

#endif