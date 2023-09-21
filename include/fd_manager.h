#ifndef __SYLAR_FD_MANAGER_H__
#define __SYLAR_FD_MANAGER_H__

#include <memory>
#include <vector>
#include "thread.h"
#include "singleton.h"

namespace sylar {

/**
 * @brief 文件句柄上下文类
 * @details 管理文件句柄类型(是否socket)
 *          是否阻塞,是否关闭,读/写超时时间
 */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;

    // 通过文件句柄构造FdCtx
    FdCtx(int fd);

    // 析构函数
    ~FdCtx();

    // 是否初始化完成
    bool isInit() const { return m_isInit; }
    
    // 是否为socket
    bool isSocket() const { return m_isSocket; }

    // 句柄m_fd是否已关闭
    bool isClose() const { return m_isClose; }

    // 设置 用户是否主动设置为非阻塞 的标志
    void setUserNonblock(bool flag) { m_userNonblock = flag; }
    
    // 获取 用户是否主动设置为非阻塞 的标志
    bool getUserNonblock() const { return m_userNonblock; }

    // 设置 系统是否设置为非阻塞 的标志
    void setSystemNonblock(bool flag) { m_systemNonblock = flag; }

    // 获取 系统是否设置为非阻塞 的标志
    bool getSystemNonblock() const { return m_systemNonblock; }    

    /**
     * @brief 设置超时时间
     * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * @param[in] timeout 超时时间(毫秒)
     */
    void setTimeout(int type, uint64_t timeout);

    /**
     * @brief 获取超时时间
     * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * @return 超时时间(毫秒)
     */
    uint64_t getTimeout(int type);

private:
    //初始化
    bool init();

private:
    int m_fd;                   // 文件句柄
    bool m_isInit: 1;           // 是否已初始化
    bool m_isSocket: 1;         // 是否为socket
    bool m_isClose: 1;          // 是否句柄已关闭
    bool m_userNonblock: 1;     // 是否为用户主动设置非阻塞
    bool m_systemNonblock: 1;   // 是否hook非阻塞(默认非阻塞)
    uint64_t m_recvTimeout;     // 读超时时间(毫秒)
    uint64_t m_sendTimeout;     // 写超时时间(毫秒)
};

// 文件句柄管理类
class FdManager {
public:
    typedef RWMutex RWMutexType;

    //构造函数
    FdManager();

    /**
     * @brief 获取/创建文件句柄类FdCtx
     * @details 如果fd存在则直接获取；若不存在则根据auto_create来选择是否创建
     * @param[in] fd 文件句柄
     * @param[in] auto_create 是否自动创建
     * @return 返回对应文件句柄类FdCtx::ptr
     */
    FdCtx::ptr getFdCtx(int fd, bool auto_create = false);

    // 删除集合中的文件句柄类
    void deleteFdCtx(int fd);

private:
    RWMutexType m_mutex;               // 读写锁
    std::vector<FdCtx::ptr> m_datas;   // 文件句柄集合
};

// 文件句柄管理类的单例模式
typedef Singleton<FdManager> FdMgr;

}

#endif