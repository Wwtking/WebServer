#ifndef __SYLAR_WORKER_H__
#define __SYLAR_WORKER_H__

#include "iomanager.h"
#include "singleton.h"

namespace sylar {

class WorkerManager { 
public:
    /**
     * @brief 构造函数
    */
    WorkerManager();

    /**
     * @brief 初始化函数
     * @details 创建YAML中指定名称和线程数量的IOManager并保存
    */
    bool init();
    bool init(const std::map<std::string, std::map<std::string, std::string> >& v);

    /**
     * @brief 添加协程调度器Scheduler
    */
    void add(Scheduler::ptr s);

    /**
     * @brief 删除协程调度器Scheduler
    */
    void del(const std::string& name);

    /**
     * @brief 获取协程调度器Scheduler
    */
    Scheduler::ptr get(const std::string& name);

    /**
     * @brief 获取IOManager
    */
    IOManager::ptr getAsIOManager(const std::string& name);

    /**
     * @brief 获取协程调度器数量
    */
    uint32_t getCount();

    /**
     * @brief 停止所有调度器
    */
    void stop();

    /**
     * @brief 是否停止
    */
    bool isStop() const { return m_stop; }

    /**
     * @brief 可读性输出
    */
    std::ostream& dump(std::ostream& os);

private:
    /// 存放Scheduler协程调度器
    std::map<std::string, std::vector<Scheduler::ptr> > m_datas;
    /// 是否停止
    bool m_stop;
};

typedef Singleton<WorkerManager> WorkerMgr;

}

#endif