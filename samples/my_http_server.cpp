#include "http_server.h"
#include "log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test() {
    g_logger->setLevel(sylar::LogLevel::INFO);
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        SYLAR_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    // close
    // sylar::http::HttpServer::ptr server = std::make_shared<sylar::http::HttpServer>();
    // keep-alive
    sylar::http::HttpServer::ptr server = std::make_shared<sylar::http::HttpServer>(true);

    while(!server->bind(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "bind address error: " << addr->toString();
        sleep(2);
    }

    server->start();
}

int main(int argc, char** argv) {
    sylar::IOManager iom(1);
    iom.scheduler(&test);

    return 0;
}