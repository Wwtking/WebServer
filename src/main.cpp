#include "application.h"
#include "log.h"
#include <stdlib.h>
#include <time.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int main(int argc, char** argv) {
    setenv("TZ", ":/etc/localtime", 1);
    tzset();
    srand(time(0));
    sylar::Application app;
    if(app.init(argc, argv)) {
        SYLAR_LOG_DEBUG(g_logger) << "run";
        app.run();
    }
    return 0;
}