#ifndef __SYLAR_WS_SERVER_H__
#define __SYLAR_WS_SERVER_H__

#include "tcp_server.h"
#include "ws_servlet.h"

namespace sylar {
namespace http {


class WSServer : public TcpServer {
public:
    typedef std::shared_ptr<WSServer> ptr;

    /**
     * @brief 构造函数
     */
    WSServer(IOManager* worker = IOManager::GetThis(), 
            IOManager* acceptWorker = IOManager::GetThis());
    
    /**
     * @brief 获取WSServlet分配器
     */
    WSServletDispatch::ptr getDispatch() { return m_dispatch; }

    /**
     * @brief 设置WSServlet分配器
     */
    void setDispatch(WSServletDispatch::ptr v) { m_dispatch = v; }

private:
    /**
     * @brief 处理新连接的Socket类
     * @details 每accept到一个socket，就会执行一次，触发回调
     * @param[in] client 新连接的Socket类
     */
    void handleClient(sylar::Socket::ptr client) override;

private:
    WSServletDispatch::ptr m_dispatch;    // WSServlet分配器
    
};

}
}

#endif
