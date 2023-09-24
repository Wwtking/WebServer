#ifndef __SYLAR_MY_MODULE__
#define __SYLAR_MY_MODULE__

#include "module.h"
#include <memory>

namespace name_space {

class MyModule : public sylar::Module {
public:
    typedef std::shared_ptr<MyModule> ptr;

    MyModule();

    virtual bool onLoad() override;
    virtual bool onUnLoad() override;

    virtual bool onServerReady() override;
    virtual bool onServerUp() override;
};

}

#endif