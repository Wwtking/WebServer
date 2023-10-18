#ifndef __SYLAR_ORM_COLUMN_H__
#define __SYLAR_ORM_COLUMN_H__

#include <memory>
#include <string>
#include "tinyxml2.h"

namespace sylar {
namespace orm {

class Table;
/**
 * @brief 列封装
*/
class Column {
friend class Table;
public:
    typedef std::shared_ptr<Column> ptr;

    // 列数据类型
    enum Type {
        TYPE_NULL = 0,
        TYPE_INT8,
        TYPE_UINT8,
        TYPE_INT16,
        TYPE_UINT16,
        TYPE_INT32,
        TYPE_UINT32,
        TYPE_FLOAT,
        TYPE_DOUBLE,
        TYPE_INT64,
        TYPE_UINT64,
        TYPE_STRING,
        TYPE_TEXT,
        TYPE_BLOB,      // 二进制数据
        TYPE_TIMESTAMP  // 时间格式字符串
    };

    /**
     * @brief 初始化
     * @details 解析xml元素节点node(列)，将数据存在类中
     * @example 解析 <column name="id" type="int64" desc="唯一主键" auto_increment="true"/>
    */
    bool init(const tinyxml2::XMLElement& node);

    /**
     * @brief 获取字符串形式的默认值
    */
    std::string getDefaultValueString();

    /**
     * @brief 获取SQLite3字符串形式的默认值
    */
    std::string getSQLite3Default();

    /**
     * @brief 类成员变量定义 的字符串实现
     * @example int64_t m_id;
    */
    std::string getMemberDefine() const;

    /**
     * @brief get型类成员函数定义 的字符串实现
     * @example const int64_t& getId() { return m_id; }
    */
    std::string getGetFunDefine() const;

    /**
     * @brief set型类成员函数定义 的字符串实现
     * @example void setId(const int64_t& v);
    */
    std::string getSetFunDefine() const;

    /**
     * @brief set型类成员函数实现 的字符串实现
     * @example void UserInfo::setId(const int64_t& v) {
     *              m_id = v;
     *          }
    */
    std::string getSetFunImpl(const std::string& class_name, int idx) const;

    /**
     * @brief 解析字符串形式列数据类型，返回Type类型
    */
    static Type ParseType(const std::string& v);

    /**
     * @brief Type列数据类型转为字符串形式列数据类型
    */
    static std::string TypeToString(Type type);

    /**
     * @brief 获取字符串形式列数据类型
    */
    std::string getDTypeString() { return TypeToString(m_dtype); }

    /**
     * @brief 获取字符串形式的SQLite3列数据类型
    */
    std::string getSQLite3TypeString();

    /**
     * @brief 获取字符串形式的MySQL列数据类型
    */
    std::string getMySQLTypeString();

    /**
     * @brief 获取字符串 "bind" + 数据类型
     * @details 可得到具体绑定某种数据类型的函数名
     * @example int bindInt8(int idx, const int8_t& value);
     *          int bindUint8(int idx, const uint8_t& value);
    */
    std::string getBindString();

    /**
     * @brief 获取字符串 "get" + 数据类型
     * @details 可得到获取某列查询结果的函数名
     * @example int8_t getInt8(int idx);
     *          uint8_t getUint8(int idx);
    */
    std::string getGetString();

    /**
     * @brief 获取列名
    */
    const std::string& getName() const { return m_name;}

    /**
     * @brief 获取列数据类型
    */
    const std::string& getType() const { return m_type;}

    /**
     * @brief 获取列描述
    */
    const std::string& getDesc() const { return m_desc;}

    /**
     * @brief 获取列数据默认值
    */
    const std::string& getDefault() const { return m_default;}

    /**
     * @brief 列值是否自增
    */
    bool isAutoIncrement() const { return m_autoIncrement;}

    /**
     * @brief 获取列数据类型
    */
    Type getDType() const { return m_dtype;}

    int getIndex() const { return m_index; }

    /**
     * @brief 获取列数据自动更新值
    */
    const std::string& getUpdate() const { return m_update;}

private:
    std::string m_name;     // 列名称
    std::string m_type;     // 列数据类型
    std::string m_default;  // 列数据默认值

    // 当记录被更新时，该列的值将自动更新为当前时间戳
    std::string m_update;   // 列数据自动更新值
    std::string m_desc;     // 列描述
    int m_index;            // 列索引(表示第几列)

    bool m_autoIncrement;   // 是否自增
    Type m_dtype;           // 列数据类型
    int m_length;           // 列数据为字符串时，表示其长度

    // <column name="id" type="int64" desc="唯一主键" auto_increment="true"/>
    // <column name="name" type="string" desc="名称" length="30"/>
    // <column name="email" type="string" default="xx@xx.com"/>
    // <column name="phone" type="string"/>
    // <column name="status" type="int32" default="10"/>
    // <column name="create_time" type="timestamp"/>
    // <column name="update_time" type="timestamp" default="current_timestamp" update="current_timestamp"/>

};

}
}

#endif
