#ifndef __SYLAR_INMANAGER_H__
#define __SYLAR_INMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace sylar {

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    /**
     * @brief IO事件，继承自epoll对事件的定义
     * @details 这里只关心socket fd的读和写事件，其他epoll事件会归类到这两类事件中
     */
    enum Event {
        NONE  = 0x0,   // 无事件
        READ  = 0x1,   // 读事件(EPOLLIN)
        WRITE = 0x4    // 写事件(EPOLLOUT)
    };

private:
    /**
     * @brief socket事件上下文类（描述符-事件类型-回调函数三元组）
     * @details 每个socket fd都对应一个FdContext，包括fd的值，fd上的事件，以及fd的读写事件上下文
    */
    struct FdContext {
        typedef Mutex MutexType;

        /**
         * @brief 事件上下文
         * @details fd的每个事件都有一个事件上下文，保存这个事件的回调函数以及执行回调函数的调度器
         *          sylar对fd事件做了简化，只预留了读事件和写事件，所有的事件都被归类到这两类事件中
         */
        struct EventContext {
            Scheduler* scheduler = nullptr;  //事件执行的调度器
            Fiber::ptr fiber;     //事件回调协程
            std::function<void()> cb;    //事件回调函数
        };

        //获取事件上下文
        EventContext& getEventContext(Event event);

        //重置事件上下文
        void resetEventContext(EventContext& ctx);

        /**
         * @brief 触发事件
         * @details 根据事件类型调用对应上下文结构中的调度器去调度回调协程或回调函数
         * @param[in] event 事件类型
         */
        void triggerEvent(Event event);

        //由于fd有可读和可写两种事件，每种事件的回调函数也可以不一样，所以每个fd都需要保存两个事件类型-回调函数组合
        EventContext read;   //读事件上下文
        EventContext write;  //写事件上下文
        int fd = 0;      //事件关联的句柄
        Event events = NONE;  //该fd添加了哪些事件的回调函数
        MutexType mutex;
    };

public:
    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否将调用线程包含进去
     * @param[in] name 调度器的名称
     */
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    //析构函数
    ~IOManager();

    /**
     * @brief 添加事件
     * @details fd描述符发生了event事件时执行cb函数
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @param[in] cb 事件回调函数，如果为空，则默认把当前协程作为回调执行体
     * @return 添加成功返回0，失败返回-1
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /**
     * @brief 删除事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @attention 不会触发事件
     * @return 是否删除成功
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief 取消事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @attention 如果事件存在则触发事件
     * @return 是否取消成功
     */
    bool cancelEvent(int fd, Event event);

    /**
     * @brief 取消所有事件
     * @param[in] fd socket句柄
     * @attention 所有事件都会被触发一次
     * @return 是否取消成功
     */
    bool cancelAllEvent(int fd);

    //返回当前的IOManager
    static IOManager* GetThis();

protected:
    /**
     * @brief 通知调度器有任务要调度
     * @details 写pipe让idle协程从epoll_wait退出，待idle协程yield后Scheduler::run就可以调度其他任务
     *          如果当前没有空闲调度线程，那就没必要发通知
     */
    void tickle() override;

    //判断调度器是否已经停止
    bool stopping() override;

    /**
     * @brief idle协程
     * @details 对于IO协程调度来说，应阻塞在等待IO事件上，idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件就绪
     * 调度器无调度任务时会阻塞idle协程上，对IO调度器而言，idle状态应该关注两件事，一是有没有新的调度任务，对应Schduler::schedule()，
     * 如果有新的调度任务，那应该立即退出idle状态，并执行对应的任务；二是关注当前注册的所有IO事件有没有触发，如果有触发，那么应该执行
     * IO事件对应的回调函数
     */
    void idle() override;

    //当有新的定时器插入到定时器的首部,执行该函数
    void newTimerInsertAtFront() override;

    /**
     * @brief 判断调度器是否已经停止
     * @param[out] timeout 最近要出发的定时器事件间隔
     */
    bool stopping(uint64_t& timeout);

    /**
     * @brief 重置socket句柄上下文的容器大小
     * @param[in] size 容量大小
     */
    void fdContextsResize(size_t size);

private: 
    int m_epfd = 0;      //epoll文件句柄
    int m_tickleFds[2];  //pipe文件句柄，fd[0]读端，fd[1]写端
    std::atomic<size_t> m_pendingEventCount = {0};  //当前等待执行的事件数量
    std::vector<FdContext*> m_fdContexts;  //socket句柄上下文的容器
    RWMutexType m_mutex;
};


}

#endif