#include "application.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

int main(int argc, char** argv) {
    sylar::Application app;
    if(app.init(argc, argv)) {
        SYLAR_LOG_DEBUG(g_logger) << "run";
        app.run();
    }
    return 0;
}