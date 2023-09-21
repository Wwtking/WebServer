#include "http_connection.h"
#include "http.h"
#include "iomanager.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    sylar::Address::ptr addr = sylar::Address::LookupAny("www.sylar.top:80");
    if(!addr) {
        SYLAR_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    sylar::Socket::ptr sock = sylar::Socket::CreatTcpSocket(addr);

    bool rt = sock->connect(addr);
    if(!rt) {
        SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    sylar::http::HttpConnection::ptr con = std::make_shared<sylar::http::HttpConnection>(sock);
    sylar::http::HttpRequest::ptr req = std::make_shared<sylar::http::HttpRequest>();
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");

    con->sendHttpRequest(req);
    sylar::http::HttpResponse::ptr res = con->recvHttpResponse();
    if(!res) {
        SYLAR_LOG_INFO(g_logger) << "recv response error";
        return;
    }

    SYLAR_LOG_INFO(g_logger) << "request: " << std::endl << req->toString();
    SYLAR_LOG_INFO(g_logger) << "response: " << std::endl << res->toString();
}

void test_doRequest() {
    auto result = sylar::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
    SYLAR_LOG_INFO(g_logger) << result->toString();
}

void test_pool() {
    auto pool = sylar::http::HttpConnectionPool::CreateConnectionPool(
                        "http://www.sylar.top", "", 10, 30 * 1000, 5);
    // auto pool = std::make_shared<sylar::http::HttpConnectionPool>(
    //                     "www.sylar.top", "", 80, 10, 30 * 1000, 5);
    sylar::IOManager::GetThis()->addTimer(1000, [pool](){
        auto result = pool->doGet("/", 500);
        SYLAR_LOG_INFO(g_logger) << result->toString();
    }, true);
}

int main(int argc, char** argv) {
    sylar::IOManager iom(2);
    // iom.scheduler(&run);
    // iom.scheduler(&test_doRequest);
    iom.scheduler(&test_pool);
}