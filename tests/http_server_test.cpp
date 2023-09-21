#include "http_server.h"
#include "http.h"


void test() {
    // 创建HTTP服务器
    sylar::http::HttpServer::ptr server = std::make_shared<sylar::http::HttpServer>();

    // 用Address类封装待绑定的地址
    sylar::Address::ptr addr = sylar::Address::LookupAny("0.0.0.0:8020");

    // 绑定服务器地址(bind和listen)
    while(!server->bind(addr)) {
        sleep(2);
    }

    server->getDispatch()->addServlet("/wwt/info", [](sylar::http::HttpRequest::ptr req, 
                                                    sylar::http::HttpResponse::ptr res, 
                                                    sylar::http::HttpSession::ptr session) {
        res->setBody(req->toString());
        return 0;
    });
    server->getDispatch()->addGlobServlet("/wwt/*", [](sylar::http::HttpRequest::ptr req, 
                                                    sylar::http::HttpResponse::ptr res, 
                                                    sylar::http::HttpSession::ptr session) {
        res->setBody("Glob: \r\n" + req->toString());
        return 0;
    });

    // 启动服务器(accept和handleClient)
    server->start();
}

int main(int argc, char** argv) {
    sylar::IOManager iom(2);
    iom.scheduler(&test);

    return 0;
}