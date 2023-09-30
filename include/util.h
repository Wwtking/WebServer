#ifndef __SYLAR_UTIL_H
#define __SYLAR_UTIL_H

#include <stdint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <boost/lexical_cast.hpp>

namespace sylar {

//返回当前线程的ID
pid_t getThreadId();     // pid_t用于定义进程ID，本质上为int

//返回当前协程的ID
uint32_t getFiberId();

/**
 * @brief 获取当前的调用栈
 * @param[out] bt 保存调用栈
 * @param[in] size 最多返回层数
 * @param[in] skip 跳过栈顶的层数
*/
void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

/**
 * @brief 获取当前栈信息的字符串
 * @param[in] size 栈的最大层数
 * @param[in] skip 跳过栈顶的层数
 * @param[in] prefix 栈信息前输出的内容
*/
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

//获取当前时间的毫秒
uint64_t GetCurrentMS();

//获取当前时间的微秒
uint64_t GetCurrentUS();

// 将时间戳形式的time按format格式转化成可读性字符串time
std::string TimeToStr(time_t time = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");

// 将format格式的字符串形式time转化成时间戳形式的time
time_t StrToTime(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

/**
 * @brief 根据key值找Map数据类型中的value值
 * @param[in] m 要查找的Map数据类型容器
 * @param[in] key 要查找的key值
 * @param[in] defValue 默认value值
 * @return 返回value值，找不到返回默认值
*/
template<class Map, class Key, class Value>
Value GetParamValue(const Map& m, const Key& key, const Value& defValue = Value()) {
    auto it = m.find(key);
    if(it == m.end()) {
        return defValue;
    }
    try {
        return boost::lexical_cast<Value>(it->second);
    }
    catch(...) {
    }
    return defValue;
}

/**
 * @brief 根据key值找Map数据类型中的value值
 * @param[in] m 要查找的Map数据类型容器
 * @param[in] key 要查找的key值
 * @param[out] Value 存放找到后的value值
 * @return 返回是否找到
*/
template<class Map, class Key, class Value>
bool CheckGetParamValue(const Map& m, const Key& key, Value& Val) {
    auto it = m.find(key);
    if(it == m.end()) {
        return false;
    }
    try {
        Val = boost::lexical_cast<Value>(it->second);
        return true;
    }
    catch(...) {
    }
    return false;
}

class FilesUtil {
public:
    /**
     * @brief 存储指定路径下指定后缀名的所有文件
     * @param[out] files 存放符合条件的文件名(绝对路径)
     * @param[in] path 指定路径
     * @param[in] subfix 指定文件后缀名
    */
    static void ListAllFiles(std::vector<std::string>& files
                            , const std::string& path
                            , const std::string& subfix = "");

    static bool IsRunningPidfile(const std::string& pidfile);

    static bool Mkdir(const std::string& dirname);
};

} // namespace sylar


#endif