#include "address.h"
#include "log.h"
#include "myendian.h"
#include <stddef.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar {


/**
 * @brief 生成指定位数为prefix_len的二进制掩码
 * @param[in] prefix_len 掩码1的个数
 * @param[out] T 二进制掩码
*/
template<class T>
static T CreatMask(uint32_t prefix_len) {
    //先将1左移 (sizeof(T)*8-prefix_len) 位，然后减1，以得到一个二进制数，
    //取反后，其中高prefix_len位为1，低位为0
    return ~((1 << (sizeof(T) * 8 - prefix_len)) - 1);
}

/**
 * @brief 用于计算给定值val的二进制表示中包含的1的个数
 * @param[in] T 给定值val
 * @param[out] uint32_t 二进制表示val中1的个数
*/
template<class T>
static uint32_t CountBits(T val) {
    uint32_t count = 0;
    while(val) {
        val &= val - 1;  //将val的最低位的1变为0
        count++;
    }
    return count;
}


// 通过sockaddr指针创建Address，实现多态
Address::ptr Address::Create(const sockaddr* address, socklen_t addrlen) {
    if(!address) {
        return nullptr;
    }

    Address::ptr result = nullptr;    //多态实现
    switch (address->sa_family)
    {   
        // IPv4地址
        case AF_INET:
            result = std::make_shared<IPv4Address>(*(const sockaddr_in*)address);
            break;
        
        // IPv6地址
        case AF_INET6:
            result = std::make_shared<IPv6Address>(*(const sockaddr_in6*)address);
            break;

        // Unix域套接字地址
        case AF_UNIX:
            result = std::make_shared<UnixAddress>(*(const sockaddr_un*)address, addrlen); 
            break;

        // 未知地址
        default:
            result = std::make_shared<UnkownAddress>(*address);
            break;
    }
    return result;
}

// 地址/域名解析：通过给一个域名或地址来解析ip地址
// 如果传入host为域名、服务器名等，则通过getaddrinfo传入，可能会得到多个result(记录ip地址相关信息)
// 如果传入host为ip地址，则通过getaddrinfo传入，只能得到一个result(记录ip地址相关信息)
// 因为IP地址是唯一的，而一个主机名可能对应多个IP地址(例如，IPv4和IPv6地址)
bool Address::Lookup(std::vector<Address::ptr>& results, const std::string& host,
                            int family, int socktype, int protocol) {
    addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;

    std::string node;              //主机名或IP地址字符串
    const char* service = NULL;    //服务名或端口号

    /**
     * @brief void *memchr(const void *ptr, int value, size_t num);
     * @param[in] ptr 指向内存区域的指针，函数将在此区域进行搜索
     * @param[in] value 要搜索的字符的 ASCII 值
     * @param[in] num 要搜索的字节数
     * @return 若找到返回指向该字符的指针；找不到返回nullptr
    */
    // www.sylar.top[:80]  192.168.88.130[:80]  2001:1:2:3::4[:80]
    const char* begin = (const char*)memchr(host.c_str(), '[', host.size());
    if(begin) {
        // 找 '[' 前面的主机名或IP地址字符串
        node = host.substr(0, begin - host.c_str());
        // 找 '[]' 中的服务名或端口号
        const char* end = (const char*)memchr(begin, ']', host.size() - (begin - host.c_str()));
        if(*(begin + 1) == ':') {    // 可能直接加服务名[http]
            begin++;
        }
        begin++;
        service = host.substr(begin - host.c_str(), end - begin).c_str();
    }
    else {  
        // www.sylar.top:80  192.168.88.130:80
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            // 不存在第二个':'，不能用于ipv6地址
            if(!memchr(service + 1, ':', host.size() - (service - host.c_str()) - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
        else {
            // www.sylar.top  192.168.88.130
            // 没有'['和':',说明输入没有服务名或端口号
            node = host;
        }
    }

    // SYLAR_LOG_DEBUG(g_logger) << node << "  " << service;

    int rt = getaddrinfo(node.c_str(), service, &hints, &result);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "Address::Lookup getaddrinfo(" << node << ", " 
                                << service << ") rt=" << rt << " errno=" << errno 
                                << " errstr=" << strerror(errno);
        return false;
    }
    
    // 循环放入所有符合条件的地址信息
    for(addrinfo* next = result; next != nullptr; next = next->ai_next) {
        results.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
    }

    freeaddrinfo(result);    // 释放 getaddrinfo() 函数动态分配的内存
    return !results.empty();
}

// 域名解析，通过字符串形式host地址返回任意一个解析得到的地址
Address::ptr Address::LookupAny(const std::string& host, 
                            int family, int socktype, int protocol) {
    std::vector<Address::ptr> results;
    if(Lookup(results, host, family, socktype, protocol)) {
        return results[0];
    }
    return nullptr;
}

// 域名解析，通过字符串形式host地址返回任意一个解析得到的IPAddress地址
std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host, 
                            int family, int socktype, int protocol) {
    std::vector<Address::ptr> results;
    if(Lookup(results, host, family, socktype, protocol)) {
        for(auto& i : results) {
            //只有指向派生类IPv4Address或IPv6Address的基类Address才能强制转换为IPAddress(多态时，才能上转下)
            //此时，强转得到的IPAddress指针指向派生类IPv4Address或IPv6Address，实现多态
            IPAddress::ptr ipaddr = std::dynamic_pointer_cast<IPAddress>(i);  //强转失败返回nullptr
            if(ipaddr) {
                return ipaddr;
            }
        }
    }
    return nullptr;
}

// 通过网卡设备来解析ip地址，返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
// struct ifaddrs {
//     struct ifaddrs  *ifa_next;      // 指向下一个接口的指针
//     char            *ifa_name;      // 接口名称
//     unsigned int    ifa_flags;      // 接口标志
//     struct sockaddr *ifa_addr;      // 接口地址
//     struct sockaddr *ifa_netmask;   // 掩码地址
//     struct sockaddr *ifa_ifu;       // 接口用途
//     union {
//         struct sockaddr *ifu_broadaddr;  // 广播地址
//         struct sockaddr *ifu_dstaddr;    // 目标地址
//     } ifa_ifu;
//     void            *ifa_data;      // 接口访问权限控制信息
// };
bool Address::GetInterfaceAddress(std::multimap<std::string, 
        std::pair<Address::ptr, uint32_t> >& results, int family) {
    struct ifaddrs *result;

    /**
     * @brief 获取当前计算机上所有网络接口的信息
     * @details int getifaddrs(struct ifaddrs **ifap);
     * @param[in] ifap 指向struct ifaddrs结构体指针的指针，将网络接口信息填充到该结构体链表中
     * @return 成功返回0，失败返回-1
    */
    if(getifaddrs(&result)) {
        SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress getifaddrs " 
                            <<"errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    try {
        // 循环遍历得到的所有网络接口信息
        for(ifaddrs *next = result; next != nullptr; next = next->ifa_next) {
            // 若与所要求的ip地址类型不符，直接跳过
            if(next->ifa_addr->sa_family != family && family != AF_UNSPEC) {
                continue;
            }

            Address::ptr addr;          //网卡地址
            uint32_t prefix_len = ~0u;  //子网掩码位数
            switch(next->ifa_addr->sa_family) {
                // IPv4地址
                case AF_INET:  // case语句里定义的变量，一直存在直到switch结束，加{}可解决
                {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                    uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                    prefix_len = CountBits(netmask);    //计算掩码位数
                    break;
                }

                // IPv6地址
                case AF_INET6:
                {
                    addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                    for(int i = 0; i < 16; i++) {
                        uint8_t netmask = (uint8_t)((sockaddr_in6*)next->ifa_netmask)->sin6_addr.s6_addr[i];
                        prefix_len += CountBits(netmask);
                    }
                    break;
                }
                    
                default:
                    break;
            }

            if(addr && prefix_len != ~0u) {
                results.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
            }
        }
    }
    catch(...) {
        SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress exception";
        freeifaddrs(result);
        return false;
    }

    freeifaddrs(result);    // 释放 getifaddrs() 函数动态分配的内存
    return !results.empty();
}

// 通过网卡设备来解析ip地址，获取指定网卡的地址和子网掩码位数
bool Address::GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t> >& results, 
        const std::string& interface_name, int family) {
    // 返回 0.0.0.0 表示任意地址
    if(interface_name.empty() || interface_name == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            // IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
            results.push_back(std::make_pair(std::make_shared<IPv4Address>(), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            // IPv6Address();
            results.push_back(std::make_pair(std::make_shared<IPv6Address>(), 0u));
        }
        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t> > result;
    if(!GetInterfaceAddress(result, family)) {
        return false;
    }
    
    // 因为指定网卡iface的ip地址可能有多个，所以需要全部找出
    // equal_range 用于在有序容器中查找特定元素的范围（map插入数据根据键值排序）
    // 它返回一个指向范围的迭代器对(pair)，包含了第一个不小于该元素的位置和第一个大于该元素的位置
    auto its = result.equal_range(interface_name);
    for(auto it = its.first; it != its.second; it++) {
        results.push_back(it->second);
    }
    return !results.empty();
}


// 返回协议簇
int Address::getFamily() const {
    return getAddr()->sa_family;
}

// 将地址和端口号输出为字符串形式
std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

// 重载小于号比较函数
bool Address::operator<(const Address& addr) const {
    int minlen = std::min(getAddrlen(), addr.getAddrlen());
    /**
     * @brief int memcmp(const void* ptr1, const void* ptr2, std::size_t num);
     * @details 按字节比较两个内存块的内容，并返回一个整数来表示比较结果
     * @param[in] ptr1_ptr2 指向要进行比较的内存块的指针
     * @param[in] num 要比较的字节数
     * @return ptr1 的内容按字节大于 ptr2 的内容，则返回一个正整数
               ptr1 的内容按字节小于 ptr2 的内容，则返回一个负整数
               ptr1 的内容按字节等于 ptr2 的内容，则返回 0
    */
    int result = memcmp(getAddr(), addr.getAddr(), minlen);
    if(result < 0) {
        return true;
    }
    else if(result > 0) {
        return false;
    }
    else {
        return getAddrlen() < addr.getAddrlen();
    }
}

// 重载等号函数
bool Address::operator==(const Address& addr) const {
    return getAddrlen() == addr.getAddrlen() && 
            memcmp(getAddr(), addr.getAddr(), getAddrlen()) == 0;
}

// 重载不等号函数
bool Address::operator!=(const Address& addr) const {
    return !(*this == addr);
}


/**
 * @brief 通过域名,IP,服务器名创建IPAddress
 * @details 介绍addrinfo：
 *              struct addrinfo {
 *                  int              ai_flags;       // 用于控制getaddrinfo()行为的标志
 *                  int              ai_family;      // 所需地址族(AF_INET、AF_INET6、AF_UNSPEC等)
 *                  int              ai_socktype;    // 套接字类型(SOCK_STREAM(TCP)、SOCK_DGRAM(UDP)等)
 *                  int              ai_protocol;    // 使用的协议(IPPROTO_TCP、IPPROTO_UDP等)
 *                  size_t           ai_addrlen;     // 地址结构的大小
 *                  struct sockaddr *ai_addr;        // 地址结构指针
 *                  char            *ai_canonname;   // 规范名(canonical name)
 *                  struct addrinfo *ai_next;        // 指向下一个地址信息结构的指针
 *              };
 *          ai_socktype 和 ai_protocol 的关系：
 *              (1)如果 ai_protocol 字段是 0（未指定），则会根据 ai_socktype 字段的值来选择一个适合的协议
 *              (2)如果 ai_protocol 字段非零，则会根据指定的协议来选择 ai_socktype 的值
*/
IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));
    //hints.ai_flags = AI_NUMERICSERV;  //提示getaddrinfo()函数只能用数值形式的node和service,不能用主机名和服务名
    hints.ai_family = AF_UNSPEC;      //表示不限制返回的地址族,可以返回IPv4或IPv6的地址

    /**
     * @brief int getaddrinfo(const char *node, const char *service,
     *                              const struct addrinfo *hints,
     *                              struct addrinfo **res);
     * @details 将主机名(IP地址)和服务名(端口号)解析为与之关联的地址信息结构体，将结果存在res中(主机名解析出的IP地址)
     * @param[in] node 指向主机名或IP地址字符串的指针
     * @param[in] service 指向服务名或端口号的指针
     * @return 函数执行成功返回0；函数执行失败返回非零
    */
    int rt = getaddrinfo(address, NULL, &hints, &results);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "IPAddress::Create(" << address << ", " << port << ") rt=" 
                                << rt << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }

    try {
        // dynamic_pointer_cast用于智能指针的强制转换，功能类似于dynamic_cast
        // 处理基类和派生类之间的转换，或者处理多态类型之间的转换
        // 派生类可转换为基类(随便转)；多态时，基类可转换为派生类
        IPAddress::ptr addr = std::dynamic_pointer_cast<IPAddress>(
                            Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        //只有指向派生类IPv4Address或IPv6Address的基类Address才能强制转换为IPAddress(多态时，才能上转下)
        if(addr) {
            addr->setPort(port);
        }
        freeaddrinfo(results);    // 释放 getaddrinfo() 函数动态分配的内存
        return addr;
    }
    catch(...) {
        freeaddrinfo(results);
        return nullptr;
    }
}


// 使用点分十进制地址字符串创建IPv4Address
IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port) {
    sockaddr_in addr;
    memset(&addr, 0 , sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = byteSwapToBigEndian(port);
    /**
     * @brief int inet_pton(int af, const char *src, void *dst);
     * @details converts the character string [src] into a network address structure in the [af] address family, 
     *          then copies the network address structure to [dst]. 
     *          [af] argument must be either AF_INET or AF_INET6.
     * @return 返回值为1表示转换成功，返回值为0表示输入的地址字符串格式无效，返回值为-1表示转换错误
    */
    int result = inet_pton(AF_INET, address, &addr.sin_addr);
    if(result <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", " << port << ") rt=" 
                                << result << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    return std::make_shared<IPv4Address>(addr);
}

// IPv4Address::IPv4Address() {
//     memset(&m_addr, 0, sizeof(m_addr));
//     m_addr.sin_family = AF_INET;
// }

IPv4Address::IPv4Address(const sockaddr_in& address) {
    m_addr = address;
}

IPv4Address::IPv4Address(uint32_t addr, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    //输入IP和端口号后，计算机会给我们处理成小端字节序
    //主机字节序(小端字节序)转换成网络字节序(大端字节序)
    m_addr.sin_addr.s_addr = byteSwapToBigEndian(addr);
    m_addr.sin_port = byteSwapToBigEndian(port);
}

// 获取IPv4地址信息，读写
sockaddr* IPv4Address::getAddr() {
    return (sockaddr*)&m_addr;
}

// 获取IPv4地址信息，只读
const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

// 获取地址信息的长度
socklen_t IPv4Address::getAddrlen() const {
    return sizeof(m_addr);
}

// 可读性输出IPv4地址和端口号
std::ostream& IPv4Address::insert(std::ostream& os) const {
    // s_addr和sin_port为大端序，在小端序计算机内读取处理要转回小端序
    uint32_t addr = byteSwapToLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xFF) << "."
        << ((addr >> 16) & 0xFF) << "."
        << ((addr >> 8) & 0xFF) << "."
        << (addr & 0xFF);
    os << ":" << byteSwapToLittleEndian(m_addr.sin_port);
    return os;
}

// 获取该地址的广播地址(prefix_len为子网掩码位数)
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) const {
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in addr(m_addr);
    addr.sin_addr.s_addr |= byteSwapToBigEndian(~CreatMask<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(addr);
}

// 获取该地址的网段(网络地址)
IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) const {
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in addr(m_addr);
    addr.sin_addr.s_addr &= byteSwapToBigEndian(CreatMask<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(addr);
}

// 获取子网掩码地址
IPAddress::ptr IPv4Address::subnetAddress(uint32_t prefix_len) const {
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = byteSwapToBigEndian(CreatMask<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(subnet);
}

// 获取端口号
uint16_t IPv4Address::getPort() const {
    //sin_port为大端序，在小端序计算机内读取返回要转回小端序，方便之后处理
    return byteSwapToLittleEndian(m_addr.sin_port);
}

// 设置端口号
void IPv4Address::setPort(uint16_t port) {
    m_addr.sin_port = byteSwapToBigEndian(port);
}


// 使用IPv6地址字符串创建IPv6Address
IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = byteSwapToBigEndian(port);
    int result = inet_pton(AF_INET6, address, &addr.sin6_addr);
    if(result <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "IPv6Address::Create(" << address << ", " << port << ") rt=" 
                                << result << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }
    return std::make_shared<IPv6Address>(addr);
}

IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& address) {
    m_addr = address;
}

IPv6Address::IPv6Address(const uint8_t addr[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteSwapToBigEndian(port);
    /**
     * @brief void *memcpy(void *dest, const void *src, size_t n);
     * @details copies [n] bytes from memory area [src] to memory area [dest]
     * @return return a pointer to dest.
    */
    memcpy(m_addr.sin6_addr.s6_addr, addr, 16);
}

// 获取IPv6地址信息，读写
sockaddr* IPv6Address::getAddr() {
    return (sockaddr*)&m_addr;
}

// 获取IPv6地址信息，只读
const sockaddr* IPv6Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

// 获取地址信息的长度
socklen_t IPv6Address::getAddrlen() const {
    return sizeof(m_addr);
}

// 可读性输出IPv6地址和端口号
std::ostream& IPv6Address::insert(std::ostream& os) const {
    // 若系统是小端序，addr[0]：由s6_addr[0]和s6_addr[1]组成的小端序16位整数
    // 例：若s6_addr[0] = 0x11，s6_addr[1] = 0x22，则addr[0] = 0x2211
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    os << "[";

    // IPv6地址化简规则如下：
    // 2001:0DB8:0001:0023:0002:0003:200C:417A --> 2001:DB8:1:23:2:3:200C:417A
    // 2001:0DB8:0000:0023:0000:0000:200C:417A --> 2001:DB8::23:0:0:200C:417A
    // 0000:0000:0000:0000:0001:0000:0000:0000 --> ::1:0:0:0
    // 0000:0000:0000:0000:0000:0000:0000:0000 --> ::
    bool zero_flag = false;
    for(int i = 0; i < 8; i++) {
        if(addr[i] == 0 && !zero_flag) {
            continue;
        }
        if(i > 0 && addr[i-1] == 0 && !zero_flag) {
            zero_flag = true;
            os << ":";
        }
        if(i) {
            os << ":";
        }
        os << std::hex << byteSwapToBigEndian(addr[i]) << std::dec;
        //os << std::hex << addr[i] << std::dec;   //测试:字节颠倒
    }
    if(!zero_flag && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteSwapToLittleEndian(m_addr.sin6_port);
    return os;
}

// 获取该地址的广播地址(prefix_len为子网掩码位数)
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) const {
    if(prefix_len > 128) {
        return nullptr;
    }
    sockaddr_in6 addr(m_addr);
    int num1 = prefix_len % 8;
    int num2 = prefix_len / 8;
    addr.sin6_addr.s6_addr[num2] |= ~CreatMask<uint8_t>(num1);
    for(int i = num2 + 1; i < 16; i++) {
        addr.sin6_addr.s6_addr[i] |= 0xFF;
    }
    return std::make_shared<IPv6Address>(addr);
}

// 获取该地址的网段(网络地址)
IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) const {
    if(prefix_len > 128) {
        return nullptr;
    }
    sockaddr_in6 addr(m_addr);
    int num1 = prefix_len % 8;
    int num2 = prefix_len / 8;
    addr.sin6_addr.s6_addr[num2] &= CreatMask<uint8_t>(num1);
    for(int i = num2 + 1; i < 16; i++) {
        addr.sin6_addr.s6_addr[i] &= 0x00; 
    }
    return std::make_shared<IPv6Address>(addr);
}

// 获取子网掩码地址
IPAddress::ptr IPv6Address::subnetAddress(uint32_t prefix_len) const {
    if(prefix_len > 128) {
        return nullptr;
    }
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    int num1 = prefix_len % 8;
    int num2 = prefix_len / 8;
    subnet.sin6_addr.s6_addr[num2] = CreatMask<uint8_t>(num1);
    for(int i = 0; i < num2; i++) {
        subnet.sin6_addr.s6_addr[i] = 0xFF;
    }
    return std::make_shared<IPv6Address>(subnet);
}

// 获取端口号
uint16_t IPv6Address::getPort() const {
    return byteSwapToLittleEndian(m_addr.sin6_port);
}

// 设置端口号
void IPv6Address::setPort(uint16_t port) {
    m_addr.sin6_port = byteSwapToBigEndian(port);
}


// sockaddr_un结构体中sun_path成员的长度；sun_path是字符数组，末尾有'\0',所以减1(长度不包含结束符'\0')
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    /**
     * @brief size_t offsetof(type, member);
     * @details return the offset of the field [member] from the start of the structure [type]
    */
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const sockaddr_un& addr, socklen_t addrlen) {
    m_addr = addr;
    m_length = addrlen;
}

UnixAddress::UnixAddress(const std::string& path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size();

    if(m_length > MAX_PATH_LEN) {
        throw std::logic_error("path too long!");
    }

    //复制字符串内容到目标数组，包括结尾的'\0'
    memcpy(m_addr.sun_path, path.c_str(), m_length + 1);
    m_length += offsetof(sockaddr_un, sun_path);
}

// 获取UnixSocket地址信息(文件系统路径)，读写
sockaddr* UnixAddress::getAddr() {
    return (sockaddr*)&m_addr;
}

// 获取UnixSocket地址信息(文件系统路径)，只读
const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}

// 获取地址信息的长度
socklen_t UnixAddress::getAddrlen() const {
    return m_length;
}

// 设置地址信息的长度
void UnixAddress::setAddrlen(uint32_t len) {
    m_length = len;
}

// 获取字符串形式的UnixSocket地址
std::string UnixAddress::getPath() const {
    std::stringstream ss;
    //可不写，当sun_path[0] == '\0'时，m_length == offsetof(sockaddr_un, sun_path)，地址为空
    if(m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(m_addr.sun_path + 1, 
                                    m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}

// 可读性输出UnixSocket地址
std::ostream& UnixAddress::insert(std::ostream& os) const {
    //可不写，当sun_path[0] == '\0'时，m_length == offsetof(sockaddr_un, sun_path)，地址为空
    if(m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1, 
                                    m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}


UnkownAddress::UnkownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnkownAddress::UnkownAddress(const sockaddr& address) {
    m_addr = address;
}

sockaddr* UnkownAddress::getAddr() {
    return &m_addr;
}

const sockaddr* UnkownAddress::getAddr() const {
    return &m_addr;
}

socklen_t UnkownAddress::getAddrlen() const {
    return sizeof(m_addr);
}

std::ostream& UnkownAddress::insert(std::ostream& os) const {
    return os << "[UnkownAddress family = " << m_addr.sa_family << "]";
}


// 流式输出Address
std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.insert(os);
}


}