#include <iostream>
#include <vector>
#include <string>
#include "log.h"
#include "fiber.h"
#include "thread.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber() {
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
    //sylar::Fiber::GetThis()->swapOut();
    sylar::Fiber::YieldToHold();
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
}
void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "test_fiber begin";
    sylar::Fiber::GetThis();
    sylar::Fiber::ptr f(new sylar::Fiber(run_in_fiber, 0, false));
    f->swapIn();
    SYLAR_LOG_INFO(g_logger) << "test_fiber after swapIn";
    f->swapIn();
    SYLAR_LOG_INFO(g_logger) << "test_fiber end";
}


int main(int argc, char** argv) {
    sylar::Thread::setCurrThreadName("main");
    std::vector<sylar::Thread::ptr> thr;
    for(int i = 0; i < 3; ++i) {
        thr.push_back(sylar::Thread::ptr(new sylar::Thread(test_fiber, 
                                        "name_" + std::to_string(i))));
    }
    for(const auto& i : thr) {
        i->join();
    }
    return 0;
}