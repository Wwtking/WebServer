#include "http_server.h"
#include "http_session.h"

namespace sylar {
namespace http {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 构造函数
HttpServer::HttpServer(bool isKeepAlive, IOManager* worker, IOManager* acceptWorker) 
    :TcpServer(worker, acceptWorker)
    ,m_isKeepAlive(isKeepAlive) {
    m_dispatch = std::make_shared<ServletDispatch>();
}

// 每accept到一个socket，就会触发回调执行一次
void HttpServer::handleClient(sylar::Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient: " << client->toString();
    HttpSession::ptr session = std::make_shared<HttpSession>(client);  //流式Socket类

    while(true) {
        // 接收HTTP请求并解析
        HttpRequest::ptr req = session->recvHttpRequest();
        if(!req) {
            SYLAR_LOG_WARN(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << client->toString() << " keep_alive=" << m_isKeepAlive;
            return;
        }

        // 发送HTTP响应
        // 响应是close还是keepalive，取决于请求的isClose和m_isKeepalive
        HttpResponse::ptr res = std::make_shared<HttpResponse>(req->getVersion(), 
                                                req->isClose() || !m_isKeepAlive);
        // res->setBody("hello client");
        res->setHeader("Server", getName());
        m_dispatch->handle(req, res, session);
        session->sendHttpResponse(res);

        // SYLAR_LOG_INFO(g_logger) << "HttpRequest: \n" << req->toString();
        // SYLAR_LOG_INFO(g_logger) << "HttpResponse: \n" << res->toString();

        // 如果不是长连接状态，则收发一次HTTP后断开连接
        if(!m_isKeepAlive || req->isClose() || res->isClose()) {
            SYLAR_LOG_WARN(g_logger) << "m_isKeepAlive=" << m_isKeepAlive 
                                    << " req->isClose()=" << (req->isClose() ? "true" : "false")
                                    << " res->isClose()=" << (res->isClose() ? "true" : "false");
            break;
        }
    }
    session->close();
}

}
}