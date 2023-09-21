#include <iostream>
#include "hook.h"
#include "log.h"
#include "iomanager.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_hook_sleep() {
    sylar::IOManager iom(1);
    iom.scheduler([](){
        sleep(2);
        SYLAR_LOG_INFO(g_logger) << "sleep 2";
    });
    iom.scheduler([](){
        sleep(3);
        SYLAR_LOG_INFO(g_logger) << "sleep 3";
    });
    SYLAR_LOG_INFO(g_logger) << "test_hook";
}

void test_hook_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  // 创建socket

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));    //清零
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);      //端口
    addr.sin_addr.s_addr = inet_addr("39.156.66.14"); //IP地址  inet_addr()为IP地址转换函数
    //inet_pton(AF_INET, "39.156.66.14", &addr.sin_addr.s_addr);

    SYLAR_LOG_INFO(g_logger) << "connect begin";
    int rt = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));  //成功返回0，失败-1
    SYLAR_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;
    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sockfd, data, sizeof(data), 0);    //成功返回发送字节数
    SYLAR_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;
    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);
    rt = recv(sockfd, &buff[0], buff.size(), 0);    //成功返回接收字节数
    SYLAR_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;
    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    SYLAR_LOG_INFO(g_logger) << buff;
}

int main(int argc, char** argv) {
    // test_hook_sleep();

    sylar::IOManager iom(1, false);
    iom.scheduler(&test_hook_socket);

    return 0;
}