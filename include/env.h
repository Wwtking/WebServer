#ifndef __SYLAR_ENV_H__
#define __SYLAR_ENV_H__

#include "thread.h"
#include "singleton.h"
#include <map>
#include <vector>

namespace sylar {

class Env {
public:
    typedef RWMutex RWMutexType;

    /**
     * @brief 初始化
     * @details 将传入启动参数存放，并提取出工作目录的绝对路径
    */
    bool init(int argc, char** argv);

    /**
     * @brief 添加启动参数
    */
    void addArg(const std::string& key, const std::string& val);

    /**
     * @brief 查询启动参数是否存在
    */
    bool hasArg(const std::string& key);

    /**
     * @brief 删除启动参数
    */
    void delArg(const std::string& key);

    /**
     * @brief 获取启动参数，若不存在返回默认
    */
    const std::string& getArg(const std::string& key, const std::string& def = "");
    
    /**
     * @brief 打印所有启动参数
    */
    void printArgs();

    /**
     * @brief 添加启动参数说明
    */
    void addHelp(const std::string& key, const std::string& val);

    /**
     * @brief 删除启动参数说明
    */
    void delHelp(const std::string& key);

    /**
     * @brief 打印所有启动参数说明
    */
    void printHelps();

    /**
     * @brief 设置系统环境变量
     * @param[in] name 环境变量名字
     * @param[in] value 环境变量值
    */
    bool setEnv(const std::string& name, const std::string& value);

    /**
     * @brief 获取系统环境变量
     * @param[in] name 环境变量名字
     * @param[in] def 默认值
    */
    std::string getEnv(const std::string& name, const std::string& def = "");
    
    /**
     * @brief 获取可执行文件的名称
    */
    const std::string& getProgram() const { return m_program; }

    /**
     * @brief 获取可执行文件的绝对路径
    */
    const std::string& getExe() const { return m_exe; }

    /**
     * @brief 获取可执行文件的所在路径
    */
    const std::string& getCwd() const { return m_cwd; }

    /**
     * @brief 获取工作目录的绝对路径
    */
    const std::string& getWorkPath() const { return m_workPath; }
    
    /**
     * @brief 获取工作目录下某文件/文件夹的绝对路径
     * @param[in] path 文件/文件夹名称
    */
    std::string getAbsolutePath(const std::string& path);

    /**
     * @brief 获取工作目录下配置文件的绝对路径
    */
    std::string getDefaultConfPath();

    /**
     * @brief 获取要配置文件的绝对路径
    */
    std::string getConfPath();

private:
    RWMutexType m_mutex;
    std::string m_program;  // 当前应用名称(可执行文件)
    std::string m_exe;      // 可执行文件的绝对路径
    std::string m_cwd;      // 可执行文件所在路径
    std::string m_workPath; // 工作目录的绝对路径
    std::map<std::string, std::string> m_args;                  // 所有启动参数 <"-file", "xxx">
    std::vector<std::pair<std::string, std::string> > m_helps;  // 启动参数说明 <"-p", "print help">
};

typedef Singleton<Env> EnvMgr;

}

#endif