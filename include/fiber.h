#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "log.h"

namespace sylar {

class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    //协程状态
    enum State {
        INIT,    //初始化状态
        HOLD,    //暂停状态
        EXEC,    //执行中状态
        TERM,    //结束状态
        READY,   //可执行状态
        EXCEPT   //异常状态
    };

private:
    //无参构造,放在private中,只能类内使用
    Fiber();

public:
    /**
     * @brief 有参构造函数
     * @param[in] cb 协程执行的函数
     * @param[in] stacksize 协程栈大小
     * @param[in] runInScheduler 记录该协程是否通过调度器来运行
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool runInScheduler = true);

    //析构函数
    ~Fiber();

    /**
     * @brief 重置协程执行函数,并设置状态,重复利用已结束的协程,复用其栈空间
     * @pre 只有在状态为 INIT, TERM, EXCEPT 时才可以重置
     * @post 重置后状态为 INIT
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     * @pre 只有在状态不为 EXEC 时才可以swapIn
     * @post 切换后状态为 EXEC
     */
    void swapIn();

    //将当前协程切换到后台
    void swapOut();

    // void call();

    // void back();

    //返回协程id
    uint64_t getId() const { return m_id; }

    //返回协程状态
    State getState() const { return m_state; }

public:
    //设置当前线程的运行协程
    static void SetThis(Fiber* f);

    //返回当前协程,如果没有,则创建主协程返回
    static Fiber::ptr GetThis();

    //将当前协程切换到后台,并设置为READY状态
    static void YieldToReady();

    //将当前协程切换到后台,并设置为HOLD状态
    static void YieldToHold();

    //返回当前协程的总数量
    static uint64_t TotalFibers();

    //协程执行函数
    static void MainFunc();

    //支持使用caller线程进行调度时的协程执行函数
    // static void CallerMainFunc();

    //获取当前协程id
    static uint64_t GetFiberId();

private:
    uint64_t m_id = 0;            //协程id
    uint32_t m_stacksize = 0;     //协程运行栈大小
    State m_state = INIT;         //协程状态
    std::function<void()> m_cb;   //协程运行函数
    ucontext_t m_ctx;             //协程上下文
    void* m_stack = nullptr;      //协程运行栈指针
    bool m_runInScheduler = true; //记录该协程是否通过调度器来运行

};



}


#endif