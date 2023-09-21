#include "log.h"
#include "config.h"
#include "daemon.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 重启等待时间，在下次重新创建子进程进入前要先清除掉资源，需要睡眠等待
static ConfigVar<uint32_t>::ptr g_restart_wait_time = 
        Config::Lookup("restart.wait_time", (uint32_t)5, "restart wait time");

namespace {
struct RestartTimeConfigInit {
    RestartTimeConfigInit() {
        g_restart_wait_time->addListener([](const uint32_t& oldVal, const uint32_t& newVal){
            g_restart_wait_time->setValue(newVal);
        });
    }
};
static RestartTimeConfigInit _Init;
}

// 可读性输出ProcessInfo信息
std::string ProcessInfo::toString() const {
    std::stringstream ss;
    ss << "[parent_id=" << parent_id
        << " child_id=" << child_id
        << " parent_start_time=" << TimeToStr(parent_start_time)
        << " child_start_time=" << TimeToStr(child_start_time)
        << " restart_count=" << restart_count
        << "]";
    return ss.str();
}

static int real_daemon(int argc, char** argv
                , std::function<int(int _argc, char** _argv)> main_cb) {
    /**
     * @brief int daemon(int nochdir, int noclose);
     * @details 调用daemon()会将当前进程以守护进程的形式运行；
     *          daemon()中会执行fork()创建子进程，创建成功之后会退出父进程，让子进程变成真正的孤儿进程
     * @param[in] nochdir 当其为0时，daemon将更改进程的根目录为root("/")
     * @param[in] noclose 当其为0时，daemon将进程的stdin, stdout, stderr都重定向到/dev/null
     *                    也就是说键盘的输入将对进程无任何影响，进程的输出也不会输出到终端
    */
    daemon(1, 0);
    ProcessInfoMgr::GetInstance()->parent_id = getpid();
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
    while(true) {
        // 创建子进程，从这里便开始了两个进程同时进行
        pid_t pid = fork();     
        // fork成功之后，会创建子进程，“复制”一份主进程，主进程和子进程同时在运行
        // fork会执行两次，返回两次值，一个是父进程的、一个是子进程的
        // 在父进程中，fork返回新创建子进程的进程ID；在子进程中，fork返回0
        // fork将主进程的资源全部拷贝了一份给子进程，两个进程的资源是独立互不影响的
        if(pid == 0) {
            // 子进程返回
            ProcessInfoMgr::GetInstance()->child_id = getpid();
            ProcessInfoMgr::GetInstance()->child_start_time = time(0);
            SYLAR_LOG_DEBUG(g_logger) << "process start pid=" << getpid();
            return main_cb(argc, argv);
        }
        else if(pid > 0) {
            // 父进程返回
            int status = 0;
            /**
             * @brief pid_t waitpid(pid_t pid, int *status, int options);
             * @details 调用waitpid()函数时，当指定等待的子进程已经停止运行或结束，则会立即返回
             *      如果子进程还没有停止运行或结束，则调用waitpid()函数的父进程则会被阻塞，暂停运行
             * @param[in] pid 要等待的子进程ID
             * @param[in] status 保存子进程的状态信息
             * @param[in] options 控制waitpid()函数的选项，默认设为0
             * @return 成功则返回子进程的进程号；有错误发生返回-1
            */
            waitpid(pid, &status, 0);
            if(status) {
                // 子进程异常结束
                SYLAR_LOG_ERROR(g_logger) << "child crash, pid=" << pid
                                            << " status=" << status;
            }
            else {
                // 子进程正常结束，不需要重启
                SYLAR_LOG_DEBUG(g_logger) << "child finished, pid=" << pid;
                break;
            }
        }
        else {
            SYLAR_LOG_ERROR(g_logger) << "fork fail, return=" << pid
                                    << " errno=" << errno 
                                    << " errstr=" << strerror(errno);
        }

        ProcessInfoMgr::GetInstance()->restart_count++;
        // while(true)在下次重新创建子进程前要先清除掉资源，需要睡眠等待
        sleep(g_restart_wait_time->getValue());
    }
    return 0;
}

int start_daemon(int argc, char** argv
                , std::function<int(int _argc, char** _argv)> main_cb
                , bool isDaemon) {
    // 不需要守护进程
    if(!isDaemon) {
        // getpid()获取当前调用进程的进程ID
        ProcessInfoMgr::GetInstance()->parent_id = getpid();
        ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
        return main_cb(argc, argv);
    }
    return real_daemon(argc, argv, main_cb);
}

}