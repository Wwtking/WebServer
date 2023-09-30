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


}

#endif
