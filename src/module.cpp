#include "module.h"
#include "library.h"
#include "env.h"

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static ConfigVar<std::string>::ptr g_module_path = 
        Config::Lookup("module.path", std::string("module"), "module path");

Module::Module(std::string name, std::string version
                            , std::string filename
                            , uint32_t type) 
    :m_name(name)
    ,m_version(version)
    ,m_filename(filename)
    ,m_id(name + "/" + version)
    ,m_type(type) {
}

void Module::onBeforeArgsParse(int argc, char** argv) {
}

void Module::onAfterArgsParse(int argc, char** argv) {
}

bool Module::onLoad() {
    return true;
}

bool Module::onUnLoad() {
    return true;
}

bool Module::onServerReady() {
    return true;
}

bool Module::onServerUp() {
    return true;
}

std::string Module::toString() const {
    std::stringstream ss;
    ss << "Module name=" << m_name 
        << " version=" << m_version 
        << " filename=" << m_filename
        << std::endl;
    return ss.str();
}


ModuleManager::ModuleManager() {
}

void ModuleManager::init() {
    auto path = EnvMgr::GetInstance()->getCwd() + g_module_path->getValue();
    // std::string path = g_module_path->getValue();

    // 把path路径下所有动态库文件的绝对路径列出
    std::vector<std::string> files;
    FilesUtil::ListAllFiles(files, path, ".so");

    // 遍历所有动态库，找到各动态库中的Module创建函数CreateModule
    // 根据不同动态库的CreateModule，创建出不同多态的Module
    for(auto& i : files) {
        Module::ptr module = Library::GetModule(i);
        if(module) {
            addModule(module);
        }
    }
}

void ModuleManager::addModule(Module::ptr m) {
    MutexType::WriteLock lock(m_mutex);
    m_modules[m->getId()] = m;
    m_typeModules[m->getType()][m->getId()] = m;
}

void ModuleManager::delModule(const std::string& key) {
    MutexType::WriteLock lock(m_mutex);
    auto it = m_modules.find(key);
    if(it == m_modules.end()) {
        return;
    }
    m_modules.erase(it);

    Module::ptr module = it->second;
    m_typeModules[module->getType()].erase(key);
    if(m_typeModules[module->getType()].empty()) {
        m_typeModules.erase(module->getType());
    }

    lock.unlock();
    module->onUnLoad();
}

void ModuleManager::delAllModules() {
    MutexType::ReadLock lock(m_mutex);
    auto temp = m_modules;
    lock.unlock();

    for(auto& i : temp) {
        delModule(i.first);
    }
}

Module::ptr ModuleManager::getModule(const std::string& key) {
    MutexType::ReadLock lock(m_mutex);
    auto it = m_modules.find(key);
    return it == m_modules.end() ? nullptr : it->second;
}

void ModuleManager::listModulesByType(uint32_t type, std::vector<Module::ptr>& m) {
    MutexType::ReadLock lock(m_mutex);
    auto it = m_typeModules.find(type);
    if(it == m_typeModules.end()) {
        return;
    }
    for(auto& i : it->second) {
        m.push_back(i.second);
    }
}

void ModuleManager::listAllModules(std::vector<Module::ptr>& m) {
    MutexType::ReadLock lock(m_mutex);
    for(auto& i : m_modules) {
        m.push_back(i.second);
    }
}

void ModuleManager::foreach(uint32_t type, std::function<void(Module::ptr)> cb) {
    std::vector<Module::ptr> modules;
    listModulesByType(type, modules);
    for(auto& i : modules) {
        cb(i);
    }
}

}