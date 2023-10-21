#include "socket.h"
#include "log.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "hook.h"
#include "macro.h"
#include <netinet/tcp.h>


namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 创建TCP Socket(满足Address地址类型)
Socket::ptr Socket::CreatTcpSocket(Address::ptr address) {
    return std::make_shared<Socket>(address->getFamily(), TCP, 0);
}

// 创建UDP Socket(满足Address地址类型)
Socket::ptr Socket::CreatUdpSocket(Address::ptr address) {
    return std::make_shared<Socket>(address->getFamily(), UDP, 0);
}

// 创建IPv4的TCP Socket
Socket::ptr Socket::CreatIPv4TcpSocket() {
    return std::make_shared<Socket>(IPv4, TCP, 0);
}

// 创建IPv4的UDP Socket
Socket::ptr Socket::CreatIPv4UdpSocket() {
    return std::make_shared<Socket>(IPv4, UDP, 0);
}

// 创建IPv6的TCP Socket
Socket::ptr Socket::CreatIPv6TcpSocket() {
    return std::make_shared<Socket>(IPv6, TCP, 0);
}

// 创建IPv6的UDP Socket
Socket::ptr Socket::CreatIPv6UdpSocket() {
    return std::make_shared<Socket>(IPv6, UDP, 0);
}

// 创建Unix的TCP Socket
Socket::ptr Socket::CreatUnixTcpSocket() {
    return std::make_shared<Socket>(Unix, TCP, 0);
}

// 创建Unix的UDP Socket
Socket::ptr Socket::CreatUnixUdpSocket() {
    return std::make_shared<Socket>(Unix, UDP, 0);
}

// Socket构造函数
Socket::Socket(int family, int type, int protocol) 
    :m_sock(-1)
    ,m_family(family)
    ,m_type(type)
    ,m_protocol(protocol)
    ,m_isConnected(false) {
}

// 析构函数
Socket::~Socket() {
    close();
}

// 获取发送超时时间(毫秒)
uint64_t Socket::getSendTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->getFdCtx(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return (uint64_t)-1;
}

// 设置发送超时时间(毫秒)
void Socket::setSendTimeout(uint64_t time) {
    struct timeval tm{int(time / 1000), int(time % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_SNDTIMEO, tm);
    FdCtx::ptr ctx = FdMgr::GetInstance()->getFdCtx(m_sock);
    if(ctx) {
        ctx->setTimeout(SO_SNDTIMEO, time);
    }
}

// 获取接收超时时间(毫秒)
uint64_t Socket::getRecvTimeout() {
    FdCtx::ptr ctx = FdMgr::GetInstance()->getFdCtx(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return (uint64_t)-1;
}

// 设置接收超时时间(毫秒)
void Socket::setRecvTimeout(uint64_t time) {
    struct timeval tm{int(time / 1000), int(time % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tm);
    FdCtx::ptr ctx = FdMgr::GetInstance()->getFdCtx(m_sock);
    if(ctx) {
        ctx->setTimeout(SO_RCVTIMEO, time);
    }
}

// 获取sockopt
bool Socket::getOption(int level, int optname, void* optval, socklen_t* optlen) {
    // ::getsockopt()表示调用全局的getsockopt()函数，而不是在当前命名空间或类中定义的函数
    int rt = ::getsockopt(m_sock, level, optname, optval, (socklen_t*)optlen);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "getsockopt sockfd=" << m_sock << " level=" << level
            << " optname=" << optname << " errno=" << errno << " errstr=" << strerror(errno); 
        return false;
    }
    return true;
}   

// 设置sockopt
bool Socket::setOption(int level, int optname, const void* optval, socklen_t optlen) {
    int rt = ::setsockopt(m_sock, level, optname, optval, (socklen_t)optlen);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "setsockopt sockfd=" << m_sock << " level=" << level
            << " optname=" << optname << " errno=" << errno << " errstr=" << strerror(errno); 
        return false;
    }
    return true;
}

/**
 * @brief 客户端需要调用connect()连接服务器 
 *        int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
 * @param[in] sockfd socket文件描述符
 * @param[in] addr 传入参数，指定服务器端地址信息，含IP地址和端口号
 * @param[in] addrlen 传入参数，传入sizeof(addr)大小
 * @return 成功:0    失败:-1，设置errno
 */
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    if(!isValid()) {
        // 在客户端，socket()下一层即为connect()，所以给connect()赋予newSocket的功能
        if(SYLAR_UNLIKELY(!newSocket())) {
            return false;
        }
    }
    m_remoteAddress = addr;    //更新远端地址,对于客户端来说,远端地址为服务器地址

    // 通信协议族不一样
    if(SYLAR_UNLIKELY(m_family != addr->getFamily())) {
        SYLAR_LOG_ERROR(g_logger) << "connect sock.family(" << m_family << ") addr.family("
                        << addr->getFamily() << ") not equal, addr=" << addr->toString();
        return false;
    }

    if(timeout_ms == (uint64_t)-1) {
        int rt = ::connect(m_sock, addr->getAddr(), addr->getAddrlen());
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "connect(" << m_sock << ", " << addr->toString()
                            << ") fail, errno=" << errno << " errstr=" << strerror(errno); 
            close();
            return false;
        }
    }
    else {
        int rt = ::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrlen(), timeout_ms);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "connect_with_timeout(" << m_sock << ", " << addr->toString()
                            << ") fail, errno=" << errno << " errstr=" << strerror(errno); 
            close();
            return false;
        }
    }

    m_isConnected = true;
    getLocolAddress();
    getRemoteAddress();
    return true;
}

// 服务器程序所监听的网络地址和端口号通常是固定不变的，
// 客户端程序得知服务器程序的地址和端口号后就可以向服务器发起连接，
// 因此服务器需要调用bind绑定一个固定的网络地址和端口号
/**
 * @brief 对套接字进行地址和端口的绑定，将参数sockfd和addr(端口号和地址)绑定在一起 
 *        int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
 * @param[in] sockfd socket文件描述符
 * @param[in] addr 传入参数，指定服务器端地址信息，含IP地址和端口号
 * @param[in] addrlen 传入参数，传入sizeof(addr)大小
 * @return 成功:0    失败:-1，设置errno
 */
bool Socket::bind(const Address::ptr addr) {
    if(!isValid()) {
        // 在服务器端，socket()下一层即为bind()，所以给bind()赋予newSocket的功能
        if(SYLAR_UNLIKELY(!newSocket())) {
            return false;
        }
    }

    // 通信协议族不一样
    if(SYLAR_UNLIKELY(m_family != addr->getFamily())) {
        SYLAR_LOG_ERROR(g_logger) << "bind sock.family(" << m_family << ") addr.family("
                        << addr->getFamily() << ") not equal, addr=" << addr->toString();
        return false;
    }
    
    int rt = ::bind(m_sock, addr->getAddr(), addr->getAddrlen());
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "bind(" << m_sock << ", " << addr->toString()
                        << ") fail, errno=" << errno << " errstr=" << strerror(errno); 
        close();
        return false;
    }
    getLocolAddress();   //服务器初始化本地地址，此时没有远端地址
    return true;
}

//socket三次握手是在listen中完成，accept只从完成连接的队列中拿出一个连接
/**
 * @brief 设立服务器端套接字sock来监听客户端传来的连接请求
 *        int listen(int sock, int backlog);
 * @param[in] sock 希望进入等待连接请求状态的套接字文件描述符,
 *                 传递的描述符套接字参数成为服务器端套接字(监听套接字!!!!)
 * @param[in] backlog 连接请求等待队列的长度,若为5,则队列长度为5,表示最多使5个连接请求进入队列,
 *                    其中也包含排队建立3次握手队列和刚刚建立3次握手队列的连接数之和
 * @return 成功:0    失败:-1
 */
bool Socket::listen(int backlog) {
    if(!isValid()) {
        SYLAR_LOG_ERROR(g_logger) << "listen fail, sock=-1";
        return false;
    }

    int rt = ::listen(m_sock, backlog);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "listen(" << m_sock << ", " << backlog
                        << ") fail, errno=" << errno << " errstr=" << strerror(errno); 
        return false;
    }
    return true;
}

//如果服务器调用accept()时还没有客户端的连接请求，就阻塞等待直到有客户端连接上来
/**
 * @brief accept()函数接受连接请求等待队列中待处理的客户端连接请求
 *        int accept(int sockfd, struct sockaddr *addr, socklen_t *addlen);
 * @param[in] sockfd 服务器套接字的文件描述符
 * @param[in] addr 传出参数，返回的连接成功的信息，所以我们不需要对这个套接字进行初始化。返回链接客户端地址信息，含IP地址和端口号
 * @param[in] addrlen 传入传出参数，一开始传入一个参数防止溢出，调用完成之后长度会发生改变。返回真正接收到地址结构体的大小
 * @return 成功:返回一个新的socket文件描述符，用于和客户端通信。    失败:-1，设置errno
*/
Socket::ptr Socket::accept() {
    if(!isValid()) {
        SYLAR_LOG_ERROR(g_logger) << "accept fail, sock=-1";
        return nullptr;
    }

    // 保存accept()生成的新socket
    Socket::ptr sock = std::make_shared<Socket>(m_family, m_type, m_protocol);  
    // 不需要用addr和addrlen来接收客户端地址信息，因为会保存在返回值Socket::ptr中
    int fd = ::accept(m_sock, nullptr, nullptr);
    if(fd == -1) {
        SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sock << ", nullptr, nullptr) fail,"
                            << " errno=" << errno << " errstr=" << strerror(errno); 
        return nullptr;
    }
    
    if(sock->initSocket(fd)) {
        return sock;
    }
    return nullptr;
}

/**
 * @brief int close(int fd);
 * @param[in] fd 需要关闭的文件或套接字的文件描述符
 * @return 成功:0    失败:-1
*/
bool Socket::close() {
    if(!m_isConnected && m_sock == -1) {
        return true;
    }
    m_isConnected = false;
    if(m_sock != -1) {
        // 在hook重写的close()函数中已经将句柄从FdManager中删除了
        if(::close(m_sock)) {
            SYLAR_LOG_ERROR(g_logger) << "close(" << m_sock << ") fail,"
                            << " errno=" << errno << " errstr=" << strerror(errno); 
            return false;
        }
        m_sock = -1;
    }
    return true;
}

// 使用 read() 和 write() 适用于通用文件I/O操作，更接近于底层数据传输操作，可以用于非连接型套接字或面向连接的套接字
// 使用 send() 和 recv() 专为网络编程设计的函数，适用于面向连接的套接字(TCP)，提供更高层次的数据传输接口
// 使用 sendto() 和 recvfrom() 专为网络编程设计的函数，适用于无连接型套接字(UDP)，需要指定每个数据报的目标地址

/**
 * @brief ssize_t send(int sockfd, const void *buf, size_t len, int flags);
 *        发送buf内指定长度len的数据到指定的套接字sockfd
*/
ssize_t Socket::send(const void* buf, size_t len, int flags) {
    // 客户端在connect()成功后置 m_isConnected 为 true
    // 服务器端在accept()成功后置 m_isConnected 为 true
    if(isConnected()) {
        return ::send(m_sock, buf, len, flags);
    }
    return -1;
}

/**
 * @brief ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags); 
 * @param[in] msg:  struct msghdr {
                        void         *msg_name;       // 指向目标地址的指针
                        socklen_t    msg_namelen;     // 目标地址的长度
                        struct iovec *msg_iov;        // 数据的缓冲区数组
                        size_t       msg_iovlen;      // 缓冲区数组中的元素数量
                        void         *msg_control;    // 指向辅助数据的指针
                        socklen_t    msg_controllen;  // 辅助数据的长度
                        int          msg_flags;       // 用于指定接收和发送的标志
                    };
 * @param[in] buf: struct iovec {
                        void  *iov_base;  // 缓冲区起始地址
                        size_t iov_len;   // 缓冲区长度
                    };
*/
ssize_t Socket::send(const iovec* buf, size_t len, int flags) {
    if(isConnected()) {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buf;
        msg.msg_iovlen = len;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

/**
 * @brief ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                        const struct sockaddr *dest_addr, socklen_t addrlen);
 * @param[in] dest_addr sendto用于无连接型套接字(UDP)，所以需要指定每个数据报的目标地址dest_addr
*/
ssize_t Socket::sendTo(const void* buf, size_t len, const Address::ptr toAddr, int flags) {
    if(isConnected()) {
        return ::sendto(m_sock, buf, len, flags, toAddr->getAddr(), toAddr->getAddrlen());
    }
    return -1;
}

ssize_t Socket::sendTo(const iovec* buf, size_t len, const Address::ptr toAddr, int flags) {
    if(isConnected()) {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buf;
        msg.msg_iovlen = len;
        msg.msg_name = toAddr->getAddr();
        msg.msg_namelen = toAddr->getAddrlen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

/**
 * @brief ssize_t recv(int sockfd, void *buf, size_t len, int flags);
 *        从已连接的套接字sockfd接收数据，并将数据存在buf中，len为缓存区大小
*/
ssize_t Socket::recv(void* buf, size_t len, int flags) {
    if(isConnected()) {
        return ::recv(m_sock, buf, len, flags);
    }
    return -1;
}

/**
 * @brief ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
*/
ssize_t Socket::recv(iovec* buf, size_t len, int flags) {
    if(isConnected()) {
        struct msghdr msg;
        memset(&msg, 0 ,sizeof(msg));
        msg.msg_iov = (iovec*)buf;      //iovec数组
        msg.msg_iovlen = len;           //iovec数组长度
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

/**
 * @brief ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
 * @param[in] src_addr recvFrom用于无连接型套接字(UDP)，所以需要记录数据发送方的地址信息
*/
ssize_t Socket::recvFrom(void* buf, size_t len, Address::ptr fromAddr, int flags) {
    if(isConnected()) {
        socklen_t addrlen = fromAddr->getAddrlen();
        return ::recvfrom(m_sock, buf, len, flags, fromAddr->getAddr(), &addrlen);
    }
    return -1;
}

ssize_t Socket::recvFrom(iovec* buf, size_t len, Address::ptr fromAddr, int flags) {
    if(isConnected()) {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buf;
        msg.msg_iovlen = len;
        msg.msg_name = fromAddr->getAddr();
        msg.msg_namelen = fromAddr->getAddrlen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

// 获取本地地址
Address::ptr Socket::getLocolAddress() {
    if(m_locolAddress) {
        return m_locolAddress;    //如果存在直接返回
    }
    Address::ptr addr = nullptr;
    switch(m_family) {
        case AF_INET:
            addr = std::make_shared<IPv4Address>();
            break;
        case AF_INET6:
            addr = std::make_shared<IPv6Address>();
            break;
        case AF_UNIX:
            addr = std::make_shared<UnixAddress>();
            break;
        default:
            addr = std::make_shared<UnkownAddress>(m_family);
            break;
    }
    socklen_t addrlen = addr->getAddrlen();
    /**
     * @brief int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
     *        用于获取sockfd套接字对应的本地地址信息，并将地址信息保存在 addr 和 addrlen 中
     * @return 成功返回0，失败返回-1
    */
    if(getsockname(m_sock, addr->getAddr(), &addrlen)) {
        SYLAR_LOG_ERROR(g_logger) << "getsockname fail, sock=" << m_sock 
                    << "errno= " << errno << "errstr=" << strerror(errno);
        return nullptr;
    }

    // 如果是UnixAddress，需要设置地址长度m_length，通过setAddrlen()
    if(m_family == AF_UNIX) {
        // void setAddrlen(uint32_t len); 是派生类UnixAddress特有的，所以需要将基类强转成派生类再调用
        UnixAddress::ptr unixAddr = std::dynamic_pointer_cast<UnixAddress>(addr);
        unixAddr->setAddrlen(addrlen);
    }

    m_locolAddress = addr;
    return addr;
}

// 获取远端地址
Address::ptr Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;    //如果存在直接返回
    }
    Address::ptr addr = nullptr;
    switch(m_family) {
        case AF_INET:
            addr = std::make_shared<IPv4Address>();
            break;
        case AF_INET6:
            addr = std::make_shared<IPv6Address>();
            break;
        case AF_UNIX:
            addr = std::make_shared<UnixAddress>();
            break;
        default:
            addr = std::make_shared<UnkownAddress>(m_family);
            break;
    }
    socklen_t addrlen = addr->getAddrlen();
    /**
     * @brief int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
     *        用于获取sockfd套接字对应的远端地址信息，并将地址信息保存在 addr 和 addrlen 中
     * @return 成功返回0，失败返回-1
    */
    if(getpeername(m_sock, addr->getAddr(), &addrlen)) {
        SYLAR_LOG_ERROR(g_logger) << "getpeername fail, sock=" << m_sock 
                    << "errno= " << errno << "errstr=" << strerror(errno);
        return nullptr;
    }

    // 如果是UnixAddress，需要设置地址长度m_length，通过setAddrlen()
    if(m_family == AF_UNIX) {
        // void setAddrlen(uint32_t len); 是派生类UnixAddress特有的，所以需要将基类强转成派生类再调用
        UnixAddress::ptr unixAddr = std::dynamic_pointer_cast<UnixAddress>(addr);
        unixAddr->setAddrlen(addrlen);
    }

    m_remoteAddress = addr;
    return addr;
}

/**
 * @brief getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
 * @details 用于获取套接字sockfd的错误状态，并将错误码存储在error变量中
 * @return 如果返回值为0，且error为0，表示套接字没有发生错误；否则，error变量将包含套接字的错误码
*/
int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

// 用流输出信息
std::ostream& Socket::dump(std::ostream& os) const {
    os << "Socket information [sock=" << m_sock 
                                << ", family=" << m_family 
                                << ", type=" << m_type 
                                << ", protocol=" << m_protocol 
                                << ", isConnected=" << m_isConnected;
    if(m_locolAddress) {
        os << ", m_locolAddress=" << m_locolAddress->toString();
    }
    if(m_remoteAddress) {
        os << ", m_remoteAddress=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

// 字符串输出信息
std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

// 取消m_sock套接字上的读事件
bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(m_sock, IOManager::Event::READ);
}

// 取消m_sock套接字上的写事件
bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(m_sock, IOManager::Event::WRITE);
}

// 取消m_sock套接字上的accept事件
bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(m_sock, IOManager::Event::READ);
}

// 取消m_sock套接字上的所有事件
bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAllEvent(m_sock);
}

/**
 * @brief 使用socket()函数创建套接字  int socket(int af, int type, int protocol);
 * @param[in] af IP地址类型：AF_INET表示IPv4地址，AF_INET6表示IPv6地址
 * @param[in] type 数据传输方式/套接字类型 
 *                 SOCK_STREAM：流格式套接字/面向连接的套接字 
 *                 SOCK_DGRAM：数据报套接字/无连接的套接字
 * @param[in] protocol 传输协议：IPPROTO_TCP表示TCP传输协议，IPPTOTO_UDP表示UDP传输协议
 *                     将protocol的值设为0，系统会自动推演出应该使用什么协议
 * @return 返回值就是一个int类型的文件描述符，返回指向新创建的 socket(套接字) 的文件描述符
 */
bool Socket::newSocket() {
    m_sock = ::socket(m_family, m_type, m_protocol);
    if(SYLAR_LIKELY(m_sock != -1)) {
        setSocket();
        // 在hook重写的socket()函数中，已经将成功生成的sockfd放入到文件句柄集合了
        // FdMgr::GetInstance()->getFdCtx(m_sock, true);  //把新创建的socket放入文件句柄集合
    }
    else {
        SYLAR_LOG_ERROR(g_logger) << "socket(" << m_family << ", " << m_type << ", " << m_protocol 
                                    << ") fail, errno=" << errno << "errstr=" << strerror(errno);
        return false;
    }
    return true;
}

// 设置套接字描述符的属性
void Socket::setSocket() {
    int opt = 1;
    /**
     * @brief 将 SO_REUSEADDR 选项设置为1
     * @param[in] SOL_SOCKET 是选项的级别，表示套接字级别的选项
     * @param[in] SO_REUSEADDR 是要设置的选项名称，表示允许重用本地地址和端口
    */
    setOption(SOL_SOCKET, SO_REUSEADDR, opt);
    if(m_type == SOCK_STREAM) {
        /**
         * @brief 将 TCP_NODELAY 选项设置为1，表示禁用Nagle算法
         *        Nagle算法是一种优化算法，通过将较小的数据块组合成更大的数据包来降低网络传输的开销
         * @param[in] IPPROTO_TCP 是选项的级别, 表示TCP协议级别的选项
         * @param[in] TCP_NODELAY 是要设置的选项名称，表示禁用Nagle算法，意味着数据会立即发送而不进行缓冲
        */
        setOption(IPPROTO_TCP, TCP_NODELAY, opt);
    }
}

// 初始化Socket
bool Socket::initSocket(int sock) {
    // 在hook重写的accept()函数中，已经将成功生成的sockfd放入到文件句柄集合了
    FdCtx::ptr ctx = FdMgr::GetInstance()->getFdCtx(sock, true);  //找sock，若找不到则创建
    if(ctx && ctx->isSocket() && !ctx->isClose()) {
        m_sock = sock;
        m_isConnected = true;
        setSocket();
        getLocolAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}



namespace {
// OpenSSL初始化
struct OpenSSLInit {
    OpenSSLInit() {
        // 初始化OpenSSL库
        SSL_library_init();
        // 加载SSL错误字符串，将SSL库中的错误代码转换为可读的错误描述
        SSL_load_error_strings();
        // 加载所有支持的加密算法，会注册所有可用的加密算法
        OpenSSL_add_all_algorithms();
    }
};
static OpenSSLInit _Init;
}


// 创建TCP SSLSocket(满足Address地址类型)
Socket::ptr SSLSocket::CreatTcpSSLSocket(Address::ptr address) {
    return std::make_shared<SSLSocket>(address->getFamily(), TCP, 0);
}

// 创建IPv4的TCP SSLSocket
Socket::ptr SSLSocket::CreatIPv4TcpSSLSocket() {
    return std::make_shared<SSLSocket>(IPv4, TCP, 0);
}

// 创建IPv6的TCP SSLSocket
Socket::ptr SSLSocket::CreatIPv6TcpSSLSocket() {
    return std::make_shared<SSLSocket>(IPv6, TCP, 0);
}

// SSLSocket构造函数
SSLSocket::SSLSocket(int family, int type, int protocol) 
    :Socket(family, type, protocol) {
}

// 连接地址，客户端发起，连接服务器
bool SSLSocket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    bool v = Socket::connect(addr, timeout_ms);
    if(v) {
        // 创建SSL会话环境
        m_ctx.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free);
        // 建立SSL对象
        m_ssl.reset(SSL_new(m_ctx.get()), SSL_free);
        // SSL对象与文件描述符关联起来
        SSL_set_fd(m_ssl.get(), m_sock);
        // 建立SSL连接
        v = (SSL_connect(m_ssl.get()) == 1);
    }
    return v;
}

// 绑定地址，服务器端绑定
bool SSLSocket::bind(const Address::ptr addr) {
    return Socket::bind(addr);
}

// 监听socket，服务器端监听
bool SSLSocket::listen(int backlog) {
    return Socket::listen(backlog);
}

// 接收来自客户端的connect连接
Socket::ptr SSLSocket::accept() {
    if(!isValid()) {
        SYLAR_LOG_ERROR(g_logger) << "accept fail, sock=-1";
        return nullptr;
    }

    // 保存accept()生成的新SSLSocket
    SSLSocket::ptr sock = std::make_shared<SSLSocket>(m_family, m_type, m_protocol);  
    int fd = ::accept(m_sock, nullptr, nullptr);
    if(fd == -1) {
        SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sock << ", nullptr, nullptr) fail,"
                            << " errno=" << errno << " errstr=" << strerror(errno); 
        return nullptr;
    }
    
    sock->m_ctx = m_ctx;
    if(sock->initSocket(fd)) {
        return sock;
    }
    return nullptr;
}

// 初始化Socket
bool SSLSocket::initSocket(int sock) {
    bool v = Socket::initSocket(sock);
    if(v) {
        // 建立SSL对象
        m_ssl.reset(SSL_new(m_ctx.get()),  SSL_free);
        // SSL对象与文件描述符关联起来
        SSL_set_fd(m_ssl.get(), m_sock);
        // 接受SSL连接请求
        v = (SSL_accept(m_ssl.get()) == 1);
    }
    return v;
}

// 关闭socket
bool SSLSocket::close() {
    return Socket::close();
}

// 发送数据
ssize_t SSLSocket::send(const void* buf, size_t len, int flags) {
    if(m_ssl) {
        // 向SSL连接写入数据，将数据写入SSL连接中，经过加密后发送给对方
        return SSL_write(m_ssl.get(), buf, len);
    }
    return -1;
}

ssize_t SSLSocket::send(const iovec* buf, size_t len, int flags) {
    if(!m_ssl) {
        return -1;
    }
    int total = 0;
    for(size_t i = 0; i < len; ++i) {
        // 向SSL连接写入数据，将数据写入SSL连接中，经过加密后发送给对方
        int tmp = SSL_write(m_ssl.get(), buf[i].iov_base, buf[i].iov_len);
        if(tmp <= 0) {
            return tmp;
        }
        total += tmp;
        if(tmp != (int)buf[i].iov_len) {
            break;
        }
    }
    return total;
}

ssize_t SSLSocket::sendTo(const void* buf, size_t len, const Address::ptr toAddr, int flags) {
    SYLAR_ASSERT(false);
    return -1;
}

ssize_t SSLSocket::sendTo(const iovec* buf, size_t len, const Address::ptr toAddr, int flags) {
    SYLAR_ASSERT(false);
    return -1;
}

// 接受数据
ssize_t SSLSocket::recv(void* buf, size_t len, int flags) {
    if(m_ssl) {
        // 从SSL连接读取数据，从SSL连接中读取经过解密的数据
        return SSL_read(m_ssl.get(), buf, len);
    }
    return -1;
}

ssize_t SSLSocket::recv(iovec* buf, size_t len, int flags) {
    if(!m_ssl) {
        return -1;
    }
    int total = 0;
    for(size_t i = 0; i < len; ++i) {
        // 从SSL连接读取数据，从SSL连接中读取经过解密的数据
        int tmp = SSL_read(m_ssl.get(), buf[i].iov_base, buf[i].iov_len);
        if(tmp <= 0) {
            return tmp;
        }
        total += tmp;
        if(tmp != (int)buf[i].iov_len) {
            break;
        }
    }
    return total;
}

ssize_t SSLSocket::recvFrom(void* buf, size_t len, Address::ptr fromAddr, int flags) {
    SYLAR_ASSERT(false);
    return -1;
}

ssize_t SSLSocket::recvFrom(iovec* buf, size_t len, Address::ptr fromAddr, int flags) {
    SYLAR_ASSERT(false);
    return -1;
}

bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file) {
    // 创建一个用于SSL/TLS服务器的SSL_CTX对象
    m_ctx.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);

    // 设置SSL会话环境使用的证书文件，将证书文件加载到SSL会话环境中，以便在建立SSL连接时使用
    if(SSL_CTX_use_certificate_chain_file(m_ctx.get(), cert_file.c_str()) != 1) {
        SYLAR_LOG_ERROR(g_logger) << "SSL_CTX_use_certificate_chain_file("
                                << cert_file << ") error";
        return false;
    }

    // 设置SSL会话环境使用的私钥文件，将私钥文件加载到SSL会话环境中，以便在建立SSL连接时使用
    if(SSL_CTX_use_PrivateKey_file(m_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1) {
        SYLAR_LOG_ERROR(g_logger) << "SSL_CTX_use_PrivateKey_file("
                                << key_file << ") error";
        return false;
    }
    
    // 检查SSL会话环境中是否存在与私钥文件相对应的证书。如果存在返回1；否则返回0
    if(SSL_CTX_check_private_key(m_ctx.get()) != 1) {
        SYLAR_LOG_ERROR(g_logger) << "SSL_CTX_check_private_key cert_file="
                                << cert_file << " key_file=" << key_file;
        return false;
    }
    return true;
}

// 用流输出信息
std::ostream& SSLSocket::dump(std::ostream& os) const {
    os << "SSLSocket information [sock=" << m_sock 
                                << ", family=" << m_family 
                                << ", type=" << m_type 
                                << ", protocol=" << m_protocol 
                                << ", isConnected=" << m_isConnected;
    if(m_locolAddress) {
        os << ", m_locolAddress=" << m_locolAddress->toString();
    }
    if(m_remoteAddress) {
        os << ", m_remoteAddress=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

// 字符串输出信息
std::string SSLSocket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


}