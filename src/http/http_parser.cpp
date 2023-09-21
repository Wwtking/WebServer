#include "http_parser.h"
#include "log.h"
#include "config.h"


namespace sylar {
namespace http {


static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// HTTP请求头部的最大长度为4KB，超出说明有问题
static ConfigVar<uint64_t>::ptr g_http_request_maxHeaderSize = 
            Config::Lookup("http.request.max_header_size", 
            (uint64_t)(8 * 1024), "http request max header size");

// HTTP请求消息体的最大长度为64M，超出说明有问题
static ConfigVar<uint64_t>::ptr g_http_request_maxBodySize = 
            Config::Lookup("http.request.max_body_size", 
            (uint64_t)(64 * 1024 * 1024), "http request max body size");

// HTTP响应头部的最大长度为4KB，超出说明有问题
static ConfigVar<uint64_t>::ptr g_http_response_maxHeaderSize = 
            Config::Lookup("http.response.max_header_size", 
            (uint64_t)(8 * 1024), "http request max header size");

// HTTP响应消息体的最大长度为64M，超出说明有问题
static ConfigVar<uint64_t>::ptr g_http_response_maxBodySize = 
            Config::Lookup("http.response.max_body_size", 
            (uint64_t)(64 * 1024 * 1024), "http request max body size");

// 专门把数据定义uint64_t出来，而不直接用getValue()和setValue()
// 因为getValue()和setValue()时要加锁，频繁操作时会导致性能损失
// 而把数据uint64_t定义出来之后，就算有线程安全问题，也只是针对该数据不一致，仅仅一瞬间的数据不一致，影响不大
static uint64_t s_http_request_maxHeaderSize = g_http_request_maxHeaderSize->getValue();
static uint64_t s_http_request_maxBodySize = g_http_request_maxBodySize->getValue();
static uint64_t s_http_response_maxHeaderSize = g_http_response_maxHeaderSize->getValue();
static uint64_t s_http_response_maxBodySize = g_http_response_maxBodySize->getValue();

// 匿名空间：将命名空间的名字符号去掉，让其他文件找不到
// 对于变量或者函数来说，效果类似于static，但是static没法修饰一个类型，例如结构体、类
// 使用匿名空间就不用担心下面结构体在其它文件重名
// C++新标准提倡使用匿名命名空间，而不推荐使用static，因为static用在不同的地方涵义不同，容易造成混淆
namespace {

// 添加监听函数，配置改变时执行回调函数
struct HttpMaxSizeConfigInit {
    HttpMaxSizeConfigInit() {
        g_http_request_maxHeaderSize->addListener([](const uint64_t& oldVal, const uint64_t& newVal) {
            s_http_request_maxHeaderSize = newVal;
            SYLAR_LOG_INFO(g_logger) << "http request max header size " << "old_value=" << oldVal
                                                                        << " new_value=" << newVal;
        });
        g_http_request_maxBodySize->addListener([](const uint64_t& oldVal, const uint64_t& newVal) {
            s_http_request_maxBodySize = newVal;
            SYLAR_LOG_INFO(g_logger) << "http request max body size " << "old_value=" << oldVal
                                                                        << " new_value=" << newVal;
        });
        g_http_response_maxHeaderSize->addListener([](const uint64_t& oldVal, const uint64_t& newVal) {
            s_http_response_maxHeaderSize = newVal;
            SYLAR_LOG_INFO(g_logger) << "http response max header size " << "old_value=" << oldVal
                                                                        << " new_value=" << newVal;
        });
        g_http_response_maxBodySize->addListener([](const uint64_t& oldVal, const uint64_t& newVal) {
            s_http_response_maxBodySize = newVal;
            SYLAR_LOG_INFO(g_logger) << "http response max body size " << "old_value=" << oldVal
                                                                        << " new_value=" << newVal;
        });
    }
};
static HttpMaxSizeConfigInit _Init;  // main()函数前执行

}

// 获取HTTP请求头部的最大长度
uint64_t HttpRequestParser::GetRequestMaxHeaderSize() {
    return s_http_request_maxHeaderSize;
}

// 获取HTTP请求消息体的最大长度
uint64_t HttpRequestParser::GetRequestMaxBodySize() {
    return s_http_request_maxBodySize;
}

// 获取HTTP响应头部的最大长度
uint64_t HttpResponseParser::GetResponseMaxHeaderSize() {
    return s_http_response_maxHeaderSize;
}

// 获取HTTP响应消息体的最大长度
uint64_t HttpResponseParser::GetResponseMaxBodySize() {
    return s_http_response_maxBodySize;
}

/**
 * @brief 回调函数: 将解析得到的HTTP请求方法放入m_data
 * @param[in] data m_parser.data
 * @param[in] at 解析得到的字符串数据(HTTP请求方法)
 * @param[in] length 字符串数据长度
*/
void cb_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = (HttpRequestParser*)data;
    HttpMethod m = CharToHttpMethod(at);    //格式转换

    // 无效http请求方法
    if(m == HttpMethod::INVALID_METHOD) {
        SYLAR_LOG_WARN(g_logger) << "invalid http request method " 
                                << std::string(at, length);
        parser->setError(1000);  // 1000: invalid method
        return;
    }
    parser->getData()->setMethod(m);    //存放数据
}

// uri是由path、pat、fragment组成的
void cb_request_uri(void *data, const char *at, size_t length) {
}

/**
 * @brief 回调函数: 将解析得到的HTTP片段标识符放入m_data
 * @param[in] data m_parser.data
 * @param[in] at 解析得到的字符串数据(HTTP片段标识符)
 * @param[in] length 字符串数据长度
*/
void cb_request_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = (HttpRequestParser*)data;
    parser->getData()->setFragment(std::string(at, length));
}

/**
 * @brief 回调函数: 将解析得到的HTTP请求路径放入m_data
 * @param[in] data m_parser.data
 * @param[in] at 解析得到的字符串数据(HTTP请求路径)
 * @param[in] length 字符串数据长度
*/
void cb_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = (HttpRequestParser*)data;
    parser->getData()->setPath(std::string(at, length));
}

/**
 * @brief 回调函数: 将解析得到的HTTP请求参数放入m_data
 * @param[in] data m_parser.data
 * @param[in] at 解析得到的字符串数据(HTTP请求参数)
 * @param[in] length 字符串数据长度
*/
void cb_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = (HttpRequestParser*)data;
    parser->getData()->setQuery(std::string(at, length));
}

/**
 * @brief 回调函数: 将解析得到的HTTP版本号放入m_data
 * @param[in] data m_parser.data
 * @param[in] at 解析得到的字符串数据(HTTP版本号)
 * @param[in] length 字符串数据长度
*/
void cb_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = (HttpRequestParser*)data;

    // 格式转换
    uint8_t version = 0;
    // 比较两字符串的前length长度
    if(std::strncmp("HTTP/1.0", at, length) == 0) {
        version = 0x10;
    }
    else if(std::strncmp("HTTP/1.1", at, length) == 0) {
        version = 0x11;
    }
    else {
        // 无效http版本号
        SYLAR_LOG_WARN(g_logger) << "invalid http request version " 
                                << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(version);
}

void cb_request_header_done(void *data, const char *at, size_t length) {
}

/**
 * @brief 回调函数: 将解析得到的HTTP请求头部信息(key、value)放入m_data
 * @param[in] data m_parser.data
 * @param[in] field HTTP请求头部--key
 * @param[in] flen  key的长度
 * @param[in] value HTTP请求头部--value
 * @param[in] vlen  value的长度
*/
void cb_request_http_field(void *data, const char *field, size_t flen, 
                                        const char *value, size_t vlen) {
    HttpRequestParser* parser = (HttpRequestParser*)data;

    // http请求头部的key为空
    if(flen == 0) {
        SYLAR_LOG_WARN(g_logger) << "invalid http request field key=0";
        // 为了分段解析，这里不设置错误，
        // 因为有些header过长，读取内存设置过小的话，就需要把该header分段处理
        // header被截断，则可能会出现没有key值的情况
        // parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen), 
                                std::string(value, vlen));
}

// 构造函数
HttpRequestParser::HttpRequestParser() 
    :m_error(0) {
    m_data = std::make_shared<HttpRequest>();
    http_parser_init(&m_parser);
    m_parser.request_method = cb_request_method;
    m_parser.request_uri = cb_request_uri;
    m_parser.fragment = cb_request_fragment;
    m_parser.request_path = cb_request_path;
    m_parser.query_string = cb_request_query;
    m_parser.http_version = cb_request_version;
    m_parser.header_done = cb_request_header_done;
    m_parser.http_field = cb_request_http_field;
    m_parser.data = this;  // 使得在调用上述回调函数时，可以通过data存放解析出的数据
}

// 检查解析是否有错误
int HttpRequestParser::hasError() {
    return m_error ? m_error : http_parser_has_error(&m_parser);
}

// 检查解析是否已完成
int HttpRequestParser::isFinished() {
    return http_parser_finish(&m_parser);
}

// 解析HTTP请求协议
size_t HttpRequestParser::execute(char *data, size_t len) {
    // 对data数据进行解析，将解析得到的各部分数据，通过状态机中的自定义动作(action)进行处理
    // 在自定义动作(action)中，用函数指针执行相应函数对数据进行处理，其中函数参数void *data为this
    size_t offset = http_parser_execute(&m_parser, data, len, 0);

    /**
     * @brief void *memmove(void *str1, const void *str2, size_t n)
     * @details 从str2复制n个字符到str1，和memcpy的差别就是memmove函数处理的源内存块和目标内存块是可以重叠的
     * @param[in] str1 用于存储复制内容的目标数组
     * @param[in] str2 指向要复制的数据源
     * @param[in] n 要被复制的字节数
     * @return 返回一个指向目标存储区 str1 的指针
    */
    std::memmove(data, data + offset, len - offset);  // 移除已解析的数据
    return offset;
}

// 获取HTTP请求消息体的长度
uint64_t HttpRequestParser::getContentLen() {
    return m_data->getHeaderValue<uint64_t>("Content-Length", 0);
}


/**
 * @brief 回调函数: 将解析得到的HTTP响应状态的原因放入m_data
 * @param[in] data m_parser.data
 * @param[in] at 解析得到的字符串数据(HTTP响应状态的原因)
 * @param[in] length 字符串数据长度
*/
void cb_response_reason(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = (HttpResponseParser*)data;
    parser->getData()->setReason(std::string(at, length));
}

/**
 * @brief 回调函数: 将解析得到的HTTP响应状态放入m_data
 * @param[in] data m_parser.data
 * @param[in] at 解析得到的字符串数据(HTTP响应状态)
 * @param[in] length 字符串数据长度
*/
void cb_response_status(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = (HttpResponseParser*)data;

    /**
     * @brief atoi()函数用于把一个字符串转换成一个整型数据
     * @details 若首字母是非法字母，及无法转换的类型，输出为0
     * @example atoi("abc") == 0
     *          atoi("666abc") == 666
    */
    HttpStatus s = (HttpStatus)atoi(at);
    // 无效http响应状态
    if(s == (HttpStatus)0 || s == HttpStatus::INVALID_STATUS) {
        SYLAR_LOG_WARN(g_logger) << "invalid http response status " 
                                << (uint32_t)s;
        parser->setError(1000);
        return;
    }
    parser->getData()->setStatus(s);
}

void cb_response_chunk_size(void *data, const char *at, size_t length) {
}

/**
 * @brief 回调函数: 将解析得到的HTTP版本号放入m_data
 * @param[in] data m_parser.data
 * @param[in] at 解析得到的字符串数据(HTTP版本号)
 * @param[in] length 字符串数据长度
*/
void cb_response_version(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = (HttpResponseParser*)data;

    // 格式转换
    uint8_t version = 0;
    // 比较两字符串的前length长度
    if(std::strncmp("HTTP/1.0", at, length) == 0) {
        version = 0x10;
    }
    else if(std::strncmp("HTTP/1.1", at, length) == 0) {
        version = 0x11;
    }
    else {
        // 无效http版本号
        SYLAR_LOG_WARN(g_logger) << "invalid http response version " 
                                << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(version);
}

void cb_response_header_done(void *data, const char *at, size_t length) {
}

void cb_response_last_chunk(void *data, const char *at, size_t length) {
}

/**
 * @brief 回调函数: 将解析得到的HTTP响应头部信息(key、value)放入m_data
 * @param[in] data m_parser.data
 * @param[in] field HTTP响应头部--key
 * @param[in] flen  key的长度
 * @param[in] value HTTP响应头部--value
 * @param[in] vlen  value的长度
*/
void cb_response_http_field(void *data, const char *field, size_t flen, 
                                        const char *value, size_t vlen) {
    HttpResponseParser* parser = (HttpResponseParser*)data;

    // http请求头部的key为空
    if(flen == 0) {
        SYLAR_LOG_WARN(g_logger) << "invalid http response field key=0";
        // parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen), 
                                std::string(value, vlen));
}

// 构造函数
HttpResponseParser::HttpResponseParser() 
    :m_error(0) {
    m_data = std::make_shared<HttpResponse>();
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = cb_response_reason;
    m_parser.status_code = cb_response_status;
    m_parser.chunk_size = cb_response_chunk_size;
    m_parser.http_version = cb_response_version;
    m_parser.header_done = cb_response_header_done;
    m_parser.last_chunk = cb_response_last_chunk;
    m_parser.http_field = cb_response_http_field;
    m_parser.data = this;  // 使得在调用上述回调函数时，可以通过data存放解析出的数据
}

// 检查解析是否有错误
int HttpResponseParser::hasError() {
    return m_error ? m_error : httpclient_parser_has_error(&m_parser);
}

// 检查解析是否已完成
int HttpResponseParser::isFinished() {
    return httpclient_parser_finish(&m_parser);
}

// 解析HTTP响应协议
int HttpResponseParser::execute(char *data, size_t len, bool isChunk) {
    // 如果为chunck模式，要重新初始化置空，再解析chunk部分
    if(isChunk) {
        httpclient_parser_init(&m_parser);
    }
    int offset = httpclient_parser_execute(&m_parser, data, len, 0);
    std::memmove(data, data + offset, len - offset);  // 移除已解析的数据
    return offset;
}

// 获取HTTP响应消息体的长度
uint64_t HttpResponseParser::getContentLen() {
    return m_data->getHeaderValue<uint64_t>("Content-Length", 0);
}


}
}