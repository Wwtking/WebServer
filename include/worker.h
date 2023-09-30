#ifndef __SYLAR_WORKER_H__
#define __SYLAR_WORKER_H__

#include "iomanager.h"
#include "singleton.h"

namespace sylar {

class WorkerManager { 
public:
    WorkerManager();

    bool init();
    bool init(const std::map<std::string, std::map<std::string, std::string> >& v);

    void add(Scheduler::ptr s);
    void del(const std::string& name);
    Scheduler::ptr get(const std::string& name);
    IOManager::ptr getAsIOManager(const std::string& name);

    uint32_t getCount();

    void stop();
    bool isStop() const { return m_stop; }

    std::ostream& dump(std::ostream& os);

private:
    std::map<std::string, std::vector<Scheduler::ptr> > m_datas;
    bool m_stop;
};

typedef Singleton<WorkerManager> WorkerMgr;

}

#endif