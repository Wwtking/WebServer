#include "log.h"
#include "socket.h"
#include "iomanager.h"
#include "address.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_socket() {
    sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress("www.baidu.com[:80]");
    if(!addr) {
        SYLAR_LOG_ERROR(g_logger) << "LookupAnyIPAddress error";
        return;
    }
    SYLAR_LOG_DEBUG(g_logger) << "LookupAnyIPAddress: " << addr->toString();
    
    sylar::Socket::ptr sock = sylar::Socket::CreatTcpSocket(addr);
    //sylar::Socket::ptr sock = sylar::Socket::CreatIPv4TcpSocket();
    if(!sock) {
        SYLAR_LOG_ERROR(g_logger) << "CreatTcpSocket error";
        return;
    }
    
    if(!sock->connect(addr)) {
        SYLAR_LOG_DEBUG(g_logger) << sock->toString();
        SYLAR_LOG_ERROR(g_logger) << "connect error";
        return;
    }
    else {
        SYLAR_LOG_DEBUG(g_logger) << sock->toString();
        SYLAR_LOG_DEBUG(g_logger) << "connect success";
    }

    char buf[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buf, sizeof(buf));
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "send error";
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "recv error";
        return;
    }
    
    buffs.resize(rt);   
    SYLAR_LOG_INFO(g_logger) << buffs;

}

int main(int argc, char** argv) {
    sylar::IOManager iom;
    iom.scheduler(&test_socket);

    return 0;
}
