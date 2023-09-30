#include "hash_util.h"
#include <string.h>
#include <openssl/sha.h>


namespace sylar {

/**
 * @details base64编码方式：
            1、将待转换的字符串每三个字节分为一组，每个字节占8bit，则共有24个二进制位
            2、将上面的24个二进制位每6个一组，共分为4组
            3、在每组前面添加两个0，每组由6个变为8个二进制位，总共32个二进制位，即四个字节
            4、根据Base64编码对照表（见下图）获得对应的值
                    0　A　　　17　R　　　34　i　　　51　z
                    1　B　　　18　S　　　35　j　　　52　0
                    2　C　　　19　T　　　36　k　　　53　1
                    3　D　　　20　U　　　37　l　　　54　2
                    4　E　　　21　V　　　38　m　　　55　3
                    5　F　　　22　W　　　39　n　　　56　4
                    6　G　　　23　X　　　40　o　　　57　5
                    7　H　　　24　Y　　　41　p　　　58　6
                    8　I　　　25　Z　　　42　q　　　59　7
                    9　J　　　26　a　　　43　r　　　60　8
                    10　K　　 27　b　　　44　s　　　61　9
                    11　L　　 28　c　　　45　t　　　62　+
                    12　M　　 29　d　　　46　u　　　63　/
                    13　N　　 30　e　　　47　v
                    14　O　　 31　f　　　48　w
                    15　P　　 32　g　　　49　x
                    16　Q　　 33　h　　　50　y
 * @see https://blog.csdn.net/wo541075754/article/details/81734770
*/

std::string base64encode(const std::string& data) {
    return base64encode(data.c_str(), data.size());
}

std::string base64encode(const void* data, size_t len) {
    // 64个可打印的字符
    const char* base64 =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string ret;
    ret.reserve(len * 4 / 3 + 2);

    const unsigned char* ptr = (const unsigned char*)data;
    const unsigned char* end = ptr + len;

    while(ptr < end) {
        unsigned int packed = 0;
        int i = 0;
        int padding = 0;
        // 待转换的字符串中每三个字节分为一组
        for(; i < 3 && ptr < end; ++i, ++ptr) {
            packed = (packed << 8) | *ptr;
        }
        // 可能出现不满三个字节为一组
        if(i == 2) {
            padding = 1;
        } else if (i == 1) {
            padding = 2;
        }
        for(; i < 3; ++i) {
            packed <<= 8;
        }

        // 将上述字节组按6位为一组划分的话，至少会分成两组
        // 所以先把这两个6位append进去
        ret.append(1, base64[packed >> 18]);
        ret.append(1, base64[(packed >> 12) & 0x3f]);
        if(padding != 2) {
            // padding 可能为 0 或 1，则字节组至少有两个字节，按6位划分至少三组
            ret.append(1, base64[(packed >> 6) & 0x3f]);
        }
        if(padding == 0) {
            // 字节组有三个字节，按6位划分为四组
            ret.append(1, base64[packed & 0x3f]);
        }
        // 如果padding为2，说明字节组只有一个字节，按6位划分成两组，后两组补'='
        ret.append(padding, '=');
    }

    return ret;
}

std::string base64decode(const std::string &src) {
    std::string result;
    result.resize(src.size() * 3 / 4);
    char *writeBuf = &result[0];

    const char* ptr = src.c_str();
    const char* end = ptr + src.size();

    while(ptr < end) {
        int i = 0;
        int padding = 0;
        int packed = 0;
        // 三个字节的字节组按6位划分为四组
        for(; i < 4 && ptr < end; ++i, ++ptr) {
            // 检查末尾'='数量
            if(*ptr == '=') {
                ++padding;
                packed <<= 6;
                continue;
            }

            // 末尾只能补'='
            if (padding > 0) {
                return "";
            }

            // 将64个可打印字符转化成对应数字
            int val = 0;
            if(*ptr >= 'A' && *ptr <= 'Z') {
                val = *ptr - 'A';
            } else if(*ptr >= 'a' && *ptr <= 'z') {
                val = *ptr - 'a' + 26;
            } else if(*ptr >= '0' && *ptr <= '9') {
                val = *ptr - '0' + 52;
            } else if(*ptr == '+') {
                val = 62;
            } else if(*ptr == '/') {
                val = 63;
            } else {
                return ""; // invalid character
            }

            packed = (packed << 6) | val;
        }
        if (i != 4) {
            return "";
        }
        if (padding > 0 && ptr != end) {
            return "";
        }
        if (padding > 2) {
            return "";
        }

        // 将得到的24位packed按一个字节8位拼接
        *writeBuf++ = (char)((packed >> 16) & 0xff);
        if(padding != 2) {
            *writeBuf++ = (char)((packed >> 8) & 0xff);
        }
        if(padding == 0) {
            *writeBuf++ = (char)(packed & 0xff);
        }
    }

    result.resize(writeBuf - result.c_str());
    return result;
}

// SHA-1是一种哈希算法，用于将数据压缩并创建唯一的数字指纹
// SHA-1生成一个160位的散列值，通常以40个十六进制数字的形式表示
std::string sha1sum(const void *data, size_t len) {
    SHA_CTX ctx;
    std::string result;
    result.resize(SHA_DIGEST_LENGTH);
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, len);
    SHA1_Final((unsigned char*)&result[0], &ctx);
    return result;
}

std::string sha1sum(const std::string &data) {
    return sha1sum(data.c_str(), data.size());
}

std::string random_string(size_t len, const std::string& chars) {
    if(len == 0 || chars.empty()) {
        return "";
    }
    std::string rt;
    rt.resize(len);
    int count = chars.size();
    for(size_t i = 0; i < len; ++i) {
        rt[i] = chars[rand() % count];
    }
    return rt;
}

}