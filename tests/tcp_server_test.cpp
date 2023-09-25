#include "tcp_server.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_tcpserver() {
    // 创建TCP服务器
    sylar::TcpServer::ptr tcp_server = std::make_shared<sylar::TcpServer>();

    // 用Address类封装待绑定的地址
    sylar::Address::ptr ipv4Addr = sylar::Address::LookupAny("0.0.0.0:8020");
    // sylar::Address::ptr unixAddr = std::make_shared<sylar::UnixAddress>("/home/wwt/sylar/bin/unix_addr");
    std::vector<sylar::Address::ptr> addrs;
    std::vector<sylar::Address::ptr> fails;
    addrs.push_back(ipv4Addr);
    // addrs.push_back(unixAddr);

    // 绑定服务器地址(bind和listen)
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }

    // 启动服务器(accept和handleClient)
    tcp_server->start();
    // tcp_server->stop();
}

int main(int argc, char** argv) {
    sylar::IOManager iom(2);
    iom.scheduler(&test_tcpserver);
    
    return 0;
}