#ifndef __SYLAR_HTTP_SERVER_H__
#define __SYLAR_HTTP_SERVER_H__

#include "tcp_server.h"
#include "http_servlet.h"

namespace sylar {
namespace http {

/**
 * @brief HTTP服务器
 * @details 将收到的客户端HTTP请求解析，并发送HTTP响应
*/
class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief 构造函数
     * @param[in] isKeepAlive 是否长连接
     * @param[in] worker 调度新连接的Socket工作的协程调度器
     * @param[in] acceptWorker 调度服务器Socket执行accept的协程调度器
     */
    HttpServer(bool isKeepAlive = false, 
                IOManager* worker = IOManager::GetThis(), 
                IOManager* acceptWorker = IOManager::GetThis());

    ServletDispatch::ptr getDispatch() { return m_dispatch; }

    void setDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

private:
    /**
     * @brief 处理新连接的Socket类
     * @details 每accept到一个socket，就会执行一次，触发回调
     * @param[in] client 新连接的Socket类
     */
    void handleClient(sylar::Socket::ptr client) override;

private:
    bool m_isKeepAlive;                 // 是否长连接
    ServletDispatch::ptr m_dispatch;    // Servlet分配器
};

}
}


#endif