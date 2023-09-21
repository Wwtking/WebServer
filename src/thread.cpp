#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

static thread_local Thread* curr_thread = nullptr;
static thread_local std::string curr_thread_name = "UNKOWN";

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");


/******************* Semaphore 类函数实现 *******************/
//构造函数
Semaphore::Semaphore(uint32_t count) {
    if(sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

//析构函数
Semaphore::~Semaphore() {
    //sem_destroy(&m_semaphore);
    if(sem_destroy(&m_semaphore)) {
        throw std::logic_error("sem_destroy error");
    }
}

//获取信号量
void Semaphore::wait() {
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

//释放信号量
void Semaphore::notify() {
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}


/******************* Thread 类函数实现 *******************/
//构造函数
Thread::Thread(std::function<void()> cb, const std::string& name) 
    :m_name(name)
    ,m_cb(cb) {
    if(name.empty()) {
        m_name = "UNKOWN";
    }
    auto ret = pthread_create(&m_thread, NULL, &run, this);  //创建线程并进入run
    if(ret) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread error, thr=" << ret
                                    << " name=" << name;
        throw std::logic_error("pthread_create thread error");
    }
    m_semaphore.wait();  //保证在构造完成之后线程函数一定已经处于运行状态
}

//析构函数
//析构函数中将线程标记为“可分离的”，系统在线程终止时自动回收线程资源
//即(1)若主线程未结束，子线程先执行完则自动回收线程资源
//  (2)若主线程先结束，此时无论子线程是否执行完都会自动结束回收资源
Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread);
    }
}

//获取当前的线程指针
Thread* Thread::getCurrThread() {
    return curr_thread;
}

//获取当前线程名称
std::string& Thread::getCurrThreadName() {
    return curr_thread_name;
}

//设置当前线程名称
void Thread::setCurrThreadName(const std::string& name) {
    if(name.empty()) {
        return;
    }
    if(curr_thread) {
        curr_thread->m_name = name; //curr_thread相当于在类内定义的,所以可以访问私有成员
    }
    curr_thread_name = name;
}

//等待线程执行完成
void Thread::join() {
    if(m_thread) {
        auto ret = pthread_join(m_thread, NULL);
        if(ret) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread error, ret=" << ret
                                    << " name=" << m_name;
            throw std::logic_error("pthread_join thread error");
        }
        m_thread = 0;
    }
}

/**
 * @brief 线程执行函数
 * @details 因为是静态函数,没法用this指针,
 *          所以在pthread_create时通过run的参数列表传入this指针
*/
void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;  //void*是一个指针类型，必须强转成对应的指针才能用
    thread->m_id = getThreadId();    //获取线程id
    curr_thread = thread;
    curr_thread_name = thread->m_name;
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb);   //执行完run函数后,m_cb函数指针被释放
    thread->m_semaphore.notify();
    cb();
    return nullptr;
}

}