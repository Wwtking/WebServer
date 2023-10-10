#include "ws_connection.h"
#include "iomanager.h"
#include "hash_util.h"

void run() {
    // 开始握手建立连接
    auto it = sylar::http::WSConnection::StartShake("http://192.168.88.130:8020/sylar", 1000);
    sylar::http::WSConnection::ptr conn = it.second;
    if(!conn) {
        std::cout << it.first->toString() << std::endl;
        return;
    }

    // 数据传输，双向通信
    while(true) {
        // sendMessage(const std::string& data,  uint32_t opcode, bool fin)
        // 先 fin=false 发送，表示该消息不是最后一个片段，后续还有消息要发送
        // 最后一条消息用 fin=true 发送，表示该消息是最后一个片段，发完即结束发送
        // 这样就可以实现分片传输
        for(int i = 0; i < 10; i++) {
            conn->sendMessage(sylar::random_string(60), sylar::http::WSFrameHead::TEXT_FRAME, false);
        }
        conn->sendMessage(sylar::random_string(60), sylar::http::WSFrameHead::TEXT_FRAME, true);

        auto msg = conn->recvMessage();
        if(!msg) {
            break;
        }
        std::cout << "opcode=" << msg->getOpcode()
                  << " data=" << msg->getData() << std::endl;

        sleep(10);
    }
}

int main(int argc, char** argv) {
    srand(time(0));
    sylar::IOManager iom(2);
    iom.scheduler(&run);
}