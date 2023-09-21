#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__

#include <memory>
#include "http.h"
#include "httpserver_parser.h"
#include "httpclient_parser.h"


namespace sylar {
namespace http {

/**
 * @brief HTTP请求协议解析器
*/
class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;

    /**
     * @brief 构造函数
     * @details 初始化m_parser，创建m_data数据内存
    */
    HttpRequestParser();

    // 获取解析器
    const http_parser& getParser() { return m_parser; }

    // 获取解析后的数据
    HttpRequest::ptr getData() { return m_data; }

    // 设置错误码
    void setError(int error) { m_error = error; }

    /**
     * @brief 检查解析是否有错误
     * @return 返回0: 无错误
     *         返回1: http_parser内部错误
     *         返回1000: 遇到无效的请求方法
     *         返回1001: 遇到无效的HTTP版本号
     *         返回1002: 遇到无效的请求头部
    */
    int hasError();

    /**
     * @brief 检查解析是否已完成
     * @return 返回-1: 解析有错误
     *         返回0: 解析未完成
     *         返回1: 解析已完成
    */
    int isFinished();

    /**
     * @brief 解析HTTP请求协议
     * @param[in] data 待解析的协议数据
     * @param[in] len 待解析的协议数据长度
     * @return 返回实际解析的长度，并且将已解析的数据移除
    */
    size_t execute(char *data, size_t len);

    // 获取HTTP请求消息体的长度
    uint64_t getContentLen();
    
public:
    // 获取HTTP请求头部的最大长度
    static uint64_t GetRequestMaxHeaderSize();

    // 获取HTTP请求消息体的最大长度
    static uint64_t GetRequestMaxBodySize();

private:
    /// 解析器
    http_parser m_parser;
    /// 解析后的数据
    HttpRequest::ptr m_data;
    /// 错误码
    /// 1000: invalid method
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};


/**
 * @brief HTTP响应协议解析器
*/
class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;

    /**
     * @brief 构造函数
     * @details 初始化m_parser，创建m_data数据内存
    */
    HttpResponseParser();

    // 获取解析器
    const httpclient_parser& getParser() { return m_parser; }

    // 获取解析后的数据
    HttpResponse::ptr getData() { return m_data; }

    // 设置错误码
    void setError(int error) { m_error = error; }

    /**
     * @brief 检查解析是否有错误
     * @return 返回0: 无错误
     *         返回1: httpclient_parser内部错误
     *         返回1000: 遇到无效的响应状态
     *         返回1001: 遇到无效的HTTP版本号
     *         返回1002: 遇到无效的响应头部
    */
    int hasError();

    /**
     * @brief 检查解析是否已完成
     * @return 返回-1: 解析有错误
     *         返回0: 解析未完成
     *         返回1: 解析已完成
    */
    int isFinished();
    
    /**
     * @brief 解析HTTP响应协议
     * @param[in] data 待解析的协议数据
     * @param[in] len 待解析的协议数据长度
     * @return 返回实际解析的长度，并且将已解析的数据移除
    */
    int execute(char *data, size_t len, bool isChunk = false);

    // 获取HTTP响应消息体的长度
    uint64_t getContentLen();

public:
    // 获取HTTP响应头部的最大长度
    static uint64_t GetResponseMaxHeaderSize();

    // 获取HTTP响应消息体的最大长度
    static uint64_t GetResponseMaxBodySize();

private:
    /// 解析器
    httpclient_parser m_parser;
    /// 解析后的数据
    HttpResponse::ptr m_data;
    /// 错误码
    /// 1000: invalid status
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

}
}

#endif