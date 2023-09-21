#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <functional>
#include <vector>
#include <set>
#include "thread.h"

namespace sylar {

class TimerManager;
//定时器类
class Timer : public std::enable_shared_from_this<Timer> {
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;

    //取消定时器
    bool cancel();

    /**
     * @brief 刷新设置定时器的执行时间
     * @details 定时器的ms不变，只是从当前时间重新开始定时
     */
    bool refresh();

    /**
     * @brief 重置定时器时间
     * @param[in] ms 定时器执行间隔时间(毫秒)
     * @param[in] from_now 是否从当前时间开始计算
     */
    bool reset(uint64_t ms, bool from_now);

private:
    /**
     * @brief 构造函数
     * @param[in] ms 定时器执行间隔时间
     * @param[in] recurring 是否循环，每隔ms执行一次
     * @param[in] cb 回调函数
     * @param[in] manager 定时器管理器
     */
    Timer(uint64_t ms, bool recurring, std::function<void()> cb, TimerManager* manager);

    /**
     * @brief 构造函数
     * @param[in] next 执行的时间戳(毫秒)
     */
    Timer(uint64_t next);

private:
    uint64_t m_ms = 0;          //执行周期(执行间隔)
    uint64_t m_next = 0;        //精确的执行时间，下一次执行的时间
    bool m_recurring = false;   //是否循环定时器
    std::function<void()> m_cb = nullptr;  //回调函数
    TimerManager* m_manager = nullptr;     //定时器管理器

private:
    //定时器比较仿函数
    struct Comparator {
        /**
         * @brief 比较定时器的智能指针的大小(按执行时间排序)
         * @param[in] p1 定时器智能指针
         * @param[in] p2 定时器智能指针
         */
        bool operator()(const Timer::ptr& p1, const Timer::ptr& p2) const;
    };
};

//定时器管理类
class TimerManager {
friend class Timer;
public:
    typedef RWMutex RWMutexType;

    //构造函数
    TimerManager();

    //析构函数
    virtual ~TimerManager();

    /**
     * @brief 添加定时器
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 定时器回调函数
     * @param[in] recurring 是否循环定时器
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    /**
     * @brief 添加条件定时器
     * @param[in] ms 定时器执行间隔时间
     * @param[in] cb 定时器回调函数
     * @param[in] weak_cond 条件
     * @param[in] recurring 是否循环定时器
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, 
                                std::weak_ptr<void> weak_cond, bool recurring = false);

    //得到最近一个定时器执行的时间间隔(毫秒)
    uint64_t getNextTimer();

    /**
     * @brief 获取需要执行的定时器的回调函数列表(即获取所有已超时定时器对应的回调函数)
     * @param[out] cbs 回调函数数组(引用:输出)
     */
    void listTimeoutCbs(std::vector<std::function<void()> >& cbs);

    //是否还有定时器
    bool hasTimer();

protected:
    //检测到新添加的定时器的超时时间比当前最小的定时器还要小时，
    //TimerManager通过这个方法来通知IOManager立刻更新当前的epoll_wait超时，
    //否则新添加的定时器的执行时间将不准确。
    //当有新的定时器插入到定时器的首部,执行该函数
    virtual void newTimerInsertAtFront() = 0;

private:
    //检测服务器时间是否被调后了
    bool detectClockRollover(uint64_t now_ms);

private:
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_timers;  //定时器集合
    bool m_tickled = false;        //是否触发onTimerInsertedAtFront
    uint64_t m_previouseTime = 0;  //上次执行时间
};

}

#endif