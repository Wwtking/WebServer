#include <iostream>
#include "log.h"
#include "scheduler.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

/**
 * @bug 加入sylar::set_hook_enable(true)后，测试scheduler_test：
 *              sleep(1);
 *              sylar::Scheduler::GetThis()->scheduler(&task);
 *      出现段错误core dumped，因为改了sleep函数，
 *      复习完hook，记得回头复盘
 * @result 因为该测试是scheduler的测试，main中创建的是sylar::Scheduler sc(...)，并没有创建sylar::IOManager iom(...)，
 *         没有使用到多态，所以调用hook的sleep时用到的sylar::IOManager::GetThis()无法强制转换
*/
void task() {
    SYLAR_LOG_INFO(g_logger) << "test scheduler";
    sleep(1);
    sylar::Scheduler::GetThis()->scheduler(&task);
}

int main(int agrv, char** argv) {
    SYLAR_LOG_INFO(g_logger) << "main begin";
    sylar::Scheduler sc(1, true, "test");
    sc.start();
    sc.scheduler(&task);
    //sc.scheduler(&task);
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "main end";
}