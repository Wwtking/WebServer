#include <iostream>
#include "log.h"
#include "util.h"
#include "macro.h"
#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert() {
    //assert(false);    //通常会core dumped
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString();
    //SYLAR_ASSERT(false);
    SYLAR_ASSERT2(0 == 1, "abcd1234");
}

int main(int argc, char** argv) {
    test_assert();
    return 0;
}