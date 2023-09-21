#include <iostream>
#include "thread.h"
#include <vector>
#include "log.h"
#include "config.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
sylar::Mutex s_mutex;
//sylar::RWMutex s_mutex;
volatile int count = 0;  // volatile 关键字用来告诉编译器禁止对变量进行优化操作

void func1() {
    SYLAR_LOG_INFO(g_logger) << "name:" << sylar::Thread::getCurrThreadName()
                                    << " this.name:" << sylar::Thread::getCurrThread()->getName()
                                    << " id:" << sylar::Thread::getCurrThread()->getId();
    for(int i = 0; i < 100000; ++i) {
        //sylar::RWMutex::WriteLock lock(s_mutex);
        sylar::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void func2() {
    while(true) {
        SYLAR_LOG_INFO(g_logger) << "+++++++++++++++++++++++++++++++";
    }
}

void func3() {
    while(true) {
        SYLAR_LOG_WARN(g_logger) << "===============================";
    }
}

int main(int argc, char** argv) {
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/wwt/sylar/bin/conf/log2.yml");
    sylar::Config::LoadFromYaml(root);

    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i) {
        // thrs.push_back(std::make_shared<sylar::Thread>(func1, "name_" + std::to_string(i)));
        thrs.push_back(std::make_shared<sylar::Thread>(func2, "name_" + std::to_string(i*2)));
        thrs.push_back(std::make_shared<sylar::Thread>(func3, "name_" + std::to_string(i*2 + 1)));
    }
    for(const auto& i : thrs) {
        i->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << count;
    return 0;
}