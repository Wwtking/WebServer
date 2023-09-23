#ifndef __SYLAR_LIBRARY_H__
#define __SYLAR_LIBRARY_H__

#include "log.h"
#include "module.h"
#include <dlfcn.h>

namespace sylar {


class Library {
public:
    static Module::ptr GetModule(const std::string& path);
};

}

#endif