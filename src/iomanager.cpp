#include "iomanager.h"
#include "macro.h"
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");


//获取事件上下文
IOManager::FdContext::EventContext& IOManager::FdContext::getEventContext(Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getEventContext invalid event");
    }
}

//重置事件上下文
void IOManager::FdContext::resetEventContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

/**
 * @brief 触发事件
 * @details 根据事件类型调用对应上下文结构中的调度器去调度回调协程或回调函数
 * @param[in] event 事件类型
 */
void IOManager::FdContext::triggerEvent(Event event) {
    //若events里没有event类事件，则无法触发
    if(!(events & event)) { 
        SYLAR_LOG_ERROR(g_logger) << "fd=" << fd
            << " events=" << events
            << " triggerEvent event=" << event
            << "\nbacktrace\n" 
            << sylar::BacktraceToString(100, 2, "    ");
        return;
    }
    events = (Event)(events & ~event);  //从events中删掉event
    //事件触发完后，要把EventContext事件上下文置空
    EventContext& ctx = getEventContext(event);
    if(ctx.cb) {
        ctx.scheduler->scheduler(&ctx.cb);  //scheduler中swap后，ctx.cb为NULL
    } 
    else {
        ctx.scheduler->scheduler(&ctx.fiber); //scheduler中swap后，ctx.fiber为NULL
    }
    ctx.scheduler = nullptr;
    return;
}

/**
 * @brief 构造函数
 * @param[in] threads 线程数量
 * @param[in] use_caller 是否将调用线程包含进去
 * @param[in] name 调度器的名称
 */
IOManager::IOManager(size_t threads, bool use_caller, const std::string& name) 
    :Scheduler(threads, use_caller, name) {
    //创建epoll实例,在内核区建立红黑树(用于存储以后epoll_ctl传来的socket)和双向链表(用于存储准备就绪的事件)
    //红黑树中每个成员由描述符值和所要监控的文件描述符指向的文件表项的引用等组成
    m_epfd = epoll_create(1000);
    SYLAR_ASSERT2(m_epfd > 0, "epoll_create error");

    int rt = pipe(m_tickleFds);  //创建一个管道，往fd[1]写入的数据可以从fd[0]读出
    SYLAR_ASSERT2(!rt, "pipe create error");

    //注册pipe读句柄的可读事件，用于tickle调度协程，通过epoll_event.data.fd保存描述符
    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));  //清零
    event.events = EPOLLIN | EPOLLET;   //设置读事件和边缘触发
    event.data.fd = m_tickleFds[0];
    
    //非阻塞方式，配合边缘触发
    //epoll的ET模式下是必须使用非阻塞IO的，ET模式指的是当数据从无到有时，才通知该fd
    //数据读不完，也不会再次通知，所以read时一定要采用循环的方式一直读到read函数返回-1为止
    rt = fcntl(m_tickleFds[0], F_SETFL, fcntl(m_tickleFds[0], F_GETFD, 0) | O_NONBLOCK);
    SYLAR_ASSERT2(!rt, "fcntl error");

    //将被监听的描述符添加到红黑树中
    //将管道的读描述符加入epoll多路复用，如果管道可读，idle中的epoll_wait会返回
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT2(!rt, "epoll_ctl error");

    fdContextsResize(32);  //初始化m_fdContexts容器

    //这里直接开启了Schedluer，也就是说IOManager创建即可调度协程
    start();
}

//析构函数
IOManager::~IOManager() {
    //先等Scheduler调度完所有的任务，再关闭epoll句柄和pipe句柄
    stop();   //析构时关掉Schedluer
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

/**
 * @brief 添加事件
 * @details fd描述符发生了event事件时执行cb函数
 * @param[in] fd socket句柄
 * @param[in] event 事件类型
 * @param[in] cb 事件回调函数，如果为空，则默认把当前协程作为回调执行体
 * @return 添加成功返回0，失败返回-1
 */
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    //找到fd对应的FdContext，如果不存在，那就分配一个
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }
    else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        fdContextsResize(fd * 1.5);    //扩充1.5倍
        fd_ctx = m_fdContexts[fd];
    }

    //同一个fd不允许重复添加相同的事件
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(fd_ctx->events & event) {
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " event=" << (EPOLL_EVENTS)event
                    << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
        SYLAR_ASSERT2(!(fd_ctx->events & event), "addEvent: add same event");
    }

    //将新的事件加入epoll_wait，使用epoll_event的私有指针存储FdContext的位置
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event new_event;
    new_event.events = EPOLLET | fd_ctx->events | event;
    new_event.data.fd = fd;
    new_event.data.ptr = fd_ctx;

    //将用户空间new_event拷贝到内核中，后续可以将其转化为epitem作为节点存入红黑树中，
    //fd是红黑树中查找所对应的epitem实例的key，根据传入的op参数对红黑树进行不同的操作
    int rt = epoll_ctl(m_epfd, op, fd, &new_event);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)new_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events=" 
            << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    ++m_pendingEventCount;   //待执行IO事件数加1

    //虽然fd_ctx已加入到new_event.data.ptr中，但是new_event是引用传递，所以可以同步修改
    //找到这个fd的event事件对应的EventContext，对其中的scheduler, cb, fiber进行赋值
    fd_ctx->events = (Event)(fd_ctx->events | event);  //更新fd_ctx的events
    FdContext::EventContext& event_ctx = fd_ctx->getEventContext(event);
    SYLAR_ASSERT(!event_ctx.scheduler 
                && !event_ctx.fiber 
                && !event_ctx.cb);
    
    // 赋值scheduler和回调函数，如果回调函数为空，则把当前协程当成回调执行体
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
        event_ctx.cb.swap(cb);    //swap后，cb为NULL
    }
    else {
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC, 
                    "fiber state=" << event_ctx.fiber->getState());
    }
    return 0;
}

/**
 * @brief 删除事件
 * @param[in] fd socket句柄
 * @param[in] event 事件类型
 * @attention 不会触发事件
 * @return 是否删除成功
 */
bool IOManager::delEvent(int fd, Event event) {
    //找到fd对应的FdContext
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        SYLAR_LOG_ERROR(g_logger) << "delEvent error: fd=" << fd << " not exist";
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    //要删除的事件不存在
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) {
        SYLAR_LOG_ERROR(g_logger) << "delEvent error: fd=" << fd 
                                << " event=" << (EPOLL_EVENTS)event
                                << " not exist";
        return false;
    }

    //清除指定的事件，表示不关心这个事件了，如果清除之后结果为0，则从epoll_wait中删除该文件描述符
    int op = (fd_ctx->events & ~event) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event new_event;
    new_event.events = EPOLLET | (fd_ctx->events & ~event);  //删掉events中的event
    new_event.data.fd = fd;
    new_event.data.ptr = fd_ctx;

    //fd是红黑树中的key，根据fd找到epitem节点进行op操作，new_event记录新的事件信息
    int rt = epoll_ctl(m_epfd, op, fd, &new_event);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)new_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events=" 
            << (EPOLL_EVENTS)fd_ctx->events;
        return false;
    }

    --m_pendingEventCount;   //待执行IO事件数减1

    //重置该fd对应的event事件上下文
    fd_ctx->events = (Event)(fd_ctx->events & ~event);
    FdContext::EventContext& event_ctx = fd_ctx->getEventContext(event);
    fd_ctx->resetEventContext(event_ctx);
    return true;
}

/**
 * @brief 取消事件
 * @param[in] fd socket句柄
 * @param[in] event 事件类型
 * @attention 如果事件存在则触发事件
 * @return 是否取消成功
 */
bool IOManager::cancelEvent(int fd, Event event) {
    //找到fd对应的FdContext
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        SYLAR_LOG_ERROR(g_logger) << "cancelEvent error: fd=" << fd << " not exist";
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    //要删除的事件不存在
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) {
        SYLAR_LOG_ERROR(g_logger) << "cancelEvent error: fd=" << fd 
                                << " event=" << (EPOLL_EVENTS)event
                                << " not exist";
        return false;
    }

    //清除指定的事件，表示不关心这个事件了，如果清除之后结果为0，则从epoll_wait中删除该文件描述符
    int op = (fd_ctx->events & ~event) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event new_event;
    new_event.events = EPOLLET | (fd_ctx->events & ~event);  //删掉events中的event
    new_event.data.fd = fd;
    new_event.data.ptr = fd_ctx;

    //fd是红黑树中的key，根据fd找到epitem节点进行op操作，new_event记录新的事件信息
    int rt = epoll_ctl(m_epfd, op, fd, &new_event);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)new_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events=" 
            << (EPOLL_EVENTS)fd_ctx->events;
        return false;
    }

    // 删除之前触发一次事件
    fd_ctx->triggerEvent(event);  //triggerEvent()内会处理fd_ctx的events和EventContext
    --m_pendingEventCount;   //待执行IO事件数减1
    return true;
}

/**
 * @brief 取消所有事件
 * @param[in] fd socket句柄
 * @attention 所有事件都会被触发一次
 * @return 是否取消成功
 */
bool IOManager::cancelAllEvent(int fd) {
    //找到fd对应的FdContext
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        SYLAR_LOG_ERROR(g_logger) << "cancelEvent error: fd=" << fd << " not exist";
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    //事件为空，没有要删除的事件
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events) {
        SYLAR_LOG_DEBUG(g_logger) << "cancelEvent info: event is NULL, not need to cancel";
        return false;
    }

    // 删除全部事件
    int op = EPOLL_CTL_DEL;
    epoll_event new_event;
    new_event.events = NONE;
    new_event.data.fd = fd;
    new_event.data.ptr = fd_ctx;

    //fd是红黑树中的key，根据fd找到epitem节点进行op操作，new_event记录新的事件信息
    int rt = epoll_ctl(m_epfd, op, fd, &new_event);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)new_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events=" 
            << (EPOLL_EVENTS)fd_ctx->events;
        return false;
    }

    //触发全部已注册的事件
    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ); //triggerEvent()内会处理fd_ctx的events和EventContext
        --m_pendingEventCount;      //待执行IO事件数减1
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    
    SYLAR_ASSERT(fd_ctx->events == NONE);
    return true;
}

//返回当前的IOManager
IOManager* IOManager::GetThis() {
    //将基类的指针安全地转换成派生类的指针，并用派生类的指针可以调用非虚函数
    //在继承关系中进行向下转型：当基类指针或引用指向一个派生类对象时，通过dynamic_cast可以将其转换为派生类指针或引用，以便访问派生类特有的成员
    return dynamic_cast<IOManager*>(Scheduler::GetThis());  //强制转换
}

/**
 * @brief 通知调度器有任务要调度
 * @details 写pipe让idle协程从epoll_wait退出，待idle协程yield后Scheduler::run就可以调度其他任务
 *          如果当前没有空闲调度线程，那就没必要发通知
 */
void IOManager::tickle() {
    //SYLAR_LOG_INFO(g_logger) << "IOManager::tickle";    //调试打开
    if(!hasIdleThreads()) {
        return;
    }
    /**
     * @brief ssize_t write(int fd, const void * buf, size_t count);
     * @details write()会把参数buf所指的内存写入count个字节到参数fd所指的文件内
     * @return 成功:返回实际写入的字节数  失败:返回-1，错误代码存入errno中 
    */
    int rt = write(m_tickleFds[1], "T", 1);
    SYLAR_ASSERT(rt == 1);
}

/**
 * @brief 判断调度器是否已经停止
 * @param[out] timeout 距离最近一次要触发的定时器的时间差
 */
bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();   //输出
    return timeout == ~0ull          //等所有的定时器执行完
        && m_pendingEventCount == 0  //等所有待调度的IO事件执行完
        && Scheduler::stopping();
}

//判断调度器是否已经停止
bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

/**
 * @brief idle协程
 * @details 对于IO协程调度来说，应阻塞在等待IO事件上，idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件就绪
 * 调度器无调度任务时会阻塞idle协程上，对IO调度器而言，idle状态应该关注两件事，一是有没有新的调度任务，对应Schduler::schedule()，
 * 如果有新的调度任务，那应该立即退出idle状态，并执行对应的任务；二是关注当前注册的所有IO事件有没有触发，如果有触发，那么应该执行
 * IO事件对应的回调函数
 */
void IOManager::idle() {
    SYLAR_LOG_DEBUG(g_logger) << "IOManager::idle";

    //一次epoll_wait最多检测256个就绪事件，如果就绪事件超过了这个数，那么会在下轮epoll_wait继续处理
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS];
    //对于申请的动态数组，shared_ptr指针默认的释放规则是不支持释放数组的，
    //只能自定义对应的释放规则，才能正确地释放申请的堆内存，
    //当堆内存的引用计数为 0 时，会优先调用我们自定义的释放规则
    //第一个参数events是要进行管理的指针；第二个参数是指定的自定义析构函数
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {
        delete[] ptr;
    });
    //不能用匿名对象，匿名对象的生存空间就是代码所在的行，一旦执行完该行就被释放了
    // std::shared_ptr<epoll_event>(events, [](epoll_event* ptr) {   
    //     delete[] ptr;
    // });

    while(true) {
        uint64_t next_timeout = 0;
        if(stopping(next_timeout)) {
            SYLAR_LOG_INFO(g_logger) << "name=" << getName()
                                    << " idle stopping exit";
            break;
        }

        //阻塞在epoll_wait上，等待事件发生，返回事件的数目，并将触发的事件写入events数组中
        //判断就绪链表有无数据，有数据就返回，没有数据就sleep，等到timeout时间到后即使链表没数据也返回
        int rt = 0;
        do {
            const uint64_t MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            }
            else {
                next_timeout = MAX_TIMEOUT;
            }
            //epoll_wait函数的阻塞与在其队列中socket是否为阻塞没有关系
            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);  //超时返回0
            if(rt < 0) {
                //函数调用被信号处理函数中断，这些情况并不作为错误
                //解决方法：重新定义系统调用，忽略错误码为EINTR的情况
                if(errno == EINTR) {
                    continue;
                }
                SYLAR_LOG_ERROR(g_logger) << "epoll_wait(" << m_epfd << ") (rt="
                    << rt << ") (" << errno << ") (" << strerror(errno) << ") ";
                return;
            }
            else {
                break;
            }
            
        } while(true);

        std::vector<std::function<void()> > cbs;
        listTimeoutCbs(cbs);
        if(!cbs.empty()) {
            //SYLAR_LOG_DEBUG(g_logger) << "cbs.size()=" << cbs.size();
            //SYLAR_LOG_DEBUG(g_logger) << "rt=" << rt;
            scheduler(cbs.begin(), cbs.end());
            cbs.clear();
        }

        //遍历所有发生的事件，根据epoll_event的私有指针找到对应的FdContext，进行事件处理
        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            //ticklefd[0]用于通知协程调度，这时只需要把管道里的内容读完即可，本轮idle结束Scheduler::run会重新执行协程调度
            if(event.data.fd == m_tickleFds[0]) {
                //SYLAR_LOG_DEBUG(g_logger) << "rt=" << rt;
                //SYLAR_LOG_INFO(g_logger) << "---------------- rt = " << rt << " ----------------";
                /**
                 * @brief ssize_t read(int fd, void * buf, size_t count);
                 * @details read()会把参数fd所指的文件传送count个字节到buf指针所指的内存中
                 * @return 返回实际读取到的字节数，如果返回0，表示已到达文件尾或是无可读取的数据
                 */
                char dummy[256];
                //如果m_tickleFds[0]描述符不是非阻塞的，那这个事件一直读或一直写势必会在最后一次阻塞(因为读到没数据后就会阻塞)
                //而设置成非阻塞的，会一直读，直到没数据后会返回-1，不会阻塞，并将errno设置为EWOULDBLOCK（或者EAGAIN，是等价的）
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);  
                continue;
            }

            //通过epoll_event的私有指针获取FdContext
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            /**
             * EPOLLERR: 出错，比如写读端已经关闭的pipe
             * EPOLLHUP: 套接字对端关闭
             * 出现这两种事件，应该同时触发fd_ctx的读和写事件，否则有可能出现注册的事件永远执行不到的情况
             */
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }

            int real_events = NONE;    //真正要执行的事件
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if((fd_ctx->events & real_events) == NONE) {   //没有要执行的事件
                continue;
            }

            //剔除已经发生的事件，将剩下的事件重新加入epoll_wait，
            //如果剩下的事件为0，表示这个fd已经不需要关注了，直接从epoll中删除
            int left_events = fd_ctx->events & ~real_events;
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events=" 
                    << (EPOLL_EVENTS)fd_ctx->events;
                continue;
            }

            //处理已经发生的事件，也就是让调度器调度指定的函数或协程
            if(real_events & READ) {
                fd_ctx->triggerEvent(READ); //triggerEvent()内会处理fd_ctx的events和EventContext
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        } //end for

        //一旦处理完所有的事件，idle协程yield，这样可以让调度协程(Scheduler::run)重新检查是否有新任务要调度
        //上面triggerEvent实际也只是把对应的cb或fiber重新加入调度，要执行的话还要等idle协程退出
        Fiber::ptr curr = Fiber::GetThis();
        Fiber* temp = curr.get();    //用普通指针代替智能指针
        curr.reset();    //释放curr智能指针指向的空间,将指向空间的引用计数-1
        temp->swapOut();    //返回到调度协程

        //用YieldToHold()也可以，会完全执行完
        // Fiber::YieldToHold();

    } //end while(true)
}

void IOManager::newTimerInsertAtFront() {
    tickle();
}

/**
 * @brief 重置socket句柄上下文的容器大小
 * @details m_fdContexts容器膨胀时不会覆盖掉之前的值
 * @param[in] size 容量大小
 */
void IOManager::fdContextsResize(size_t size) {
    m_fdContexts.resize(size);
    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            //直接使用fd的值作为FdContext数组的下标，可以快速找到一个fd对应的FdContext
            m_fdContexts[i]->fd = i;
        }
    }
}


}