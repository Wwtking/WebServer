#include "tcp_server.h"
#include "log.h"
#include "bytearray.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

// 选择EchoServer输出类型: 1文本类型 2二进制类型
static int EchoServerType = 1;

/**
 * @brief echo服务器
 * @details 将收到的客户端数据原封不动地打印出来
*/
class EchoServer : public sylar::TcpServer {
public:
    /**
     * @brief 构造函数
     * @param[in] type 选择EchoServer输出类型
    */
    EchoServer(int type = 1);

    /**
     * @brief 返回echo服务器输出类型
     */
    int getType() const { return m_type; }

    /**
     * @brief 设置echo服务器输出类型
     */
    void setType(int type) { m_type = type; }

private:
    /**
     * @brief 处理新连接的Socket类
     * @details 每accept到一个socket，就会执行一次，触发回调
     * @param[in] client 新连接的Socket类
     */
    void handleClient(sylar::Socket::ptr client) override;

private:
    int m_type;    // 选择echo服务器输出类型
};

EchoServer::EchoServer(int type) 
    :m_type(type) {
}

void EchoServer::handleClient(sylar::Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient: " << client->toString();

    // 创建数据存储内存
    sylar::ByteArray::ptr ba = std::make_shared<sylar::ByteArray>();
    while(!isStop()) {
        ba->clear();    // 每接收完一次数据就清空
        std::vector<iovec> buffers;
        ba->getWriteBuffers(buffers, 1024);    // 获取可写入的内存地址

        size_t rt = client->recv(&buffers[0], buffers.size());
        // SYLAR_LOG_INFO(g_logger) << rt << "  " << (char*)buffers[0].iov_base;
        if(rt == 0) {    // 连接关闭
            SYLAR_LOG_INFO(g_logger) << "client Socket close: " << client->toString();
            return;
        }
        else if(rt < 0) {    // 连接出错
            SYLAR_LOG_INFO(g_logger) << "client Socket error rt=" << rt;
            return;
        }

        ba->setPosition(ba->getPosition() + rt);  // 内存光标移到新存储数据最后一位的下一位，新起点
        ba->setPosition(0);  // 为了toString()

        if(EchoServerType == 1) {
            // 文本输出
            std::cout << ba->toString();// << std::endl;
        }
        else {
            // 二进制输出
            std::cout << ba->toHexString();// << std::endl;
        }

        /**
         * @brief 刷新缓存区
         * @details 往硬件(如显示器)输出东西是很耗时的，所以平时写入到cout里的东西，程序会将其暂存起来，不会马上显示
         *          只有当满足下列条件之一才会将缓存的数据显示到控制台，刷新缓冲区条件:
         *              1.缓冲区被写满
         *              2.程序执行结束或者文件对象被关闭
         *              3.行缓冲遇到换行(std::endl)
         *              4.程序中调用flush()函数
        */
        std::cout.flush();
    }
}

void echo_server() {
    // 创建echo服务器
    EchoServer::ptr server = std::make_shared<EchoServer>();

    // 用Address类封装待绑定的地址
    sylar::Address::ptr addr = sylar::Address::LookupAny("0.0.0.0:8020");

    // 绑定服务器地址(bind和listen)
    while(!server->bind(addr)) {
        sleep(2);
    }

    // 启动服务器(accept和handleClient)
    server->start();
}

// 执行echo_server时，默认argc==1  argv[0]==./echo_server
// [./echo_server -t]  argc==2  argv[1]==-t
int main(int argc, char** argv) {
    if(argc < 2) {
        SYLAR_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }

    // [./echo_server -b] 用二进制输出EchoServer
    if(strcmp(argv[1], "-b") == 0) {
        EchoServerType = 2;
    }

    sylar::IOManager iom(2);
    iom.scheduler(&echo_server);

    return 0;
}