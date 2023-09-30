#include "ws_connection.h"
#include "hash_util.h"
#include "log.h"

namespace sylar {
namespace http {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// websocket 请求头格式
// GET /chat HTTP/1.1
// Host: server.example.com
// Upgrade: websocket
// Connection: Upgrade
// Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
// Origin: http://example.com
// Sec-WebSocket-Protocol: chat, superchat
// Sec-WebSocket-Version: 13

// websocket 响应头格式
// HTTP/1.1 101 Switching Protocols
// Upgrade: websocket
// Connection: Upgrade
// Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
// Sec-WebSocket-Protocol: chat

std::pair<HttpResult::ptr, WSConnection::ptr> WSConnection::StartShake(
                                        const std::string& uri,
                                        uint64_t timeout_ms,
                                        const HttpRequest::MapType& headers) {
    Uri::ptr uri_ptr = Uri::CreateUri(uri);
    if(!uri_ptr) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Status::INVALID_URI
                                            , nullptr, "invalid uri: " + uri), nullptr);
    }
    return StartShake(uri_ptr, timeout_ms, headers);
}

std::pair<HttpResult::ptr, WSConnection::ptr> WSConnection::StartShake(
                                        Uri::ptr uri,
                                        uint64_t timeout_ms,
                                        const HttpRequest::MapType& headers) {
    // 获取地址
    Address::ptr addr = uri->getAddress();
    if(!addr) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Status::INVALID_HOST
                                    , nullptr, "invalid host: " + uri->getHost()), nullptr);
    }

    // Socket绑定地址
    Socket::ptr sock = sylar::Socket::CreatTcpSocket(addr);
    if(!sock) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Status::CREATE_SOCKET_ERROR
                                    , nullptr, "create socket error: " + addr->toString()), nullptr);
    }

    // 先创建socket，再连接
    if(!sock->connect(addr, timeout_ms)) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Status::CONNECT_ERROR
                                    , nullptr, "socket connect error: " + addr->toString()), nullptr);
    }
    sock->setRecvTimeout(timeout_ms);   // 设置超时时间

    // 封装HttpConnection，再去发送Http请求
    WSConnection::ptr con = std::make_shared<WSConnection>(sock);

    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setMethod(HttpMethod::GET);
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    bool hasHost = false;
    bool hasConn = false;
    for(const auto& header : headers) {
        if(strcasecmp(header.first.c_str(), "Connection") == 0) {
            hasConn = true;
        }
        else if(strcasecmp(header.first.c_str(), "Host") == 0) {
            hasHost = !header.second.empty();
            if(!hasHost) {    // 说明只有key,没有value,不放入Header
                continue;
            }
        }
        // 函数参数的输入头部
        req->setHeader(header.first, header.second);
    }
    if(!hasHost) {
        req->setHeader("Host", uri->getHost());
    }
    if(!hasConn) {
        // 指示客户端希望与服务器之间的连接被升级为支持双向通信的协议
        req->setHeader("Connection", "Upgrade");
    }
    // 指示客户端希望将当前的HTTP协议升级为WebSocket协议
    req->setHeader("Upgrade", "websocket");
    // 指示客户端使用的WebSocket协议版本
    req->setHeader("Sec-webSocket-Version", "13");
    // 生成随机的16字节字符串的密钥
    req->setHeader("Sec-webSocket-Key", base64encode(random_string(16)));
    req->setWebSocket(true);

    // 发送HTTP请求
    int rt = con->sendHttpRequest(req);
    if(rt == 0) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Status::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + addr->toString()), nullptr);
    }
    if(rt < 0) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Status::SEND_SOCKET_ERROR
                , nullptr, "send http request fail, socket error: errno=" + std::to_string(errno)
                + " errstr=" + strerror(errno)), nullptr);
    }

    // 接收HTTP响应
    HttpResponse::ptr res = con->recvHttpResponse();
    if(!res) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Status::TIMEOUT
                    , nullptr, "recv response timeout: " + addr->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms)), nullptr);
    }

    // 要保证接收到服务器101 Switching Protocols状态码，表示已经成功切换到WebSocket协议
    if(res->getStatus() != HttpStatus::SWITCHING_PROTOCOLS) {
        return std::make_pair(std::make_shared<HttpResult>(50
                    , res, "not websocket server " + addr->toString()), nullptr);
    }

    return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Status::OK
                    , res, "ok"), con);
}

// 构造函数
WSConnection::WSConnection(Socket::ptr sock, bool owner) 
    :HttpConnection(sock, owner) {
}

// WebSocket接收消息
WSFrameMessage::ptr WSConnection::recvMessage() {
    // true 表示客户端从服务器端接收
    return WSRecvMessage(this, true);
}

// WebSocket发送消息
int32_t WSConnection::sendMessage(WSFrameMessage::ptr msg, bool fin) {
    // true 表示客户端向服务器端发送
    return WSSendMessage(this, true, msg, fin);
}

// WebSocket发送消息
int32_t WSConnection::sendMessage(const std::string& data,  uint32_t opcode, bool fin) {
    // true 表示客户端向服务器端发送
    return WSSendMessage(this, true, std::make_shared<WSFrameMessage>(opcode, data), fin);
}

// 返回发送字节数
int32_t WSConnection::ping() {
    return WSPing(this);
}

// 发送Pong包
int32_t WSConnection::pong() {
    return WSPong(this);
}

}
}