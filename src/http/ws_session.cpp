#include "log.h"
#include "config.h"
#include "ws_session.h"
#include "hash_util.h"
#include "myendian.h"


namespace sylar {
namespace http {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static ConfigVar<uint32_t>::ptr g_websocket_message_max_size = 
                Config::Lookup("websocket.message.max_size", 
                (uint32_t)1024 * 1024 * 32, "websocket message max size");


std::string WSFrameHead::toString() const {
    std::stringstream ss;
    ss << "[WSFrameHead fin=" << fin
       << " rsv1=" << rsv1
       << " rsv2=" << rsv2
       << " rsv3=" << rsv3
       << " opcode=" << opcode
       << " mask=" << mask
       << " payload=" << payload
       << "]";
    return ss.str();
}

WSFrameMessage::WSFrameMessage(uint32_t opcode, const std::string& data) 
    :m_opcode(opcode)
    ,m_data(data) {
}

WSSession::WSSession(Socket::ptr sock, bool owner) 
    :HttpSession(sock, owner) {
}

// websocket 请求头格式
// GET /chat HTTP/1.1
// Host: server.example.com
// Upgrade: websocket
// Connection: Upgrade
// Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
// Origin: http://example.com
// Sec-WebSocket-Protocol: chat, superchat
// Sec-WebSocket-Version: 13

// websocket 响应头格式
// HTTP/1.1 101 Switching Protocols
// Upgrade: websocket
// Connection: Upgrade
// Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
// Sec-WebSocket-Protocol: chat

// 握手建连阶段，接收客户端的连接，发送响应
HttpRequest::ptr WSSession::handleShake() {
    HttpRequest::ptr req = recvHttpRequest();
    if(!req) {
        SYLAR_LOG_INFO(g_logger) << "invalid http request";
        return nullptr;
    }

    // 指示客户端希望将当前的HTTP协议升级为WebSocket协议
    // WebSocket协议支持双向通信，使得客户端和服务器可以长时间保持连接并实时交换数据
    if(strcasecmp(req->getHeader("Upgrade").c_str(), "websocket") != 0) {
        SYLAR_LOG_INFO(g_logger) << "http header Upgrade != websocket";
        return nullptr;
    }
    // 指示客户端希望与服务器之间的连接被升级为支持双向通信的协议
    if(strcasecmp(req->getHeader("Connection").c_str(), "Upgrade") != 0) {
        SYLAR_LOG_INFO(g_logger) << "http header Connection != Upgrade";
        return nullptr;
    }
    // 指示客户端使用的WebSocket协议版本
    if(req->getHeaderValue<int>("Sec-WebSocket-Version") != 13) {
        SYLAR_LOG_INFO(g_logger) << "http header Sec-webSocket-Version != 13";
        return nullptr;
    }
    // Sec-WebSocket-Key是HTTP请求头中的一个字段，其value是一个随机的16字节字符串
    // 用于安全通信，在WebSocket握手时生成握手响应的Sec-WebSocket-Accept字段
    std::string key = req->getHeader("Sec-WebSocket-Key");
    if(key.empty()) {
        SYLAR_LOG_INFO(g_logger) << "http header Sec-WebSocket-Key = null";
        return nullptr;
    }

    // 服务器通过将Sec-WebSocket-Key与一个固定的GUID连接起来，
    // 使用SHA-1哈希算法生成一个Sec-WebSocket-Accept值，
    // 将其包含在握手响应头中返回给客户端
    std::string accept = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    accept = base64encode(sha1sum(accept));
    req->setWebSocket(true);

    HttpResponse::ptr res = std::make_shared<HttpResponse>(req->getVersion(), req->isClose());
    // 该状态码表示服务器已理解客户端发来的请求，并将切换到另一个协议来完成这个请求
    // 常见的使用场景是在HTTP协议升级到WebSocket协议时，客户端向服务器发送Upgrade请求
    // 服务器回复101 Switching Protocols状态码，表示已经成功切换到WebSocket协议
    res->setStatus(HttpStatus::SWITCHING_PROTOCOLS);
    res->setReason("Web Socket Protocol Handshake");
    res->setHeader("Upgrade", "websocket");
    res->setHeader("Connection", "Upgrade");
    res->setHeader("Sec-WebSocket-Accept", accept);
    res->setWebSocket(true);

    sendHttpResponse(res);

    SYLAR_LOG_DEBUG(g_logger) << req->toString();
    SYLAR_LOG_DEBUG(g_logger) << res->toString();
    return req;
}

WSFrameMessage::ptr WSSession::recvMessage() {
    // false 表示服务器端从客户端接收
    return WSRecvMessage(this, false);
}

int32_t WSSession::sendMessage(WSFrameMessage::ptr msg, bool fin) {
    // false 表示服务器端向客户端发送
    return WSSendMessage(this, false, msg, fin);
}

int32_t WSSession::sendMessage(const std::string& data,  uint32_t opcode, bool fin) {
    // false 表示服务器端向客户端发送
    return WSSendMessage(this, false, std::make_shared<WSFrameMessage>(opcode, data), fin);
}

int32_t WSSession::ping() {
    return WSPing(this);
}

int32_t WSSession::pong() {
    return WSPong(this);
}

// 服务器发送给客户端的数据包中MASK位为0，这说明服务器发送的数据帧未经过掩码处理，而客户端的数据被加密处理
// 如果服务器收到客户端发送的未经掩码处理的数据包，则会自动断开连接；
// 如果客户端收到服务端发送的经过掩码处理的数据包，也会自动断开连接；

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

WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool isClient) {
    int opcode = 0;
    std::string data;
    int curr_len = 0;
    do {
        WSFrameHead head;
        if(stream->readFixSize(&head, sizeof(head)) <= 0) {
            break;
        }

        // 如果收到的是ping包，则发送pong包
        if(head.opcode == WSFrameHead::PING) {
            SYLAR_LOG_INFO(g_logger) << "PING";
            if(WSPong(stream) <= 0) {
                break;
            }
        }
        else if(head.opcode == WSFrameHead::PONG) {
            SYLAR_LOG_INFO(g_logger) << "PONG";
        }
        else if(head.opcode == WSFrameHead::CONTINUE 
                || head.opcode == WSFrameHead::TEXT_FRAME
                || head.opcode == WSFrameHead::BIN_FRAME) {
            // 如果客户端收到服务端发送的经过掩码处理的数据包，也会自动断开连接
            if(isClient && head.mask) {
                SYLAR_LOG_INFO(g_logger) << "Client recv WSFrameHead mask != 0";
                break;
            }
            // 如果服务器收到客户端发送的未经掩码处理的数据包，则会自动断开连接
            if(!isClient && !head.mask) {
                SYLAR_LOG_INFO(g_logger) << "Server recv WSFrameHead mask != 1";
                break;
            }

            uint64_t length = 0;    // 数据长度
            if(head.payload == 126) {
                // 如果payload值是126，则后面2个字节形成的16位无符号整型数的值是payload的真实长度
                uint16_t len = 0;
                if(stream->readFixSize(&len, sizeof(len)) <= 0) {
                    break;
                }
                length = byteSwapToLittleEndian(len);
            } 
            else if(head.payload == 127) {
                // 如果payload值是127，则后面8个字节形成的64位无符号整型数的值是payload的真实长度
                uint64_t len = 0;
                if(stream->readFixSize(&len, sizeof(len)) <= 0) {
                    break;
                }
                length = byteSwapToLittleEndian(len);
            } 
            else {
                // 如果payload值为0-125，那么本身就是payload的真实长度
                length = head.payload;
            }

            // 超过WebSocket消息体最大长度
            if(curr_len + length >= g_websocket_message_max_size->getValue()) {
                SYLAR_LOG_WARN(g_logger) << "WSFrameMessage length > "
                    << g_websocket_message_max_size->getValue()
                    << " (" << (curr_len + length) << ")";
                break;
            }

            // 如果mask位为1，表示存在掩码，掩码长度为4字节
            char mask[4] = {0};
            if(head.mask) {
                if(stream->readFixSize(mask, sizeof(mask)) <= 0) {
                    break;
                }
            }

            // 读取数据
            data.resize(curr_len + length);
            if(stream->readFixSize(&data[curr_len], length) <= 0) {
                break;
            }

            // 如果数据添加了掩码，则需要解码
            if(head.mask) {
                for(int i = 0; i < (int)length; i++) {
                    data[curr_len + i] ^= mask[i % 4];   // 异或
                }
            }
            curr_len += length;

            // 当一个数据包被分成多个帧进行传输时，除了第一个帧外，其他帧都会设置opcode为0
            // 第一个帧的opcode表示数据的类型，可以是文本帧(opcode为1)或二进制帧(opcode为2)
            if(!opcode && head.opcode != WSFrameHead::CONTINUE) {
                opcode = head.opcode;    // 记录第一个帧的数据类型
            }

            // 接收到数据末尾片段
            if(head.fin) {
                SYLAR_LOG_DEBUG(g_logger) << data;
                // move()将传入的data对象转换为右值引用，表示该对象可以被移动而不是复制
                // 可以在不进行对象复制的情况下将对象的所有权从一个位置转移到另一个位置
                // 使用move()后，原始对象的状态可能会变为未定义，因此在移动后应谨慎操作原始对象
                return std::make_shared<WSFrameMessage>(opcode, std::move(data));
            }
        }
        else {
            SYLAR_LOG_DEBUG(g_logger) << "invalid opcode=" << head.opcode;
            break;
        }
    } while(true);
    stream->close();
    return nullptr;
}

int32_t WSSendMessage(Stream* stream, bool isClient, WSFrameMessage::ptr msg, bool fin) {
    do {
        int32_t total_size = 0;
        WSFrameHead head;
        memset(&head, 0, sizeof(head));
        head.fin = fin;
        head.opcode = msg->getOpcode();
        head.mask = isClient;      // true:表示客户端给服务器发，需要掩码
        size_t size = msg->getData().size();  // payload
        if(size < 126) {
            // 如果数据长度为0~125，那么本身就是payload的真实长度
            head.payload = size;
        }
        else if(size < 65536) {
            // 如果数据长度为126~2个字节最大长度，那么payload值是126
            head.payload = 126;
        }
        else {
            // 如果数据长度为2个字节最大长度~8个字节最大长度，那么payload值是127
            head.payload = 127;
        }

        // 写入头部
        if(stream->writeFixSize(&head, sizeof(head)) <= 0) {
            break;
        }
        total_size += sizeof(head);

        // 写入数据长度
        if(head.payload == 126) {
            uint16_t len = size;
            len = byteSwapToLittleEndian(len);   // 保证函数参数输入为uint16_t
            // 写入2个字节表示的数据长度
            if(stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
            total_size += sizeof(len);
        }
        else if(head.payload == 127) {
            uint64_t len = byteSwapToLittleEndian(size);
            // 写入8个字节表示的数据长度
            if(stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
            total_size += sizeof(len);
        }

        std::string& data = msg->getData();
        if(head.mask) {
            // 客户端给服务器发，需要掩码
            char mask[4] = {0};
            uint32_t rand_value = rand();   // 掩码就是4字节的随机数
            memcpy(mask, &rand_value, sizeof(mask));
            for(int i = 0; i < (int)size; i++) {
                data[i] ^= mask[i % 4];     // 加掩码
            }
            // 如果需要掩码，则需要将4字节的掩码发出，以便于接收时解码
            if(stream->writeFixSize(mask, sizeof(mask)) <= 0) {
                break;
            }
            total_size += sizeof(mask);
        }

        // 写入数据
        if(stream->writeFixSize(data.c_str(), size) <= 0) {
            break;
        }
        total_size += size;
        
        return total_size;  // 返回发送总字数
    } while(false);
    stream->close();
    return -1;
}

// WebSocket的ping包数据格式：10001001 00000000 hex：0x89 0x00
// fin=1  rsv1_2_3=000  opcode=1001  mask=0  payload=0000000
int32_t WSPing(Stream* stream) {
    WSFrameHead head;
    memset(&head, 0, sizeof(head));
    head.fin = 1;
    head.opcode = WSFrameHead::PING;
    int32_t rt = stream->writeFixSize(&head,sizeof(head));
    if(rt <= 0) {
        stream->close();
    }
    return rt;
}

// WebSocket的pong包数据格式：10001010 00000000 hex：0x8A 0x00
// fin=1  rsv1_2_3=000  opcode=1010  mask=0  payload=0000000
int32_t WSPong(Stream* stream) {
    WSFrameHead head;
    memset(&head, 0, sizeof(head));
    head.fin = 1;
    head.opcode = WSFrameHead::PONG;
    int32_t rt = stream->writeFixSize(&head,sizeof(head));
    if(rt <= 0) {
        stream->close();
    }
    return rt;
}

}
}