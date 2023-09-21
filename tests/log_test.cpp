#include <iostream>
#include "../include/log.h"
#include "../include/util.h"

int main(int argc, char** argv) {
    sylar::Logger::ptr logger(std::make_shared<sylar::Logger>());
    logger->addApender(sylar::LogAppender::ptr(std::make_shared<sylar::StdoutLogAppender>()));

    sylar::LogAppender::ptr file_appender(std::make_shared<sylar::FileLogAppender>("./file_test.txt"));
    sylar::LogFormatter::ptr fmt = std::make_shared <sylar::LogFormatter>("%d%T%t%T%p%T%m%n");
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::WARN);
    logger->addApender(file_appender);

    //创建Event事件进行测试
    sylar::LogEvent::ptr event(new sylar::LogEvent(logger, sylar::LogLevel::DEBUG, 
        __FILE__, __LINE__, 0, sylar::getThreadId(), sylar::getFiberId(), time(0), "WWT"));
    event->getSS() << "hello sylar log" <<std::endl;
    logger->log(sylar::LogLevel::DEBUG, event);

    //流式方式测试
    SYLAR_LOG_DEBUG(logger) << "test01" << std::endl;
    SYLAR_LOG_INFO(logger) << "test02" << std::endl;
    SYLAR_LOG_WARN(logger) << "test03" << std::endl;
    SYLAR_LOG_ERROR(logger) << "test04" << std::endl;
    SYLAR_LOG_FATAL(logger) << "test05" << std::endl;

    //格式化方式测试
    SYLAR_LOG_FMT_DEBUG(logger, "test%d.%s", 1, "debug");
    SYLAR_LOG_FMT_INFO(logger, "test%d.%s", 2, "info");
    SYLAR_LOG_FMT_WARN(logger, "test%d.%s", 3, "warn");
    SYLAR_LOG_FMT_ERROR(logger, "test%d.%s", 4, "error");
    SYLAR_LOG_FMT_FATAL(logger, "test%d.%s", 5, "fatal");

    auto lg = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_ERROR(lg) << "xxx" << std::endl;

    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_logDefine_config->

    return 0;

}