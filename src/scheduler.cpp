#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"


namespace sylar {


static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;    //当前协程调度器
//调度协程:use_caller为true时,与主协程不一样,属于主协程的子协程
//        use_caller为false时,调度协程与主协程一样
//加上Fiber模块的t_fiber和t_thread_fiber，每个线程总共可以记录三个协程的上下文信息
static thread_local Fiber* t_scheduler_fiber = nullptr;  //当前线程的调度协程


Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) 
    :m_name(name) {
    SYLAR_ASSERT(threads > 0);

    if(use_caller) {    //把创建协程调度器的线程放到协程调度器管理的线程池中
        --threads;
        SYLAR_ASSERT(GetThis() == nullptr);
        SetThis(this);    //在之后用到派生类IOManager::GetThis()时，用来实现多态
        Thread::setCurrThreadName(name);

        Fiber::GetThis();    //创建主协程
        //caller线程的调度协程不会被调度器调度，而且caller线程的调度协程停止时，应该返回caller线程的主协程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));  //创建调度协程
        t_scheduler_fiber = m_rootFiber.get();

        m_rootThread = getThreadId();
        m_threadIds.push_back(m_rootThread);
    }
    else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}   

//析构函数
Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if(GetThis() == this) {
        SetThis(nullptr);
    }
}

//设置当前的协程调度器
void Scheduler::SetThis(Scheduler* s) {
    t_scheduler = s;
}

//返回当前协程调度器
Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

//返回当前协程调度器的调度协程
Fiber* Scheduler::GetSchedulerFiber() {
    return t_scheduler_fiber;
}

//启动协程调度器(初始化调度线程池)
//如果只使用caller线程进行调度，那start啥也不做
void Scheduler::start() {
    SYLAR_LOG_DEBUG(g_logger) << "start";
    MutexType::Lock lock(m_mutex);
    if(m_stopping) {
        SYLAR_LOG_INFO(g_logger) << "Scheduler is stopped";
        return;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), 
                                    m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

//停止协程调度器
void Scheduler::stop() {
    SYLAR_LOG_DEBUG(g_logger) << "stop";
    if(stopping()) {
        return;
    }

    //如果use caller，那只能由caller线程发起stop
    if(m_rootFiber) {
        SYLAR_ASSERT(GetThis() == this);
    }
    else {
        SYLAR_ASSERT(GetThis() != this);  // GetThis() == nullptr
    }
    m_stopping = true;
    
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }
    if(m_rootFiber) {
        tickle();
    }

    //在use caller情况下，调度协程结束时，应该返回caller主协程
    if(m_rootFiber) {
        if(!stopping()) {
            m_rootFiber->swapIn();  //caller线程的调度协程开始执行
        }
    }
    
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for(auto& i : thrs) {
        i->join();
    }
}

// 流式输出
std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " stopping=" << m_stopping
       << " ]" << std::endl << "    ";
    for(size_t i = 0; i < m_threadIds.size(); ++i) {
        if(i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

/**
 * @bug 加入sylar::set_hook_enable(true)后，测试scheduler_test：
 *              sleep(1);
 *              sylar::Scheduler::GetThis()->scheduler(&task);
 *      出现段错误core dumped，因为改了sleep函数，
 *      复习完hook，记得回头复盘
*/
//协程调度函数(调度协程的实现)
void Scheduler::run() {
    SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
    sylar::set_hook_enable(true);
    SetThis(this);    //在之后用到派生类IOManager::GetThis()时，用来实现多态
    if(getThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();  //创建主协程和调度协程(两者一样)
    }

    // bind(&Scheduler::idle, this) 等价于 this->idle()
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));  //创建idle协程
    Fiber::ptr cb_fiber;
    FiberAndThread task;
    while(true) {
        task.reset();
        bool tickle_me = false;    //是否通知协程调度器还有任务要执行
        {   
            MutexType::Lock lock(m_mutex);
            // 遍历所有调度任务
            for(auto it = m_fibers.begin(); it != m_fibers.end(); ++it) {
                //该任务指定了具体的线程执行，并且不是当前线程
                if(it->threadId != -1 && it->threadId != getThreadId()) {
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                //任务队列的协程一定只能是INIT、READY、HOLD状态
                if(it->fiber) {
                    auto temp = it->fiber->getState();
                    SYLAR_ASSERT(temp == Fiber::INIT 
                                || temp == Fiber::READY
                                || temp == Fiber::HOLD);
                }

                //当前调度线程找到一个任务，准备开始调度，将其从任务队列中剔除，活动线程数加1
                task = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                break;
            }
            //当前线程拿完一个任务后，发现任务队列还有剩余，那么tickle一下其他线程
            tickle_me = (!m_fibers.empty()) || tickle_me;
        }

        if(tickle_me) {
            tickle();
        }

        if(task.fiber) {
            //swapIn协程，当返回时，协程要么已执行完，要么半路yield，总之任务完成，活跃线程数减1
            task.fiber->swapIn();
            --m_activeThreadCount;

            //如果是半路yield，有两种情况：(1)YieldToReady，则调度器把它重新加入到任务队列并等待调度
            //(2)YieldToHold，不会再将协程加入任务队列，协程在yield之前必须自己先将自己加入到协程的调度队列中，否则协程就处于逃逸状态
            if(task.fiber->getState() == Fiber::READY) {
                scheduler(task.fiber);
            } else if(task.fiber->getState() != Fiber::TERM 
                    && task.fiber->getState() != Fiber::EXCEPT) {   //意义不大,可删
                task.fiber->m_state = Fiber::HOLD;            
            }
            task.reset();
        }
        else if(task.cb) {
            //将执行函数封装成协程，再像上面task.fiber一样执行调度
            if(cb_fiber) {
                cb_fiber->reset(task.cb);
            } 
            else {
                cb_fiber.reset(new Fiber(task.cb));
            }
            task.reset();    //task已经封装成协程，可以调用其成员函数置空
            cb_fiber->swapIn();
            --m_activeThreadCount;

            if(cb_fiber->getState() == Fiber::READY) {
                scheduler(cb_fiber);
            } else if(cb_fiber->getState() != Fiber::TERM 
                    && cb_fiber->getState() != Fiber::EXCEPT) {   //意义不大,可删
                cb_fiber->m_state = Fiber::HOLD;
            }
            cb_fiber.reset();    //析构cb_fiber
        }
        else {   //进到这个分支情况一定是任务队列为空，调度idle协程即可
            //如果调度器没有任务，那么idle协程会不停地swapIn/yield等待新任务，不会结束
            //如果idle协程结束了，那一定是调度器停止了(调度器执行了stop)
            if(idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;

            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {   //意义不大,可删
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    } //end while(true)
}

//通知协程调度器有任务要执行
void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "Scheduler::tickle";
}

//判断调度器是否已经停止，只有当所有的任务都被执行完，调度器才可以停止
bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

//协程无任务可调度时,执行idle协程,等待新任务到来
void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "Scheduler::idle";
    while(!stopping()) {
        Fiber::YieldToHold();  //当调度器不停止且没任务时,一直来回执行idle协程
    }
}



}