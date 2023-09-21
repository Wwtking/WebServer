#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "iomanager.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int sockfd;

void test_iomanager() {
    SYLAR_LOG_INFO(g_logger) << "test_iomanager";

    //创建套接字，返回一个int类型的文件描述符，指向新创建的socket(套接字)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //把socket设置为非阻塞的方式
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));    //清零
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);      //端口
    addr.sin_addr.s_addr = inet_addr("39.156.66.10");  //IP地址  inet_addr()为IP地址转换函数
    //inet_pton(AF_INET, "39.156.66.10", &addr.sin_addr.s_addr);
    
    //客户端需要调用connect()连接服务器
    if(!connect(sockfd, (struct sockaddr *)&addr, sizeof(addr))) {
        SYLAR_LOG_INFO(g_logger) << "connect success";
    }
    else if(errno == EINPROGRESS) {
        SYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        sylar::IOManager::GetThis()->addEvent(sockfd, sylar::IOManager::READ, [](){
            SYLAR_LOG_INFO(g_logger) << "read callback";
        });
        sylar::IOManager::GetThis()->addEvent(sockfd, sylar::IOManager::WRITE, [](){
            SYLAR_LOG_INFO(g_logger) << "write callback";
            sylar::IOManager::GetThis()->cancelEvent(sockfd, sylar::IOManager::READ);
            close(sockfd);
        });
        //sylar::IOManager::GetThis()->addEvent(sockfd, sylar::IOManager::WRITE, nullptr);
    }
    else {
        SYLAR_LOG_INFO(g_logger) << "other errno=" << errno << " " << strerror(errno);
    }
}

void test() {
    sylar::IOManager iom(4, false);
    iom.scheduler(&test_iomanager);
}

sylar::Timer::ptr timer;
void test_timer() {
    sylar::IOManager iom(2);
    timer = iom.addTimer(1000, [](){
        static int count = 0;
        SYLAR_LOG_INFO(g_logger) << "hello timer count=" << count;
        if(++count == 3) {
            //timer->refresh();
            timer->reset(2000, false);
        }
        if(count == 6) {
            timer->cancel();
        }
    }, true);
}

int main(int argc, char** argv) {
    //test();

    test_timer();
    
    return 0;
}

