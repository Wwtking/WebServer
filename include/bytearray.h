#ifndef __SYLAR_BYTEARRAY_H__
#define __SYLAR_BYTEARRAY_H__

#include <memory>
#include <stdint.h>
#include <vector>
#include <sys/uio.h>

namespace sylar {

/**
 * @brief 二进制数组，提供基础类型的序列化、反序列化功能
 * @details 内部用链表存放数据，每次写入数据时，分配一个块加入到链表末尾
 *          不用数组的原因是因为当数据较大时，数组存满后要去重新分配一块更大的内存，再把数据移过去，时间消耗大
 */
class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    /**
     * @brief ByteArray的存储节点
     */
    struct Node {
        // 无参构造
        Node();

        /**
         * @brief 构造指定大小的内存块
         * @param[in] baseSize 内存块字节数
         */
        Node(size_t baseSize);

        // 析构函数
        ~Node();

        // 释放内存
        void free();

        char* date;    // 存放节点数据
        Node* next;    // 指向下一个节点
        size_t size;   // 节点大小
    };

    /**
     * @brief 使用指定长度的内存块构造ByteArray
     * @param[in] baseSize 内存块大小
     */
    ByteArray(size_t baseSize = 4096);

    // 析构函数
    ~ByteArray();

    /**
     * @brief 向ByteArray节点写入数据
     * @param[in] buff 待写入的数据缓存
     * @param[in] size 待写入的数据大小
    */
    void write(const void* buff, size_t size);

    /**
     * @brief 从当前ByteArray节点位置读出数据
     * @param[in] buff 读出的数据存放地址
     * @param[in] size 读出的数据大小
    */
    void read(void* buff, size_t size);

    /**
     * @brief 从指定ByteArray节点位置读出数据
     * @param[in] buff 读出的数据存放地址
     * @param[in] size 读出的数据大小
     * @param[in] position 指定位置
    */
    void read(void* buff, size_t size, size_t position) const;

    /**
     * @name Fixed length 固定长度
     * @brief 写入固定长度类型的数据
     * @details 数据value原本多长就写入多长，不会去压缩数据的长度
    */
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);
    
    /**
     * @name Variable length 可变长度
     * @brief 写入可变长度类型的数据
     * @details 压缩数据的高位0(无效位),每7位一组,第8位为标志位(1表示后面还有数据,0表示无数据)
     *          4字节类型压缩后实际占用内存(1 ~ 5)
     *          8字节类型压缩后实际占用内存(1 ~ 10)
    */
    void writeVint32(int32_t value);
    void writeVuint32(uint32_t value);
    void writeVint64(int64_t value);
    void writeVuint64(uint64_t value);

    // 写入float类型的数据(用uint32_t类型存储)
    void writeFloat(float value);

    // 写入double类型的数据(用uint64_t类型存储)
    void writeDouble(double value);

    /**
     * @brief 写入string类型的数据，包含字符串长度
     * @details 先写入数据长度，再写入数据
     *          分别用uint16_t、uint32_t、uint64_t作为长度类型
    */
    void writeStringF16(const std::string& str);
    void writeStringF32(const std::string& str);
    void writeStringF64(const std::string& str);
    void writeStringV64(const std::string& str);
    // 写入string类型的数据，不包含字符串长度，直接写入字符串
    void writeStringWithoutLen(const std::string str);

    /**
     * @name Fixed length 固定长度
     * @brief 从当前节点读出固定长度类型的数据
    */
    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();
    
    /**
     * @name Variable length 可变长度
     * @brief 从当前节点读出可变长度类型的数据
     * @details 因为读出的数据是压缩过的，要根据每个字节的最高位进行拼接解压
    */
    int32_t readVint32();
    uint32_t readVuint32();
    int64_t readVint64();
    uint64_t readVuint64();

    // 读取float类型的数据(用uint32_t类型存储)
    float readFloat();

    // 读取double类型的数据(用uint64_t类型存储)
    double readDouble();

    /**
     * @brief 读取string类型的数据，包含字符串长度
     * @details 先读取数据长度，再读取数据
     *          分别用uint16_t、uint32_t、uint64_t作为长度类型
    */
    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringV64();

    /**
     * @brief 从当前位置开始，将指定长度size的数据写入name文件
     * @param[in] name 文件路径
     * @param[in] size 所要读取数据的大小
     * @pre 当size超出最大可读长度时，就读完到所有可读数据为止
    */
    bool writeToFile(const std::string& name, size_t size) const;

    /**
     * @brief 从文件中读取指定长度size的数据，写入到当前节点
     * @param[in] name 文件路径
     * @param[in] size 所要读取数据的大小
     * @pre 当size超出文件数据最大长度时，就读完到文件所有数据为止
    */
    bool readFromFile(const std::string& name, size_t size);

    /**
     * @brief 清空ByteArray
     * @details 释放其它结点，只留根结点
    */
    void clear();

    // 扩容ByteArray，使其可以容纳size个数据(如果原本可以容纳，则不扩容)
    void addCapacity(size_t size);
    
    // 设置ByteArray当前位置
    void setPosition(size_t position);

    /**
     * @brief 设置ByteArray节点内的数据为大端序还是小端序
     * @param[in] endian: SYLAR_LITTLE_ENDIAN(1) 小端序
     *                    SYLAR_BIG_ENDIAN(2)    大端序
    */
    void setEndian(int8_t endian) { m_endian = endian; }

    // 获取ByteArray内存总容量
    size_t getCapacity() const { return m_capacity; }

    // 获取当前可用的内存容量
    size_t getUsableCap() const { return m_capacity - m_position; }

    // 返回当前数据的长度
    size_t getSize() const { return m_size; }

    // 返回单个内存块的大小
    size_t getBaseSize() const { return m_baseSize; }

    // 返回ByteArray当前位置
    size_t getPosition() const { return m_position; }

    // 返回可读取数据的大小
    size_t getReadSize() const { return m_size - m_position; }

    // 返回当前字节序
    int8_t getEndian() const { return m_endian; }

    // 把内存块中可读的数据全部以字符串形式输出
    std::string toString() const;

    // 把内存块中可读的数据全部以16进制的字符串形式输出
    std::string toHexString() const;

    /**
     * @brief 获取可读取的缓存，保存到iovec数组，不改变成员变量
     * @param[out] buffers 保存可读取数据的iovec数组
     * @param[in] size 读取数据的长度
     * @return 返回实际数据的长度
     */
    size_t getReadBuffers(std::vector<iovec>& buffers, size_t size) const;

    /**
     * @brief 从position位置开始，获取可读取的缓存，保存到iovec数组，不改变成员变量
     * @param[out] buffers 保存可读取数据的iovec数组
     * @param[in] size 读取数据的长度
     * @return 返回实际数据的长度
     */
    size_t getReadBuffers(std::vector<iovec>& buffers, size_t size, size_t position) const;

    /**
     * @brief 获取可写入的缓存，保存到iovec数组
     * @param[out] buffers 保存可写入内存的iovec数组
     * @param[in] size 写入的长度
     * @return 返回实际可写入内存的长度
     * @post 如果(m_position + size) > m_capacity 则需要扩容N个节点以容纳size长度
     */
    size_t getWriteBuffers(std::vector<iovec>& buffers, size_t size);

private:
    size_t m_capacity;    // 当前内存总容量
    size_t m_size;        // 当前数据的大小
    size_t m_baseSize;    // 单个内存块大小
    size_t m_position;    // 当前操作位置
    int8_t m_endian;      // 字节序,默认大端
    Node* m_root;         // 头节点
    Node* m_curr;         // 当前节点
};

}

#endif

