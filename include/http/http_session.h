#ifndef __SYLAR_HTTP_SESSION_H__
#define __SYLAR_HTTP_SESSION_H__

#include <memory>
#include "http.h"
#include "socket_stream.h"

namespace sylar {
namespace http {

// HttpSession/HttpConnection
// server.accept  socket -> Session
// client.connect socket -> Connection

/**
 * @brief HTTPSession封装
 * @details session是服务器端accept成功产生的socket
 */
class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;

    /**
     * @brief 构造函数
     * @param[in] sock Socket类型
     * @param[in] owner 是否完全控制
     *                  如果是true，那么全权管理，析构时帮忙释放Socket
     *                  如果是false，那么仅仅完成相应的操作，不帮忙析构
     */
    HttpSession(Socket::ptr sock, bool owner = false);

    /**
     * @brief 接收HTTP请求
     * @details 接收到HTTP数据并且进行解析
     * @return 返回解析后的HttpRequest封装数据
    */
    HttpRequest::ptr recvHttpRequest();

    /**
     * @brief 发生HTTP响应
     * @param[in] rsp HTTP响应
     * @return >0 发送成功
     *         =0 对方关闭
     *         <0 Socket异常
     */
    ssize_t sendHttpResponse(HttpResponse::ptr response);
    
};

}
}

#endif