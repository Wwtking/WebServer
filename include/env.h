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

    bool init(int argc, char** argv);

    void addArg(const std::string& key, const std::string& val);
    bool hasArg(const std::string& key);
    void delArg(const std::string& key);
    const std::string& getArg(const std::string& key, const std::string& def = "");
    void printArgs();

    void addHelp(const std::string& key, const std::string& val);
    void delHelp(const std::string& key);
    void printHelps();

    // 设置系统环境变量
    bool setEnv(const std::string& name, const std::string& value);
    // 获取系统环境变量
    std::string getEnv(const std::string& name, const std::string& def = "");

    const std::string& getProgram() const { return m_program; }
    const std::string& getExe() const { return m_exe; }
    const std::string& getCwd() const { return m_cwd; }
    const std::string& getWorkPath() const { return m_workPath; }
    
    std::string getAbsolutePath(const std::string& path);
    std::string getDefaultConfPath();
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