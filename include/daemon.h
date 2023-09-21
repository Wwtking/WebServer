#ifndef __SYLAR_DAEMON_H__
#define __SYLAR_DAEMON_H__

#include <functional>
#include "singleton.h"

namespace sylar {

struct ProcessInfo {
    pid_t parent_id = 0;            // 父进程id
    pid_t child_id = 0;             // 子进程id
    uint64_t parent_start_time = 0; // 父进程启动时间
    uint64_t child_start_time = 0;  // 子进程启动时间
    uint32_t restart_count = 0;     // 子进程重启的次数

    // 可读性输出信息
    std::string toString() const;
};

// 单例化，保证该类只有一个实例
typedef Singleton<ProcessInfo> ProcessInfoMgr;

/**
 * @brief 启动程序，可以选择是否用守护进程的方式
 * @param[in] argc 参数个数
 * @param[in] argv 参数值数组
 * @param[in] main_cb 启动函数
 * @param[in] is_daemon 是否用守护进程的方式
 *            生产应用时用守护进程，开发调试时用非守护进程
 * @return 返回程序的执行结果
 */
int start_daemon(int argc, char** argv
                , std::function<int(int _argc, char** _argv)> main_cb
                , bool isDaemon);

}

#endif