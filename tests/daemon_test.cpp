#include "daemon.h"
#include "iomanager.h"
#include "log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int test(int argc, char** argv) {
    SYLAR_LOG_DEBUG(g_logger) << sylar::ProcessInfoMgr::GetInstance()->toString();
    sylar::IOManager iom(1);
    iom.addTimer(1000, [](){
        static int count = 0;
        count++;
        if(count > 10) {
            exit(1);        // 异常退出，子进程重启
            // exit(0);     // 正常退出，子进程完成
        }
        SYLAR_LOG_DEBUG(g_logger) << "onTimer, count=" << count;
    }, true);
    return 0;
}

int main(int argc, char** argv) {
    // 任意输入启动参数时，选择守护进程的方式启动
    return sylar::start_daemon(argc, argv, test, argc != 1);
}