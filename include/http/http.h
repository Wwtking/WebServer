#ifndef __SYLAR_HTTP_H__
#define __SYLAR_HTTP_H__

#include <memory>
#include <map>
#include <vector>
#include <string.h>
#include <boost/lexical_cast.hpp>

namespace sylar {
namespace http {

/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

/**
 * @brief HTTP方法枚举
 */
enum class HttpMethod {
#define XX(code, name, string) \
    name = code, 

    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

/**
 * @brief HTTP方法枚举
 */
enum class HttpStatus {
#define XX(code, name, string) \
    name = code, 
    
    HTTP_STATUS_MAP(XX)
#undef XX
    INVALID_STATUS
};

/**
 * @brief 将字符串方法名转成HTTP方法类枚举
 * @param[in] s 字符串形式的HTTP方法名
 * @return HTTP方法类枚举
*/
HttpMethod StringToHttpMethod(const std::string& s);

/**
 * @brief 将字符串方法名转成HTTP方法类枚举
 * @param[in] ch 字符串形式的HTTP方法名
 * @return HTTP方法类枚举
*/
HttpMethod CharToHttpMethod(const char* ch);

/**
 * @brief 将HTTP方法类枚举转换成字符串
 * @param[in] m HTTP方法类枚举
 * @return 字符串方法名
*/
const char* HttpMethodToString(HttpMethod m);

/**
 * @brief 将HTTP状态类枚举转换成字符串
 * @param[in] s HTTP状态类枚举
 * @return 字符串状态名
*/
const char* HttpStatusToString(HttpStatus s);

/**
 * @brief 忽略大小写的比较仿函数
 */
struct CompareFunctor {
    // 忽略大小写比较字符串
    bool operator()(const std::string& left, const std::string& right) const;
};


/**
 * @brief 获取模板MapType类型的map容器的value，并转换为T类型值
 * @param[in] m map数据结构
 * @param[in] key 关键字
 * @param[in] def 默认值
 * @return 返回转换成功后的值，若转换失败返回默认值
 */
template<class MapType, class T>
T GetMapValue(const MapType& m, const std::string& key, T def = T()) {
    auto it = m.find(key);
    if(it == m.end()) {
        return def;
    }
    try {
        // it->second为string类型，可以通过lexical_cast转换为int、float、double类型等
        return boost::lexical_cast<T>(it->second);
    }
    catch(...) {
    }
    return def;
}

/**
 * @brief 获取模板MapType类型的map容器的value，并转换为T类型值
 * @param[in] m map数据结构
 * @param[in] key 关键字
 * @param[out] val 保存转换后的值
 * @param[in] def 默认值
 * @return 返回true: 转换成功, value为对应的值
 *         返回false: 不存在或者转换失败，value = def
 */
template<class MapType, class T>
bool CheckGetMapValue(const MapType& m, const std::string& key, T& value, T def = T()) {
    auto it = m.find(key);
    if(it == m.end()) {
        value = def;
        return false;
    }
    try {
        // it->second为string类型，可以通过lexical_cast转换为int、float、double类型等
        value = boost::lexical_cast<T>(it->second);
        return true;
    }
    catch(...) {
        value = def;
    }
    return false;
}


/**
 * @brief HTTP请求信息封装类
 * @example request
 *  GET /books/?sex=man&name=Professional HTTP/1.1
    Host: www.wrox.com
    User-Agent: Mozilla/5.0
    Connection: Keep-Alive
*/
class HttpRequest {
public:
    typedef std::shared_ptr<HttpRequest> ptr;
    typedef std::map<std::string, std::string, CompareFunctor> MapType;

    // 有参构造
    HttpRequest(uint8_t version = 0x11, bool close = true);

    // 析构函数
    ~HttpRequest() {}

    // 获取HTTP请求方法
    HttpMethod getMethod() const {  return m_method; }

    // 获取HTTP版本号
    uint8_t getVersion() const {  return m_version; }

    // 是否为websocket
    bool isWebSocket() const { return m_websocket; }

    // 是否关闭连接
    bool isClose() const { return m_close; }
    
    // 获取HTTP请求路径
    const std::string& getPath() const { return m_path; }

    // 获取HTTP请求参数
    const std::string& getQuery() const { return m_query; }

    // 获取片段标识符
    const std::string& getFragment() const { return m_fragment; }

    // 获取HTTP请求消息体
    const std::string& getBody() const { return m_body; }

    // 获取HTTP请求头部 map
    const MapType& getHeaders() const { return m_headers; }

    // 获取HTTP请求参数 map
    const MapType& getParams() const { return m_params; }

    // 获取HTTP请求Cookie map
    const MapType& getCookies() const { return m_cookies; }

    // 设置HTTP请求方法
    void setMethod(HttpMethod m) { m_method = m; }

    // 设置HTTP版本号
    void setVersion(uint8_t version) { m_version = version; }

    // 设置是否为websocket
    void setWebSocket(bool v) { m_websocket = v; }

    // 设置是否关闭连接
    void setClose(bool v) { m_close = v; }

    // 设置HTTP请求路径
    void setPath(const std::string& path) { m_path = path; }

    // 设置HTTP请求参数
    void setQuery(const std::string& query) { m_query = query; }

    // 设置片段标识符
    void setFragment(const std::string& fragment) { m_fragment = fragment; }

    // 设置HTTP请求消息体
    void setBody(const std::string& body) { m_body = body; }

    // 设置HTTP请求头部 map
    void setHeaders(const MapType& headers) { m_headers = headers; }

    // 设置HTTP请求参数 map
    void setParams(const MapType& params) { m_params = params; }

    // 设置HTTP请求Cookie map
    void setCookies(const MapType& cookies) { m_cookies = cookies; }

    /**
     * @brief 根据key值获取HTTP请求头部的value值
     * @return 若存在返回value值，若不存在返回默认值def
    */
    const std::string& getHeader(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 根据key值获取HTTP请求参数的value值
     * @return 若存在返回value值，若不存在返回默认值def
    */
    const std::string& getParam(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 根据key值获取HTTP请求Cookie的value值
     * @return 若存在返回value值，若不存在返回默认值def
    */
    const std::string& getCookie(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 根据key值设置HTTP请求头部的value值
    */
    void setHeader(const std::string& key, const std::string& value);

    /**
     * @brief 根据key值设置HTTP请求参数的value值
    */
    void setParam(const std::string& key, const std::string& value);

    /**
     * @brief 根据key值设置HTTP请求Cookie的value值
    */
    void setCookie(const std::string& key, const std::string& value);

    /**
     * @brief 根据key值删除HTTP请求头部的value值
    */
    void delHeader(const std::string& key);

    /**
     * @brief 根据key值删除HTTP请求参数的value值
    */
    void delParam(const std::string& key);

    /**
     * @brief 根据key值删除HTTP请求Cookie的value值
    */
    void delCookie(const std::string& key);

    /**
     * @brief 判断HTTP请求的头部参数是否存在
     * @param[in] key 关键字
     * @param[out] value 如果头部参数存在,value非空则赋值
     * @return 返回是否存在
    */
    bool hasHeader(const std::string& key, std::string* value = nullptr) const;

    /**
     * @brief 判断HTTP请求的请求参数是否存在
     * @param[in] key 关键字
     * @param[out] value 如果头部参数存在,value非空则赋值
     * @return 返回是否存在
    */
    bool hasParam(const std::string& key, std::string* value = nullptr) const;

    /**
     * @brief 判断HTTP请求的Cookie参数是否存在
     * @param[in] key 关键字
     * @param[out] value 如果头部参数存在,value非空则赋值
     * @return 返回是否存在
    */
    bool hasCookie(const std::string& key, std::string* value = nullptr) const;

    /**
     * @brief 获取HTTP请求头部的value，并转换为T类型值
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 返回转换成功后的值，若转换失败返回默认值
    */
    template<class T>
    T getHeaderValue(const std::string& key, T def = T()) {
        return GetMapValue<MapType, T>(m_headers, key, def);
    }

    /**
     * @brief 获取HTTP请求头部的value，并转换为T类型值
     * @param[in] key 关键字
     * @param[out] value 保存转换后的值
     * @param[in] def 默认值
     * @return 返回true: 转换成功, value为对应的值
     *         返回false: 不存在或者转换失败，value = def
    */
    template<class T>
    bool checkGetHeaderValue(const std::string& key, T& value, T def = T()) {
        return CheckGetMapValue<MapType, T>(m_headers, key, value, def);
    }

    /**
     * @brief 获取HTTP请求参数的value，并转换为T类型值
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 返回转换成功后的值，若转换失败返回默认值
    */
    template<class T>
    T getParamValue(const std::string& key, T def = T()) {
        return GetMapValue<MapType, T>(m_params, key, def);
    }

    /**
     * @brief 获取HTTP请求参数的value，并转换为T类型值
     * @param[in] key 关键字
     * @param[out] value 保存转换后的值
     * @param[in] def 默认值
     * @return 返回true: 转换成功, value为对应的值
     *         返回false: 不存在或者转换失败，value = def
    */
    template<class T>
    bool checkGetParamValue(const std::string& key, T& value, T def = T()) {
        return CheckGetMapValue<MapType, T>(m_params, key, value, def);
    }

    /**
     * @brief 获取HTTP请求Cookie的value，并转换为T类型值
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 返回转换成功后的值，若转换失败返回默认值
    */
    template<class T>
    T getCookieValue(const std::string& key, T def = T()) {
        return GetMapValue<MapType, T>(m_cookies, key, def);
    }

    /**
     * @brief 获取HTTP请求Cookie的value，并转换为T类型值
     * @param[in] key 关键字
     * @param[out] value 保存转换后的值
     * @param[in] def 默认值
     * @return 返回true: 转换成功, value为对应的值
     *         返回false: 不存在或者转换失败，value = def
    */
    template<class T>
    bool checkGetCookieValue(const std::string& key, T& value, T def = T()) {
        return CheckGetMapValue<MapType, T>(m_cookies, key, value, def);
    }

    // 可读性输出HTTP请求所有信息
    std::ostream& dump(std::ostream& os) const;

    // 字符串方式输出HTTP请求所有信息
    std::string toString() const;

private:
    HttpMethod m_method;        // HTTP请求方法
    uint8_t m_version;          // HTTP版本号
    bool m_websocket;           // 是否为websocket
    bool m_close;               // 是否已关闭
    std::string m_path;         // 请求路径
    std::string m_query;        // 请求参数
    std::string m_fragment;     // 片段标识符
    std::string m_body;         // 请求消息体
    MapType m_headers;          // 请求头部 map
    MapType m_params;           // 请求参数 map
    MapType m_cookies;          // 请求Cookie map

    /**
     * @brief URL:http://www.aspxfans.com:8080/news/index.asp?boardID=5&ID=24618&page=1#name
     * @param[http]                       协议
     * @param[www.aspxfans.com]           主机(或域名)部分
     * @param[8080]                       端口部分
     * @param[/news/index.asp]            请求路径path
     * @param[boardID=5&ID=24618&page=1]  请求参数query
     * @param[name]                       片段标识符fragment
    */
    /**
     * @brief key: val 这种形式为headers
     * @example request
     *  GET /books/?sex=man&name=Professional HTTP/1.1
        Host: www.wrox.com
        User-Agent: Mozilla/5.0
        Connection: Keep-Alive
    */
};


/**
 * @brief HTTP响应信息封装类
 * @example response
 *  HTTP/1.0 200 OK
    Pragma: no-cache
    Content-Type: text/html
    Content-Length: 14988
    Connection: close
*/
class HttpResponse {
public:
    typedef std::shared_ptr<HttpResponse> ptr;
    typedef std::map<std::string, std::string, CompareFunctor> MapType;

    // 有参构造
    HttpResponse(uint8_t version = 0x11, bool close = true);

    // 析构函数
    ~HttpResponse() {}

    // 获取HTTP响应状态
    HttpStatus getStatus() const {  return m_status; }

    // 获取HTTP版本号
    uint8_t getVersion() const {  return m_version; }

    // 是否为websocket
    bool isWebSocket() const { return m_websocket; }

    // 是否连接关闭
    bool isClose() const { return m_close; }

    // 获取HTTP响应状态的原因
    const std::string& getReason() const { return m_reason; }

    // 获取HTTP响应消息体
    const std::string& getBody() const { return m_body; }

    // 获取HTTP响应头部map
    const MapType& getHeaders() const { return m_headers; }

    // 获取HTTP响应Cookie
    const std::vector<std::string>& getCookies() { return m_cookies; }

    // 设置HTTP响应状态
    void setStatus(HttpStatus s) { m_status = s; }

    // 设置HTTP版本号
    void setVersion(uint8_t version) { m_version = version; }

    // 设置是否为websocket
    void setWebSocket(bool v) { m_websocket = v; }

    // 设置是否连接关闭
    void setClose(bool v) { m_close = v; }

    // 设置HTTP响应状态的原因
    void setReason(const std::string& reason) { m_reason = reason; }

    // 设置HTTP响应消息体
    void setBody(const std::string& body) { m_body = body; }

    // 设置HTTP响应头部map
    void setHeaders(const MapType& headers) { m_headers = headers; }

    // 设置HTTP响应Cookie
    void setCookies(const std::vector<std::string>& cookies) { m_cookies = cookies; }

    /**
     * @brief 根据key值获取HTTP响应头部的value值
     * @return 若存在返回value值，若不存在返回默认值def
    */
    const std::string& getHeader(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 根据key值设置HTTP响应头部的value值
    */
    void setHeader(const std::string& key, const std::string& value);

    /**
     * @brief 根据key值删除HTTP响应头部的value值
    */
    void delHeader(const std::string& key);

    /**
     * @brief 判断HTTP响应的头部参数是否存在
     * @param[in] key 关键字
     * @param[out] value 如果头部参数存在,value非空则赋值
     * @return 返回是否存在
    */
    bool hasHeader(const std::string& key, std::string* value = nullptr) const;

    /**
     * @brief 获取HTTP响应头部的value，并转换为T类型值
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 返回转换成功后的值，若转换失败返回默认值
    */
    template<class T>
    T getHeaderValue(const std::string& key, T def = T()) {
        return GetMapValue<MapType, T>(m_headers, key, def);
    }

    /**
     * @brief 获取HTTP响应头部的value，并转换为T类型值
     * @param[in] key 关键字
     * @param[out] value 保存转换后的值
     * @param[in] def 默认值
     * @return 返回true: 转换成功, value为对应的值
     *         返回false: 不存在或者转换失败，value = def
    */
    template<class T>
    bool checkGetHeaderValue(const std::string& key, T& value, T def = T()) {
        return CheckGetMapValue<MapType, T>(m_headers, key, value, def);
    }

    // 可读性输出HTTP请求所有信息
    std::ostream& dump(std::ostream& os);

    // 字符串方式输出HTTP请求所有信息
    std::string toString();

private:
    HttpStatus m_status;                // HTTP响应状态
    uint8_t m_version;                  // HTTP版本号
    bool m_websocket;                   // 是否为websocket
    bool m_close;                       // 是否已关闭
    std::string m_reason;               // 响应状态的原因
    std::string m_body;                 // 响应消息体
    MapType m_headers;                  // 响应头部map
    std::vector<std::string> m_cookies; // 响应Cookie
    
    /**
     * @example response
     *  HTTP/1.0 200 OK
        Pragma: no-cache
        Content-Type: text/html
        Content-Length: 14988
        Connection: close
    */
};

}
}


#endif