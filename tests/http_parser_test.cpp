#include "http_parser.h"
#include "log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

// HTTP请求数据
// head和body之间要多一个空行\r\n
const char* request_data = "GET / HTTP/1.1\r\n"
                            "Host: www.sylar.top\r\n"
                            "Content-Length: 10\r\n\r\n"
                            "1234567890";

void test_request() {
    sylar::http::HttpRequestParser parser;
    std::string data = request_data;

    // 解析HTTP请求数据，但不解析消息体body，不会把消息体放入m_body中
    // 只能等解析完头部后，将data解析完的数据移除，data剩余的数据就是消息体body
    size_t offset = parser.execute(&data[0], data.size());
    SYLAR_LOG_INFO(g_logger) << "hasError=" << parser.hasError() 
                    << " isFinished=" << parser.isFinished() 
                    << " execute_data_offset=" << offset
                    << " Content-Length=" << parser.getContentLen() 
                    << " data_total_size=" << data.size();

    data.resize(data.size() - offset);    // data剩余的数据才是消息体body
    SYLAR_LOG_INFO(g_logger) << parser.getData()->toString();
    SYLAR_LOG_INFO(g_logger) << data;
}

// HTTP响应数据
// head和body之间要多一个空行\r\n
const char* response_data = "HTTP/1.1 200 OK\r\n"
                            "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
                            "Server: Apache\r\n"
                            "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
                            "ETag: \"51-47cf7e6ee8400\"\r\n"
                            "Accept-Ranges: bytes\r\n"
                            "Content-Length: 81\r\n"
                            "Cache-Control: max-age=86400\r\n"
                            "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
                            "Connection: Close\r\n"
                            "Content-Type: text/html\r\n\r\n"
                            "<html>\r\n"
                            "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
                            "</html>\r\n";

void test_response() {
    sylar::http::HttpResponseParser parser;
    std::string data = response_data;

    // 解析HTTP请求数据，但不解析消息体body，不会把消息体放入m_body中
    // 只能等解析完头部后，将data解析完的数据移除，data剩余的数据就是消息体body
    int offset = parser.execute(&data[0], data.size());
    SYLAR_LOG_INFO(g_logger) << "hasError=" << parser.hasError() 
                    << " isFinished=" << parser.isFinished() 
                    << " execute_data_offset=" << offset
                    << " Content-Length=" << parser.getContentLen() 
                    << " data_total_size=" << data.size();

    data.resize(data.size() - offset);    // data剩余的数据才是消息体body
    SYLAR_LOG_INFO(g_logger) << parser.getData()->toString();
    SYLAR_LOG_INFO(g_logger) << data;
}

int main(int argc, char** argv) {
    test_request();
    SYLAR_LOG_DEBUG(g_logger) << "----------------------------------";
    test_response();

    return 0;
}
