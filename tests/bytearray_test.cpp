#include "bytearray.h"
#include "log.h"
#include "macro.h"
#include <vector>
#include <ctime>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

/**
 * @brief 测试固定长度类型的写和读
*/
void test_fixedLen() {

/**
 * @param[in] type 存储的数据类型
 * @param[in] len 存储的数据个数
 * @param[in] writeType 写函数类型
 * @param[in] readType 读函数类型
 * @param[in] baseSize 内存块大小
 * @param[in] pos 开始读数据的位置
 * @pre 先写入数据，再根据pos的位置读数据
*/
#define FUNC(type, len, writeType, readType, baseSize, pos) { \
    std::vector<type> vec; \
    for(int i = 0; i < len; i++) { \
        vec.push_back(rand()); \
    } \
    sylar::ByteArray::ptr arr = std::make_shared<sylar::ByteArray>(baseSize); \
    for(auto& i : vec) { \
        arr->writeType(i); \
    } \
    arr->setPosition(pos * sizeof(type)); \
    for(int i = pos; i < len; i++) { \
        type date = arr->readType(); \
        SYLAR_ASSERT(date == vec[i]); \
    } \
    SYLAR_ASSERT(arr->getReadSize() == 0); \
    SYLAR_LOG_INFO(g_logger) << "ByteArray: " << #writeType << "/" << #readType \
                                << "  m_capacity=" << arr->getCapacity() \
                                << "  m_size=" << arr->getSize() \
                                << "  m_baseSize=" << arr->getBaseSize(); \
}

    FUNC(int8_t, 100, writeFint8, readFint8, 4, 10);
    FUNC(uint8_t, 100, writeFuint8, readFuint8, 4, 10);
    FUNC(int16_t, 100, writeFint16, readFint16, 4, 10);
    FUNC(uint16_t, 100, writeFuint16, readFuint16, 4, 10);
    FUNC(int32_t, 100, writeFint32, readFint32, 4, 10);
    FUNC(uint32_t, 100, writeFuint32, readFuint32, 4, 10);
    FUNC(int64_t, 100, writeFint64, readFint64, 4, 10);
    FUNC(uint64_t, 100, writeFuint64, readFuint64, 4, 10);

#undef FUNC

}

/**
 * @brief 测试可变长度类型的写和读
*/
void test_VariableLen() {

/**
 * @param[in] type 存储的数据类型
 * @param[in] len 存储的数据个数
 * @param[in] writeType 写函数类型
 * @param[in] readType 读函数类型
 * @param[in] baseSize 内存块大小
 * @pre 写入数据时压缩字节，读出数据时解压字节
*/
#define FUNC(type, len, writeType, readType, baseSize) { \
    std::vector<type> vec; \
    for(int i = 0; i < len; i++) { \
        vec.push_back(rand()); \
    } \
    sylar::ByteArray::ptr arr = std::make_shared<sylar::ByteArray>(baseSize); \
    for(auto& i : vec) { \
        arr->writeType(i); \
    } \
    arr->setPosition(0); \
    for(auto& i : vec) { \
        type date = arr->readType(); \
        SYLAR_ASSERT(date == i); \
    } \
    SYLAR_ASSERT(arr->getReadSize() == 0); \
    SYLAR_LOG_INFO(g_logger) << "ByteArray: " << #writeType << "/" << #readType \
                                << "  m_capacity=" << arr->getCapacity() \
                                << "  m_size=" << arr->getSize() \
                                << "  m_baseSize=" << arr->getBaseSize(); \
}

    FUNC(int32_t, 100, writeVint32, readVint32, 1);
    FUNC(uint32_t, 100, writeVuint32, readVuint32, 1);
    FUNC(int64_t, 100, writeVint64, readVint64, 1);
    FUNC(uint64_t, 100, writeVuint64, readVuint64, 1);

#undef FUNC

}

/**
 * @brief 测试字符串数据的写和读
*/
void test_string() {

/**
 * @param[in] type 存储的数据类型
 * @param[in] len 存储的数据个数
 * @param[in] writeType 写函数类型
 * @param[in] readType 读函数类型
 * @param[in] baseSize 内存块大小
 * @pre 先写入字符串长度，再写入字符串数据
*/
#define FUNC(type, len, writeType, readType, baseSize) { \
    std::vector<type> vec; \
    for(int i = 0; i < len; i++) { \
        vec.push_back(std::to_string(i)); \
    } \
    sylar::ByteArray::ptr arr = std::make_shared<sylar::ByteArray>(baseSize); \
    for(auto& i : vec) { \
        arr->writeType(i); \
    } \
    arr->setPosition(0); \
    for(auto& i : vec) { \
        type date = arr->readType(); \
        SYLAR_ASSERT(date == i); \
    } \
    SYLAR_ASSERT(arr->getReadSize() == 0); \
    SYLAR_LOG_INFO(g_logger) << "ByteArray: " << #writeType << "/" << #readType \
                                << "  m_capacity=" << arr->getCapacity() \
                                << "  m_size=" << arr->getSize() \
                                << "  m_baseSize=" << arr->getBaseSize(); \
}
    // 先写长度，再写数据
    FUNC(std::string, 100, writeStringF16, readStringF16, 1);
    FUNC(std::string, 100, writeStringF32, readStringF32, 1);
    FUNC(std::string, 100, writeStringF64, readStringF64, 1);
    FUNC(std::string, 100, writeStringV64, readStringV64, 1);

#undef FUNC

}

/**
 * @brief 测试将数据写到文件中和从文件中读出数据
*/
void test_file() {

/**
 * @param[in] type 存储的数据类型
 * @param[in] len 存储的数据个数
 * @param[in] writeType 写函数类型
 * @param[in] readType 读函数类型
 * @param[in] baseSize 内存块大小
 * @param[in] pos 写入文件的数据起点
 * @pre 4字节类型压缩后实际占用内存(1 ~ 5)，可能超1字节
 *      8字节类型压缩后实际占用内存(1 ~ 10)，可能超2字节
 *      所以 writeToFile() 和 readFromFile() 中的 size 为 len * (sizeof(type) + 2)
*/
#define FUNC(type, len, writeType, readType, baseSize, pos) { \
    std::vector<type> vec; \
    for(int i = 0; i < len; i++) { \
        vec.push_back(rand()); \
    } \
    sylar::ByteArray::ptr arr = std::make_shared<sylar::ByteArray>(baseSize); \
    for(auto& i : vec) { \
        arr->writeType(i); \
    } \
    arr->setPosition(0); \
    for(auto& i : vec) { \
        type date = arr->readType(); \
        SYLAR_ASSERT(date == i); \
    } \
    SYLAR_ASSERT(arr->getReadSize() == 0); \
    arr->setPosition(pos); \
    arr->writeToFile("/home/wwt/sylar/bin/" #readType ".dat", len * (sizeof(type) + 2)); \
    sylar::ByteArray::ptr arr2 = std::make_shared<sylar::ByteArray>(baseSize); \
    arr2->readFromFile("/home/wwt/sylar/bin/" #readType ".dat", len * (sizeof(type) + 2)); \
    arr2->setPosition(0); \
    SYLAR_ASSERT(arr->toString() == arr2->toString()); \
    SYLAR_ASSERT(arr->getPosition() == pos); \
    SYLAR_ASSERT(arr2->getPosition() == 0); \
    SYLAR_LOG_INFO(g_logger) << "ByteArray: " << #writeType << "/" << #readType \
                                << "  arr->m_size=" << arr->getSize() \
                                << "  arr2->m_size=" << arr2->getSize(); \
}
    // 固定长度
    FUNC(int8_t, 100, writeFint8, readFint8, 1, 10);
    FUNC(uint8_t, 100, writeFuint8, readFuint8, 1, 20);
    FUNC(int16_t, 100, writeFint16, readFint16, 1, 30);
    FUNC(uint16_t, 100, writeFuint16, readFuint16, 1, 40);
    FUNC(int32_t, 100, writeFint32, readFint32, 1, 50);
    FUNC(uint32_t, 100, writeFuint32, readFuint32, 1, 60);
    FUNC(int64_t, 100, writeFint64, readFint64, 1, 70);
    FUNC(uint64_t, 100, writeFuint64, readFuint64, 1, 80);

    // 可变长度
    FUNC(int32_t, 100, writeVint32, readVint32, 1, 90);
    FUNC(uint32_t, 100, writeVuint32, readVuint32, 1, 100);
    FUNC(int64_t, 100, writeVint64, readVint64, 1, 110);
    FUNC(uint64_t, 100, writeVuint64, readVuint64, 1, 120);

#undef FUNC

}


int main(int argc, char** argv) {
    srand((unsigned)time(0));
    //test_fixedLen();
    //test_VariableLen();
    //test_string();
    test_file();

    return 0;
}