#ifndef __SYLAR_SOCKET_H__
#define __SYLAR_SOCKET_H__

#include <memory>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "address.h"
#include "noncopyable.h"

namespace sylar {

/**
 * @brief Socket封装类
 */
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    // 协议族类型
    enum Family {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        Unix = AF_UNIX
    };

    // 套接字类型
    enum Type {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    /**
     * @brief 创建TCP Socket(满足Address地址类型)
     * @param[in] address 地址
     */
    static Socket::ptr CreatTcpSocket(Address::ptr address);

    /**
     * @brief 创建UDP Socket(满足Address地址类型)
     * @param[in] address 地址
     */
    static Socket::ptr CreatUdpSocket(Address::ptr address);

    // 创建IPv4的TCP Socket
    static Socket::ptr CreatIPv4TcpSocket();

    // 创建IPv4的UDP Socket
    static Socket::ptr CreatIPv4UdpSocket();

    // 创建IPv6的TCP Socket
    static Socket::ptr CreatIPv6TcpSocket();

    // 创建IPv6的UDP Socket
    static Socket::ptr CreatIPv6UdpSocket();

    // 创建Unix的TCP Socket
    static Socket::ptr CreatUnixTcpSocket();

    // 创建Unix的UDP Socket
    static Socket::ptr CreatUnixUdpSocket();

    /**
     * @brief Socket构造函数
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @param[in] type 套接字类型(SOCK_STREAM(TCP)、SOCK_DGRAM(UDP)等)
     * @param[in] protocol 使用的协议(IPPROTO_TCP、IPPROTO_UDP等)
     */
    Socket(int family, int type, int protocol = 0);

    // 析构函数
    ~Socket();

    // 获取发送超时时间(毫秒)
    uint64_t getSendTimeout();

    // 设置发送超时时间(毫秒)
    void setSendTimeout(uint64_t time);

    // 获取接收超时时间(毫秒)
    uint64_t getRecvTimeout();

    // 设置接收超时时间(毫秒)
    void setRecvTimeout(uint64_t time);

    // 获取sockopt，原型为getsockopt()
    bool getOption(int level, int optname, void* optval, socklen_t* optlen);

    // 获取sockopt模板
    template<class T>
    bool getOption(int level, int optname, T& optval) {
        socklen_t optlen = sizeof(T);
        return getOption(level, optname, &optval, &optlen);
    }

    // 设置sockopt，原型为setsockopt()
    bool setOption(int level, int optname, const void* optval, socklen_t optlen);

    // 设置sockopt模板
    template<class T>
    bool setOption(int level, int optname, const T& optval) {
        return setOption(level, optname, &optval, sizeof(T));
    }

    /**
     * @brief 连接地址，客户端发起，连接服务器
     * @param[in] addr 目标地址
     * @param[in] timeout_ms 超时时间(毫秒)
     */
    bool connect(const Address::ptr addr, uint64_t timeout_ms = (uint64_t)-1);

    /**
     * @brief 绑定地址，服务器端绑定
     * @param[in] addr 地址
     * @return 是否绑定成功
     */
    bool bind(const Address::ptr addr);

    /**
     * @brief 监听socket，服务器端监听
     * @param[in] backlog 未完成连接队列的最大长度
     * @result 返回监听是否成功
     * @pre 必须先 bind 成功
     */
    bool listen(int backlog = SOMAXCONN);

    /**
     * @brief 接收来自客户端的connect连接
     * @return 成功返回新生成的socket，失败返回nullptr
     * @pre Socket必须 bind , listen 成功
     */
    Socket::ptr accept();

    // 关闭socket
    bool close();
    
    /**
     * @brief 发送数据
     * @param[in] buf 待发送数据的内存
     * @param[in] len 待发送数据的长度
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    ssize_t send(const void* buf, size_t len, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buf 待发送数据的内存(iovec数组)
     * @param[in] len 待发送数据的长度(iovec长度)
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    ssize_t send(const iovec* buf, size_t len, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buf 待发送数据的内存
     * @param[in] len 待发送数据的长度
     * @param[in] toAddr 发送的目标地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    ssize_t sendTo(const void* buf, size_t len, const Address::ptr toAddr, int flags = 0);

    /**
     * @brief 发送数据
     * @param[in] buf 待发送数据的内存(iovec数组)
     * @param[in] len 待发送数据的长度(iovec长度)
     * @param[in] toAddr 发送的目标地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 发送成功对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    ssize_t sendTo(const iovec* buf, size_t len, const Address::ptr toAddr, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buf 接收数据的内存
     * @param[in] len 接收数据的内存大小
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    ssize_t recv(void* buf, size_t len, int flags = 0);

    /**
     * @brief 接收数据
     * @param[out] buf 接收数据的内存(iovec数组)
     * @param[in] len 接收数据的内存大小(iovec数组长度)
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    ssize_t recv(iovec* buf, size_t len, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buf 接收数据的内存
     * @param[in] len 接收数据的内存大小
     * @param[out] fromAddr 发送端地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    ssize_t recvFrom(void* buf, size_t len, Address::ptr fromAddr, int flags = 0);

    /**
     * @brief 接受数据
     * @param[out] buf 接收数据的内存(iovec数组)
     * @param[in] len 接收数据的内存大小(iovec数组长度)
     * @param[out] fromAddr 发送端地址
     * @param[in] flags 标志字
     * @return
     *      @retval >0 接收到对应大小的数据
     *      @retval =0 socket被关闭
     *      @retval <0 socket出错
     */
    ssize_t recvFrom(iovec* buf, size_t len, Address::ptr fromAddr, int flags = 0);

    /**
     * @brief 获取本地地址
     * @details 对于客户端来说，本地地址是自身地址，远端地址是服务器地址
     *          对于服务器来说，本地地址是自身地址，远端地址是客户端地址
    */
    Address::ptr getLocolAddress();

    // 获取远端地址
    Address::ptr getRemoteAddress();

    // 获取socket句柄
    int getSocket() const { return m_sock; }

    // 获取套接字使用的协议簇
    int getFamily() const { return m_family; }

    // 获取套接字的类型
    int getType() const { return m_type; }

    // 获取套接字使用的具体协议
    int getProtocol() const { return m_protocol; }

    // 返回是否已成功连接
    bool isConnected() const { return m_isConnected; }

    // 返回套接字是否有效
    bool isValid() const { return m_sock != -1; }

    // 获取错误信息
    int getError();

    // 用流输出信息
    std::ostream& dump(std::ostream& os) const;

    // 字符串输出信息
    std::string toString() const;

    // 取消m_sock套接字上的读事件
    bool cancelRead();

    // 取消m_sock套接字上的写事件
    bool cancelWrite();

    // 取消m_sock套接字上的accept事件
    bool cancelAccept();

    // 取消m_sock套接字上的所有事件
    bool cancelAll();
 
protected:
    // 创建socket
    bool newSocket();

    // 设置socket的属性
    void setSocket();

    // 初始化Socket
    bool initSocket(int sock);

private:
    int m_sock;                     // socket句柄
    int m_family;                   // 协议簇
    int m_type;                     // 类型
    int m_protocol;                 // 协议
    bool m_isConnected;             // 是否连接
    Address::ptr m_locolAddress;    // 本地地址
    Address::ptr m_remoteAddress;   // 远端地址

};

}

#endif