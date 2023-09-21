#include "bytearray.h"
#include "log.h"
#include "myendian.h"
#include <string.h>
#include <math.h>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("syetem");

// 无参构造
ByteArray::Node::Node() 
    :date(nullptr)
    ,next(nullptr)
    ,size(0) {
}

// 构造指定大小的内存块
ByteArray::Node::Node(size_t baseSize) 
    :date(new char[baseSize])
    ,next(nullptr)
    ,size(baseSize) {
}

// 析构函数
ByteArray::Node::~Node() {
    free();
}

// 释放内存
void ByteArray::Node::free() {
    if(date) {
        delete[] date;
        date = nullptr;
    }
}


// 使用指定长度的内存块构造ByteArray
ByteArray::ByteArray(size_t baseSize) 
    :m_capacity(baseSize)
    ,m_size(0)
    ,m_baseSize(baseSize)
    ,m_position(0)
    ,m_endian(SYLAR_BIG_ENDIAN)
    ,m_root(new Node(baseSize))
    ,m_curr(m_root) {
}

// 析构函数
ByteArray::~ByteArray() {
    Node* temp = m_root;
    while(temp) {
        m_curr = temp->next;
        temp->free();
        delete temp;
        temp = m_curr;
    }
}


// 向ByteArray节点写入数据
void ByteArray::write(const void* buff, size_t size) {
    if(size == 0) {
        return;
    }
    addCapacity(size);  //扩容到可以容纳size

    size_t npos = m_position % m_baseSize;  // 在当前内存块中已使用的内存大小
    size_t ncap = m_curr->size - npos;      // 在当前内存块中剩余的内存大小
    size_t bpos = 0;                        // buff的索引
    while(size > 0) {
        if(size <= ncap) {
            memcpy(m_curr->date + npos, (const char*)buff + bpos, size);
            m_position += size;
            npos += size;
            ncap -= size;
            size = 0;
            // 下一个内存块存在并且当前内存块正好用完，则转到下一个内存块
            if(m_curr->next && ncap == 0 && m_curr->size == npos) {
                m_curr = m_curr->next;
            }
        }
        else {
            memcpy(m_curr->date + npos, (const char*)buff + bpos, ncap);
            m_curr = m_curr->next;
            m_position += ncap;
            size -= ncap;
            bpos += ncap;
            npos = 0;
            ncap = m_curr->size;
        }
    }
    if(m_position > m_size) {
        m_size = m_position;
    }
}

// 从当前ByteArray节点位置读出数据
void ByteArray::read(void* buff, size_t size) {
    // 所读数据大小size超出可读数据大小
    if(size > getReadSize()) {
        throw std::out_of_range("read out of range: size=" + std::to_string(size)
                                + " read_size=" + std::to_string(getReadSize()));
    }
    
    size_t npos = m_position % m_baseSize;  // 在当前内存块中已使用的内存大小
    size_t ncap = m_curr->size - npos;      // 在当前内存块中剩余的内存大小
    size_t bpos = 0;                        // buff的索引
    while(size > 0) {
        if(size <= ncap) {
            memcpy((char*)buff + bpos, m_curr->date + npos, size);
            m_position += size;
            npos += size;
            ncap -= size;
            size = 0;
            // 下一个内存块存在并且当前内存块正好用完，则转到下一个内存块
            if(m_curr->next && ncap == 0 && m_curr->size == npos) {
                m_curr = m_curr->next;
            }
        }
        else {
            memcpy((char*)buff + bpos, m_curr->date + npos, ncap);
            m_curr = m_curr->next;
            m_position += ncap;
            size -= ncap;
            bpos += ncap;
            npos = 0;
            ncap = m_curr->size;
        }
    }
}

// 从指定ByteArray节点位置读出数据
void ByteArray::read(void* buff, size_t size, size_t position) const {
    // 所读数据位置position超出可读数据位置
    if(position > m_size) {
        throw std::out_of_range("read out of range: position=" + std::to_string(position)
                                + " m_size=" + std::to_string(m_size));
    }

    // 所读数据大小size超出可读数据大小
    if(size > getReadSize()) {
        throw std::out_of_range("read out of range: size=" + std::to_string(size)
                                + " read_size=" + std::to_string(getReadSize()));
    }

    //找position位置对应的内存块
    Node* curr = m_root;
    size_t count = position / m_baseSize;
    while(count--) {
        curr = curr->next;
    }

    size_t npos = position % m_baseSize;    // 在当前内存块中已使用的内存大小
    size_t ncap = curr->size - npos;        // 在当前内存块中剩余的内存大小
    size_t bpos = 0;                        // buff的索引
    while(size > 0) {
        if(size <= ncap) {
            memcpy((char*)buff + bpos, curr->date + npos, size);
            size = 0;
        }
        else {
            memcpy((char*)buff + bpos, curr->date + npos, ncap);
            curr = curr->next;
            size -= ncap;
            bpos += ncap;
            npos = 0;
            ncap = curr->size;
        }
    }
}


// Fixed length 固定长度
// 写入固定长度类型的数据
void ByteArray::writeFint8(int8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeFuint8(uint8_t value) {
    write(&value, sizeof(value));
}

// ByteArray内部存储数据默认为大端字节序(因为用于网络传输)
// 本机为小端字节序，所以在存放/读取时要转换字节序
#define FUNC() { \
    if(m_endian != SYLAR_BYTE_ORDER) { \
        value = byteSwap(value); \
    } \
    write(&value, sizeof(value)); \
}

void ByteArray::writeFint16(int16_t value) {
    FUNC();
}

void ByteArray::writeFuint16(uint16_t value) {
    FUNC();
}

void ByteArray::writeFint32(int32_t value) {
    FUNC();
}

void ByteArray::writeFuint32(uint32_t value) {
    FUNC();
}

void ByteArray::writeFint64(int64_t value) {
    FUNC();
}

void ByteArray::writeFuint64(uint64_t value) {
    FUNC();
}

#undef FUNC

// 对int32_t类型数据进行编码，变为uint32_t类型，方便压缩高位无效0
static uint32_t EncodeZigzag32(const int32_t& value) {
    // 方法一
    if(value < 0) {
        return (uint32_t)(-value) * 2 - 1;
    }
    else {
        return (uint32_t)value * 2;
    }

    // 方法二
    // return (value << 1) ^ (value >> 31);
}

// 对int64_t类型数据进行编码，变为uint64_t类型，方便压缩高位无效0
static uint64_t EncodeZigzag64(const int64_t& value) {
    // 方法一
    if(value < 0) {
        return (uint64_t)(-value) * 2 - 1;
    }
    else {
        return (uint64_t)value * 2;
    }

    // 方法二
    // return (value << 1) ^ (value >> 31);
}

// 对编码后的数据进行解码，恢复原数据
static int32_t DecodeZigzag32(const uint32_t& value) {
    return (value >> 1) ^ -(value & 1);
}

// 对编码后的数据进行解码，恢复原数据
static int64_t DecodeZigzag64(const uint64_t& value) {
    return (value >> 1) ^ -(value & 1);
}

// Variable length 可变长度
void ByteArray::writeVint32(int32_t value) {
    writeVuint32(EncodeZigzag32(value));
}

void ByteArray::writeVuint32(uint32_t value) {
    uint8_t result[5];
    uint8_t i = 0;
    // value >= 0x80 说明当前数据无法用7位表示完，需要用一个字节最高位置1来表示后面还有数据
    while(value >= 0x80) {
        // uint32_t强转成uint8_t会导致高位丢失
        result[i++] = (uint8_t)value | 0x80;
        value >>= 7;
    }
    // 最后剩余的数据可以用7位表示完，最高位为0表示后面没有数据了
    result[i++] = (uint8_t)value;
    write(result, i);
}

void ByteArray::writeVint64(int64_t value) {
    writeVuint64(EncodeZigzag64(value));
}

void ByteArray::writeVuint64(uint64_t value) {
    uint8_t result[10];
    uint8_t i = 0;
    // value >= 0x80 说明当前数据无法用7位表示完，需要用一个字节最高位置1来表示后面还有数据
    while(value >= 0x80) {
        // uint32_t强转成uint8_t会导致高位丢失
        result[i++] = (uint8_t)value | 0x80;
        value >>= 7;
    }
    // 最后剩余的数据可以用7位表示完，最高位为0表示后面没有数据了
    result[i++] = (uint8_t)value;
    write(result, i);
}

// 写入float类型的数据(用uint32_t类型存储)
void ByteArray::writeFloat(float value) {
    uint32_t result;
    // memcpy函数用于将一段内存的数据按字节复制到另一个内存位置。
    // 它可以用于在不同类型之间进行二进制数据的拷贝，无论是原生数据类型还是自定义的结构体。
    // 将浮点数的二进制数据拷贝到一个无符号32位整型变量中
    memcpy(&result, &value, sizeof(value));
    writeFuint32(result);
}

// 写入double类型的数据(用uint64_t类型存储)
void ByteArray::writeDouble(double value) {
    uint64_t result;
    memcpy(&result, &value, sizeof(value));
    writeFuint32(result);
}

// 存放字符串
void ByteArray::writeStringF16(const std::string& str) {
    writeFuint16(str.size());        //先放入长度
    write(str.c_str(), str.size());  //再放入数据
}

void ByteArray::writeStringF32(const std::string& str) {
    writeFuint32(str.size());        //先放入长度
    write(str.c_str(), str.size());  //再放入数据
}

void ByteArray::writeStringF64(const std::string& str) {
    writeFuint64(str.size());        //先放入长度
    write(str.c_str(), str.size());  //再放入数据
}

void ByteArray::writeStringV64(const std::string& str) {
    writeVuint64(str.size());        //先放入长度
    write(str.c_str(), str.size());  //再放入数据
}

void ByteArray::writeStringWithoutLen(const std::string str) {
    write(str.c_str(), str.size());
}


// 读取固定长度类型的数据
int8_t ByteArray::readFint8() {
    int8_t result;
    read(&result, sizeof(result));
    return result;
}

uint8_t ByteArray::readFuint8() {
    uint8_t result;
    read(&result, sizeof(result));
    return result;
}

// ByteArray内部存储数据默认为大端字节序(因为用于网络传输)
// 本机为小端字节序，所以在存放/读取时要转换字节序
#define FUNC(type) { \
    type result; \
    read(&result, sizeof(result)); \
    if(m_endian != SYLAR_BYTE_ORDER) { \
        result = byteSwap(result); \
    } \
    return result; \
}

int16_t ByteArray::readFint16() {
    FUNC(int16_t);
}

uint16_t ByteArray::readFuint16() {
    FUNC(uint16_t);
}

int32_t ByteArray::readFint32() {
    FUNC(int32_t);
}

uint32_t ByteArray::readFuint32() {
    FUNC(uint32_t);
}

int64_t ByteArray::readFint64() {
    FUNC(int64_t);
}

uint64_t ByteArray::readFuint64() {
    FUNC(uint64_t);
}

#undef FUNC

// 读取可变长度类型的数据
int32_t ByteArray::readVint32() {
    return DecodeZigzag32(readVuint32());
}

uint32_t ByteArray::readVuint32() {
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        // 每次读取8位数据，如果>= 0x80，说明最高位为1，后面还有数据
        uint8_t value = readFuint8();
        if(value >= 0x80) {
            // 最高位1为标志位，不属于原本数据
            result |= ((uint32_t)value & 0x7F) << i;
        }
        else {
            result |= (uint32_t)value << i;
            break;
        }
    }
    return result;
}

int64_t ByteArray::readVint64() {
    return DecodeZigzag64(readVuint64());
}

uint64_t ByteArray::readVuint64() {
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        // 每次读取8位数据，如果>= 0x80，说明最高位为1，后面还有数据
        uint8_t value = readFuint8();
        if(value >= 0x80) {
            // 最高位1为标志位，不属于原本数据
            result |= ((uint64_t)value & 0x7F) << i;
        }
        else {
            result |= (uint64_t)value << i;
            break;
        }
    }
    return result;
}

// 读取float类型的数据(用uint32_t类型存储)
float ByteArray::readFloat() {
    float result;
    uint32_t temp = readFuint32();
    memcpy(&result, &temp, sizeof(temp));
    return result;
}

// 读取double类型的数据(用uint64_t类型存储)
double ByteArray::readDouble() { 
    double result;
    uint64_t temp = readFuint64();
    memcpy(&result, &temp, sizeof(temp));
    return result;
}

// 读取字符串
std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();    //先读长度
    std::string result;
    result.resize(len);
    read(&result[0], len);    //再读数据
    return result;
}

std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();    //先读长度
    std::string result;
    result.resize(len);
    read(&result[0], len);    //再读数据
    return result;
}

std::string ByteArray::readStringF64() {
    uint64_t len = readFuint64();    //先读长度
    std::string result;
    result.resize(len);
    read(&result[0], len);    //再读数据
    return result;
}

std::string ByteArray::readStringV64() {
    uint64_t len = readVuint64();    //先读长度
    std::string result;
    result.resize(len);
    read(&result[0], len);    //再读数据
    return result;
}


// 从当前位置开始，将指定长度size的数据写入name文件
bool ByteArray::writeToFile(const std::string& name, size_t size) const {
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs) {
        SYLAR_LOG_ERROR(g_logger) << "writeToFile(" << name << ", " << size 
                                << ") fail, open " << name << " file error";
        return false;
    }
    
    // 所读数据大小size超出可读数据大小
    size_t read_size = getReadSize();
    if(size > read_size) {
        size = read_size;
    }

    Node* curr = m_curr;    // const成员函数，不可修改成员变量
    size_t npos = m_position % m_baseSize; // 在当前内存块中已使用的内存大小
    size_t ncap = curr->size - npos;       // 在当前内存块中剩余的内存大小
    while(size > 0) {
        if(size <= ncap) {
            ofs.write(curr->date + npos, size);  //向文件内写数据
            size = 0;
        }
        else {
            ofs.write(curr->date + npos, ncap);
            curr = curr->next;
            size -= ncap;
            npos = 0;
            ncap = curr->size;
        }
    }
    return true;
}

// 从文件中读取指定长度size的数据，写入到当前节点
bool ByteArray::readFromFile(const std::string& name, size_t size) {
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if(!ifs) {
        SYLAR_LOG_ERROR(g_logger) << "readFromFile(" << name << ", " << size 
                                << ") fail, open " << name << " file error";
        return false;
    }

    char buff[m_baseSize];
    // char* buff = new char[m_baseSize];
    while(!ifs.eof() && size > 0) {
        size_t len = size > m_baseSize ? m_baseSize : size;
        // 连续使用 ifs.read() 读取文件时，文件光标的位置会随着每次读取而移动
        ifs.read(buff, len);  //把文件内指定数量的数据读到buff中，指定数量超出则会读到末尾
        write(buff, ifs.gcount());   //gcount()表示最后一次读取操作成功读取的字符数
        size -= ifs.gcount();
    }
    return true;
}


// 只留根结点，释放其它结点
void ByteArray::clear() {
    Node* temp = m_root->next;  //从根节点的下一个节点开始释放
    while(temp) {
        m_curr = temp->next;
        temp->free();
        delete temp;
        temp = m_curr;
    }
    // 初始化成员变量
    m_capacity = m_baseSize;
    m_position = m_size = 0;
    m_root->next = nullptr;
    m_curr = m_root;
}

// 扩容ByteArray
void ByteArray::addCapacity(size_t size) {
    size_t usableCap = getUsableCap();    //剩余可用内存大小
    if(size == 0 || usableCap >= size) {
        return;
    }
    size -= usableCap;      //存放剩余数据还需要的内存大小
    size_t count = ceil((double)size / m_baseSize);  //存放剩余数据需要的内存块个数
    Node* curr = m_root;
    while(curr->next) {
        curr = curr->next;
    }

    Node* first = nullptr;
    while(count--) {
        curr->next = new Node(m_baseSize);
        if(!first) {
            first = curr->next;
        }
        curr = curr->next;
        m_capacity += m_baseSize;
    }

    if(usableCap == 0) {
        m_curr = first;
    }
}

// 设置ByteArray当前位置
void ByteArray::setPosition(size_t position) {
    if(position < 0 || position > m_capacity) {
        throw std::out_of_range("setPosition out of range: position=" + std::to_string(position) 
                                + "capacity=" + std::to_string(m_capacity));
    }

    m_position = position;
    // 因为 getReadSize() 是 m_size - m_position; 
    if(m_position > m_size) {
        // 如果不更新 m_size，则 getReadSize() 为负
        m_size = m_position;
    }

    m_curr = m_root;
    // 采用该方式是因为可能每个节点内存块大小不一样
    while(position > m_curr->size) {
        position -= m_curr->size;
        m_curr = m_curr->next;
    }
    if(position == m_curr->size) {
        m_curr = m_curr->next;
    }
}

// 把内存块中可读的数据全部以字符串形式输出
std::string ByteArray::toString() const {
    std::string result;
    size_t len = getReadSize();
    if(len == 0) {
        return result;
    }
    result.resize(len);
    read(&result[0], len, m_position);  //不改变成员变量的读取
    return result;
}

// 把内存块中可读的数据全部以16进制的字符串形式输出(格式:FF FF FF)
std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;
    for(size_t i = 0; i < str.size(); i++) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        // 使用 std::setw() 函数指定输出的字段宽度为 2
        // 如果字段宽度小于输出内容的宽度，则输出将被截断；如果字段宽度大于输出内容的宽度，则输出将使用填充字符填充
        // 使用 std::setfill() 函数指定填充字符为零（‘0’）
        ss << std::setw(2) << std::setfill('0') 
            << std::hex << (uint32_t)str[i] << " ";  // 强转后为str[i]的ASCII值
    }
    return ss.str();
}


// 获取可读取的缓存，保存到iovec数组
size_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, size_t size) const {
    // 所读数据大小size超出可读数据大小
    size_t read_size = getReadSize();
    if(size > read_size) {
        size = read_size;
    }
    size_t len = size;  //保存所读数据大小

    Node* curr = m_curr;
    size_t npos = m_position % m_baseSize; // 在当前内存块中已使用的内存大小
    size_t ncap = curr->size - npos;       // 在当前内存块中剩余的内存大小
    struct iovec iov;
    while(size > 0) {
        if(size <= ncap) {
            iov.iov_base = curr->date + npos;
            iov.iov_len = size;
            size = 0;
        }
        else {
            iov.iov_base = curr->date + npos;
            iov.iov_len = ncap;
            curr = curr->next;
            size -= ncap;
            npos = 0;
            ncap = curr->size;
        }
        buffers.push_back(iov);
    }
    return len;
}

// 从position位置开始，获取可读取的缓存，保存到iovec数组
size_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, size_t size, size_t position) const {
    // 所读数据位置position超出可读数据位置
    if(position > m_size) {
        return 0;
    }

    // 所读数据大小size超出可读数据大小
    size_t read_size = m_size - position;
    if(size > read_size) {
        size = read_size;
    }
    size_t len = size;  //保存所读数据大小

    //找position位置对应的内存块
    Node* curr = m_root;
    size_t count = position / m_baseSize;
    while(count--) {
        curr = curr->next;
    }

    size_t npos = position % m_baseSize; // 在当前内存块中已使用的内存大小
    size_t ncap = curr->size - npos;     // 在当前内存块中剩余的内存大小
    struct iovec iov;
    while(size > 0) {
        if(size <= ncap) {
            iov.iov_base = curr->date + npos;
            iov.iov_len = size;
            size = 0;
        }
        else {
            iov.iov_base = curr->date + npos;
            iov.iov_len = ncap;
            curr = curr->next;
            size -= ncap;
            npos = 0;
            ncap = curr->size;
        }
        buffers.push_back(iov);
    }
    return len;
}

// 获取可写入的缓存，保存到iovec数组
size_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, size_t size) {
    if(size == 0) {
        return 0;
    }
    addCapacity(size);
    size_t len = size;

    Node* curr = m_curr;
    size_t npos = m_position % m_baseSize; // 在当前内存块中已使用的内存大小
    size_t ncap = curr->size - npos;       // 在当前内存块中剩余的内存大小
    struct iovec iov;
    while(size > 0) {
        if(size <= ncap) {
            iov.iov_base = curr->date + npos;
            iov.iov_len = size;
            size = 0;
        }
        else {
            iov.iov_base = curr->date + npos;
            iov.iov_len = ncap;
            curr = curr->next;
            size -= ncap;
            npos = 0;
            ncap = curr->size;
        }
        buffers.push_back(iov);
    }
    return len;
}

}