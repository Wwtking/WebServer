#include "util.h"
#include "log.h"
#include "fiber.h"
#include "env.h"
#include <execinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <fstream>


namespace sylar 
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

//返回当前线程的ID
pid_t getThreadId() {
    return syscall(SYS_gettid);
}    

//返回当前协程的ID
uint32_t getFiberId() {
    return sylar::Fiber::GetFiberId();
}

//获取当前的调用栈
void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** buffer = (void**)malloc(size * sizeof(void*));
    //把获取到的一系列函数调用关系信息放在buffer中,返回值为buffer中实际得到的返回地址数量
    size_t length = backtrace(buffer, size);

    //将buffer中的返回地址都对应到具体的函数名,失败则返回NULL
    char** strings = backtrace_symbols(buffer, length);
    if(strings == NULL) {
        SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
        free(buffer);
        return;
    }

    for(size_t i = skip; i < length; ++i) {
        bt.push_back(strings[i]);
    }

    free(buffer);
    free(strings);
}

//获取当前栈信息的字符串
std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(auto i : bt) {
        ss << prefix << i << std::endl;
    }
    return ss.str();
}

// int gettimeofday(struct timeval *tv, struct timezone *tz);
// struct timeval {
//     time_t      tv_sec;     /* seconds秒 */
//     suseconds_t tv_usec;    /* microseconds微秒 */
// };
//获取当前时间的毫秒
uint64_t GetCurrentMS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

//获取当前时间的微秒
uint64_t GetCurrentUS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

// 将时间戳形式的time按format格式转化成可读性字符串time
std::string TimeToStr(time_t time, const std::string& format) {
    struct tm tm;
    localtime_r(&time, &tm);
    char str[64];
    strftime(str, sizeof(str), format.c_str(), &tm);
    return str;
}

// 将format格式的字符串形式time转化成时间戳形式的time
time_t StrToTime(const char* str, const char* format) {
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    if(!strptime(str, format, &tm)) {
        return 0;
    }
    return mktime(&tm);
}

// 存储指定路径下指定后缀名的所有文件
void FilesUtil::ListAllFiles(std::vector<std::string>& files
                            , const std::string& path
                            , const std::string& subfix) {
    // 检查该路径是否存在
    if(access(path.c_str(), 0)) {
        SYLAR_LOG_DEBUG(g_logger) << "the calling process cann't access the file path: "
                                    << path;
        return;
    }

    // 打开文件夹
    DIR* dir = opendir(path.c_str());
    if(!dir) {
        SYLAR_LOG_DEBUG(g_logger) << "open dir fail: "<< path;
        return;
    }

    struct dirent* dp = nullptr;
    // 依次读出指定目录中的所有文件
    while((dp = readdir(dir)) != nullptr) {
        if(dp->d_type == DT_DIR) {
            // 读到的是文件夹，继续递归读下一层目录
            // Linux中每个目录中都有隐藏目录.和.. 表示当前目录和上一级目录
            // 把当前目录.和上一级目录..都去掉，避免死循环遍历目录
            if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                continue;
            }
            ListAllFiles(files, path + "/" + dp->d_name, subfix);
        }
        else if(dp->d_type == DT_REG) {
            // 读到的是文件
            if(subfix.empty()) {
                // 未指定文件后缀，则全部存入
                files.push_back(path + "/" + dp->d_name);
            }
            else {
                std::string name(dp->d_name);
                if(name.size() < subfix.size()) {
                    continue;
                }
                // 直接比较后缀，相同则放入
                if(!strcmp(name.substr(name.size() - subfix.size()).c_str(), subfix.c_str())) {
                    files.push_back(path + "/" + name);
                }
            }
        }
    }
    closedir(dir);  // 关闭目录流
}

bool FilesUtil::IsRunningPidfile(const std::string& pidfile) {
    // 获取linux操作系统下文件的属性
    // if(__lstat(pidfile.c_str()) != 0) {
    //     return false;
    // }
    // 检查该路径下pidfile文件是否存在
    if(access(pidfile.c_str(), F_OK)) {
        return false;
    }
    
    std::string content;
    std::ifstream ifs(pidfile);
    // 验证pidfile文件是否存在，并且是否有进程ID的内容
    if(!ifs || !std::getline(ifs, content)) {
        return false;
    }
    
    if(content.empty()) {
        return false;
    }

    auto pid = atoi(content.c_str());
    if(pid < 1) {
        return false;
    }

    /**
     * @brief int kill(pid_t pid, int sig);
     * @details 用于向任何进程组或进程发送信号
     *          通常会利用sig值为零来检验某个进程是否仍在执行
     * @return 成功返回0；失败返回-1，errno设为ESRCH表示所指定的进程不存在
    */
    if(kill(pid, 0)) {
        // SYLAR_LOG_ERROR(g_logger) << "errno=" << errno 
        //                         << " errst=" << strerror(errno);
        return false;
    }

    return true;
}

// mkdir的mode模式如下：
// S_IRWXU 00700权限，代表该文件所有者拥有读，写和执行操作的权限
// S_IRUSR 00400权限，代表该文件所有者拥有可读的权限
// S_IWUSR 00200权限，代表该文件所有者拥有可写的权限
// S_IXUSR 00100权限，代表该文件所有者拥有执行的权限
// S_IRWXG 00070权限，代表该文件用户组拥有读，写和执行操作的权限
// S_IRGRP 00040权限，代表该文件用户组拥有可读的权限
// S_IWGRP 00020权限，代表该文件用户组拥有可写的权限
// S_IXGRP 00010权限，代表该文件用户组拥有执行的权限
// S_IRWXO 00007权限，代表其他用户拥有读，写和执行操作的权限
// S_IROTH 00004权限，代表其他用户拥有可读的权限
// S_IWOTH 00002权限，代表其他用户拥有可写的权限
// S_IXOTH 00001权限，代表其他用户拥有执行的权限
static int _Mkdir(const char* dirname) {
    if(!access(dirname, F_OK)) {
        return 0;
    }

    // 以指定权限来创建目录
    return mkdir(dirname, 0775);
}

bool FilesUtil::Mkdir(const std::string& dirname) {
    if(!access(dirname.c_str(), F_OK)) {
        return true;
    }

    // 直接赋值不行，因为 const char* 类型不能用于初始化 char*
    // char* path = dirname.c_str();
    /**
     * @brief char *strdup(const char *str);
     * @details 将字符串str复制到新建立的空间并把该地址返回
     *          该函数会用malloc()创建，最后返回的地址要free
    */
    char* path = strdup(dirname.c_str());
    /**
     * @brief char *strchr(const char *str, int c)
     * @details 用于查找字符串中某个字符，并返回该字符在字符串中第一次出现的位置
     * @param[in] str 要查找的字符串
     * @param[in] c 要查找的字符
     * @return 找到则返回指向该字符的指针，未找到该字符则返回NULL
    */
    char* ptr = strchr(path + 1, '/');
    // /home/wwt/server_work
    // 逐层建立目录
    while(ptr) {
        *ptr = '\0';    // 将'/'替换为结束符'\0'，按级划分建立目录
        if(_Mkdir(path)) {
            free(path);
            return false;
        }
        *ptr = '/';
        ptr = strchr(ptr + 1, '/');
    }
    // 建立最深一层的目录
    if(_Mkdir(path)) {
        free(path);
        return false;
    }
    free(path);
    return true;
}

}