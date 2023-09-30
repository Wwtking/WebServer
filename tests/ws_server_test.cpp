#include "ws_server.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    // 创建HTTP服务器
    sylar::http::WSServer::ptr server = std::make_shared<sylar::http::WSServer>();

    // 用Address类封装待绑定的地址
    sylar::Address::ptr addr = sylar::Address::LookupAny("0.0.0.0:8020");

    auto func = [](sylar::http::HttpRequest::ptr req
                , sylar::http::WSFrameMessage::ptr msg
                , sylar::http::WSSession::ptr session) {
        session->sendMessage(msg);
        return 0;
    };

    server->getDispatch()->addServlet("/sylar", func);

    // 绑定服务器地址(bind和listen)
    while(!server->bind(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(2);
    }

    // 启动服务器(accept和handleClient)
    server->start();
}

int main(int argc, char** argv) {
    srand(time(0));
    sylar::IOManager iom(2);
    iom.scheduler(&run);
}