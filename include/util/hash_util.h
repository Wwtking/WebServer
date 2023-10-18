#ifndef __SYLAR_UTIL_HASH_UTIL_H__
#define __SYLAR_UTIL_HASH_UTIL_H__

#include <stdint.h>
#include <string>
#include <vector>

namespace sylar {

// base64编码
std::string base64encode(const std::string &data);
std::string base64encode(const void *data, size_t len);

// base64解码
std::string base64decode(const std::string &src);

std::string sha1sum(const std::string &data);
std::string sha1sum(const void *data, size_t len);

std::string random_string(size_t len
        ,const std::string& chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

/**
 * @brief 将字符串str1中的find替换为replaceWith
 * @param[in] str 原字符串
 * @param[in] find 待替换字符
 * @param[in] replaceWith 要替换字符
*/
std::string replace(const std::string &str, char find, char replaceWith);
std::string replace(const std::string &str, char find, const std::string &replaceWith);
std::string replace(const std::string &str, const std::string &find, const std::string &replaceWith);

/**
 * @brief 将字符串str按照delim分隔，将分割结果返回，最多分割max个
 * @param[in] str 原字符串
 * @param[in] delim 分隔字符
 * @param[in] max 最大分割数
*/
std::vector<std::string> split(const std::string &str, char delim, size_t max = ~0);
std::vector<std::string> split(const std::string &str, const char *delims, size_t max = ~0);

}

#endif
