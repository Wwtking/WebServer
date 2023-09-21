#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <iostream>
#include <memory>
#include <functional>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <atomic>

#include "noncopyable.h"

namespace sylar
{

//信号量
class Semaphore : Noncopyable {
public:
    //构造函数
    Semaphore(uint32_t count = 0);

    //析构函数
    ~Semaphore();

    //获取信号量
    void wait();

    //释放信号量
    void notify();

private:
    //删除拷贝构造,禁止该类使用拷贝构造
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

private:
    sem_t m_semaphore;
};


/**
 * @brief 局部锁的模板实现
 * @details 构造时加锁，析构时解锁,省去了自己去操作lock和unlock
*/
template<class T>
struct ScopedLockImpl {
public:
    /**
     * @brief 构造函数
     * @param[in] mutex Mutex
    */
    ScopedLockImpl(T& mutex) 
        :m_mutex(mutex) {
        m_locked = true;
        m_mutex.lock();
    }

    //析构函数,自动释放锁
    ~ScopedLockImpl() {
        unlock();
    }

    //加锁
    void lock() {
        if(!m_locked) {
            m_locked = true;
            m_mutex.lock();
        }
    }

    //解锁
    void unlock() {
        if(m_locked) {
            m_locked = false;
            m_mutex.unlock();
        }
    }

private:
    T& m_mutex;
    bool m_locked;  //是否已上锁
};

/**
 * @brief 局部读锁模板实现
 * @details 构造时加锁,析构时解锁,省去了自己去操作lock和unlock
*/
template<class T>
struct ReadScopedLockImpl {
public:
    //构造函数,加读锁
    ReadScopedLockImpl(T& mutex) 
        :m_mutex(mutex) {
        m_locked = true;
        m_mutex.rdlock();
    }

    //析构函数,自动释放锁
    ~ReadScopedLockImpl() {
        unlock();
    }

    //加锁
    void lock() {
        if(!m_locked) {
            m_locked = true;
            m_mutex.rdlock();
        }
    }

    //解锁
    void unlock() {
        if(m_locked) {
            m_locked = false;
            m_mutex.unlock();
        }
    }

private:
    T& m_mutex;
    bool m_locked;  //是否已上锁
};


/**
 * @brief 局部写锁模板实现
 * @details 构造时加锁,析构时解锁,省去了自己去操作lock和unlock
*/
template<class T>
struct WriteScopedLockImpl {
public:
    //构造函数,加写锁
    WriteScopedLockImpl(T& mutex) 
        :m_mutex(mutex) {
        m_locked = true;
        m_mutex.wrlock();
    }

    //析构函数,自动释放锁
    ~WriteScopedLockImpl() {
        unlock();
    }

    //加锁
    void lock() {
        if(!m_locked) {
            m_locked = true;
            m_mutex.wrlock();
        }
    }

    //解锁
    void unlock() {
        if(m_locked) {
            m_locked = false;
            m_mutex.unlock();
        }
    }

private:
    T& m_mutex;
    bool m_locked;  //是否已上锁
};


//互斥量:适用于读写操作次数接近
class Mutex : Noncopyable {
public:
    //局部锁
    typedef ScopedLockImpl<Mutex> Lock;

    //构造函数
    Mutex() {
        pthread_mutex_init(&m_mutex, NULL);
    }

    //析构函数
    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    //加锁
    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    //解锁
    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }

private:    
    pthread_mutex_t m_mutex;    //锁类型的结构
};

//空锁(用于调试)
class NullMutex : Noncopyable {
public:
    typedef ScopedLockImpl<NullMutex> Lock;

    NullMutex() {}

    ~NullMutex() {}

    void lock() {}

    void unlock() {}
};


//读写互斥量:适用于读写操作次数差别较大,如多读少写
class RWMutex : Noncopyable {
public:
    //局部读锁
    typedef ReadScopedLockImpl<RWMutex> ReadLock;

    //局部写锁
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    //构造函数
    RWMutex() {
        pthread_rwlock_init(&m_lock, NULL);
    }

    //析构函数
    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    //上读锁
    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    //上写锁
    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    //解锁
    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }

private:    
    pthread_rwlock_t m_lock;    //读写锁
};

//空读写锁(用于调试)
class NullRWMutex : Noncopyable {
public:
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;

    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    
    NullRWMutex() {}

    ~NullRWMutex() {}

    void rdlock() {}

    void wrlock() {}

    void unlock() {}
};


//自旋锁
class Spinlock : Noncopyable {
public:
    typedef ScopedLockImpl<Spinlock> Lock;

    //构造函数
    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    //析构函数
    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }

    //加自旋锁
    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    //解锁
    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }

private:
    pthread_spinlock_t m_mutex;   //自旋锁
};


/**
 * @brief 原子锁:与自旋锁原理类似
 * @details Spinlock自旋锁在占锁尝试过几次之后,若还未占到锁,就会释放对cpu的占用,进入内核态
 *          而atomic原子锁,会一直进行尝试,一直占用cpu
*/
class CASLock : Noncopyable {
public:
    /// 局部锁
    typedef ScopedLockImpl<CASLock> Lock;

    //构造函数
    CASLock() {
        //m_mutex = ATOMIC_FLAG_INIT;  // 初始化为 false
        m_mutex.clear();
    }

    //析构函数
    ~CASLock() {
    }

    //加锁(使用 test_and_set 函数将其设置为 true 表示加锁)
    void lock() {
        //设置m_mutex的状态为true,并返回m_mutex是否被成功设置,若成功返回true
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    //解锁(使用 clear 函数将其设置为 false 表示解锁)
    void unlock() {
        //设置m_mutex的状态为false
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }

private:
    //atomic_flag 是一种简单的原子布尔类型，只支持两种操作：test-and-set 和 clear
    volatile std::atomic_flag m_mutex;    //原子状态
};


//线程类
class Thread : Noncopyable {
public:
    typedef std::shared_ptr<Thread> ptr;

    //构造函数
    //线程入口函数类型为void()，如果带参数，则需要用std::bind进行绑定
    Thread(std::function<void()> cb, const std::string& name);

    //析构函数
    ~Thread();

    //获取线程名称
    const std::string& getName() const { return m_name; }

    //获取线程id
    pid_t getId() const { return m_id; }

    //获取当前的线程指针
    static Thread* getCurrThread();

    //获取当前线程名称
    static std::string& getCurrThreadName();

    //设置当前线程名称
    static void setCurrThreadName(const std::string& name);

    //等待线程执行完成
    void join();

private:
    //线程执行函数
    static void* run(void* arg);

private:
    std::string m_name = "UNKOWN";  //线程名称
    pid_t m_id = -1;                //线程id
    pthread_t m_thread = 0;         //线程结构(unsigned long int类型)
    std::function<void()> m_cb;     //线程执行函数
    Semaphore m_semaphore;          //信号量
};



} // namespace sylar


#endif