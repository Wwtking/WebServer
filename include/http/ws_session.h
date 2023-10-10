#ifndef __SYLAR_WS_SESSION_H__
#define __SYLAR_WS_SESSION_H__

#include <memory>
#include "http_session.h"

namespace sylar {
namespace http {

// WsSession/WsConnection
// server.accept  socket -> Session
// client.connect socket -> Connection

/*  WebSocket协议格式(头部+内容)
     0               1               2               3                -字节数
     0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7  -位数
    +-+-+-+-+-------+-+-------------+-------------------------------+
    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    |I|S|S|S|  (4)  |A|     (7)     |            (16/64)            |
    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    | |1|2|3|       |K|             |                               |
    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
    |   Extended payload length continued, if payload len == 127    |
    + - - - - - - - - - - - - - - - +-------------------------------+
    |  payload length (continued)   | Masking-key, if MASK set to 1 |
    +-------------------------------+-------------------------------+
    |    Masking-key (continued)    |          Payload Data         |
    +-------------------------------- - - - - - - - - - - - - - - - +
    :                     Payload Data continued ...                :
    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
    |                     Payload Data continued ...                |
    +---------------------------------------------------------------+
*/

#pragma pack(1) // 用于告诉编译器按照1字节对齐结构体成员变量，而不是默认的对齐方式
struct WSFrameHead {
    enum OPCODE {
        /// 表示数据分片传输时的中间帧(除第一个帧)
        CONTINUE = 0,
        /// 文本帧
        TEXT_FRAME = 1,
        /// 二进制帧
        BIN_FRAME = 2,
        /// %x3-7 预留给以后的非控制帧
        /// 断开连接
        CLOSE = 8,
        /// PING
        PING = 0x9,
        /// PONG
        PONG = 0xA
    };
    uint32_t opcode: 4;    // 表示​有效数据的状态
    bool rsv3: 1;          // 用于扩展定义，如果没有扩展约定的情况则必须为0
    bool rsv2: 1;
    bool rsv1: 1;
    bool fin: 1;           // 表示这是消息的最后一个片段，为1则是消息尾部
    uint32_t payload: 7;   // 以字节为单位的有效数据长度
    bool mask: 1;          // 表示有效数据是否添加掩码

    // 可读性输出
    std::string toString() const;
};
#pragma pack()  // 用于取消对齐方式的设置，恢复默认的对齐方式


/**
 * @brief WebSocket消息体封装
*/
class WSFrameMessage {
public:
    typedef std::shared_ptr<WSFrameMessage> ptr;

    /**
     * @brief 构造函数
    */
    WSFrameMessage(uint32_t opcode = 0, const std::string& data = "");

    /**
     * @brief 获取消息体类型
    */
    uint32_t getOpcode() const { return m_opcode; }

    /**
     * @brief 设置消息体类型
    */
    void setOpcode(uint32_t opcode) { m_opcode = opcode; }

    /**
     * @brief 获取消息体内容(不可修改)
    */
    const std::string& getData() const { return m_data; }

    /**
     * @brief 获取消息体内容(可修改)
    */
    std::string& getData() { return m_data; }

    /**
     * @brief 设置消息体内容
    */
    void setData(const std::string& data) { m_data = data; }

private:
    uint32_t m_opcode;      // 消息体类型
    std::string m_data;     // 消息体内容
};


/**
 * @brief WSSession封装
 * @details 在握手和挥手阶段，都是用Http协议来请求和响应的
 *          在双向通信时，是使用WebSocket协议通信的
*/
class WSSession : public HttpSession {
public:
    typedef std::shared_ptr<WSSession> ptr;

    /**
     * @brief 构造函数
    */
    WSSession(Socket::ptr sock, bool owner = false);

    /**
     * @brief 握手建连阶段，接收客户端的连接，发送响应
     * @return 返回Http请求
    */
    HttpRequest::ptr handleShake();

    /**
     * @brief WebSocket接收消息
     * @return 返回接收到的WebSocket消息体
    */
    WSFrameMessage::ptr recvMessage();

    /**
     * @brief WebSocket发送消息
     * @param[in] msg 要发送的WebSocket消息体
     * @param[in] fin 是否为消息的最后一个片段
     * @return 返回发送字节数
    */
    int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);

    /**
     * @brief WebSocket发送消息
     * @param[in] data 要发送的WebSocket消息体内容
     * @param[in] opcode 要发送的WebSocket消息体类型
     * @param[in] fin 是否为消息的最后一个片段
     * @return 返回发送字节数
    */
    int32_t sendMessage(const std::string& data,  uint32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);

    /**
     * @brief 发送Ping包
     * @return 返回发送字节数
    */
    int32_t ping();

    /**
     * @brief 发送Pong包
     * @return 返回发送字节数
    */
    int32_t pong();
};


/**
 * @brief WebSocket接收消息
 * @param[in] stream 流式结构，因为WSSession、HttpSession都间接继承自stream
 * @param[in] isClient 是否为客户端接收消息
 * @return 返回接收到的WebSocket消息体
*/
WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool isClient);

/**
 * @brief WebSocket发送消息
 * @param[in] stream 流式结构，因为WSSession、HttpSession都间接继承自stream
 * @param[in] isClient 是否为客户端发送消息
 * @param[in] msg 要发送的WebSocket消息体
 * @param[in] fin 是否为消息的最后一个片段
 * @return 返回发送字节数
*/
int32_t WSSendMessage(Stream* stream, bool isClient, WSFrameMessage::ptr msg, bool fin);

/**
 * @brief 发送Ping包
 * @param[in] stream 流式结构，因为WSSession、HttpSession都间接继承自stream
 * @return 返回发送字节数
*/
int32_t WSPing(Stream* stream);

/**
 * @brief 发送Pong包
 * @param[in] stream 流式结构，因为WSSession、HttpSession都间接继承自stream
 * @return 返回发送字节数
*/
int32_t WSPong(Stream* stream);

}
}


#endif