#include "worker.h"
#include "config.h"
#include "util.h"


namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_worker_config
    = Config::Lookup("workers", std::map<std::string, std::map<std::string, std::string> >(), "worker config");


WorkerManager::WorkerManager() 
    :m_stop(false) {
}

bool WorkerManager::init() {
    auto m = g_worker_config->getValue();
    return init(m);
}

bool WorkerManager::init(const std::map<std::string, std::map<std::string, std::string> >& v) {
    // map的key为workers.xx  map的value的key为workers.xx.thread_num
    for(auto& i : v) {
        std::string name = i.first; // IOManager名称
        int thread_num = GetParamValue(i.second, "thread_num", 1);   // 线程池数量
        int worker_num = GetParamValue(i.second, "worker_num", 1);

        // worker_num == 1
        for(int j = 0; j < worker_num; j++) {
            Scheduler::ptr s;
            if(!j) {
                // 创建YAML中指定名称和线程数量的IOManager
                s = std::make_shared<IOManager>(thread_num, false, name);
            }
            else {
                s = std::make_shared<IOManager>(thread_num, false, name + "-" + std::to_string(j));
            }
            add(s);
        }
    }
    // SYLAR_LOG_DEBUG(g_logger) << "----------------------" << getCount();
    m_stop = m_datas.empty();
    return true;
}

void WorkerManager::add(Scheduler::ptr s) {
    m_datas[s->getName()].push_back(s);
}

void WorkerManager::del(const std::string& name) {
    auto it = m_datas.find(name);
    if(it != m_datas.end()) {
        m_datas.erase(it);
    }
}

Scheduler::ptr WorkerManager::get(const std::string& name) {
    auto it = m_datas.find(name);
    if(it == m_datas.end()) {
        return nullptr;
    }
    if(it->second.size() == 1) {
        return it->second[0];
    }
    return it->second[rand() % it->second.size()];
}

IOManager::ptr WorkerManager::getAsIOManager(const std::string& name) {
    return std::dynamic_pointer_cast<IOManager>(get(name));
}

uint32_t WorkerManager::getCount() {
    return m_datas.size();
}

void WorkerManager::stop() {
    if(m_stop) {
        return;
    }
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            n->scheduler([](){});
            n->stop();
        }
    }
    m_datas.clear();
    m_stop = true;
}

std::ostream& WorkerManager::dump(std::ostream& os) {
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            n->dump(os) << std::endl;
        }
    }
    return os;
}

}