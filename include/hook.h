#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__

#include <unistd.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>


namespace sylar {

// 当前线程是否hook
bool is_hook_enable();

// 设置当前线程的hook状态
void set_hook_enable(bool flag);

}

// 在C++方式下编译，变量和函数的命名规则很复杂，因为C++中的函数重载，参数检查等
// 以C方式编译，变量名和函数名之前被统一加上了一个下划线_
extern "C" {    //extern{} 括号中的函数和变量都被声明为extern


/**
 * @brief hook之后，要留出原函数的接口(_f)，例如sleep为hook后的函数，sleep_f为原接口函数，
 *        所以要将sleep_f声明，便于用户使用
 * @details 原函数：
 *              unsigned int sleep(unsigned int seconds);
 *          原声明： 
 *              extern unsigned int (*sleep_f)(unsigned int seconds); 
 *              其中sleep_f是一个函数指针，指向sleep
 * @param extern 告诉编译器：此变量/函数是在别处定义的，要在此处引用
 * @param typedef 的一种用法：为复杂的声明定义一个别名
 *        所以下面两句代码相当于上述原声明
*/
typedef unsigned int (*sleep_fun)(unsigned int seconds);  //用新别名sleep_fun替换sleep_f
extern sleep_fun sleep_f;    //原声明的最简化版

// typedef关键字来定义了一个函数指针类型usleep_fun
// 该函数指针类型可以指向 返回类型为int,参数为useconds_t 的函数
// 这个函数指针类型可以用来声明和定义变量，使其指向满足该函数类型条件的函数
// 例如：
// int my_sleep(useconds_t seconds) {
//     // 实现自定义的睡眠函数
// }
// sleep_fun my_sleep_ptr = my_sleep;    // 声明并定义一个函数指针变量
typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

// socket
typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern connect_fun connect_f;

typedef int (*accept_fun)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern accept_fun accept_f;

// read
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;

typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;

typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

// write
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
extern send_fun send_f;

typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
extern sendto_fun sendto_f;

typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

typedef int (*close_fun)(int fd);
extern close_fun close_f;

// 关于socket状态的函数
typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */ );
extern fcntl_fun fcntl_f;

typedef int (*ioctl_fun)(int d, int request, ...);
extern ioctl_fun ioctl_f;

typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f;

typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

extern int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms);

}

#endif