#include "application.h"
#include "log.h"
#include <stdlib.h>
#include <time.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int main(int argc, char** argv) {
    // 设置环境变量
    setenv("TZ", ":/etc/localtime", 1);
    // 调用tzset函数会将时区信息加载到环境变量中，保存在环境变量"TZ"中
    tzset();
    srand(time(0));
    sylar::Application app;
    if(app.init(argc, argv)) {
        SYLAR_LOG_DEBUG(g_logger) << "run";
        app.run();
    }
    return 0;
}