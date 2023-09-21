#ifndef __SYLAR_MYENDIAN_H__
#define __SYLAR_MYENDIAN_H__

#include <iostream>
#include <byteswap.h>

#define SYLAR_LITTLE_ENDIAN  1
#define SYLAR_BIG_ENDIAN     2

namespace sylar {

/**
 * @brief 8字节类型的字节序转化
 * @details struct enable_if<true, T> { 
 *              using type = T; 
 *          };
 *          为 true 时，enable_if<true, T> 部分会实例化，定义了一个名为type的类型别名，类型为T
 *          为 false 时，enable_if 不会定义 type，因此无法通过 enable_if::type 访问，会报错
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteSwap(T val) {
    //将64位整数的字节顺序颠倒，从而实现大端和小端之间的转换
    return (T)bswap_64((uint64_t)val);
}

// 4字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteSwap(T val) {
    return (T)bswap_32((uint32_t)val);
}

// 2字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteSwap(T val) {
    return (T)bswap_16((uint16_t)val);
}

// 通过检查 BYTE_ORDER 的值来判断当前系统的字节序(大端字节序、小端字节序)
#if BYTE_ORDER == BIG_ENDIAN
// 系统使用大端字节序
#define SYLAR_BYTE_ORDER  SYLAR_BIG_ENDIAN
#else
// 系统使用小端字节序
#define SYLAR_BYTE_ORDER  SYLAR_LITTLE_ENDIAN
#endif


#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN
// 当前编译环境的字节序是大端序

// 在大端机器上从大端序转成小端序
template<class T>
T byteSwapToLittleEndian(T val) {
    return byteSwap(val);
}

// 在大端机器上从小端序转成大端序
template<class T>
T byteSwapToBigEndian(T val) {
    return byteSwap(val);
}

#else
// 当前编译环境的字节序是小端序

// 在小端机器上从大端序转成小端序
template<class T>
T byteSwapToLittleEndian(T val) {
    return byteSwap(val);
}

// 在小端机器上从小端序转成大端序
template<class T>
T byteSwapToBigEndian(T val) {
    return byteSwap(val);
}

#endif

}

#endif