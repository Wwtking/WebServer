/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef httpclient_parser_h
#define httpclient_parser_h

#include "http_common.h"

typedef struct httpclient_parser { 
  int cs;             // 错误标志
  size_t body_start;  // 消息体开始
  int content_len;    // 消息体长度
  int status;
  int chunked;
  int chunks_done;
  int close;
  size_t nread;       // 已解析的长度
  size_t mark;        // 内容标记
  size_t field_start; // 键开始
  size_t field_len;   // 键长度

  void *data;

  // 函数指针，用于指向回调函数
  field_cb http_field;      // 处理HTTP响应头部
  element_cb reason_phrase; // 处理HTTP响应状态的原因
  element_cb status_code;   // 处理HTTP响应状态
  element_cb chunk_size;    // 
  element_cb http_version;  // 处理HTTP版本号
  element_cb header_done;   // 
  element_cb last_chunk;    // 
  
} httpclient_parser;

/**
 * @brief 初始化parser
*/
int httpclient_parser_init(httpclient_parser *parser);

/**
 * @brief 检查解析是否已完成
 * @return 有错误返回-1，未完成返回0，已完成返回1
*/
int httpclient_parser_finish(httpclient_parser *parser);

/**
 * @brief 解析HTTP响应协议
 * @param[in] data 待解析的协议数据
 * @param[in] len 待解析的协议数据长度
 * @param[in] off 偏移量
 * @return 返回实际解析data的长度，它不是一次性解析完的，是分段解析的
*/
int httpclient_parser_execute(httpclient_parser *parser, const char *data, size_t len, size_t off);

/**
 * @brief 检查解析是否有错误
 * @return 无错误返回1，有错误返回0
*/
int httpclient_parser_has_error(httpclient_parser *parser);

/**
 * @brief 检查解析是否已完成
 * @return 未完成返回0，已完成返回1
*/
int httpclient_parser_is_finished(httpclient_parser *parser);

#define httpclient_parser_nread(parser) (parser)->nread 

#endif
