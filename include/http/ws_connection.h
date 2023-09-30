#ifndef __SYLAR_WS_CONNECTION_H__
#define __SYLAR_WS_CONNECTION_H__

#include "http_connection.h"
#include "ws_session.h"

namespace sylar {
namespace http {

/**
 * @brief WSConnection封装
 * @details 在握手和挥手阶段，都是用Http协议来请求和响应的
 *          在双向通信时，是使用WebSocket协议通信的
*/
class WSConnection : public HttpConnection {
public:
    typedef std::shared_ptr<WSConnection> ptr;

    /**
     * @brief 握手建连阶段，客户端发起连接，创建WSConnection
     * @param[in] uri 请求的字符串uri
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @return 返回Http响应结果和WSConnection封装
    */
    static std::pair<HttpResult::ptr, WSConnection::ptr> StartShake(
                                            const std::string& uri,
                                            uint64_t timeout_ms,
                                            const HttpRequest::MapType& headers = {});

    /**
     * @brief 握手建连阶段，客户端发起连接，创建WSConnection
     * @param[in] uri Uri结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @return 返回Http响应结果和WSConnection封装
    */
    static std::pair<HttpResult::ptr, WSConnection::ptr> StartShake(
                                            Uri::ptr uri,
                                            uint64_t timeout_ms,
                                            const HttpRequest::MapType& headers = {});

    /**
     * @brief 构造函数
    */
    WSConnection(Socket::ptr sock, bool owner = true);

    /**
     * @brief 握手建连阶段，接收客户端的连接，发送响应
     * @return 返回Http请求
    */
    HttpRequest::ptr handleShake();

    /**
     * @brief WebSocket接收消息
     * @return 返回接收到的WebSocket消息体
    */
    WSFrameMessage::ptr recvMessage();

    /**
     * @brief WebSocket发送消息
     * @param[in] msg 要发送的WebSocket消息体
     * @param[in] fin 是否为消息的最后一个片段
     * @return 返回发送字节数
    */
    int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);

    /**
     * @brief WebSocket发送消息
     * @param[in] data 要发送的WebSocket消息体内容
     * @param[in] opcode 要发送的WebSocket消息体类型
     * @param[in] fin 是否为消息的最后一个片段
     * @return 返回发送字节数
    */
    int32_t sendMessage(const std::string& data,  uint32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);

    /**
     * @brief 发送Ping包
     * @return 返回发送字节数
    */
    int32_t ping();

    /**
     * @brief 发送Pong包
     * @return 返回发送字节数
    */
    int32_t pong();

};

}
}

#endif