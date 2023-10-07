#ifndef __SYLAER_MODULE_H__
#define __SYLAER_MODULE_H__

#include "config.h"
#include "thread.h"
#include "singleton.h"
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>

namespace sylar {

/**
 * @brief Module基类
*/
class Module {
public:
    typedef std::shared_ptr<Module> ptr;

    enum Type {
        MODULE = 0,
        ROCK = 1
    };

    /**
     * @brief 构造函数
    */
    Module(std::string name, std::string version
                        , std::string filename
                        , uint32_t type = MODULE);

    /**
     * @brief 析构函数
    */
    virtual ~Module() {}

    /**
     * @brief 解析启动参数前执行的函数
    */
    virtual void onBeforeArgsParse(int argc, char** argv);

    /**
     * @brief 解析启动参数后执行的函数
    */
    virtual void onAfterArgsParse(int argc, char** argv);
    
    /**
     * @brief 服务器启动前要执行的加载函数
    */
    virtual bool onLoad();

    /**
     * @brief 卸载函数
    */
    virtual bool onUnLoad();

    /**
     * @brief 服务器绑定好端口之后的回调函数
    */
    virtual bool onServerReady();
    
    virtual bool onServerUp();

    /**
     * @brief 可读性输出
    */
    virtual std::string toString() const;

    const std::string& getName() const { return m_name; }
    const std::string& getVersion() const { return m_version; }
    const std::string& getFilename() const { return m_filename; }
    const std::string& getId() const { return m_id; }
    uint32_t getType() const { return m_type; }

    void setName(const std::string& name) { m_name = name; }
    void setVersion(const std::string& version) { m_version = version; }
    void setFilename(const std::string& filename) { m_filename = filename; }
    void setId(const std::string& id) { m_id = id; }
    void setType(uint32_t type) { m_type = type; }

protected:
    std::string m_name;         // 名称
    std::string m_version;      // 版本号
    std::string m_filename;     // 路径
    std::string m_id;           // 标识(名称 + 版本号)
    uint32_t m_type;            // 类型

};

/**
 * @brief Module管理类
*/
class ModuleManager {
public:
    typedef RWMutex MutexType;
    
    /**
     * @brief 构造函数
    */
    ModuleManager();

    /**
     * @brief 初始化
     * @details 根据动态库创建出多态的Module，并保存下来
    */
    void init();

    /**
     * @brief 添加Module
    */
    void addModule(Module::ptr m);

    /**
     * @brief 删除Module
    */
    void delModule(const std::string& key);

    /**
     * @brief 删除所有Module
    */
    void delAllModules();

    /**
     * @brief 获取Module
    */
    Module::ptr getModule(const std::string& key);

    /**
     * @brief 根据类型type列出所有Module，存在在m中
    */
    void listModulesByType(uint32_t type, std::vector<Module::ptr>& m);

    /**
     * @brief 列出所有Module，存在在m中
    */
    void listAllModules(std::vector<Module::ptr>& m);

    /**
     * @brief 根据类型type，将所有Module执行cb函数
    */
    void foreach(uint32_t type, std::function<void(Module::ptr)> cb);

private:
    /// 读写锁
    MutexType m_mutex;
    /// 存放所有Module
    std::unordered_map<std::string, Module::ptr> m_modules;
    /// 根据类型存放所有Module
    std::unordered_map<uint32_t, 
            std::unordered_map<std::string, Module::ptr> > m_typeModules;
};

typedef Singleton<ModuleManager> ModuleMgr;

}


#endif