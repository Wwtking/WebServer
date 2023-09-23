#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include <memory>
#include <vector>
#include "noncopyable.h"
#include "socket.h"
#include "iomanager.h"
#include "config.h"

namespace sylar {

struct TcpServerConf {
    typedef std::shared_ptr<TcpServerConf> ptr;

    std::vector<std::string> address;
    int keepalive = 0;
    int timeout = 1000 * 60;
    std::string name = "wwt";
    std::string type = "http";
    std::string accept_worker;
    std::string process_worker;

    bool isValid() const { 
        return !address.empty();
    }

    bool operator==(const TcpServerConf& conf) const {
        return this->address == conf.address
            && this->keepalive == conf.keepalive
            && this->timeout == conf.keepalive
            && this->name == conf.name
            && this->type == conf.type
            && this->accept_worker == accept_worker
            && this->process_worker == process_worker;
    }
};

// 类型转换模板类片特化(YAML String 转换成 TcpServerConf)
template<>
class LexicalCast<std::string, TcpServerConf> {
public:
    TcpServerConf operator()(const std::string& str) {
        TcpServerConf conf;
        YAML::Node node = YAML::Load(str);
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        conf.type = node["type"].as<std::string>(conf.type);
        conf.accept_worker = node["accept_worker"].as<std::string>();
        conf.process_worker = node["process_worker"].as<std::string>();
        if(node["address"].IsDefined()) {
            for(int i = 0; i < (int)node["address"].size(); i++) {
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }
        return conf;
    }
};

// 类型转换模板类片特化(TcpServerConf 转换成 YAML String)
template<>
class LexicalCast<TcpServerConf, std::string> {
public:
    std::string operator()(const TcpServerConf& conf) {
        YAML::Node node(YAML::NodeType::Map);
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        node["name"] = conf.name;
        node["type"] = conf.type;
        node["accept_worker"] = conf.accept_worker;
        node["process_worker"] = conf.process_worker;
        for(auto& i : conf.address) {
            node["address"].push_back(YAML::Load(i));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief TCP服务器封装
 * @details 实现启动、运行、关闭服务器整个流程
 *          (1)用Socket来封装Address地址，通过Socket来bind对应地址和进行listen，
 *          (2)然后开始accept来自客户端的socket(等待用户connect)，成功accept后生成新的Socket用于通信，
 *          (3)然后开始执行指定的任务(即调用回调函数handleClient)，
 *          (4)最后停止服务器服务
 */
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    /**
     * @brief 构造函数
     * @param[in] worker 调度新连接的Socket工作的协程调度器
     * @param[in] acceptWorker 调度服务器Socket执行accept的协程调度器
     */
    TcpServer(IOManager* worker = IOManager::GetThis(), 
                IOManager* acceptWorker = IOManager::GetThis());

    /**
     * @brief 析构函数
     * @details 关闭服务器所有Socket
     */
    virtual ~TcpServer();

    /**
     * @brief 服务器绑定地址(单个地址)
     * @details 实际上完成服务器的bind和listen操作
     * @param[in] addr 要绑定的地址
     * @return 返回是否绑定成功
     */
    virtual bool bind(const Address::ptr addr);

    /**
     * @brief 服务器绑定地址(多个地址)
     * @details 实际上完成服务器的bind和listen操作
     * @param[in] addrs 要绑定的地址数组
     * @param[in] fails 绑定失败的地址数组
     * @return 返回是否绑定成功
     */
    virtual bool bind(const std::vector<Address::ptr> addrs, std::vector<Address::ptr>& fails);

    /**
     * @brief 启动服务器
     * @details 完成服务器的accept操作，并且accept成功后开始执行指定任务
     * @pre 需要bind和listen成功后执行
     * @return 返回是否启动成功
    */
    virtual bool start();

    /**
     * @brief 停止服务器
     * @details 取消服务器Socket所有事件并关闭
     */
    virtual void stop();

    /**
     * @brief 返回服务器接收超时时间(毫秒)
     */
    uint64_t getRecvTimeout() const { return m_recvTimeout; }

    /**
     * @brief 设置服务器接收超时时间(毫秒)
     */
    void setRecvTimeout(uint64_t timeout) { m_recvTimeout = timeout; }

    /**
     * @brief 判断服务器是否停止
     */
    bool isStop() const { return m_isStop; }

    /**
     * @brief 设置服务器是否停止
     */
    void setStop(bool v) { m_isStop = v; }
    
    /**
     * @brief 返回服务器名称
     */
    const std::string& getName() const { return m_name; }

    /**
     * @brief 设置服务器名称
     */
    void setName(std::string name) { m_name = name; }

    TcpServerConf::ptr getConf() const { return m_conf; }
    void setConf(TcpServerConf::ptr conf) { m_conf = conf; }
    void setConf(const TcpServerConf& conf);

protected:
    /**
     * @brief 处理新连接的Socket类
     * @details 每accept到一个socket，就会执行一次，触发回调
     * @param[in] client 新连接的Socket类
     */
    virtual void handleClient(Socket::ptr client);

    /**
     * @brief 完成服务器单个Socket的accept的操作
     * @param[in] sock 服务器绑定的Socket
     */
    virtual void startAccept(Socket::ptr sock);

protected:
    std::vector<Socket::ptr> m_socket;  // 服务器端绑定的Socket数组
    IOManager* m_worker;                // 调度新连接的Socket工作的协程调度器
    IOManager* m_acceptWorker;          // 调度服务器Socket执行accept的协程调度器
    std::string m_name;                 // 服务器名称
    uint64_t m_recvTimeout;             // 服务器接收超时时间(毫秒)
    bool m_isStop;                      // 服务是否停止

    TcpServerConf::ptr m_conf;          // 服务器配置
};

}

#endif