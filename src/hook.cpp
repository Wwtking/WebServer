#include "hook.h"
#include "log.h"
#include "config.h"
#include "fiber.h"
#include "thread.h"
#include "iomanager.h"
#include "fd_manager.h"
#include <dlfcn.h>


namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<uint64_t>::ptr g_tcp_connect_timeout = 
    sylar::Config::Lookup("tcp.connect.timeout", (uint64_t)5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;  //表示当前线程是否启用hook

//将所有要hook的接口封装
#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

//初始化，封装原函数接口
bool hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return true;
    }
    //void *dlsym(void *handle, const char *symbol);
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX)    //调用XX()
#undef XX
    is_inited = true;
    return true;
}

static uint64_t s_connect_timeout = -1;    //connect连接超时时间

//定义结构，生成一个全局静态的结构实例，使其在main函数之前就要被创造，会执行构造函数
struct _HookInit {
    //在main函数之前就会执行该构造函数
    _HookInit() {
        hook_init();

        s_connect_timeout = g_tcp_connect_timeout->getValue();
        g_tcp_connect_timeout->addListener([](const uint64_t& oldVal, const uint64_t& newVal){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "tcp connect timeout changed from "
                                            << oldVal << " to " << newVal;
            s_connect_timeout = newVal;
        });
    }
};
static _HookInit s_hook_init;  //类实例

//当前线程是否hook
bool is_hook_enable() {
    return t_hook_enable;
}

//设置当前线程的hook状态
void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}
    
}

// 定时器条件
struct timer_info {
    int cancelled = 0;   //条件变量
};

// hook的高并发其实就是io任何的操作只要没有ready(被阻塞)就添加到event让出执行时间，等到ready了再回来继续执行，
// 并且添加定时器以防在一段时间内一直没有ready，充分利用了需要阻塞的协程的执行时间
// 让用户在使用同步接口下而实现异步的功能
/**
 * @brief 读和写两大类函数模板
 * @details 传一个要hook的函数进来，将其改为异步
 * @param[in] fd 文件句柄
 * @param[in] fun 原函数
 * @param[in] hook_fun_name 原函数名称
 * @param[in] event IOManager里添加的事件类型(READ或者WRITE)
 * @param[in] timeout_type FdManager中的超时类型(SO_RCVTIMEO或者SO_SNDTIMEO)
 * @param[in] args 原函数的其它参数
*/
template<typename OriginFun, typename...Args>    //typename...处理可变参数
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
                    uint32_t event, int timeout_type, Args&&... args) {
    //如果不用hook的话，执行原函数
    if(!sylar::is_hook_enable()) {
        //完美转发：指std::forward会将输入的参数原封不动地传递到下一个函数中
        return fun(fd, std::forward<Args>(args)...);
    }

    //看fd存不存在，若不存在说明不是socket句柄，执行原函数
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->getFdCtx(fd);
    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    //若句柄已关闭，设置相应错误并返回
    if(ctx->isClose()) {
        errno = EBADF;   //表示socket已经关闭
        return -1;
    }

    //如果fd不是socket句柄，或者用户已经设置成非阻塞，则直接执行原函数
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t timeout_ms = ctx->getTimeout(timeout_type);  //获取超时时间(需要等待的时间)
    std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();  //设置超时条件

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);  //先执行一遍原函数，如果是有效的，直接返回(即不用等待)
    //遇到中断，重新执行
    while(n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    //阻塞(需要等待)
    if(n == -1 && errno == EAGAIN) {
        sylar::IOManager* iom = sylar::IOManager::GetThis();
        sylar::Timer::ptr timer;   //记录添加的定时器，可能后续会取消
        std::weak_ptr<timer_info> winfo(tinfo);
        
        //设置定时器是为了等该hook函数超时后退出该函数
        if(timeout_ms != (uint64_t)-1) {
            timer = iom->addConditionTimer(timeout_ms, [iom, winfo, fd, event](){
                auto t = winfo.lock();    //拿出条件唤醒:返回shared_ptr类型的指针
                if(!t || t->cancelled) {  //如果条件不存在或者被设置成错误，直接返回
                    return;    //不是return -1; 因为定时回调函数并不在该函数中
                }
                t->cancelled = ETIMEDOUT;  //设置成 ETIMEDOUT 错误
                //并把事件取消掉，因为超时了就不需要后续继续循环执行，只执行一次就退出
                iom->cancelEvent(fd, (sylar::IOManager::Event)event);
            }, winfo);
        }
        
        //添加该事件是为了等有数据来的时候，通知协程返回该hook函数继续执行
        int rt = iom->addEvent(fd, (sylar::IOManager::Event)event);  //添加事件，cb为空，以当前协程为执行对象
        if(rt == -1) {
            //添加事件失败，打印日志，如果有定时器，就把定时器取消掉
            SYLAR_LOG_ERROR(sylar::g_logger) << hook_fun_name << " addEvent("
                                    << fd << ", " << event << ") error";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } 
        else {
            //添加事件成功，则把时间让出来，实现异步
            //SYLAR_LOG_INFO(g_logger) << "do_io before YieldToHold";
            sylar::Fiber::YieldToHold();
            //SYLAR_LOG_INFO(g_logger) << "do_io after YieldToHold";
            //有两点会返回到这里：(1)当条件定时器到达定时时间，说明没数据到来，执行cancelEvent唤醒回来，就退出该hook函数
            //                   (2)addEvent，当有数据回来时，会唤醒回来，事件通知有数据来了，之后继续循环执行
            if(timer) {
                timer->cancel();
            }
            if(tinfo->cancelled) {    //说明是从定时器超时唤醒回来的
                errno = tinfo->cancelled;   // errno = ETIMEDOUT;
                return -1;
            }
            goto retry;  //继续循环回去读数据，因为上述事件只是通知有数据来了
        }
    }
    return n;
}


// extern "C" 的主要作用是为了方便C++代码与C代码进行交互
// 当在 extern "C" 块中定义函数或变量时，它们会按照C语言的规则进行处理，以保持与C代码的兼容性
extern "C" {

// 声明一下name_f并且置成nullptr
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX)    //调用XX()
#undef XX

// hook后重新定义的sleep函数
unsigned int sleep(unsigned int seconds) {
    if(!sylar::is_hook_enable()) {
        return sleep_f(seconds);
    }

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    // iom->addTimer(seconds * 1000, [fiber, iom](){
    //     iom->scheduler(fiber);
    // });

    // 因为schedule是模板函数，所以前面要声明它的模板类型sylar::Fiber::ptr，同时它有默认参数，所以要声明默认参数size_t thread
    // schedule函数的签名为 void (*)(Fiber::ptr, size_t)，而不是 void (*)(Fiber::ptr)，默认参数在签名中不起作用
    // 将其分配给 void (*)(Fiber::ptr, size_t) 变量，则有关默认参数的信息将丢失：无法通过该变量利用默认参数，只能在绑定(bind)时手动提供默认值
    // std::bind(  ( void (sylar::Scheduler::*)(sylar::Fiber::ptr, size_t thread) )&sylar::IOManager::schedule, iom, fiber, -1  );
    iom->addTimer(seconds * 1000, std::bind( (void (sylar::IOManager::*)
            (sylar::Fiber::ptr, size_t thread))&sylar::IOManager::scheduler, 
            iom, fiber, -1) );    //这里传入的iom相当于this，负责调用scheduler成员函数

    sylar::Fiber::YieldToHold();
    //定时器超时后，将执行iom->scheduler(fiber); 调度器执行fiber时，将会回到这里
    return 0;
}

// hook后重新定义的usleep函数(usec：微秒us)
int usleep(useconds_t usec) {
    if(!sylar::is_hook_enable()) {
        return usleep_f(usec);
    }

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    // iom->addTimer(usec / 1000, [fiber, iom](){    //因为定时器精度是ms，所以存在误差
    //     iom->scheduler(fiber);
    // });
    iom->addTimer(usec / 1000, std::bind( (void (sylar::IOManager::*)
            (sylar::Fiber::ptr, size_t thread))&sylar::IOManager::scheduler, 
            iom, fiber, -1) );

    sylar::Fiber::YieldToHold();
    return 0;
}

// hook后重新定义的nanosleep函数(req->tv_sec：秒s  req->tv_nsec：纳秒ns)
int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!sylar::is_hook_enable()) {
        return nanosleep_f(req, rem);
    }

    uint64_t timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    // iom->addTimer(timeout_ms, [iom, fiber](){    //因为定时器精度是ms，所以存在误差
    //     iom->scheduler(fiber);
    // });
    iom->addTimer(timeout_ms, std::bind( (void (sylar::IOManager::*)
            (sylar::Fiber::ptr, size_t thread))&sylar::IOManager::scheduler, 
            iom, fiber, -1) );

    sylar::Fiber::YieldToHold();
    return 0;
}

// socket
// socket创建句柄时不会阻塞，所以hook复刻socket函数只需要将创建的句柄加入FdManager
int socket(int domain, int type, int protocol) {
    if(!sylar::is_hook_enable()) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd < 0) {    //创建失败
        return fd;
    }
    sylar::FdMgr::GetInstance()->getFdCtx(fd, true);
    return fd;
}

// 在sockfd是阻塞模式下，connect()函数会一直到有明确的结果才会返回(连接成功0或连接失败-1)
// 在sockfd是非阻塞模式下，调用connect()函数，此时无论connect()函数是否连接成功都会立即返回；
// 在非阻塞模式下，如果返回-1，并不一定表示连接出错，如果此时错误码是EINPROGRESS，则表示正在尝试连接
int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!sylar::is_hook_enable()) {
        //确保connect()函数在建立连接的过程中遵循所设置的超时时间,
        //如果连接建立时间超过设定的超时时间，connect()函数将中断并返回错误
        struct timeval tm = {(int)(timeout_ms/1000), (int)((timeout_ms%1000)*1000)};  //将ms转成s和us
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tm, sizeof(tm));  //用setsockopt设置超时时间
        return connect_f(sockfd, addr, addrlen);
    }

    //看fd存不存在，若不存在说明不是socket句柄，执行原函数
    //若存在，说明之前已经在getFdCtx()创建过sockfd，并且设置为非阻塞模式(FdCtx构造函数)
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->getFdCtx(sockfd);
    if(!ctx) {
        return connect_f(sockfd, addr, addrlen);
    }

    //若句柄已关闭，设置相应错误并返回
    if(ctx->isClose()) {
        errno = EBADF;   //表示socket已经关闭
        return -1;
    }

    //如果fd不是socket句柄，或者用户已经设置成非阻塞，则直接执行原函数
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return connect_f(sockfd, addr, addrlen);
    }

    int n = connect_f(sockfd, addr, addrlen);   //先去执行一次函数
    // connect连接成功，直接返回
    if(n == 0) {
        return 0;
    }
    // connect真的连接失败，直接返回
    if(n == -1 && errno != EINPROGRESS) {
        return n;
    }

    // 在非阻塞模式下，如果返回-1，并不一定表示连接出错，如果此时错误码是EINPROGRESS，则表示正在尝试连接
    // 执行到这里，说明还在等待connect连接(errno == EINPROGRESS)
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    sylar::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();
    std::weak_ptr<timer_info> winfo(tinfo);

    // 设置定时器是为了等该hook函数超时后退出该函数
    if(timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(timeout_ms, [winfo, sockfd, iom](){
            auto t = winfo.lock();    //拿出条件唤醒:返回shared_ptr类型的指针
            if(!t || t->cancelled) {  //如果条件不存在或者被设置成错误，直接返回
                return;
            }
            t->cancelled = ETIMEDOUT;  //设置成 ETIMEDOUT 错误
            iom->cancelEvent(sockfd, sylar::IOManager::WRITE);  //触发写事件，返回hook函数
        }, winfo);
    }

    //添加该事件是为了等有数据来的时候，通知协程返回该hook函数继续执行
    //用可写事件，因为只要connect连接上了，这个fd句柄就是可写的(即就会触发可写事件)
    int rt = iom->addEvent(sockfd, sylar::IOManager::WRITE);  //添加事件，cb为空，以当前协程为执行对象
    if(rt == -1) {
        //添加事件失败，打印日志，如果有定时器，就把定时器取消掉
        SYLAR_LOG_ERROR(sylar::g_logger) << "connect addEvent(" << sockfd << ", WRITE) error";
        if(timer) {
            timer->cancel();
        }
        return -1;
    } 
    else {
        //添加事件成功，则把时间让出来，实现异步
        // SYLAR_LOG_INFO(g_logger) << "connect before YieldToHold";
        sylar::Fiber::YieldToHold();
        // SYLAR_LOG_INFO(g_logger) << "connect after YieldToHold";
        //有两点会返回到这里：(1)当条件定时器到达定时时间，说明没数据到来，执行cancelEvent唤醒回来，就退出该hook函数
        //                   (2)addEvent，当有数据回来时，会唤醒回来，事件通知有数据来了，之后继续循环执行
        if(timer) {
            timer->cancel();
        }
        if(tinfo->cancelled) {    //说明是从定时器超时唤醒回来的
            errno = tinfo->cancelled;   // errno = ETIMEDOUT;
            return -1;
        }
    }

    // 执行到这里的时候，说明connect()已经完全执行完毕，但是并不知道connect是否成功，因为：
    //      connect连接成功时，sockfd变为可写（连接建立时，写缓冲区空闲，所以可写）
    //      connect连接失败时，sockfd既可读又可写（由于有未决的错误，从而可读又可写）
    // 所以无论connect是否成功，都会触发写事件swapIn回来，需要getsockopt去判断是否成功
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt_f(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)) {   //检查有没有错误
        return -1;    //getsockopt()发生错误
    }

    if(error) {     //有错误，标记errno并返回-1
        errno = error;
        return -1;
    }
    else {
        return 0;   //没错误返回0
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, sylar::s_connect_timeout);
}

// sockfd为阻塞：如果没有连接请求到达，accept函数将会阻塞(暂停执行)直到有连接请求到达为止
// sockfd为非阻塞：如果没有连接请求到达，accept函数会立即返回，并且返回值为-1，同时设置errno为EAGAIN
//                此时还在等待连接请求，通过epoll_wait监测到sockfd可读后，说明有请求，重新再执行该函数获取新的文件描述符
// 成功:返回一个新的socket文件描述符，用于和客户端通信，需要放入FdManager
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    // 用读事件，因为accept成功后，sockfd将可读(即触发读事件)
    int fd = do_io(sockfd, accept_f, "accept", sylar::IOManager::READ, SO_RCVTIMEO, addr, addrlen); 
    if(fd >= 0) {
        sylar::FdMgr::GetInstance()->getFdCtx(fd, true);   //这个得到的fd是用于客户端和服务器通信的句柄
    }
    return fd;
}

// read：用于从文件描述符 fd 所代表的文件或套接字中读取数据到 buf 中
// fd为阻塞：如果没有数据可读，程序将会阻塞(暂停执行)直到有数据可读或者遇到文件末尾
// fd为非阻塞：如果没有数据可读，read函数会立即返回，并且返回值为-1，同时设置errno为EAGAIN
//            此时还在等待fd可读，通过epoll_wait监测到fd可读后，说明有数据可读，重新再执行该函数去读取数据
ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", sylar::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", sylar::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

// write：用于将数据从 buf 写入到文件描述符 fd 所代表的文件或套接字中
// fd为阻塞：如果无法立即将所有的数据写入到fd中，write函数会阻塞程序，直到数据被完全写入或者发生错误
// fd为非阻塞：如果无法立即写入，write函数会立即返回，并且返回值为-1，同时设置errno为EAGAIN
//            此时还在等待fd可写，通过epoll_wait监测到fd可写后，说明可以写入数据，重新再执行该函数去写入数据
ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", sylar::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    return do_io(sockfd, send_f, "send", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    return do_io(sockfd, sendto_f, "sendto", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return do_io(sockfd, sendmsg_f, "sendmsg", sylar::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

// close关闭句柄不会阻塞，所以hook复刻close函数只需要将句柄从FdManager中删除，并且将该句柄的所有事件触发一次
int close(int fd) {
    if(!sylar::is_hook_enable()) {
        return close_f(fd);
    }
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->getFdCtx(fd);
    if(ctx) {
        auto iom = sylar::IOManager::GetThis();
        if(iom) {
            iom->cancelAllEvent(fd);   //取消fd的所有事件(都触发一次)
        }
        sylar::FdMgr::GetInstance()->deleteFdCtx(fd);  //删除fd
    }
    return close_f(fd);
}

// 关于socket状态的函数
int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);    //开始可变参数列表
    switch(cmd) {
        // hook复刻fcntl_f(fd, F_SETFL, arg); 
        // 系统已默认设置成O_NONBLOCK，但是会根据用户是否设置O_NONBLOCK来选择是否用原函数
        // 上述do_io模版函数，如果检测到用户设置过O_NONBLOCK，则直接用原函数(因为也是异步非阻塞)
        case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->getFdCtx(fd);
            if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);   //用户是否显式设置非阻塞
            if(ctx->getSystemNonblock()) {   //hook内部设置的非阻塞(FdCtx::init()时设置了)
                arg |= O_NONBLOCK;
            }
            else {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }

        // hook复刻fcntl_f(fd, F_GETFL);
        // 根据用户是否设置过非阻塞来返回，虽然使用的肯定是非阻塞(因为系统已默认设置)，
        // 但是如果用户未设置非阻塞，返回的状态却带O_NONBLOCK，会影响用户体验
        case F_GETFL:
        {
            va_end(va);
            int flag = fcntl_f(fd, cmd);
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->getFdCtx(fd);
            if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                return flag;
            }
            // 要看是否为用户显式设置的非阻塞，只有用户自己设置了非阻塞，才会返回带有O_NONBLOCK
            // 如果用户未设置非阻塞，系统默认设置非阻塞(FdCtx::init()时设置了)，也要返回是未设置非阻塞
            if(ctx->getUserNonblock()) {
                return flag | O_NONBLOCK;
            }
            else {
                return flag & ~O_NONBLOCK;
            }
        }

        // 可变参数为 int 类型
        case F_DUPFD: 
        case F_DUPFD_CLOEXEC: 
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif  
        {
            // int fcntl(int fd, int cmd, int arg);
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }

        // 可变参数为 void 类型
        case F_GETFD:
        case F_GETOWN: 
        case F_GETSIG: 
        case F_GETLEASE: 
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ: 
#endif  
        {
            // int fcntl(int fd, int cmd);
            va_end(va);
            return fcntl_f(fd, cmd);
        }
        
        // 可变参数为 struct flock* 类型
        case F_SETLK: 
        case F_SETLKW: 
        case F_GETLK: 
        {
            // int fcntl(int fd, int cmd, struct flock* arg);
            struct flock* arg = va_arg(va, struct flock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
         }

        // 可变参数为 struct f_owner_ex* 类型
        case F_GETOWN_EX: 
        case F_SETOWN_EX: 
        {
            // int fcntl(int fd, int cmd, struct f_owner_ex* arg);
            struct f_owner_ex* arg = va_arg(va, struct f_owner_ex*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

/**
 * @brief ioctl是设备驱动程序中对设备的I/O通道进行管理的函数
 * @param[in] d 文件描述符
 * @param[in] request 交互协议，设备驱动将根据 request 执行对应操作
 * @param[in] arg... 可变参数arg，The third argument is an untyped pointer to memory
*/
int ioctl(int d, long unsigned int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    // 该函数不会阻塞，只是在hook复刻该函数时，如果用该函数 设置/清除非阻塞标志，要记录setUserNonblock()
    // FIONBIO标志为设置/清除非阻塞标志
    if(request == FIONBIO) {
        bool user_nonblock = !!(*(int*)arg);  // (*(int*)arg)=1时，为设置成非阻塞； =0为阻塞
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->getFdCtx(d);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);  // 如果是要设置/清除socket非阻塞标志，则需要记录m_userNonblock
    }
    return ioctl_f(d, request, arg);
}

// 获取套接字描述符的属性，不用复刻
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

// 设置套接字描述符的属性
// 当用户调用setsockopt()执行的是设置超时时间的操作时，则需要把超时时间根据optname来放到FdCtx::ptr中(调用setTimeout())
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!sylar::is_hook_enable()) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }

    // setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &optval, optlen);  //optval存放超时时间
    // setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &optval, optlen);
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->getFdCtx(sockfd);
            if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                return setsockopt_f(sockfd, level, optname, optval, optlen);
            }
            const struct timeval* tm = (const struct timeval*)optval;
            uint64_t timeout_ms = tm->tv_sec * 1000 + tm->tv_usec / 1000;  //超时时间
            ctx->setTimeout(optname, timeout_ms);    //设置sockfd的超时时间
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}
