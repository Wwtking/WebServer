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

class Module {
public:
    typedef std::shared_ptr<Module> ptr;

    enum Type {
        MODULE = 0,
        ROCK = 1
    };

    Module(std::string name, std::string version
                        , std::string filename
                        , uint32_t type = MODULE);

    virtual ~Module() {}

    virtual void onBeforeArgsParse(int argc, char** argv);
    virtual void onAfterArgsParse(int argc, char** argv);

    virtual bool onLoad();
    virtual bool onUnLoad();

    virtual bool onServerReady();
    virtual bool onServerUp();

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
    std::string m_name;
    std::string m_version;
    std::string m_filename;
    std::string m_id;
    uint32_t m_type;

};

class ModuleManager {
public:
    typedef RWMutex MutexType;

    ModuleManager();

    void init();

    void addModule(Module::ptr m);
    void delModule(const std::string& key);
    void delAllModules();
    Module::ptr getModule(const std::string& key);

    void listModulesByType(uint32_t type, std::vector<Module::ptr>& m);
    void listAllModules(std::vector<Module::ptr>& m);
    void foreach(uint32_t type, std::function<void(Module::ptr)> cb);

private:
    MutexType m_mutex;
    std::unordered_map<std::string, Module::ptr> m_modules;
    std::unordered_map<uint32_t, 
            std::unordered_map<std::string, Module::ptr> > m_typeModules;
};

typedef Singleton<ModuleManager> ModuleMgr;

}


#endif