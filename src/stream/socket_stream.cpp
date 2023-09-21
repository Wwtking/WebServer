#include "socket_stream.h"

namespace sylar {

// 构造函数
SocketStream::SocketStream(Socket::ptr sock, bool owner) 
    :m_socket(sock) 
    ,m_owner(owner) {
}

// 析构函数
SocketStream::~SocketStream() {
    if(m_owner && m_socket) {
        m_socket->close();
    }
}

// 重写读数据函数
ssize_t SocketStream::read(void* buff, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return m_socket->recv(buff, length);
}

// 重写读数据函数
ssize_t SocketStream::read(ByteArray::ptr buff, size_t length) {
    if(!isConnected()) {
        return -1;
    }

    std::vector<iovec> buffer;
    buff->getWriteBuffers(buffer, length);
    ssize_t rt = m_socket->recv(&buffer[0], buffer.size());
    if(rt > 0) {
        // 用到readFixSize()时，写入buff一部分，继续往后写剩余部分，所以要setPosition
        buff->setPosition(buff->getPosition() + rt);
    }
    return rt;
}

// 重写写数据函数
ssize_t SocketStream::write(const void* buff, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return m_socket->send(buff, length);
}

// 重写写数据函数
ssize_t SocketStream::write(ByteArray::ptr buff, size_t length) {
    if(!isConnected()) {
        return -1;
    }

    std::vector<iovec> buffer;
    buff->getReadBuffers(buffer, length);
    ssize_t rt = m_socket->send(&buffer[0], buffer.size());
    if(rt > 0) {
        // 用到writeFixSize()时，读完buff一部分，继续往后读剩余部分，所以要setPosition
        buff->setPosition(buff->getPosition() + rt);
    }
    return rt;
}

// 关闭Socket
bool SocketStream::close() {
    if(m_socket) {
        return m_socket->close();
    }
    return true;
}

// 判断是否已成功连接
bool SocketStream::isConnected() const {
    return m_socket && m_socket->isConnected();
}

}