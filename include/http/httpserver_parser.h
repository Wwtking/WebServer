#ifndef http11_parser_h
#define http11_parser_h

#include "http_common.h"

typedef struct http_parser { 
  int cs;             // 错误标志
  size_t body_start;  // 消息体开始
  int content_len;    // 消息体长度
  size_t nread;       // 已解析的长度
  size_t mark;        // 内容标记
  size_t field_start; // 键开始
  size_t field_len;   // 键长度
  size_t query_start; // 请求参数开始
  int xml_sent;
  int json_sent;

  void *data;

  int uri_relaxed;

  // 函数指针，用于指向回调函数
  field_cb http_field;        // 处理HTTP请求头部
  element_cb request_method;  // 处理HTTP请求方法
  element_cb request_uri;     // 处理HTTP请求uri
  element_cb fragment;        // 处理HTTP片段标识符
  element_cb request_path;    // 处理HTTP请求路径
  element_cb query_string;    // 处理HTTP请求参数
  element_cb http_version;    // 处理HTTP版本号
  element_cb header_done;     // 
  
} http_parser;


/**
 * @brief 初始化parser
*/
int http_parser_init(http_parser *parser);

/**
 * @brief 检查解析是否已完成
 * @return 有错误返回-1，未完成返回0，已完成返回1
*/
int http_parser_finish(http_parser *parser);

/**
 * @brief 解析HTTP请求协议
 * @param[in] data 待解析的协议数据
 * @param[in] len 待解析的协议数据长度
 * @param[in] off 偏移量
 * @return 返回实际解析data的长度，它不是一次性解析完的，是分段解析的
*/
size_t http_parser_execute(http_parser *parser, const char *data, size_t len, size_t off);

/**
 * @brief 检查解析是否有错误
 * @return 无错误返回1，有错误返回0
*/
int http_parser_has_error(http_parser *parser);  // 有错误返回1，无错误返回0

/**
 * @brief 检查解析是否已完成
 * @return 未完成返回0，已完成返回1
*/
int http_parser_is_finished(http_parser *parser);

#define http_parser_nread(parser) (parser)->nread 

#endif
