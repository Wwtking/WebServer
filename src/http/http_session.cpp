#include "log.h"
#include "http_session.h"
#include "http_parser.h"

namespace sylar {
namespace http {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 构造函数
HttpSession::HttpSession(Socket::ptr sock, bool owner) 
    :SocketStream(sock, owner) {
}

// 接收HTTP请求
HttpRequest::ptr HttpSession::recvHttpRequest() {
    HttpRequestParser::ptr parser = std::make_shared<HttpRequestParser>();
    size_t buff_size = HttpRequestParser::GetRequestMaxHeaderSize();
    // size_t buff_size = 100;
    // char* buffer(new char[buff_size]);    // 普通数组
    // 智能指针数组，自定义析构
    std::shared_ptr<char> buffer(new char[buff_size], [](char* ptr) {
        delete[] ptr;
    });
    char* buff = buffer.get();
    size_t offset = 0;

    while(true) {
        // 读数据，offset是上次未解析数据的偏移
        int len = read(buff + offset, buff_size - offset);
        if(len <= 0) {
            // SYLAR_LOG_DEBUG(g_logger) << "====================read len <= 0";
            close();
            return nullptr;
        }
        // SYLAR_LOG_DEBUG(g_logger) << "read: len=" << len << " content:" << buff;
        
        // 解析数据，len + offset为本次新读取的+上次未解析的
        size_t nparser = parser->execute(buff, len + offset);
        if(parser->hasError()) {
            // SYLAR_LOG_DEBUG(g_logger) << "====================execute hasError";
            close();
            return nullptr;
        }
        // SYLAR_LOG_DEBUG(g_logger) << "parser: nparser=" << nparser << " content:" << buff;

        // 更新偏移，解析完的数据将被移除，剩余未解析的数据为最新偏移
        offset = len + offset - nparser;
        // 缓存区满了都无数据解析，说明解析有问题
        if(offset == buff_size) {
            // SYLAR_LOG_DEBUG(g_logger) << "====================offset == buff_size";
            close();
            return nullptr;
        }

        if(parser->isFinished()) {
            break;
        }
    }

    size_t length = parser->getContentLen();
    if(length > 0) {
        std::string body;
        body.resize(length);

        // body部分不会被解析，所以要手动写入
        if(length > offset) {
            // body部分未完全读完，需要继续向后读取一部分
            std::memcpy(&body[0], buff, offset);
            if(readFixSize(&body[offset], length - offset) <= 0) {
                // SYLAR_LOG_DEBUG(g_logger) << "====================readFixSize <= 0";
                close();
                return nullptr;
            }
        }
        else {
            std::memcpy(&body[0], buff, length);
        }
        parser->getData()->setBody(body);
    }

    // 根据接收到的Http请求设置连接状态(长连接or关闭连接)
    std::string conStatus = parser->getData()->getHeader("Connection");
    if(!conStatus.empty()) {
        if(strcasecmp(conStatus.c_str(), "Keep-Alive") == 0) {
            parser->getData()->setClose(false);
        }
        else {
            parser->getData()->setClose(true);
        }
    }

    return parser->getData();
}

// 发生HTTP响应
ssize_t HttpSession::sendHttpResponse(HttpResponse::ptr response) {
    std::string res = response->toString();
    return writeFixSize(res.c_str(), res.size());  // 写入固定长度的数据
}

}
}