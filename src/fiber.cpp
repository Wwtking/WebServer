#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "util.h"
#include "scheduler.h"
#include <atomic>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id(0);      //协程id
static std::atomic<uint64_t> s_fiber_count(0);   //总的协程数

static thread_local Fiber* t_fiber = nullptr;            //表示当前协程
static thread_local Fiber::ptr t_threadFiber = nullptr;  //表示主协程

//Yaml配置协程运行栈的大小
static sylar::ConfigVar<uint32_t>::ptr g_fiber_stacksize = 
    sylar::Config::Lookup<uint32_t>("fiber.stacksize", 128 * 1024, "fiber stack size");


//内存分配模版类(malloc)
class MallocStackAllocator {
public:
    //分配内存
    static void* Alloc(size_t size) {
        return (void*)malloc(size);
    }

    //释放内存
    static void DealAlloc(void* ptr, size_t size) {
        free(ptr);
        return;
    }
};

//内存分配模版类(mmap)
class MMapStackAllocator {

};

//切换两种内存分配方式
using StackAllocator = MallocStackAllocator;
//using StackAllocator = MMapStackAllocator;


//无参构造,放在private中,只能类内使用
//用于创建主协程
Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);   //设置当前线程的运行协程

    //初始化m_ctx,将当前上下文保存在m_ctx中,成功返回0
    if(getcontext(&m_ctx)) {  
        SYLAR_ASSERT2(false, "getcontext error");
    }

    ++s_fiber_count;

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}


/**
 * @brief 有参构造函数(用于创建子协程)
 * @param[in] cb 协程执行的函数
 * @param[in] stacksize 协程栈大小
 * @param[in] runInScheduler 记录该协程是否通过调度器来运行
 */
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool runInScheduler) 
    :m_id(++s_fiber_id) 
    ,m_cb(cb)
    ,m_runInScheduler(runInScheduler) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stacksize->getValue();
    m_stack = StackAllocator::Alloc(m_stacksize);  //栈用于当上下文切换时保存现场

    //初始化m_ctx,将当前上下文保存在m_ctx中,成功返回0
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext error");
    }
    //m_ctx.uc_link = &t_threadFiber->m_ctx;  //当前协程结束后返回到主协程
    m_ctx.uc_link = nullptr;  //设置当前context执行结束之后要执行的下一个context
    m_ctx.uc_stack.ss_sp = m_stack;         //指定了m_ctx所用栈的位置
    m_ctx.uc_stack.ss_size = m_stacksize;   //指定了m_ctx所用栈的大小

    //绑定切换到m_ctx上下文时的执行函数和函数的参数,不是立即进入MainFunc,需要切换到该上下文时才进入
    // if(m_runInScheduler) {
    //     makecontext(&m_ctx, &Fiber::MainFunc, 0);
    // }
    // else {
    //     makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    // }
    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber  id=" << m_id;
}

//析构函数
Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) {
        SYLAR_ASSERT(m_state == TERM || m_state == INIT ||m_state == EXCEPT);
        StackAllocator::DealAlloc(m_stack, m_stacksize);
    }
    else {    //没有子协程,只析构主协程
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);
        if(t_fiber == this) {
            SetThis(nullptr);
        }
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber  id=" << m_id;
}

/**
 * @brief 重置协程执行函数,并设置状态,重复利用已结束的协程,复用其栈空间
 * @pre 只有在状态为 INIT, TERM, EXCEPT 时才可以重置
 * @post 重置后状态为 INIT
 */
void Fiber::reset(std::function<void()> cb) {
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM || m_state == INIT ||m_state == EXCEPT);
    m_cb = cb;

    //初始化m_ctx,将当前上下文保存在m_ctx中,成功返回0
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext error");
    }
    //m_ctx.uc_link = &t_threadFiber->m_ctx;
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    //绑定切换到m_ctx上下文时的执行函数和函数的参数
    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    m_state = INIT;
}

/**
 * @brief 将当前协程切换到运行状态,调度协程切换到后台
 * @pre 只有在状态不为 EXEC 时才可以切换
 * @post 切换后状态为 EXEC
 */
// void Fiber::swapIn() {
//     SetThis(this);
//     SYLAR_ASSERT(m_state != EXEC);
//     m_state = EXEC;
//     //保存当前上下文到第一个参数,然后切换到第二个参数上下文
//     if(swapcontext(&Scheduler::GetSchedulerFiber()->m_ctx, &m_ctx)) {
//         SYLAR_ASSERT2(false, "swapcontext error");
//     }
// }

// //将当前协程切换到后台,切换回主协程
// void Fiber::swapOut() {
//     SetThis(t_threadFiber.get());
//     //保存当前上下文到第一个参数,然后切换到第二个参数上下文
//     if(swapcontext(&m_ctx, &Scheduler::GetSchedulerFiber()->m_ctx)) {
//         SYLAR_ASSERT2(false, "swapcontext error");
//     }
// }

// void Fiber::call() {
//     SetThis(this);
//     SYLAR_ASSERT(m_state != EXEC);
//     m_state = EXEC;
//     //保存当前上下文到第一个参数,然后切换到第二个参数上下文
//     if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
//         SYLAR_ASSERT2(false, "swapcontext error");
//     }
// }

// void Fiber::back() {
//     SetThis(t_threadFiber.get());
//     //保存当前上下文到第一个参数,然后切换到第二个参数上下文
//     if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
//         SYLAR_ASSERT2(false, "swapcontext error");
//     }
// }

/**
 * @brief 将当前协程切换到运行状态，将 主协程/调度协程 切换到后台
 * @pre 只有在状态不为 EXEC 时才可以切换
 * @post 切换后状态为 EXEC
 */
void Fiber::swapIn() {
    SetThis(this);    //设置当前协程为子协程
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(m_runInScheduler) {
        //保存当前上下文到调度协程
        if(swapcontext(&Scheduler::GetSchedulerFiber()->m_ctx, &m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext error");
        }
    }
    else {
        //保存当前上下文到主协程
        if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext error");
        }
    }
}

//将当前协程切换到后台，切换回主协程/调度协程 
void Fiber::swapOut() {
    SetThis(t_threadFiber.get());    //设置当前协程为主协程
    if(m_runInScheduler) {
        //恢复到调度协程
        if(swapcontext(&m_ctx, &Scheduler::GetSchedulerFiber()->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext error");
        }
    }
    else {
        //恢复到主协程
        if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext error");
        }
    }
}


//设置当前线程的运行协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

//返回当前协程,如果没有,则创建主协程返回
Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        //shared_from_this()函数作用是把this封装成智能指针
        //shared_from_this()是来自继承的父类enable_shared_from_this的成员函数
        //普通指针和智能指针都可以调用,即就是子类对象调用父类成员函数
        return t_fiber->shared_from_this();  
    }
    Fiber::ptr main_fiber(new Fiber);    //创建主协程
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

//将当前协程切换到后台,并设置为READY状态,再切换回主协程
void Fiber::YieldToReady() {
    Fiber::ptr curr = GetThis();
    curr->m_state = READY;
    curr->swapOut();  //之后当主协程swapIn回来后,是到这里接着执行,所以该函数会被执行完,curr会自动释放
}

//将当前协程切换到后台,并设置为HOLD状态,,再切换回主协程
void Fiber::YieldToHold() {
    Fiber::ptr curr = GetThis();
    curr->m_state = HOLD;
    curr->swapOut();  //之后当主协程swapIn回来后,是到这里接着执行,所以该函数会被执行完,curr会自动释放
}

//返回当前协程的总数量
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

//协程执行函数
void Fiber::MainFunc() {
    Fiber::ptr curr = GetThis();  // GetThis()的shared_from_this()方法让引用计数加1
    SYLAR_ASSERT(curr);
    try {
        curr->m_cb();    //当curr为GetThis()中创建的主协程,此时没有回调函数,执行会抛出异常
        curr->m_cb = NULL;    //防止m_cb()中引用了智能指针参数,使得引用计数+1
        curr->m_state = TERM;
    } catch(std::exception& e) {
        curr->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << e.what()
                                << " fiber_id=" << curr->getId()
                                << std::endl 
                                << sylar::BacktraceToString();
    } catch(...) {   //catch(…)能捕获所有数据类型的异常对象
        curr->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                                << " fiber_id=" << curr->getId()
                                << std::endl 
                                << sylar::BacktraceToString();
    }

    //SetThis(t_threadFiber.get());  //当前协程即将结束,即将返回主协程,将当前协程改为主协程

    //当前协程的 uc_link 为 nullptr,所以当前协程结束后将直接结束,不会回到主协程
    //为了避免这种情况情况发生,有两种方法:
    //  (1)构造新协程时,将协程的 uc_link = &t_threadFiber->m_ctx;
    //  (2)采用下面这种方法,在当前协程函数执行完后,再用 swapOut 切换回主协程
    //     要注意,切换回主协程后,子协程并未完全结束,所以在子协程执行函数中定义的智能指针还未释放
    //     缺点：存在野指针问题    优点：可重复利用协程栈
    
    Fiber* temp = curr.get();    //用普通指针代替智能指针
    curr.reset();    //释放curr智能指针指向的空间,将指向空间的引用计数-1
    temp->swapOut();    //当前协程结束后返回到主协程
    SYLAR_ASSERT2(false, "never reach");
}

//支持使用caller线程进行调度时的协程执行函数
// void Fiber::CallerMainFunc() {
//     Fiber::ptr curr = GetThis();
//     SYLAR_ASSERT(curr);
//     try {
//         curr->m_cb();    //当curr为GetThis()中创建的主协程,此时没有回调函数,执行会抛出异常
//         curr->m_cb = NULL;    //防止m_cb()中引用了智能指针参数,使得引用计数+1
//         curr->m_state = TERM;
//     } catch(std::exception& e) {
//         curr->m_state = EXCEPT;
//         SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << e.what()
//                                 << " fiber_id=" << curr->getId()
//                                 << std::endl 
//                                 << sylar::BacktraceToString();
//     } catch(...) {   //catch(…)能捕获所有数据类型的异常对象
//         curr->m_state = EXCEPT;
//         SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
//                                 << " fiber_id=" << curr->getId()
//                                 << std::endl 
//                                 << sylar::BacktraceToString();
//     }
    
//     Fiber* temp = curr.get();    //用普通指针代替智能指针
//     curr.reset();    //释放curr智能指针指向的空间,将指向空间的引用计数-1
//     temp->back();    //当前协程结束后返回到主协程
//     SYLAR_ASSERT2(false, "never reach");
// }

//获取当前协程id
uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}


}