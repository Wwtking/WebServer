#include "fd_manager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "hook.h"

namespace sylar {

// 通过文件句柄构造FdCtx
FdCtx::FdCtx(int fd) 
    :m_fd(fd)
    ,m_isInit(false)
    ,m_isSocket(false)
    ,m_isClose(false)
    ,m_userNonblock(false)
    ,m_systemNonblock(false)
    ,m_recvTimeout(-1)
    ,m_sendTimeout(-1) {
    init();
}

// 析构函数
FdCtx::~FdCtx() {
}

// 初始化
bool FdCtx::init() {
    if(m_isInit) {
        return true;
    }
    m_recvTimeout = -1;   // (uint64_t)-1 == 0xffffffffffffffff
    m_sendTimeout = -1;

    // stat()用来判断没有打开的文件,而fstat()用来判断打开的文件
    // fstat()用来将参数 m_fd 所指向的文件状态复制到参数 fd_stat 所指向的结构中
    struct stat fd_stat;
    if(-1 == fstat(m_fd, &fd_stat)) {    // 执行成功返回0，失败返回-1
        m_isInit = false;
        m_isSocket = false;
    }
    else {
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode); //S_ISSOCK(st_mode)用于判断是否为一个socket文件
    }

    // hook内部设置的非阻塞
    if(m_isSocket) {
        int flag = fcntl_f(m_fd, F_GETFL, 0);    //获取文件状态
        // 如果文件状态不包括O_NONBLOCK，则重新设置文件状态，加入O_NONBLOCK
        if(!(flag & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flag | O_NONBLOCK);
        }
        m_systemNonblock = true;
    }
    else {
        m_systemNonblock = false;
    }

    m_userNonblock = false;
    m_isClose = false;
    return m_isInit;
}

/**
 * @brief 设置超时时间
 * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
 * @param[in] timeout 超时时间(毫秒)
 */
void FdCtx::setTimeout(int type, uint64_t timeout) {
    switch(type) {   
        //读超时
        case SO_RCVTIMEO:
            m_recvTimeout = timeout;
            break;
        //写超时
        case SO_SNDTIMEO:
            m_sendTimeout = timeout;
            break;
        default:
            break;
    }
}

/**
 * @brief 获取超时时间
 * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
 * @return 超时时间(毫秒)
 */
uint64_t FdCtx::getTimeout(int type) {
    switch(type) {
        //读超时
        case SO_RCVTIMEO:
            return m_recvTimeout;
        //写超时
        case SO_SNDTIMEO:
            return m_sendTimeout;
        default:
            return -1;
    }
}


//构造函数
FdManager::FdManager() {
    m_datas.resize(64);
}

/**
 * @brief 获取/创建文件句柄类FdCtx
 * @details 如果fd存在则直接获取；若不存在则根据auto_create来选择是否创建
 * @param[in] fd 文件句柄
 * @param[in] auto_create 是否自动创建
 * @return 返回对应文件句柄类FdCtx::ptr
 */
FdCtx::ptr FdManager::getFdCtx(int fd, bool auto_create) {
    if(fd < 0) {
        return nullptr;
    }
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        if(auto_create == false) {
            return nullptr;
        }
        //m_datas.resize(fd * 1.5);  //加的是读锁
    }
    else {
        if(m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    lock.unlock();

    //需要创建文件句柄类FdCtx
    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx = std::make_shared<FdCtx>(fd);
    if((int)m_datas.size() <= fd) {
        m_datas.resize(fd * 1.5);
    }
    m_datas[fd] = ctx;
    return ctx;
}

// 删除集合中的文件句柄类
void FdManager::deleteFdCtx(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        return;
    }
    m_datas[fd].reset();
}

}