#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <list>
#include <atomic>
#include "thread.h"
#include "fiber.h"

namespace sylar {

/**
 * @brief 协程调度器
 * @details 封装的是N-M的协程调度器(N个线程运行M个协程)
 *          内部有一个线程池,支持协程在线程池里面切换
 */
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否使用caller线程(当前线程)用作调度
     * @param[in] name 协程调度器名称
    */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    //析构函数
    virtual ~Scheduler();

    //返回协程调度器名称
    const std::string& getName() const { return m_name; }

    //设置当前的协程调度器
    static void SetThis(Scheduler* s);

    //返回当前协程调度器
    static Scheduler* GetThis();

    //返回当前协程调度器的调度协程
    static Fiber* GetSchedulerFiber();

    //启动协程调度器(初始化调度线程池)
    void start();

    //停止协程调度器
    void stop();

    // 流式输出
    std::ostream& dump(std::ostream& os);

    /**
     * @brief 添加任务,开始调度协程
     * @param[in] fc 协程或执行函数来充当任务
     * @param[in] thread 指定执行的线程id, -1表示任意线程
     */
    template<class FiberOrCb>
    void scheduler(FiberOrCb fc, size_t thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = schedulerNolock(fc, thread);
        }

        if(need_tickle) {
            tickle();
        }
    }

    /**
     * @brief 按照迭代器批量添加任务,开始调度协程
     * @param[in] begin 任务数组的开始
     * @param[in] end 任务数组的结束
     */
    template<class InputIterator>
    void scheduler(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            for(auto it = begin; it != end; ++it) {
                need_tickle = schedulerNolock(*it, -1) || need_tickle;
            }
        }

        if(need_tickle) {
            tickle();
        }
    }

private:
    //添加任务(无锁)
    template<class FiberOrCb>
    bool schedulerNolock(FiberOrCb fc, size_t thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

protected:
    //协程调度函数(调度协程的实现)
    void run();

    //通知协程调度器有任务要执行
    virtual void tickle();

    //判断调度器是否已经停止
    virtual bool stopping();

    //协程无任务可调度时,执行idle协程,等待新任务到来
    virtual void idle();

    //是否有空闲线程
    bool hasIdleThreads() { return m_idleThreadCount > 0; }

private:
    /**
     * @brief 调度任务，协程/函数二选一，可指定在哪个线程上调度
     * @details 用协程、执行函数来充当任务
     *          同时可以用threadId来指定在某个具体的线程上执行
    */
    struct FiberAndThread {
        Fiber::ptr fiber;    //协程
        std::function<void()> cb;  //协程执行函数
        int threadId;    //线程id

        /**
         * @brief 构造函数
         * @param[in] f 协程
         * @param[in] thrId 线程id
        */
        FiberAndThread(Fiber::ptr f, uint64_t thrId) 
            :fiber(f), threadId(thrId) {
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程指针
         * @param[in] thrId 线程id
         * @post *f = nullptr
        */
        FiberAndThread(Fiber::ptr* f, uint64_t thrId) 
            :threadId(thrId) {
            fiber.swap(*f);
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数
         * @param[in] thrId 线程id
        */
        FiberAndThread(std::function<void()> f, uint64_t thrId) 
            :cb(f), threadId(thrId) {
        }

        /**
         * @brief 构造函数
         * @param[in] f 协程执行函数指针
         * @param[in] thrId 线程id
         * @post *c = nullptr
        */
        FiberAndThread(std::function<void()>* f, uint64_t thrId) 
            :threadId(thrId) {
            cb.swap(*f);
        }

        //无参构造函数
        FiberAndThread() 
            :threadId(-1) {
        }

        //重置数据
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            threadId = -1;
        }
    };

private: 
    MutexType m_mutex;    //互斥锁
    std::string m_name;   //协程调度器名称
    std::vector<Thread::ptr> m_threads;   //线程池
    std::list<FiberAndThread> m_fibers;   //待执行的任务队列
    Fiber::ptr m_rootFiber;  //use_caller为true时有效,表示调度协程(不是主协程)
protected:
    std::vector<uint64_t> m_threadIds;  //线程id数组
    size_t m_threadCount = 0;   //线程数量
    std::atomic<size_t> m_activeThreadCount = {0};  //工作线程数量
    std::atomic<size_t> m_idleThreadCount = {0};  //空闲线程数量
    bool m_stopping = false;   //是否正在停止
    bool m_autoStop = false;   //是否自动停止
    int m_rootThread = 0;  //use_caller为true时,caller线程id
};  


}











#endif