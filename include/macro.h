#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <string>
#include <assert.h>
#include "log.h"
#include "util.h"


/**
 * @brief __builtin_expect(EXP, N)
 *          意思是：EXP == N的概率很大
 * @details if(__builtin_expect(x, 1))  //等价于if(x)，x大概率为1，执行if后面的语句的机会更大
 *          if(__builtin_expect(x, 0))  //等价于if(x)，x大概率为0，执行else后面的语句的机会更大   
 * @param __GNUC__ 是GCC编译器的预定义宏，用于表示代码是否在GCC编译器下编译
 * @param __llvm__ 是LLVM编译器的预定义宏，用于表示代码是否在LLVM编译器下编译
*/
#if defined __GNUC__ || __llvm__
/// LIKELY 宏的封装, 告诉编译器优化，条件大概率成立，即x大概率为真
#   define SYLAR_LIKELY(x)      __builtin_expect(!!(x), 1)
/// UNLIKELY 宏的封装, 告诉编译器优化，条件大概率不成立，即x大概率为假
#   define SYLAR_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#else
#   define SYLAR_LIKELY(x)      (x)
#   define SYLAR_UNLIKELY(x)    (x)
#endif


// 断言宏封装
// 断言触发的机率很小，所以用SYLAR_UNLIKELY
#define SYLAR_ASSERT(x) \
    if(SYLAR_UNLIKELY(!(x))) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " << #x \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(64, 2, "    "); \
        assert(x); \
    }

// 断言宏封装
#define SYLAR_ASSERT2(x, w) \
    if(SYLAR_UNLIKELY(!(x))) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " << #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(64, 2, "    "); \
        assert(x); \
    }


#endif