#include "timer.h"
#include "util.h"

namespace sylar {

bool Timer::Comparator::operator()(const Timer::ptr& left
                            ,const Timer::ptr& right) const {
    if(!left && !right) {
        return false;
    }
    if(!left) {
        return true;
    }
    if(!right) {
        return false;
    }
    if(left->m_next < right->m_next) {
        return true;
    }
    if(left->m_next > right->m_next) {
        return false;
    }
    return left.get() < right.get();
}

/**
 * @brief 构造函数
 * @param[in] ms 定时器执行间隔时间
 * @param[in] recurring 是否循环，每隔ms执行一次
 * @param[in] cb 回调函数
 * @param[in] manager 定时器管理器
 */
Timer::Timer(uint64_t ms, bool recurring, std::function<void()> cb, TimerManager* manager) 
    :m_ms(ms)
    ,m_recurring(recurring)
    ,m_cb(cb)
    ,m_manager(manager) {
    m_next = sylar::GetCurrentMS() + m_ms;  //下次执行时间 = 当前时间 + 执行时间间隔
}

/**
 * @brief 构造函数
 * @param[in] next 执行的时间戳(毫秒)
 */
Timer::Timer(uint64_t next) 
    :m_next(next) {
}

//取消定时器
bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()) {
            return false;
        }
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

/**
 * @brief 刷新设置定时器的执行时间
 * @details 定时器的ms不变，只是从当前时间重新开始定时
 */
bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()) {
            return false;
        }
        //不能直接修改m_next，是因为set按照m_next来排序，直接修改会影响整个数据结构(set不可以修改key值)
        //应该先删除
        m_manager->m_timers.erase(it);
        //再修改时间
        m_next = m_ms + sylar::GetCurrentMS();
        //最后再添加回去
        //insert函数在插入元素成功时返回一个pair对象，其中包含两部分：
        //pair::first：迭代器，指向已插入元素在容器中的位置（或者是一个已经存在的具有相同键的元素）
        //pair::second：布尔值，表示是否插入成功。如插入成功为true；如果已经存在具有相同键的元素，则为false
        it = m_manager->m_timers.insert(shared_from_this()).first;  
        bool at_front = (it == m_manager->m_timers.begin()) && !m_manager->m_tickled;
        if(at_front) {
            m_manager->m_tickled = true;  //只触发一次
        }
        lock.unlock();

        if(at_front) {
            m_manager->newTimerInsertAtFront();
        }
        return true;
    }
    return false;
}

/**
 * @brief 重置定时器时间
 * @param[in] ms 定时器执行间隔时间(毫秒)
 * @param[in] from_now 是否从当前时间开始计算
 */
bool Timer::reset(uint64_t ms, bool from_now) {
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()) {
            return false;
        }
        //应该先删除
        m_manager->m_timers.erase(it);
        //再修改时间
        uint64_t start = 0;
        if(from_now) {
            start = sylar::GetCurrentMS();
        }
        else {
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;
        //最后再添加回去
        it = m_manager->m_timers.insert(shared_from_this()).first;
        bool at_front = (it == m_manager->m_timers.begin()) && !m_manager->m_tickled;
        if(at_front) {
            m_manager->m_tickled = true;  //只触发一次
        }
        lock.unlock();

        if(at_front) {
            m_manager->newTimerInsertAtFront();
        }
        return true;
    }
    return false;

}


//构造函数
TimerManager::TimerManager() {
    m_previouseTime = sylar::GetCurrentMS();
}

//析构函数
TimerManager::~TimerManager() {

}

/**
 * @brief 添加定时器
 * @param[in] ms 定时器执行间隔时间
 * @param[in] cb 定时器回调函数
 * @param[in] recurring 是否循环定时器
 */
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
    //在friend中使用private构造函数，可以用new，但不能用make_shared，因为make_shared不是Timer的friend
    // Timer::ptr timer = std::make_shared<Timer>(ms, recurring, cb, this);
    Timer::ptr timer(new Timer(ms, recurring, cb, this));
    RWMutexType::WriteLock lock(m_mutex);
    auto it = m_timers.insert(timer).first;
    //使用m_tickled，在发生频繁添加时，不用频繁触发newTimerInsertAtFront
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if(at_front) {
        m_tickled = true;  //只触发一次
    }
    lock.unlock();

    if(at_front) {
        newTimerInsertAtFront();
    }
    return timer;
}


//条件定时器中带条件的回调函数，只有满足条件时才执行cb()
static void ConditionTimerCb(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    //weak_ptr的成员函数lock():返回一个shared_ptr类型的指针
    //若weak_cond指针为空，或者指向的堆内存已经被释放，会返回一个空的shared_ptr指针
    std::shared_ptr<void> temp = weak_cond.lock();
    if(temp) {
        cb();
    }
}
/**
 * @brief 添加条件定时器，到定时时间还需满足条件才能执行回调函数
 * @param[in] ms 定时器执行间隔时间
 * @param[in] cb 定时器回调函数
 * @param[in] weak_cond 条件
 * @param[in] recurring 是否循环定时器
 */
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
                            ,std::weak_ptr<void> weak_cond, bool recurring) {
    return addTimer(ms, std::bind(&ConditionTimerCb, weak_cond, cb), recurring);
}

//得到最近一个定时器执行的时间间隔(毫秒)
uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()) {
        //uint64_t的最大值
        return ~0ull;   //u表示unsigned无符号，l表示long长整数
    }

    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = sylar::GetCurrentMS();
    if(next->m_next <= now_ms) {
        return 0;
    }
    else {
        return next->m_next - now_ms;
    }
}

/**
 * @brief 获取需要执行的定时器的回调函数列表(即获取所有已超时定时器对应的回调函数)
 * @param[out] cbs 回调函数数组(引用:输出)
 */
void TimerManager::listTimeoutCbs(std::vector<std::function<void()> >& cbs) {
    uint64_t now_ms = sylar::GetCurrentMS();
    std::vector<Timer::ptr> timeout;

    RWMutexType::WriteLock lock(m_mutex);
    if(m_timers.empty()) {
        return;
    }
    //检查一下系统时间有没有被调后，若系统时间调后了1个小时以上，那就触发全部定时器
    bool rollover = detectClockRollover(now_ms);
    if(!rollover && (*m_timers.begin())->m_next > now_ms) {
        return;
    }

    //在friend中使用private构造函数，可以用new，但不能用make_shared
    //Timer::ptr now_timer = std::make_shared<Timer>(now_ms);
    Timer::ptr now_timer(new Timer(now_ms));
    //upper_bound()函数用于在指定区域内查找大于目标值的第一个元素
    //该函数仅适用于已排好序的序列，已排好序指的是[first, last)区域内所有成立的元素都位于不成立元素的前面
    auto it = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);
    timeout.insert(timeout.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(timeout.size());

    for(auto& timer: timeout) {
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        }
        else {
            timer->m_cb = nullptr;  //防止回调函数中有用到智能指针，不置空的话永远不会减1
        }
    }
}

//是否还有定时器
bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

//检测服务器时间是否被调后了
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if(now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previouseTime = now_ms;
    return rollover;
}

}