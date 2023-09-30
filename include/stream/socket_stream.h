#ifndef __SYLAR_SOCKET_STREAM_H__
#define __SYLAR_SOCKET_STREAM_H__

#include "stream.h"
#include "socket.h"

namespace sylar {

/**
 * @brief Socket流
*/
class SocketStream : public Stream {
public:
    /**
     * @brief 构造函数
     * @param[in] sock Socket类
     * @param[in] owner 是否完全控制
     *                  如果是true，那么全权管理，析构时帮忙释放Socket
     *                  如果是false，那么仅仅完成相应的操作，不帮忙析构
     */
    SocketStream(Socket::ptr sock, bool owner = false);

    /**
     * @brief 析构函数
     * @details 如果m_owner==true，则close
     */
    ~SocketStream();

    /**
     * @brief 读数据
     * @param[out] buff 存放接收到的数据
     * @param[in] length 要接收数据的大小
     * @return
     *      @retval >0 返回实际接收到数据的大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    ssize_t read(void* buff, size_t length) override;
    ssize_t read(ByteArray::ptr buff, size_t length) override;

    /**
     * @brief 写数据
     * @param[out] buff 存放要发送的数据
     * @param[in] length 要发送数据的大小
     * @return
     *      @retval >0 返回实际发送数据的大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    ssize_t write(const void* buff, size_t length) override;
    ssize_t write(ByteArray::ptr buff, size_t length) override;

    /**
     * @brief 关闭Socket
    */
    virtual bool close() override;

    /**
     * @brief 获取Socket
    */
    Socket::ptr getSocket() const { return m_socket; }

    /**
     * @brief 获取SocketStream是否完全控制
    */
    bool isOwner() const { return m_owner; }

    /**
     * @brief 设置SocketStream是否完全控制
    */
    void setOwner(bool v) { m_owner = v; }

protected:
    /**
     * @brief 判断是否已成功连接
     * @details 放入private的原因是: 对外提供接口破坏类的封装性，该类只实现流功能
    */
    bool isConnected() const;

protected:
    Socket::ptr m_socket;   // Socket类
    bool m_owner;           // 是否主控
};

}

#endif