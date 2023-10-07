#include "library.h"
#include "env.h"

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

typedef Module* (*create_module)();
typedef void (*destory_module)(Module*);

/**
 * @brief Module析构类
*/
class ModuleClose {
public:
    /**
     * @brief 构造函数
    */
    ModuleClose(void* handle, destory_module destory) 
        :m_handle(handle)
        ,m_destory(destory) {
    }

    /**
     * @brief 重载()运算符
    */
    void operator()(Module* m) {
        std::string name = m->getName();
        std::string version = m->getVersion();
        std::string path = m->getFilename();
        m_destory(m);   // 析构类
        auto rt = dlclose(m_handle);    // 关闭句柄
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
    void* m_handle;             // dlopen()返回的句柄
    destory_module m_destory;   // 销毁Module类函数
};


// 根据path路径下的动态库，找到动态库中的Module创建函数CreateModule
// 根本不同模块的CreateModule，就可以实现不同的多态
Module::ptr Library::GetModule(const std::string& path) {
    // 打开一个动态链接库，并返回一个句柄给调用进程
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if(!handle) {
        SYLAR_LOG_ERROR(g_logger) << "cannot load library path="
                                << path << " error=" << dlerror();
        return nullptr;
    }

    // 从动态库中获取符号（全局变量与函数符号）地址，通常用于获取函数符号地址
    // create 指向 Module* CreateModule() 函数地址
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

    // create() 调用 CreateModule()
    // ModuleCloser(handle, destory) 是自定义智能指针析构函数
    // 析构时关闭句柄，删除new的module
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