#include "ws_server.h"

namespace sylar {
namespace http {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 构造函数
WSServer::WSServer(IOManager* worker, IOManager* acceptWorker) 
    :TcpServer(worker, acceptWorker) {
    m_dispatch = std::make_shared<WSServletDispatch>();
}

// 每accept到一个socket，就会触发回调执行一次
void WSServer::handleClient(sylar::Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient: " << client->toString();
    WSSession::ptr session = std::make_shared<WSSession>(client);  //流式Socket类

    do {
        // 处理来自客户端的握手建连
        HttpRequest::ptr header = session->handleShake();
        if(!header) {
            SYLAR_LOG_INFO(g_logger) << "handleShake error";
            break;
        }

        // 获取客户端的请求路径对应的servlet
        WSServlet::ptr servlet = m_dispatch->getMatchWSServlet(header->getPath());
        if(!servlet) {
            SYLAR_LOG_INFO(g_logger) << "no match WSServlet";
            break;
        }

        // 执行连接成功的回调
        int rt = servlet->onConnect(header, session);
        if(rt) {
            SYLAR_LOG_INFO(g_logger) << "onConnect return " << rt;
            break;
        }

        while(true) {
            // 接收来自客户端的数据并处理
            WSFrameMessage::ptr msg = session->recvMessage();
            if(!msg) {
                break;
            }
            rt = servlet->handle(header, msg, session);
            // 返回非0时说明出错，直接关闭
            if(rt) {
                SYLAR_LOG_INFO(g_logger) << "handle return " << rt;
                break;
            }
        }

        // 执行关闭的回调
        servlet->onClose(header, session);
    } while(false);
    session->close();
}

}
}