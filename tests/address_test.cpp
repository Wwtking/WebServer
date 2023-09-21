#include <iostream>
#include <vector>
#include <map>
#include "log.h"
#include "address.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void lookup_test() {
    /**
     * @brief Lookup Test
    */
    SYLAR_LOG_INFO(g_logger) << "Lookup Test:";
    std::vector<sylar::Address::ptr> results;
    //bool flag = sylar::Address::Lookup(results, "www.baidu.com[:80]");
    //bool flag = sylar::Address::Lookup(results, "www.baidu.com:80");
    //bool flag = sylar::Address::Lookup(results, "www.baidu.com[:80]", AF_INET, SOCK_STREAM);
    bool flag = sylar::Address::Lookup(results, "localhost[:3080]");
    if(!flag) {
        SYLAR_LOG_ERROR(g_logger) << "Lookup fail";
        return;
    }
    for(const auto& i : results) {
        SYLAR_LOG_INFO(g_logger) << i->toString();
    }

    /**
     * @brief LookupAny Test
    */
    SYLAR_LOG_INFO(g_logger) << "LookupAny Test:";
    //auto addr = sylar::Address::LookupAny("www.baidu.com[:80]");
    auto addr = sylar::Address::LookupAny("localhost[:4080]");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << addr->toString();
    }
    else {
        SYLAR_LOG_ERROR(g_logger) << "Lookup addr is null";
    }

    /**
     * @brief LookupAnyIPAddress Test
    */
    SYLAR_LOG_INFO(g_logger) << "LookupAnyIPAddress Test:";
    auto ipAddr = sylar::Address::LookupAnyIPAddress("www.baidu.com[:80]");
    if(ipAddr) {
        SYLAR_LOG_INFO(g_logger) << ipAddr->toString();
    }
    else {
        SYLAR_LOG_ERROR(g_logger) << "Lookup ipAddr is null";
    }
}

void interface_test() {
    /**
     * @brief 测试获取所有网卡的地址信息
    */
    std::multimap<std::string, std::pair<sylar::Address::ptr, uint32_t> > results;
    bool flag = sylar::Address::GetInterfaceAddress(results, AF_UNSPEC);
    if(!flag) {
        SYLAR_LOG_ERROR(g_logger) << "GetInterfaceAddress fail";
        return;
    }
    for(const auto& i : results) {
        SYLAR_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << "/" <<i.second.second;
    }

    /**
     * @brief 测试获取指定网卡的地址信息
    */
    std::vector<std::pair<sylar::Address::ptr, uint32_t> > result;
    std::string interface_name = "ens33";
    flag = sylar::Address::GetInterfaceAddress(result, interface_name);
    if(!flag) {
        SYLAR_LOG_ERROR(g_logger) << "GetInterfaceAddress fail";
        return;
    }
    for(const auto& i : result) {
        SYLAR_LOG_INFO(g_logger) << interface_name << " - " << i.first->toString() << "/" <<i.second;
    }
}

void ipv4_test() {
    //auto addr = sylar::IPAddress::Create("www.baidu.com");
    auto addr = sylar::IPAddress::Create("192.168.88.130");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << addr->toString();
        SYLAR_LOG_INFO(g_logger) << addr->broadcastAddress(24)->toString();  // 广播地址
        SYLAR_LOG_INFO(g_logger) << addr->networkAddress(24)->toString();    // 网段地址
        SYLAR_LOG_INFO(g_logger) << addr->subnetAddress(24)->toString();     // 子网掩码
    }
}

void ipv6test() {
    auto addr = sylar::IPAddress::Create("fe80::612b:38bd:22aa:68d4");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << addr->toString();
        SYLAR_LOG_INFO(g_logger) << addr->broadcastAddress(96)->toString();  // 广播地址
        SYLAR_LOG_INFO(g_logger) << addr->networkAddress(96)->toString();    // 网段地址
        SYLAR_LOG_INFO(g_logger) << addr->subnetAddress(96)->toString();     // 子网掩码
    }
}


int main(int argc, char** argv) {
    //lookup_test();

    interface_test();

    //ipv4_test();

    //ipv6test();

    return 0;
}