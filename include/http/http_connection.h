#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include <memory>
#include "http.h"
#include "socket_stream.h"
#include "uri.h"
#include <list>
#include "thread.h"

namespace sylar {
namespace http {

// HttpSession/HttpConnection
// server.accept  socket -> Session
// client.connect socket -> Connection

/**
 * @brief HTTP响应结果
 */
struct HttpResult {
    typedef std::shared_ptr<HttpResult> ptr;

    /**
     * @brief 状态码定义
     */
    enum class Status {
        OK = 0,                     /// 正常
        INVALID_URI = 1,            /// 非法URL
        INVALID_HOST = 2,           /// 无法解析HOST
        CREATE_SOCKET_ERROR = 3,    /// 创建Socket失败
        CONNECT_ERROR = 4,          /// 连接失败
        SEND_CLOSE_BY_PEER = 5,     /// 连接被对端关闭
        SEND_SOCKET_ERROR = 6,      /// 发送请求产生Socket错误
        TIMEOUT = 7,                /// 超时
        POOL_GET_CONNECTION = 8,    /// 从连接池中取连接失败
        POOL_INVALID_CONNECTION = 9 /// 无效的连接
    };  

    /**
     * @brief 构造函数
     * @param[in] _status 状态码
     * @param[in] _response HTTP响应结构体
     * @param[in] _description 状态描述
     */
    HttpResult(int _status
            , HttpResponse::ptr _response
            , const std::string& _description)
        :status(_status)
        ,response(_response)
        ,description(_description) {
    }

    /**
     * @brief 可读性输出
     */
    std::string toString() const;

    int status;                 // 状态码 
    HttpResponse::ptr response; // HTTP响应结构体
    std::string description;    // 状态描述
};

class HttpConnectionPool;

/**
 * @brief HTTPConnection封装
 * @details session是服务器端accept成功产生的socket
 */
class HttpConnection : public SocketStream {
friend class HttpConnectionPool;
public:
    typedef std::shared_ptr<HttpConnection> ptr;

    /**
     * @brief 发送HTTP的GET请求
     * @param[in] uri 请求的字符串uri
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    static HttpResult::ptr DoGet(const std::string& uri
                                ,uint64_t timeout_ms
                                ,const HttpRequest::MapType& headers = {}
                                ,const std::string& body = "");

    /**
     * @brief 发送HTTP的GET请求
     * @param[in] uri Uri结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    static HttpResult::ptr DoGet(Uri::ptr uri
                                ,uint64_t timeout_ms
                                ,const HttpRequest::MapType& headers = {}
                                ,const std::string& body = "");

    /**
     * @brief 发送HTTP的POST请求
     * @param[in] uri 请求的字符串uri
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    static HttpResult::ptr DoPost(const std::string& uri
                                ,uint64_t timeout_ms
                                ,const HttpRequest::MapType& headers = {}
                                ,const std::string& body = "");

    /**
     * @brief 发送HTTP的POST请求
     * @param[in] uri Uri结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    static HttpResult::ptr DoPost(Uri::ptr uri
                                ,uint64_t timeout_ms
                                ,const HttpRequest::MapType& headers = {}
                                ,const std::string& body = "");

    /**
     * @brief 发送HTTP请求
     * @param[in] method 请求类型
     * @param[in] uri 请求的字符串uri
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    static HttpResult::ptr DoRequest(HttpMethod method
                                    ,const std::string& uri
                                    ,uint64_t timeout_ms
                                    ,const HttpRequest::MapType& headers = {}
                                    ,const std::string& body = "");

    /**
     * @brief 发送HTTP请求
     * @param[in] method 请求类型
     * @param[in] uri Uri结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    static HttpResult::ptr DoRequest(HttpMethod method
                                    ,Uri::ptr uri
                                    ,uint64_t timeout_ms
                                    ,const HttpRequest::MapType& headers = {}
                                    ,const std::string& body = "");

    /**
     * @brief 发送HTTP请求
     * @param[in] request Http请求结构体
     * @param[in] uri Uri结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @return 返回HTTP结果结构体
     */
    static HttpResult::ptr DoRequest(HttpRequest::ptr request
                                    ,Uri::ptr uri
                                    ,uint64_t timeout_ms);

    /**
     * @brief 构造函数
     * @param[in] sock Socket类型
     * @param[in] owner 是否完全控制
     *                  如果是true，那么全权管理，析构时帮忙释放Socket
     *                  如果是false，那么仅仅完成相应的操作，不帮忙析构
     */
    HttpConnection(Socket::ptr sock, bool owner = false);

    /**
     * @brief 析构函数
    */
    ~HttpConnection();

    /**
     * @brief 接收HTTP响应
     * @details 接收到HTTP数据并且进行解析
     * @return 返回解析后的HttpResponse封装数据
    */
    HttpResponse::ptr recvHttpResponse();

    /**
     * @brief 发生HTTP请求
     * @param[in] request HTTP请求
     * @return >0 发送成功
     *         =0 对方关闭
     *         <0 Socket异常
     */
    ssize_t sendHttpRequest(HttpRequest::ptr request);
    
private:
    uint64_t m_createTime = 0;  // 该连接的创建时间
    uint64_t m_request = 0;     // 该连接的请求次数(使用次数)
};

/**
 * @brief HTTPConnection连接池
 * @details 该连接池中存放着许多Keep-Alive到固定域名的HTTPConnection
 */
class HttpConnectionPool {
public:
    typedef std::shared_ptr<HttpConnectionPool> ptr;
    typedef Mutex MutexType;

    /**
     * @brief 创建一个HTTPConnection连接池
     * @param[in] uri 请求的字符串uri
     * @param[in] vhost 固定域名
     * @param[in] maxSize 连接池的最大容量
     * @param[in] maxAliveTime 连接池中连接的最大存活时间
     * @param[in] maxRequest 连接池中连接的最大请求次数
     * @return 返回连接池
     */
    static HttpConnectionPool::ptr CreateConnectionPool(const std::string& uri,
                                                        const std::string& vhost,
                                                        uint32_t maxSize,
                                                        uint32_t maxAliveTime,
                                                        uint32_t maxRequest);

    /**
     * @brief 构造函数
     * @param[in] host 固定域名
     * @param[in] vhost 固定域名
     * @param[in] port 端口号
     * @param[in] maxSize 连接池的最大容量
     * @param[in] maxAliveTime 连接池中连接的最大存活时间
     * @param[in] maxRequest 连接池中连接的最大请求次数
     */
    HttpConnectionPool(const std::string& host, 
                    const std::string& vhost, 
                    uint32_t port,
                    uint32_t maxSize,
                    uint32_t maxAliveTime,
                    uint32_t maxRequest);

    /**
    * @brief 从连接池中拿连接，如果没有则创建新连接
    * @param[in] timeout_ms 超时时间(毫秒)
    * @return 返回连接池
    */
    HttpConnection::ptr getConnection(uint64_t timeout_ms);

    /**
     * @brief 发送HTTP的GET请求
     * @param[in] uri 请求的字符串uri
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    HttpResult::ptr doGet(const std::string& uri
                        ,uint64_t timeout_ms
                        ,const HttpRequest::MapType& headers = {}
                        ,const std::string& body = "");

    /**
     * @brief 发送HTTP的GET请求
     * @param[in] uri Uri结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    HttpResult::ptr doGet(Uri::ptr uri
                        ,uint64_t timeout_ms
                        ,const HttpRequest::MapType& headers = {}
                        ,const std::string& body = "");

    /**
     * @brief 发送HTTP的POST请求
     * @param[in] uri 请求的字符串uri
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    HttpResult::ptr doPost(const std::string& uri
                        ,uint64_t timeout_ms
                        ,const HttpRequest::MapType& headers = {}
                        ,const std::string& body = "");

    /**
     * @brief 发送HTTP的POST请求
     * @param[in] uri Uri结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    HttpResult::ptr doPost(Uri::ptr uri
                        ,uint64_t timeout_ms
                        ,const HttpRequest::MapType& headers = {}
                        ,const std::string& body = "");

    /**
     * @brief 发送HTTP请求
     * @param[in] method 请求类型
     * @param[in] uri 请求的字符串uri
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    HttpResult::ptr doRequest(HttpMethod method
                            ,const std::string& uri
                            ,uint64_t timeout_ms
                            ,const HttpRequest::MapType& headers = {}
                            ,const std::string& body = "");

    /**
     * @brief 发送HTTP请求
     * @param[in] method 请求类型
     * @param[in] uri Uri结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
    HttpResult::ptr doRequest(HttpMethod method
                            ,Uri::ptr uri
                            ,uint64_t timeout_ms
                            ,const HttpRequest::MapType& headers = {}
                            ,const std::string& body = "");

    /**
     * @brief 发送HTTP请求
     * @param[in] req HTTP请求结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @return 返回HTTP结果结构体
     */
    HttpResult::ptr doRequest(HttpRequest::ptr request
                            ,uint64_t timeout_ms);

private:
    /**
    * @brief 处理用完之后的连接
    * @details 如果该连接已失效则直接删掉，如果还可以继续用就放回连接池
    * @param[in] ptr 待处理的连接
    * @param[in] pool 该连接所属的连接池
    */
    static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

private:
    std::string m_host;         // 该连接池连接到的固定域名
    std::string m_vhost;        // 固定域名
    uint32_t m_port;            // 端口号
    uint32_t m_maxSize;         // 连接池的最大连接数
    uint32_t m_maxAliveTime;    // 连接池中连接的最大存活时间
    uint32_t m_maxRequest;      // 连接池中连接的最大请求次数

    MutexType m_mutex;
    std::list<HttpConnection*> m_pool;      // 连接池
    std::atomic<uint32_t> m_total = {0};    // 连接池数量

    // m_maxSize并不是绝对的连接池最大数量，如果当前连接数已经达到m_maxSize，又有新的连接要用，则还是要继续创建
    // 只不过当连接用完放回去的时候，如果连接池达到m_maxSize，则直接释放掉该连接，相当于短连接
};


}
}

#endif