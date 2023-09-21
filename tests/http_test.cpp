#include "http.h"
#include "log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_request() {
    sylar::http::HttpRequest::ptr req = std::make_shared<sylar::http::HttpRequest>();
    req->setClose(false);
    req->setPath("/books/");
    req->setQuery("sex=man&name=Professional");
    req->setFragment("People");
    req->setBody("hello sylar");
    req->setHeader("Host", "www.wrox.com");
    req->setHeader("Code", "123456");
    SYLAR_LOG_INFO(g_logger) << "\n" << req->toString();

    SYLAR_LOG_INFO(g_logger) << req->getHeaderValue("Number", 10.1);
    SYLAR_LOG_INFO(g_logger) << req->getHeaderValue("Code", 666666);
}

void test_response() {
    sylar::http::HttpResponse::ptr res = std::make_shared<sylar::http::HttpResponse>();
    res->setStatus(sylar::http::HttpStatus::NOT_FOUND);
    res->setClose(true);
    res->setReason(sylar::http::HttpStatusToString(res->getStatus()));
    res->setBody("hello sylar");
    res->setHeader("Content-Type", "text/html");
    res->setHeader("Code", "123456");
    SYLAR_LOG_INFO(g_logger) << "\n" << res->toString();

    SYLAR_LOG_INFO(g_logger) << res->getHeaderValue("Number", 10.1);
    SYLAR_LOG_INFO(g_logger) << res->getHeaderValue("Code", 666666);
}

int main(int argc, char** argv) {
    test_request();
    test_response();

    return 0;
}