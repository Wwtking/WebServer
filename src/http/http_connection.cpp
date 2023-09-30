#include "log.h"
#include "http_connection.h"
#include "http_parser.h"

namespace sylar {
namespace http {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");


// HTTP响应结果的可读性输出
std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult: status=" << status 
        << " description=" << description 
        << " response=" << (response ? response->toString() : "nullptr")
        << "]";
    return ss.str();
}

// 构造函数
HttpConnection::HttpConnection(Socket::ptr sock, bool owner) 
    :SocketStream(sock, owner) {
}

// 析构函数
HttpConnection::~HttpConnection() {
    SYLAR_LOG_DEBUG(g_logger) << "~HttpConnection()";
}

/**
 * @brief 接收HTTP响应
 * @details HTTP消息体包括两种格式: 固定Content-Length 和 chunk
    (1) 固定Content-Length    HTTP的响应头包含Content-Length来指明数据的长度
    (2) chunk    不用确定响应数据的大小，即一边产生数据，一边发给客户端
        每个Chunk块分为头部和正文两部分，头部指定正文的字符总数，正文就是指定长度的实际内容
        chunk是分段传输，编码格式：[chunk size][\r\n][chunk data][\r\n]...[chunk size = 0][\r\n][\r\n]
        chunk的结束符为[0][\r\n][\r\n]  十六进制ASCII码为30 0d 0a 0d 0a  十进制ASCII码为48 13 10 13 10
*/
HttpResponse::ptr HttpConnection::recvHttpResponse() {
    HttpResponseParser::ptr parser = std::make_shared<HttpResponseParser>();
    size_t buff_size = HttpResponseParser::GetResponseMaxHeaderSize();
    // char* buffer(new char[buff_size]);    // 普通数组
    // 智能指针数组，自定义析构
    std::shared_ptr<char> buffer(new char[buff_size + 1], [](char* ptr) {
        delete[] ptr;
    });
    char* buff = buffer.get();
    size_t offset = 0;

    // 读取并解析HTTP响应头部并解析
    while(true) {
        // 读数据，offset是上次未解析数据的偏移
        int len = read(buff + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        // SYLAR_LOG_DEBUG(g_logger) << "read: len=" << len << " content:" << buff;
        
        buff[len + offset] = '\0';
        // 解析数据，len + offset为本次新读取的+上次未解析的
        size_t nparser = parser->execute(buff, len + offset, false);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        // SYLAR_LOG_DEBUG(g_logger) << "parser: nparser=" << nparser << " content:" << buff;

        // 更新偏移，解析完的数据将被移除，剩余未解析的数据为最新偏移
        offset = len + offset - nparser;
        // 缓存区满了都无数据解析，说明解析有问题
        if(offset == buff_size) {
            close();
            return nullptr;
        }

        if(parser->isFinished()) {
            break;
        }
    }

    // 读取HTTP响应数据body，如果是chunk格式要解析chunk头部
    auto& client_parser = parser->getParser();
    std::string body;
    // chunk格式，要解析头部再读取正文
    if(client_parser.chunked) {
        int len = offset;       // 当前读取到的body长度，即Chunk块长度
        do {
            bool flag = true;
            // 该while循环是为了确保能拿到Chunk正文的长度
            do {
                // SYLAR_LOG_DEBUG(g_logger) << "len=" << len;
                // 如果不加条件，在最后解析chunk的结束标志 0 \r\n \r\n 时会再次read，已没有内容可读，会报错
                // len == 0 表示要开始解析下一块chunk
                if(!flag || len == 0) {
                    int rt = read(buff + len, buff_size - len);
                    // SYLAR_LOG_DEBUG(g_logger) << "rt=" << rt;
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    len += rt;
                }

                buff[len] = '\0';
                // 解析chunk格式的头部部分，不解析正文部分
                size_t nparser = parser->execute(buff, len, true);
                if(parser->hasError()) {
                    close();
                    return nullptr;
                }

                len -= nparser;    // 未解析的数据长度
                if(len == (int)buff_size) {
                    close();
                    return nullptr;
                }
                flag = false;   // flag的目的是当前while中解析未完成，可回去继续读
            } while(!parser->isFinished());

            // parser->isFinished()后就有content_len了
            // content_len记录的是当前chunk块的正文长度
            SYLAR_LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len; 

            // 解析时 main := (Response | Chunked_Header) @done; 没有匹配后面的\r\n
            // 所以 execute 执行后，将已解析的移除，并没有移除\r\n，所以buff的首字符就是\r\n
            // 而解析 chunk 时，记录 chunk_size 在 \r\n 前，所以 content_len 只包括 chunk内容长度
            // 而 read 读取的 len 包括 \r\n，所以长度+2
            if(client_parser.content_len <= len - 2) {  // len中包含chunk正文长度 + '\r\n'两个字符
                // 当前chunk块的正文长度content_len小于len，说明buff中还有下一个chunk块的信息
                body.append(buff, client_parser.content_len);
                memmove(buff, buff + client_parser.content_len + 2, len - client_parser.content_len - 2);
                len -= client_parser.content_len + 2;
            }
            else {
                // 当前chunk块的正文长度content_len大于len，说明当前chunk块的正文还没有被完全读完，继续读直到达到content_len
                body.append(buff, len);
                int left = client_parser.content_len + 2 - len;
                while(left > 0) {
                    int rt = read(buff, left > (int)buff_size ? (int)buff_size : left);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    body.append(buff, rt);
                    left -= rt;
                }
                // body.resize(body.size() - 2);    // 减去body最后的'\r\n'
                len = 0;
            }
        } while(!client_parser.chunks_done);
        parser->getData()->setBody(body);
    }
    else {  // 非chunk格式，直接根据Content-Length读取body
        size_t length = parser->getContentLen();
        if(length > 0) {
            body.resize(length);

            // body部分不会被解析，所以要手动写入
            if(length > offset) {
                // body部分未完全读完，需要继续向后读取一部分
                std::memcpy(&body[0], buff, offset);
                if(readFixSize(&body[offset], length - offset) <= 0) {
                    close();
                    return nullptr;
                }
            }
            else {
                std::memcpy(&body[0], buff, length);
            }
            parser->getData()->setBody(body);
        }
    }

    return parser->getData();
}

// 发生HTTP请求
ssize_t HttpConnection::sendHttpRequest(HttpRequest::ptr request) {
    std::string res = request->toString();
    return writeFixSize(res.c_str(), res.size());  // 写入固定长度的数据
}

// 发送HTTP的GET请求
HttpResult::ptr HttpConnection::DoGet(const std::string& uri
                                    ,uint64_t timeout_ms
                                    ,const HttpRequest::MapType& headers
                                    ,const std::string& body) {
    Uri::ptr uri_ptr = Uri::CreateUri(uri);
    if(!uri_ptr) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::INVALID_URI
                                            , nullptr, "invalid uri: " + uri);
    }
    return DoGet(uri_ptr, timeout_ms, headers, body);
}

// 发送HTTP的GET请求
HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri
                                    ,uint64_t timeout_ms
                                    ,const HttpRequest::MapType& headers 
                                    ,const std::string& body) {
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

// 发送HTTP的POST请求
HttpResult::ptr HttpConnection::DoPost(const std::string& uri
                                    ,uint64_t timeout_ms
                                    ,const HttpRequest::MapType& headers 
                                    ,const std::string& body) {
    Uri::ptr uri_ptr = Uri::CreateUri(uri);
    if(!uri_ptr) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::INVALID_URI
                                            , nullptr, "invalid uri: " + uri);
    }
    return DoPost(uri_ptr, timeout_ms, headers, body);
}

// 发送HTTP的POST请求
HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri
                                    ,uint64_t timeout_ms
                                    ,const HttpRequest::MapType& headers 
                                    ,const std::string& body) {
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

// 发送HTTP请求
HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                                        ,const std::string& uri
                                        ,uint64_t timeout_ms
                                        ,const HttpRequest::MapType& headers 
                                        ,const std::string& body) {
    Uri::ptr uri_ptr = Uri::CreateUri(uri);
    if(!uri_ptr) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::INVALID_URI
                                            , nullptr, "invalid uri: " + uri);
    }
    return DoRequest(method, uri_ptr, timeout_ms, headers, body);
}

// 发送HTTP请求
HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                                        ,Uri::ptr uri
                                        ,uint64_t timeout_ms
                                        ,const HttpRequest::MapType& headers 
                                        ,const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setMethod(method);
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    bool hasHost = false;
    for(const auto& header : headers) {
        if(strcasecmp(header.first.c_str(), "Connection") == 0) {
            if(strcasecmp(header.second.c_str(), "Keep-Alive") == 0) {
                req->setClose(false);   // 长连接
            }
        }
        if(strcasecmp(header.first.c_str(), "Host") == 0) {
            hasHost = !header.second.empty();
            if(!hasHost) {    // 说明只有key,没有value,不放入Header
                continue;
            }
        }
        req->setHeader(header.first, header.second);
    }
    if(!hasHost) {
        req->setHeader("Host", uri->getHost());
    }
    req->setBody(body);
    return DoRequest(req, uri, timeout_ms);
}

// 发送HTTP请求
HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr request
                                        ,Uri::ptr uri
                                        ,uint64_t timeout_ms) {
    // 获取地址
    Address::ptr addr = uri->getAddress();
    if(!addr) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::INVALID_HOST
                                        , nullptr, "invalid host: " + uri->getHost());
    }

    // Socket绑定地址
    Socket::ptr sock = sylar::Socket::CreatTcpSocket(addr);
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::CREATE_SOCKET_ERROR
                                        , nullptr, "create socket error: " + addr->toString());
    }

    // 先创建socket，再连接
    if(!sock->connect(addr, timeout_ms)) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::CONNECT_ERROR
                                        , nullptr, "socket connect error: " + addr->toString());
    }
    sock->setRecvTimeout(timeout_ms);   // 设置超时时间

    // 封装HttpConnection，再去发送Http请求
    HttpConnection::ptr con = std::make_shared<HttpConnection>(sock);
    int rt = con->sendHttpRequest(request);
    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::SEND_CLOSE_BY_PEER, nullptr
                        , "send http request fail, socket closed by peer: " + addr->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::SEND_SOCKET_ERROR, nullptr
                        , "send http request fail, socket error: errno=" + std::to_string(errno)
                        + " errstr=" + strerror(errno));
    }

    // 接收Http响应
    HttpResponse::ptr res = con->recvHttpResponse();
    if(!res) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::TIMEOUT, nullptr 
                                            , "recv http response timeout: " + addr->toString()
                                            + " timeout_ms=" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Status::OK, res, "ok");
}

// 创建一个HTTPConnection连接池
HttpConnectionPool::ptr HttpConnectionPool::CreateConnectionPool(const std::string& uri,
                                                                const std::string& vhost,
                                                                uint32_t maxSize,
                                                                uint32_t maxAliveTime,
                                                                uint32_t maxRequest) {
    Uri::ptr uri_ptr = Uri::CreateUri(uri);
    if(!uri_ptr) {
        SYLAR_LOG_ERROR(g_logger) << "uri parser error, invalid uri=" << uri;
        return nullptr;
    }
    return std::make_shared<HttpConnectionPool>(uri_ptr->getHost(), vhost, uri_ptr->getPort()
                                                , maxSize, maxAliveTime, maxRequest);
}

// 构造函数
HttpConnectionPool::HttpConnectionPool(const std::string& host, 
                                    const std::string& vhost, 
                                    uint32_t port,
                                    uint32_t maxSize,
                                    uint32_t maxAliveTime,
                                    uint32_t maxRequest) 
    :m_host(host) 
    ,m_vhost(vhost) 
    ,m_port(port) 
    ,m_maxSize(maxSize) 
    ,m_maxAliveTime(maxAliveTime) 
    ,m_maxRequest(maxRequest) {
}

// 从连接池中拿连接，如果没有则创建新连接
HttpConnection::ptr HttpConnectionPool::getConnection(uint64_t timeout_ms) {
    HttpConnection* ptr = nullptr;
    std::list<HttpConnection*> invalid_con;

    MutexType::Lock lock(m_mutex);
    // 连接池有可用的连接，直接拿出来用
    while(!m_pool.empty()) {
        auto it = *m_pool.begin();
        m_pool.pop_front();
        if(!it->isConnected()) {
            invalid_con.push_back(it);
            continue;
        }
        if(it->m_createTime + m_maxAliveTime <= GetCurrentMS()) {
            invalid_con.push_back(it);
            continue;
        }
        ptr = it;
        break;
    }
    lock.unlock();

    // 删掉失效的连接
    for(auto i : invalid_con) {
        // SYLAR_LOG_ERROR(g_logger) << "m_total=" << m_total 
        //                         << " m_maxSize=" << m_maxSize
        //                         << "  |  m_request=" << i->m_request
        //                         << " m_maxRequest=" << m_maxRequest
        //                         << "  |  m_createTime=" << i->m_createTime;
        delete i;
    }
    m_total -= invalid_con.size();

    // 连接池没有可用的连接，需要新创建
    if(!ptr) {
        auto addr = Address::LookupAnyIPAddress(m_host);
        if(!addr) {
            SYLAR_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        // 因为上述LookupAnyIPAddress(m_host)中m_host只有域名没有端口号，所以要手动设置
        addr->setPort(m_port);

        Socket::ptr sock = Socket::CreatTcpSocket(addr);
        if(!sock) {
            SYLAR_LOG_ERROR(g_logger) << "create sock fail: " << addr->toString();
            return nullptr;
        }

        if(!sock->connect(addr, timeout_ms)) {
            SYLAR_LOG_ERROR(g_logger) << "sock connect fail: " << addr->toString();
            return nullptr;
        }
        
        sock->setRecvTimeout(timeout_ms);

        ptr = new HttpConnection(sock);
        ptr->m_createTime = GetCurrentMS();
        ++m_total;
    }

    // std::placeholders::_1为占位符，_1用于代替回调函数中的第一个参数
    return HttpConnection::ptr(ptr, std::bind(&ReleasePtr, std::placeholders::_1, this));
    // 上述语句的使用类比如下：
    // std::shared_ptr<char> buffer(new char[buff_size + 1], [](char* ptr) {
    //     delete[] ptr;
    // });
}

// 处理用完之后的连接
void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
    ptr->m_request++;

    // 连接已失效，则直接删掉
    if(!ptr->isConnected() 
            || pool->m_total > pool->m_maxSize
            || ptr->m_createTime + pool->m_maxAliveTime <= GetCurrentMS()
            || ptr->m_request >= pool->m_maxRequest) {
        // SYLAR_LOG_ERROR(g_logger) << "m_total=" << pool->m_total 
        //                         << " m_maxSize=" << pool->m_maxSize
        //                         << "  |  m_request=" << ptr->m_request
        //                         << " m_maxRequest=" << pool->m_maxRequest
        //                         << "  |  m_createTime=" << ptr->m_createTime;
        delete ptr;
        pool->m_total--;
        return;
    }

    // 连接还可以继续用，就放回连接池
    MutexType::Lock lock(pool->m_mutex);
    pool->m_pool.push_back(ptr);
}

// 发送HTTP的GET请求
HttpResult::ptr HttpConnectionPool::doGet(const std::string& uri
                                        ,uint64_t timeout_ms
                                        ,const HttpRequest::MapType& headers
                                        ,const std::string& body) {
    return doRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

// 发送HTTP的GET请求
HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri
                                        ,uint64_t timeout_ms
                                        ,const HttpRequest::MapType& headers
                                        ,const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
        << (uri->getQuery().empty() ? "" : "?")
        << uri->getQuery()
        << (uri->getFragment().empty() ? "" : "#")
        << uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}

// 发送HTTP的POST请求
HttpResult::ptr HttpConnectionPool::doPost(const std::string& uri
                                        ,uint64_t timeout_ms
                                        ,const HttpRequest::MapType& headers
                                        ,const std::string& body) {
    return doRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

// 发送HTTP的POST请求
HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri
                                        ,uint64_t timeout_ms
                                        ,const HttpRequest::MapType& headers
                                        ,const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
        << (uri->getQuery().empty() ? "" : "?")
        << uri->getQuery()
        << (uri->getFragment().empty() ? "" : "#")
        << uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}

// 发送HTTP请求
HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                                            ,const std::string& uri
                                            ,uint64_t timeout_ms
                                            ,const HttpRequest::MapType& headers
                                            ,const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setMethod(method);
    // 只需要设置path，而不用设置query和fragment，因为path里面包括了query和fragment
    req->setPath(uri);
    // 连接池中的连接必须是Keep-Alive，否则用完close后，下次再用还需重新创建，失去连接池的意义
    req->setClose(false);
    bool hasHost = false;
    for(const auto& header : headers) {
        if(strcasecmp(header.first.c_str(), "Connection") == 0) {
            if(strcasecmp(header.second.c_str(), "Keep-Alive") == 0) {
                req->setClose(false);   // 长连接
            }
        }
        else if(strcasecmp(header.first.c_str(), "Host") == 0) {
            hasHost = !header.second.empty();
            if(!hasHost) {    // 说明只有key,没有value,不放入Header
                continue;
            }
        }
        req->setHeader(header.first, header.second);
    }
    if(!hasHost) {
        if(m_vhost.empty()) {
            req->setHeader("Host", m_host);
        }
        else {
            req->setHeader("Host", m_vhost);
        }
        
    }
    req->setBody(body);
    return doRequest(req, timeout_ms);
}

// 发送HTTP请求
HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                                            ,Uri::ptr uri
                                            ,uint64_t timeout_ms
                                            ,const HttpRequest::MapType& headers
                                            ,const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
        << (uri->getQuery().empty() ? "" : "?")
        << uri->getQuery()
        << (uri->getFragment().empty() ? "" : "#")
        << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

// 发送HTTP请求
HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr request
                                            ,uint64_t timeout_ms) {
    HttpConnection::ptr con = getConnection(timeout_ms);  // 获取连接
    if(!con) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::POOL_GET_CONNECTION, nullptr
            , "get connection in pool fail: Host=" + m_host + ":" + std::to_string(m_port));
    }

    Socket::ptr sock = con->getSocket();
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::POOL_INVALID_CONNECTION, nullptr
            , "get socket in connection from pool fail: Host=" + m_host + ":" + std::to_string(m_port));
    }
    sock->setRecvTimeout(timeout_ms);

    // 发送HTTP请求
    int rt = con->sendHttpRequest(request);
    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::SEND_CLOSE_BY_PEER, nullptr
            , "send http request fail, socket closed by peer: " + m_host + ":" + std::to_string(m_port));
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::SEND_SOCKET_ERROR, nullptr
                        , "send http request fail, socket error: errno=" + std::to_string(errno)
                        + " errstr=" + strerror(errno));
    }

    // 接收HTTP响应
    HttpResponse::ptr res = con->recvHttpResponse();
    if(!res) {
        return std::make_shared<HttpResult>((int)HttpResult::Status::TIMEOUT, nullptr 
                        , "recv http response timeout: " + m_host + ":" + std::to_string(m_port)
                        + " timeout_ms=" + std::to_string(timeout_ms));
    }
    res->setClose(false);   // 线程池中连接为长连接
    return std::make_shared<HttpResult>((int)HttpResult::Status::OK, res, "ok");
}



}
}