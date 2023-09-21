#include "../include/log.h"
#include <map>
#include <ctype.h>
#include <tuple>
#include <time.h>
#include "config.h"

namespace sylar {

/******************* LogLevel 类函数实现 *******************/
//将日志级别文本输出：level为日志级别
const char* LogLevel::ToString(LogLevel::Level level) {
    switch(level) {                       //#name是把name字符串化
#define FUNC(name) \
    case LogLevel::name: \
        return #name; \
        break;

    FUNC(DEBUG);
    FUNC(INFO);
    FUNC(WARN);
    FUNC(ERROR);
    FUNC(FATAL);
#undef FUNC
    default:
        return "UNKOWN";
    }
    return "UNKOWN";
}

//将文本转换为日志级别
LogLevel::Level LogLevel::FromString(const std::string& str) {
#define FUNC(name1, name2) \
    if(str == #name1 || str == #name2) { \
        return LogLevel::name1; \
    }

    FUNC(DEBUG, debug);
    FUNC(INFO,info);
    FUNC(WARN, warn);
    FUNC(ERROR, error);
    FUNC(FATAL, fatal);
    return LogLevel::UNKOWN;
#undef FUNC
}


/******************* LogEvent 类函数实现 *******************/
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
            const char* file, int32_t line, uint32_t elapse,
            uint32_t threadId, uint32_t fiberId, uint64_t time,
            const std::string& threadName) 
    :m_logger(logger),
    m_level(level),
    m_file(file),
    m_line(line),
    m_elapse(elapse),
    m_threadId(threadId),
    m_fiberId(fiberId),
    m_time(time),
    m_threadName(threadName) {
}

// 格式化写入日志内容
void LogEvent::format(const char* fmt, ...) {
    va_list vl;
    va_start(vl, fmt);    //将vl初始化为指向可变参数列表第一个参数

    char* buf;
    //vsnprintf(buf, sizeof(fmt)+1, fmt, vl)   函数重载
    if(vasprintf(&buf, fmt, vl) != -1) {
        m_ss << std::string(buf);    // char*转为string
        free(buf);
    }

    va_end(vl);
}


/******************* LogEventWrap 类函数实现 *******************/
LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}


/******************* LogFormatter 类函数实现 *******************/
LogFormatter::LogFormatter(const std::string& pattern) 
    :m_pattern(pattern) {
    init();
}

//返回字符串类型日志文本
std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}
//返回输出流类型日志文本
std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    for(auto& i : m_items) {
        i->format(ofs, logger, level, event);
    }
    return ofs;
}


// FormatItem 类的多态实现
//多态：输出日志内容 m_ss
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();       // 对应 std::stringstream m_ss;    //日志内容流
    }
};

//多态：输出日志级别 m_level
class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

//多态：输出日志耗时 m_elapse
class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

//多态：输出日志器名称 m_logger
class LoggerNameFormatItem : public LogFormatter::FormatItem {
public:
    LoggerNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        //os << logger->getName();
        os << event->getLogger()->getName();
    }
};

//多态：输出线程ID m_threadId
class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

//多态：输出协程ID m_fiberId
class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

//多态：输出线程名称 m_threadName
class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

//多态：输出文件名称 m_file
class FileNameFormatItem : public LogFormatter::FormatItem {
public:
    FileNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

//多态：输出行号 m_line
class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

//多态：输出换行符\n
class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

//多态：输出Tab \t
class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
};

//多态：输出m_string
class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str) :m_string(str) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

//多态：输出日期时间，将时间戳 m_time 转化为字符串 %Y-%m-%D %H:%M:%S 格式
class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S") 
        :m_format(format) {
        if(format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char str[64];
        strftime(str, sizeof(str), m_format.c_str(), &tm);
        os << str;
    }
private:
    std::string m_format;
};


// 把 m_pattern 中 % 后的字符都分离出来
// m_pattern 为 %d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
// %xxx  %xxx{xxx}  %%
void LogFormatter::init() {
    //str, format, type
    size_t len = m_pattern.size();
    std::vector< std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for(size_t i = 0; i < len; ++i) {
        if(m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        // 以下是 m_pattern[i] == '%' 的情况
        if(i + 1 < len && m_pattern[i+1] == '%') {
            nstr.append(1, '%');
            continue;
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;
        std::string str;
        std::string fmt;
        //%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
        while ( n < len )    // %xxx  %xxx{xxx}  %%
        {
            if( fmt_status == 0 ) {
                if( isalpha(m_pattern[n] )) {        // isalpha判断字符是否为英文字母，若为英文字母，返回非0（小写字母为2，大写字母为1）。若不是字母，返回0
                    ++n;
                    if(n == len) {
                        str = m_pattern.substr(i + 1, n - i - 1);
                    }
                    continue;
                }
                if( m_pattern[n] == '{' ) {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1;
                    fmt_begin = n + 1;
                    ++n;
                    continue;
                }
                if( m_pattern[n] != '}' ) {         //说明 m_pattern[n] 不是字母，不是 '{'，也不是 '}'
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
            }
            else {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin, n - fmt_begin);               //fmt表示{}中的内容
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if( n == len ) {
                if( str.empty() ) {         // 等价于 fmt_status == 0
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if( fmt_status == 0 ) {
            if( !nstr.empty() ) {
                vec.push_back(std::make_tuple(nstr, "", 0));     // "" 写为 std::string()也可以，表示创建匿名对象，空字符串
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        }
        else {
            std::cout << "pattern parse error: " << m_pattern << "  -  " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern error>>", fmt, 0));
        }
    }
    if( !nstr.empty() ) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    
    // map的value值是一个函数指针，指向下面Lambda表达式所表示的函数
    // 每次调用函数指针，即调用it->second()时，就会执行Lambda表达式所表示的函数，
    // 返回FormatItem::ptr(std::make_shared<C>(fmt)); 即一个指向派生类的基类指针（实现多态）
    static std::map< std::string, std::function<FormatItem::ptr(const std::string& fmt)> > s_format_items = {
#define FUNC(str, C) \
    { #str, [](const std::string& fmt) { return FormatItem::ptr(std::make_shared<C>(fmt)); } }

    FUNC(m, MessageFormatItem),          //m:消息
    FUNC(p, LevelFormatItem),                //p:日志级别
    FUNC(r, ElapseFormatItem),               //r:累计毫秒数
    FUNC(c, LoggerNameFormatItem),      //c:日志名称
    FUNC(t, ThreadIdFormatItem),            //t:线程id
    FUNC(n, NewLineFormatItem),           //n:换行
    FUNC(d, DateTimeFormatItem),          //d:时间
    FUNC(f, FileNameFormatItem),           //f:文件名
    FUNC(l, LineFormatItem),                  //l:行号
    FUNC(T, TabFormatItem),                  //T:Tab
    FUNC(F, FiberIdFormatItem),             //F:协程id
    FUNC(N, ThreadNameFormatItem)     //N:线程名称
#undef FUNC
    };

    for(const auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(std::make_shared<StringFormatItem>(std::get<0>(i)));
            //m_items.push_back(FormatItem::ptr(std::make_shared<StringFormatItem>(std::get<0>(i))));
        }
        else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it != s_format_items.end()) {
                m_items.push_back(it->second(std::get<1>(i)));     //执行 函数指针(参数) 相当于调用了该函数
            }
            else {
                m_items.push_back(std::make_shared<StringFormatItem>("<<error format %" + std::get<0>(i) + ">>"));
                m_error = true;
            }
        }
        //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
}


/******************* LogAppender 类函数实现 *******************/
//更改日志格式器
void LogAppender::setFormatter(LogFormatter::ptr formatter) { 
    MutexType::Lock lock(m_mutex);
    m_formatter = formatter; 

    //若formatter为空，说明没有赋日志格式，则没有自己的日志格式器
    m_hasFormatter = m_formatter ? true : false;
}

//获取日志格式器
LogFormatter::ptr LogAppender::getFormatter() const {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

//设置日志级别
void LogAppender::setLevel(LogLevel::Level level) { 
    MutexType::Lock lock(m_mutex);
    m_level = level; 
}

//获取日志级别
LogLevel::Level LogAppender::getLevel() const { 
    MutexType::Lock lock(m_mutex);
    return m_level; 
}


/******************* Logger 类函数实现 *******************/
//构造函数
Logger::Logger(const std::string& name) 
    :m_name(name),
    m_level(LogLevel::DEBUG) {
    m_formatter = std::make_shared<LogFormatter>("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");
}

//写日志：level 日志级别，event 日志事件
void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    MutexType::Lock lock(m_mutex);
    if(level >= m_level) {
        auto self = shared_from_this();
        if(!m_appenders.empty()) {
            for(auto& i : m_appenders) {
                i->log(self, level, event);
            }
        }
        else if(m_root) { 
            m_root->log(level, event);
        }
    }
}

//写debug级别日志
void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}

//写info级别日志
void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}

//写warn级别日志
void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}

//写error级别日志
void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}

//写fatal级别日志
void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

//添加日志目标
void Logger::addApender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    if(!appender->getFormatter()) {
        LogAppender::MutexType::Lock ll(appender->m_mutex);
        //如果没有这步的话，下面m_appenders中存入的就是没有日志格式的appender
        //在后续中，这个appender的成员变量m_formatter指针为空指针，无法后续调用
        //应当与Logger对象中的m_formatter指针指向同一地址
        appender->m_formatter = m_formatter;  //友元，相当于appender->setFormatter(m_formatter);
        appender->setHasFormatter(false);    //appender没有自己的formatter，要用logger的formatter
    }
    m_appenders.push_back(appender);
}

//删除日志目标
void Logger::delAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    for(auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if(*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

//清空日志目标
void Logger::clearAppenders() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

//返回日志级别
LogLevel::Level Logger::getLevel() const { 
    MutexType::Lock lock(m_mutex);
    return m_level; 
}

//设置日志级别
void Logger::setLevel(LogLevel::Level level) { 
    MutexType::Lock lock(m_mutex);
    m_level = level; 
}

//设置日志格式
void Logger::setFormatter(const LogFormatter::ptr& val) {
    MutexType::Lock lock(m_mutex); 
    m_formatter = val;
    for(auto& i : m_appenders) {
        //LogAppender::MutexType::Lock ll(i->m_mutex); 
        if(!i->getHasFormatter()) {
            i->setFormatter(m_formatter);  //setFormatter内已上锁,上面不能再加锁,避免死锁
        }
    }
}

//设置日志格式(用字符串)
void Logger::setFormatter(const std::string& val) {
    LogFormatter::ptr new_val = std::make_shared<LogFormatter>(val);
    if(new_val->isError()) {
        std::cout << "Logger setFormatter name=" << m_name
                  << " value=" << val << " invalid formatter"
                  << std::endl;
        return;
    }
    setFormatter(new_val);
}

//获取日志格式器
LogFormatter::ptr Logger::getFormatter() const { 
    MutexType::Lock lock(m_mutex); 
    return m_formatter; 
}

//将日志器的配置转成YAML String
std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex); 
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    for(const auto& i: m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


/******************* LoggerManager 类函数实现 *******************/
//构造函数
LoggerManager::LoggerManager() {
    m_root = std::make_shared<Logger>();
    m_root->addApender(LogAppender::ptr(std::make_shared<StdoutLogAppender>()));
    m_loggers["root"] = m_root;
    init();
}

//根据日志器名称获取日志器,找到则返回,没找到则新建
Logger::ptr LoggerManager::getLogger(const std::string& name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }
    
    Logger::ptr logger = std::make_shared<Logger>(name);
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

//将所有日志器的配置转成YAML String
std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(const auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

//初始化
void LoggerManager::init() {

}


/******************* StdoutLogAppender 类函数实现 *******************/
//输出到控制台的Appender
void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    MutexType::Lock lock(m_mutex);
    if(level >= m_level) {
        m_formatter->format(std::cout, logger, level, event);
    }
}

//将日志输出目标的配置转成YAML String
std::string StdoutLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

/******************* FileLogAppender 类函数实现 *******************/
//输出到文件的Appender
FileLogAppender::FileLogAppender(const std::string& filename) 
    :m_filename(filename) {
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        //周期性地重新打开文件,防止在另一个终端文件被删除,可以及时重新打开,但之前的日志还是被删除了
        uint64_t nowTime = event->getTime();
        if(nowTime >= (m_lastTime + 3)) {    //超过一定时间
            reopen();
            m_lastTime = nowTime;
        }

        MutexType::Lock lock(m_mutex);
        if(!m_formatter->format(m_filestream, logger, level, event)) {
            std::cout << "error" << std::endl;
        }
    }
}

//将日志输出目标的配置转成YAML String
std::string FileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

//重新打开日志文件,成功返回true
bool FileLogAppender::reopen() {
    MutexType::Lock lock(m_mutex);
    if(m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app); //追加方式:在文件末尾添加新内容
    return !! m_filestream;
}


/******************* 用 YAML 来配置 Logger *******************/
struct LogAppenderDefine {
    int type = 0;    //1:File  2:Stdout  3:Logserver
    LogLevel::Level level = LogLevel::UNKOWN;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& lad) const {
        return (type == lad.type && 
                level == lad.level &&
                formatter == lad.formatter &&
                file == lad.file);
    }
};

struct logDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKOWN;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const logDefine& ld) const {
        return (name == ld.name && 
                level == ld.level &&
                formatter == ld.formatter &&
                appenders == ld.appenders);
    }

    bool operator<(const logDefine& ld) const {
        return name < ld.name;
    }
};

// node["name"].IsDefined() 
// 判断node中name这个属性是否存在，存在副作用：name不存在时会创建name属性(没有值)

//类型转换模板类片特化(YAML String 转换成 logDefine)
template<>
class LexicalCast<std::string, logDefine> {
public:
    logDefine operator()(const std::string& s) {
        logDefine ld;
        YAML::Node node = YAML::Load(s);
        if(!node["name"].IsDefined()) {
            std::cout << "log config error: name is null, "
                        << node << std::endl;
            throw std::logic_error("log config error: name is null");
        }
        ld.name = node["name"].as<std::string>();
        if(node["level"].IsDefined()) {
            ld.level = LogLevel::FromString(node["level"].as<std::string>());
        }
        if(node["formatter"].IsDefined()) {
            ld.formatter = node["formatter"].as<std::string>();
        }

        if(node["appenders"].IsDefined()) {
            for(size_t i = 0; i < node["appenders"].size(); i++) {
                LogAppenderDefine lad;
                auto n = node["appenders"][i];
                if(!n["type"].IsDefined()) {
                    std::cout << "log config error: appender_type is null, "
                            << n << std::endl;
                    continue;
                }
                auto appender_type = n["type"].as<std::string>();
                if(appender_type == "FileLogAppender") {    //File
                    lad.type = 1;
                    if(!n["file"].IsDefined()) {
                        std::cout << "log config error: FileLogAppender file is null, "
                                << n << std::endl;
                        continue;
                    }
                    lad.file = n["file"].as<std::string>();
                }
                else if(appender_type == "StdoutLogAppender") {    //Stdout
                    lad.type = 2;
                }
                else {    //未定义类型
                    std::cout << "log config error: appender_type is invalid, "
                            << n << std::endl;
                    continue;
                }
                if(n["level"].IsDefined()) {
                    lad.level = LogLevel::FromString(n["level"].as<std::string>());
                }
                if(n["formatter"].IsDefined()) {
                    lad.formatter = n["formatter"].as<std::string>();
                }
                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};
//类型转换模板类片特化(logDefine 转换成 YAML String)
template<>
class LexicalCast<logDefine, std::string> {
public:
    std::string operator()(const logDefine& ld) {
        YAML::Node node(YAML::NodeType::Map);
        node["name"] = ld.name;
        if(ld.level != LogLevel::UNKOWN) {
            node["level"] = LogLevel::ToString(ld.level);
        }
        if(!ld.formatter.empty()) {    //约定优于配置，没有就不写
            node["formatter"] = ld.formatter;
        }
        for(const auto& i : ld.appenders) {
            YAML::Node n;
            if(i.type == 1) {
                n["type"] = "FileLogAppender";
                n["file"] = i.file;
            }
            else if(i.type == 2) {
                n["type"] = "StdoutLogAppender";
            }
            if(i.level != LogLevel::UNKOWN) {
                n["level"] = LogLevel::ToString(i.level);
            }
            if(!i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }
            node["appenders"].push_back(n);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

static sylar::ConfigVar<std::set<logDefine> >::ptr g_logDefine_config = 
    sylar::Config::Lookup("logs", std::set<logDefine>(), "logDefine");

struct LogIniter {
    LogIniter() {
        g_logDefine_config->addListener([](const std::set<logDefine>& oldVal, 
                                        const std::set<logDefine>& newVal) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
            for(const auto& i : newVal) {
                Logger::ptr logger;
                auto it = oldVal.find(i);
                if(it == oldVal.end()) {
                    //新增loger
                    logger = SYLAR_LOG_NAME(i.name);    //增加日志名为i.name的logger
                }
                else {
                    if(!(i == *it)) {
                        //修改logger
                        logger = SYLAR_LOG_NAME(i.name);    //查找日志名为i.name的logger
                    }
                    else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();     //构造logger时，可能有默认appender，先清空
                for(const auto& j : i.appenders) {
                    LogAppender::ptr ap;
                    if(j.type == 1) {
                        ap = std::make_shared<FileLogAppender>(j.file);
                    }
                    else if(j.type == 2) {
                        ap = std::make_shared<StdoutLogAppender>();
                    }
                    ap->setLevel(j.level);
                    if(!j.formatter.empty()) {
                        LogFormatter::ptr fmt = std::make_shared<LogFormatter>(j.formatter);
                        if(!fmt->isError()){
                            ap->setFormatter(fmt);
                        }
                        else {
                            std::cout << "log.name=" << i.name << " appender.type=" << j.type
                                    << " formatter=" << j.formatter << " is invalid" << std::endl;
                            //continue;
                        } 
                    }
                    logger->addApender(ap);
                }
            }

            for(const auto& i : oldVal) {
                auto it = newVal.find(i);
                if(it == newVal.end()) {
                    //删除logger
                    Logger::ptr logger = SYLAR_LOG_NAME(i.name);
                    logger->clearAppenders();
                    logger->setLevel((LogLevel::Level)100);
                }
            }
        });     
    }
};

static LogIniter __log_init;
 

}




