#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include <iostream>
#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <vector>
#include <map>


namespace sylar {

class IPAddress;

/**
 * @brief 网络地址的基类,抽象类
 */
class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    /**
     * @brief 通过sockaddr指针创建Address,实现多态
     * @param[in] addr sockaddr指针
     * @return 返回和sockaddr相匹配的Address,失败返回nullptr
     */
    static Address::ptr Create(const sockaddr* address, socklen_t addrlen);

    /**
     * @brief 域名解析，通过字符串形式host地址返回所有解析得到的地址
     * @param[out] results 保存所有满足条件的Address
     * @param[in] host 域名、正常IP地址加端口号，例：（方括号为可选内容）
     *                 www.sylar.top[:80]  192.168.88.130[:80]  2001:1:2:3::4[:80]
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @param[in] socktype 套接字类型(SOCK_STREAM(TCP)、SOCK_DGRAM(UDP)等)
     * @param[in] protocol 使用的协议(IPPROTO_TCP、IPPROTO_UDP等)
     * @return 返回是否转换成功
    */
    static bool Lookup(std::vector<Address::ptr>& results, const std::string& host,
            int family = AF_INET, int socktype = 0, int protocol = 0);

    /**
     * @brief 域名解析，通过字符串形式host地址返回任意一个解析得到的地址
     * @param[in] host 域名、正常IP地址加端口号，例：（方括号为可选内容）
     *                 www.sylar.top[:80]  192.168.88.130[:80]  2001:1:2:3::4[:80]
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @param[in] socktype 套接字类型(SOCK_STREAM(TCP)、SOCK_DGRAM(UDP)等)
     * @param[in] protocol 使用的协议(IPPROTO_TCP、IPPROTO_UDP等)
     * @return 返回满足条件的任意Address,失败返回nullptr
    */
    static Address::ptr LookupAny(const std::string& host, 
            int family = AF_INET, int socktype = 0, int protocol = 0);

    /**
     * @brief 域名解析，通过字符串形式host地址返回任意一个解析得到的IPAddress地址
     * @param[in] host 域名、正常IP地址加端口号，例：（方括号为可选内容）
     *                 www.sylar.top[:80]  192.168.88.130[:80]  2001:1:2:3::4[:80]
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @param[in] socktype 套接字类型(SOCK_STREAM(TCP)、SOCK_DGRAM(UDP)等)
     * @param[in] protocol 使用的协议(IPPROTO_TCP、IPPROTO_UDP等)
     * @return 返回满足条件的任意IPAddress,失败返回nullptr
    */
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host, 
            int family = AF_INET, int socktype = 0, int protocol = 0);

    /**
     * @brief 通过网卡设备来解析ip地址，返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
     * @param[out] results 保存本机所有地址
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @return 是否获取成功
     */
    static bool GetInterfaceAddress(std::multimap<std::string, 
            std::pair<Address::ptr, uint32_t> >& results, int family = AF_INET);

    /**
     * @brief 通过网卡设备来解析ip地址，获取指定网卡的地址和子网掩码位数
     * @param[out] results 保存指定网卡的地址
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @return 是否获取成功
     */
    static bool GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t> >& results, 
            const std::string& interface_name, int family = AF_INET);

    // 虚析构函数
    virtual ~Address() {};

    // virtual ~Address() = 0;    //纯虚析构函数的声明，必须要加下面定义
    // Address::~Address() {}

    // 返回协议簇
    int getFamily() const;

    // 返回sockaddr指针,读写
    virtual sockaddr* getAddr() = 0;

    // 返回sockaddr指针,只读
    virtual const sockaddr* getAddr() const = 0;

    // 返回sockaddr的长度(以字节为单位)
    virtual socklen_t getAddrlen() const = 0;

    // 可读性输出地址和端口号
    virtual std::ostream& insert(std::ostream& os) const = 0;

    // 将地址和端口号输出为字符串形式
    std::string toString() const;

    // 重载比较运算符
    bool operator<(const Address& addr) const;
    bool operator==(const Address& addr) const;
    bool operator!=(const Address& addr) const;

};

/**
 * @brief IP地址的基类,抽象类
 */
class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    /**
     * @brief 通过域名,IP,服务器名创建IPAddress
     * @param[in] address 域名,IP,服务器名等.举例: www.sylar.top
     * @param[in] port 端口号
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    // 虚析构函数
    virtual ~IPAddress() {};

    // virtual ~IPAddress() = 0;    //纯虚析构函数的声明，必须要加下面定义
    // IPAddress::~IPAddress() {}

    /**
     * @brief 获取该地址的广播地址
     * @param[in] prefix_len 子网掩码位数(注意是位数)
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) const = 0;

    /**
     * @brief 获取该地址的网段(网络地址)
     * @param[in] prefix_len 子网掩码位数(注意是位数)
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len) const = 0;

    /**
     * @brief 获取子网掩码地址
     * @param[in] prefix_len 子网掩码位数(注意是位数)
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    virtual IPAddress::ptr subnetAddress(uint32_t prefix_len) const = 0;

    // 返回端口号
    virtual uint16_t getPort() const = 0;

    // 设置端口号
    virtual void setPort(uint16_t port) = 0;

};

/**
 * @brief IPv4地址类
 */
class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    /**
     * @brief 使用点分十进制地址字符串创建IPv4Address
     * @param[in] addr 点分十进制地址,如"192.168.1.1"
     * @param[in] port 端口号
     * @return 返回IPv4Address,失败返回nullptr
     */
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    // 无参构造函数
    //IPv4Address();

    // 通过sockaddr_in构造IPv4Address
    IPv4Address(const sockaddr_in& address);

    /**
     * @brief 通过二进制地址构造IPv4Address
     * @param[in] address 二进制地址address，INADDR_ANY相当于0.0.0.0
     * @param[in] port 端口号
     */
    IPv4Address(uint32_t addr = INADDR_ANY, uint16_t port = 0);

    ~IPv4Address() {}

    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrlen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) const override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) const override;
    IPAddress::ptr subnetAddress(uint32_t prefix_len) const override;
    uint16_t getPort() const override;
    void setPort(uint16_t port) override;

private:
    sockaddr_in m_addr;
    // 结构体 sockaddr_in 如下：
    // struct sockaddr_in {
    //     unsigned short sin_family; // 地址族，IPv4的地址族为AF_INET
    //     unsigned short sin_port;   // 端口
    //     struct in_addr sin_addr;   // IP地址，IPv4的地址用一个32位整数来表示
    //     char sin_zero[8];          // 填充位，填零即可
    // };
    // 其中结构体 in_addr 为：
    // struct in_addr {
    //     uint32_t s_addr;   // 4byte 32bit
    // };
};

/**
 * @brief IPv6地址类
 */
class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;

    /**
     * @brief 使用IPv6地址字符串创建IPv6Address
     * @param[in] addr IPv6地址字符串,如"2001:0db8:85a3:0000:0000:8a2e:0370:7334"
     * @param[in] port 端口号
     * @return 返回IPv6Address,失败返回nullptr
     */
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    // 无参构造函数
    IPv6Address();

    // 通过sockaddr_in6构造IPv6Address
    IPv6Address(const sockaddr_in6& address);

    // 通过IPv6二进制地址构造IPv6Address
    IPv6Address(const uint8_t addr[16], uint16_t port = 0);

    ~IPv6Address() {}

    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrlen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) const override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) const override;
    IPAddress::ptr subnetAddress(uint32_t prefix_len) const override;
    uint16_t getPort() const override;
    void setPort(uint16_t port) override;

private:
    sockaddr_in6 m_addr;
    // 结构体 sockaddr_in6 如下：
    // struct sockaddr_in6 {
    //     unsigned short sin6_family; // 地址族，IPv6的地址族为AF_INET6
    //     in_port_t sin6_port;        // 端口
    //     uint32_t sin6_flowinfo;     // IPv6流控信息
    //     struct in6_addr sin6_addr;  // IPv6地址，实际为一个128位的结构体
    //     uint32_t sin6_scope_id;     // IPv6 scope-id
    // };
    // 其中结构体 in6_addr 为：
    // struct in6_addr {
    //     unsigned char s6_addr[16];   // 16byte 128bit
    // };
};

/**
 * @brief UnixSocket地址类
 * @details Unix域套接字使用文件系统路径作为套接字地址
 */
class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    // 无参构造函数
    UnixAddress();

    // 通过sockaddr_un构造UnixAddress
    UnixAddress(const sockaddr_un& addr, socklen_t addrlen);

    // 通过文件系统路径构造UnixAddress
    UnixAddress(const std::string& path);

    ~UnixAddress() {}

    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrlen() const override;
    void setAddrlen(uint32_t len);
    std::string getPath() const;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr_un m_addr;
    socklen_t m_length;    // sockaddr_un的实际长度(sun_path实际路径名长度)
    // 结构体 sockaddr_un 如下：
    // struct sockaddr_un {
    //     sa_family_t sun_family;    // 地址家族，通常为 AF_UNIX
    //     char        sun_path[108]; // 套接字路径名
    // };
};

/**
 * @brief 未知地址类
 */
class UnkownAddress : public Address {
public:
    typedef std::shared_ptr<UnkownAddress> ptr;

    // 通过family构造
    UnkownAddress(int family);

    // 通过sockaddr_in6构造
    UnkownAddress(const sockaddr& Address);

    ~UnkownAddress() {}

    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrlen() const override;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr m_addr;
    // 结构体 sockaddr 如下：
    // struct sockaddr {
    //     sa_family_t sa_family;      // 地址族，表示地址类型
    //     char        sa_data[14];    // 地址数据，具体格式取决于地址族类型
    // };
};


/**
 * @brief 流式输出Address
*/
std::ostream& operator<<(std::ostream& os, const Address& addr);


}

#endif