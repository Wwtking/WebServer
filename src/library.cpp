#include "library.h"
#include "env.h"

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

typedef Module* (*create_module)();
typedef void (*destory_module)(Module*);


class ModuleClose {
public:
    ModuleClose(void* handle, destory_module destory) 
        :m_handle(handle)
        ,m_destory(destory) {
    }

    void operator()(Module* m) {
        std::string name = m->getName();
        std::string version = m->getVersion();
        std::string path = m->getFilename();
        m_destory(m);
        auto rt = dlclose(m_handle);
        if(!rt) {
            SYLAR_LOG_INFO(g_logger) << "destory module=" << name
                                    << " version=" << version
                                    << " path=" << path
                                    << " handle=" << m_handle
                                    << " success";
        }
        else {
            SYLAR_LOG_ERROR(g_logger) << "dlclose handle fail handle=" << m_handle 
                                    << " name=" << name
                                    << " version=" << version
                                    << " path=" << path
                                    << " error=" << dlerror();
        }
    }

private:
    void* m_handle;
    destory_module m_destory;
};


Module::ptr Library::GetModule(const std::string& path) {
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if(!handle) {
        SYLAR_LOG_ERROR(g_logger) << "cannot load library path="
                                << path << " error=" << dlerror();
        return nullptr;
    }

    create_module create = (create_module)dlsym(handle, "CreateModule");
    if(!create) {
        SYLAR_LOG_ERROR(g_logger) << "cannot load symbol CreateModule in "
                                << path << " error=" << dlerror();
        dlclose(handle);
        return nullptr;
    }

    destory_module destory = (destory_module)dlsym(handle, "DestoryModule");
    if(!destory) {
        SYLAR_LOG_ERROR(g_logger) << "cannot load symbol DestoryModule in "
                                << path << " error=" << dlerror();
        dlclose(handle);
        return nullptr;
    }

    Module::ptr module(create(), ModuleClose(handle, destory));
    module->setFilename(path);
    SYLAR_LOG_INFO(g_logger) << "load module name=" << module->getName()
                            << " version=" << module->getVersion()
                            << " path=" << module->getFilename()
                            << " success";
    Config::LoadFromConfDir(EnvMgr::GetInstance()->getConfPath(), true);
    return module;
}

}