#ifndef __SYLAR_STREAM_H__
#define __SYLAR_STREAM_H__

#include <memory>
#include "bytearray.h"

namespace sylar {


/**
 * @brief 流结构基类，后续实现具体流结构则直接继承，如Socket流结构
 * @details readFixSize/writeFixSize基于read/write实现，所以继承后只需重新实现read/write
 * @note 为什么需要readFixSize/writeFixSize? 解释如下:
 * ssize_t recv(int sockfd, void *buf, size_t len, int flags);
 * 当协议把数据接收完毕，recv函数就把sockfd的接收缓冲区中的数据copy到buf中，
 * 但可能协议接收到的数据大于buf的长度，则需要调用几次recv函数才能把sockfd接收缓冲中的数据copy完，
 * recv函数仅仅是copy数据，真正的接收数据是协议来完成的，recv函数返回其实际copy的字节数。
 * 所以需要readFixSize/writeFixSize，指定固定长度，保证接收/发送的数据长度一定是所指定的长度
*/
class Stream {
public:
    typedef std::shared_ptr<Stream> ptr;

    // 纯虚析构函数，需要类外实现
    // virtual ~Stream() = 0;

    /**
     * @brief 虚析构函数
     */
    virtual ~Stream() {}

    /**
     * @brief 读数据
     * @param[out] buffer 存放接收到的数据
     * @param[in] length 要接收数据的大小
     * @return
     *      @retval >0 返回实际接收到数据的大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual ssize_t read(void* buffer, size_t length) = 0;
    virtual ssize_t read(ByteArray::ptr buffer, size_t length) = 0;

    /**
     * @brief 读固定长度的数据
     * @param[out] buffer 存放接收到的数据
     * @param[in] length 要接收数据的大小
     * @return
     *      @retval >0 返回实际接收到数据的大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual ssize_t readFixSize(void* buffer, size_t length);
    virtual ssize_t readFixSize(ByteArray::ptr buffer, size_t length);

    /**
     * @brief 写数据
     * @param[out] buffer 存放要发送的数据
     * @param[in] length 要发送数据的大小
     * @return
     *      @retval >0 返回实际发送数据的大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual ssize_t write(const void* buffer, size_t length) = 0;
    virtual ssize_t write(ByteArray::ptr buffer, size_t length) = 0;

    /**
     * @brief 写固定长度的数据
     * @param[out] buffer 存放要发送的数据
     * @param[in] length 要发送数据的大小
     * @return
     *      @retval >0 返回实际发送数据的大小
     *      @retval =0 被关闭
     *      @retval <0 出现流错误
     */
    virtual ssize_t writeFixSize(const void* buffer, size_t length);
    virtual ssize_t writeFixSize(ByteArray::ptr buffer, size_t length);

    virtual bool close() = 0;
};

}

#endif