#ifndef __SYLAR_UTIL_H
#define __SYLAR_UTIL_H

#include <stdint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>

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