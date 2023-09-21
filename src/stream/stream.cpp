#include "stream.h"
#include "log.h"
#include "config.h"

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// Socket中recv/send时的最大buff长度
static ConfigVar<uint64_t>::ptr g_socket_max_buff_size = Config::Lookup(
        "socket.buff_size", (uint64_t)(16 * 1024), "socket max buff size");

// 匿名空间：将命名空间的名字符号去掉，让其他文件找不到，效果类似于static
namespace {
// 添加监听函数，配置改变时执行回调函数
struct SocketBuffSizeInit {
    SocketBuffSizeInit() {
        g_socket_max_buff_size->addListener([](const uint64_t& oldVal, const uint64_t& newVal) {
            g_socket_max_buff_size->setValue(newVal);
            SYLAR_LOG_INFO(g_logger) << "Socket max buff size" << " old_value=" << oldVal
                                                               << " new_value=" << newVal;
        });
    }
};
static SocketBuffSizeInit _Init;
}

// 读固定长度的数据
ssize_t Stream::readFixSize(void* buffer, size_t length) {
    uint64_t offset = 0;
    uint64_t size = length;
    const uint64_t maxLen = g_socket_max_buff_size->getValue();

    // 一次读不完，多次读
    while(size > 0) {
        ssize_t rt = read((char*)buffer + offset, std::min(size, maxLen));
        if(rt <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "readFixSize error rt=" << rt 
                << " errno=" << errno << " errstr=" << strerror(errno);
            return rt;
        }
        offset += rt;
        size -= rt;
    }
    return length;
}

// 读固定长度的数据
ssize_t Stream::readFixSize(ByteArray::ptr buffer, size_t length) {
    uint64_t size = length;
    const uint64_t maxLen = g_socket_max_buff_size->getValue();

    // 一次读不完，多次读
    while(size > 0) {
        ssize_t rt = read(buffer, std::min(size, maxLen));
        if(rt <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "readFixSize error rt=" << rt 
                << " errno=" << errno << " errstr=" << strerror(errno);
            return rt;
        }
        size -= rt;
    }
    return length;
}

// 写固定长度的数据
ssize_t Stream::writeFixSize(const void* buffer, size_t length) {
    uint64_t offset = 0;
    uint64_t size = length;
    const uint64_t maxLen = g_socket_max_buff_size->getValue();

    // 一次写不完，多次写
    while(size > 0) {
        ssize_t rt = write((const char*)buffer + offset, std::min(size, maxLen));
        if(rt <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "writeFixSize error rt=" << rt 
                << " errno=" << errno << " errstr=" << strerror(errno);
            return rt;
        }
        offset += rt;
        size -= rt;
    }
    return length;
}

// 写固定长度的数据
ssize_t Stream::writeFixSize(ByteArray::ptr buffer, size_t length) {
    uint64_t size = length;
    const uint64_t maxLen = g_socket_max_buff_size->getValue();

    // 一次写不完，多次写
    while(size > 0) {
        ssize_t rt = write(buffer, std::min(size, maxLen));
        if(rt <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "writeFixSize error rt=" << rt 
                << " errno=" << errno << " errstr=" << strerror(errno);
            return rt;
        }
        size -= rt;
    }
    return length;
}

}