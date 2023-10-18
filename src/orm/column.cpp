#include "column.h"
#include "log.h"
#include "orm_util.h"

namespace sylar {
namespace orm {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("orm");

// 解析字符串形式列数据类型
Column::Type Column::ParseType(const std::string& v) {
#define XX(a, b, c) \
    if(#b == v) { \
        return a; \
    } else if(#c == v) { \
        return a; \
    }

    XX(TYPE_INT8, int8_t, int8);
    XX(TYPE_UINT8, uint8_t, uint8);
    XX(TYPE_INT16, int16_t, int16);
    XX(TYPE_UINT16, uint16_t, uint16);
    XX(TYPE_INT32, int32_t, int32);
    XX(TYPE_UINT32, uint32_t, uint32);
    XX(TYPE_FLOAT, float, float);
    XX(TYPE_INT64, int64_t, int64);
    XX(TYPE_UINT64, uint64_t, uint64);
    XX(TYPE_DOUBLE, double, double);
    XX(TYPE_STRING, string, std::string);
    XX(TYPE_TEXT, text, std::string);
    XX(TYPE_BLOB, blob, blob);
    XX(TYPE_TIMESTAMP, timestamp, datetime);

#undef XX
    return TYPE_NULL;
}

// Type列数据类型转为字符串形式列数据类型
std::string Column::TypeToString(Type type) {
#define XX(a, b) \
    if(a == type) { \
        return #b; \
    }

    XX(TYPE_INT8, int8_t);
    XX(TYPE_UINT8, uint8_t);
    XX(TYPE_INT16, int16_t);
    XX(TYPE_UINT16, uint16_t);
    XX(TYPE_INT32, int32_t);
    XX(TYPE_UINT32, uint32_t);
    XX(TYPE_FLOAT, float);
    XX(TYPE_INT64, int64_t);
    XX(TYPE_UINT64, uint64_t);
    XX(TYPE_DOUBLE, double);
    XX(TYPE_STRING, std::string);
    XX(TYPE_TEXT, std::string);
    XX(TYPE_BLOB, std::string);
    XX(TYPE_TIMESTAMP, int64_t);
#undef XX
    return "null";
}

// 获取字符串形式的SQLite3列数据类型
std::string Column::getSQLite3TypeString() {
#define XX(a, b) \
    if(a == m_dtype) {\
        return #b; \
    }

    XX(TYPE_INT8, INTEGER);
    XX(TYPE_UINT8, INTEGER);
    XX(TYPE_INT16, INTEGER);
    XX(TYPE_UINT16, INTEGER);
    XX(TYPE_INT32, INTEGER);
    XX(TYPE_UINT32, INTEGER);
    XX(TYPE_FLOAT, REAL);
    XX(TYPE_INT64, INTEGER);
    XX(TYPE_UINT64, INTEGER);
    XX(TYPE_DOUBLE, REAL);
    XX(TYPE_STRING, TEXT);
    XX(TYPE_TEXT, TEXT);
    XX(TYPE_BLOB, BLOB);
    XX(TYPE_TIMESTAMP, TIMESTAMP);
#undef XX
    return "";
}

// 获取字符串形式的MySQL列数据类型
std::string Column::getMySQLTypeString() {
#define XX(a, b) \
    if(a == m_dtype) {\
        return #b; \
    }

    XX(TYPE_INT8, tinyint);
    XX(TYPE_UINT8, tinyint unsigned);
    XX(TYPE_INT16, smallint);
    XX(TYPE_UINT16, smallint unsigned);
    XX(TYPE_INT32, int);
    XX(TYPE_UINT32, int unsigned);
    XX(TYPE_FLOAT, float);
    XX(TYPE_INT64, bigint);
    XX(TYPE_UINT64, bigint unsigned);
    XX(TYPE_DOUBLE, double);
    XX(TYPE_TEXT, text);
    XX(TYPE_BLOB, blob);
    XX(TYPE_TIMESTAMP, timestamp);
#undef XX
    if(m_dtype == TYPE_STRING) {
        return "varchar(" + std::to_string(m_length ? m_length : 128) + ")";
    }
    return "";
}

// "bind" + 数据类型
std::string Column::getBindString() {
#define XX(a, b) \
    if(a == m_dtype) { \
        return "bind" #b; \
    }
    XX(TYPE_INT8, Int8);
    XX(TYPE_UINT8, Uint8);
    XX(TYPE_INT16, Int16);
    XX(TYPE_UINT16, Uint16);
    XX(TYPE_INT32, Int32);
    XX(TYPE_UINT32, Uint32);
    XX(TYPE_FLOAT, Float);
    XX(TYPE_INT64, Int64);
    XX(TYPE_UINT64, Uint64);
    XX(TYPE_DOUBLE, Double);
    XX(TYPE_STRING, String);
    XX(TYPE_TEXT, String);
    XX(TYPE_BLOB, Blob);
    XX(TYPE_TIMESTAMP, Time);
#undef XX
    return "";
}

// "get" + 数据类型
std::string Column::getGetString() {
#define XX(a, b) \
    if(a == m_dtype) { \
        return "get" #b; \
    }
    XX(TYPE_INT8, Int8);
    XX(TYPE_UINT8, Uint8);
    XX(TYPE_INT16, Int16);
    XX(TYPE_UINT16, Uint16);
    XX(TYPE_INT32, Int32);
    XX(TYPE_UINT32, Uint32);
    XX(TYPE_FLOAT, Float);
    XX(TYPE_INT64, Int64);
    XX(TYPE_UINT64, Uint64);
    XX(TYPE_DOUBLE, Double);
    XX(TYPE_STRING, String);
    XX(TYPE_TEXT, String);
    XX(TYPE_BLOB, Blob);
    XX(TYPE_TIMESTAMP, Time);
#undef XX
    return "";
}

// 获取字符串形式的默认值
std::string Column::getDefaultValueString() {
    if(m_default.empty()) {
        return "";
    }
    if(m_dtype <= TYPE_DOUBLE) {
        return m_default;
    }
    if(m_dtype <= TYPE_BLOB) {
        return "\"" + m_default + "\"";
    }
    if(m_default == "current_timestamp") {
        return "time(0)";
    }
    return "sylar::Str2Time(\"" + m_default + "\")";
}

// 获取SQLite3字符串形式的默认值
std::string Column::getSQLite3Default() {
    if(m_dtype <= TYPE_UINT64) {
        if(m_default.empty()) {
            return "0";
        }
        return m_default;
    }
    if(m_dtype <= TYPE_BLOB) {
        if(m_default.empty()) {
            return "''";
        }
        return "'" + m_default + "'";
    }
    if(m_default.empty()) {
        return "'1980-01-01 00:00:00'";
    }
    return m_default;
}

// node为一个xml元素节点，
// 如<column name="id" type="int64" desc="唯一主键" auto_increment="true"/>
bool Column::init(const tinyxml2::XMLElement& node) {
    // 获取 XML 元素节点中的 name 属性值，并保存到 m_name
    if(!node.Attribute("name")) {
        SYLAR_LOG_ERROR(g_logger) << "column name not exists";
        return false;
    }
    m_name = node.Attribute("name");

    // 获取 XML 元素节点中的 type 属性值，并保存到 m_type
    if(!node.Attribute("type")) {
        SYLAR_LOG_ERROR(g_logger) << "column name=" << m_name
                                << " type is null";
        return false;
    }
    m_type = node.Attribute("type");

    // 解析 m_type，结果保存到 m_dtype
    m_dtype = ParseType(m_type);
    if(m_dtype == TYPE_NULL) {
        SYLAR_LOG_ERROR(g_logger) << "column name=" << m_name
                                << " type=" << m_type
                                << " type is invalid";
        return false;
    }

    // 获取 XML 元素节点中的 desc 属性值，并保存到 m_desc
    if(node.Attribute("desc")) {
        m_desc = node.Attribute("desc");
    }

    // 获取 XML 元素节点中的 default 属性值，并保存到 m_default
    if(node.Attribute("default")) {
        m_default = node.Attribute("default");
    }

    // 获取 XML 元素节点中的 update 属性值，并保存到 m_update
    if(node.Attribute("update")) {
        m_update = node.Attribute("update");
    }

    // 获取 XML 元素节点中的 length 属性值，并保存到 m_length
    if(node.Attribute("length")) {
        m_length = node.IntAttribute("length");
    } else {
        m_length = 0;
    }

    // 获取 XML 元素节点中的 autoIncrement 属性值，并保存到 m_autoIncrement
    m_autoIncrement = node.BoolAttribute("auto_increment", false);
    return true;
}

// set型类成员函数实现 的字符串实现
// void UserInfo::setId(const int64_t& v) {
//     m_id = v;
// }
std::string Column::getSetFunImpl(const std::string& class_name, int idx) const {
    std::stringstream ss;
    ss << "void " << GetAsClassName(class_name) << "::" << GetAsSetFunName(m_name) 
        << "(const " << TypeToString(m_dtype) << "& v) {" << std::endl;
    ss << "    " << GetAsMemberName(m_name) << " = v;" << std::endl;
    //ss << "    _flags |= " << (1ul << idx) << "ul;" << std::endl;
    ss << "}" << std::endl;
    return ss.str();
}

// 类成员变量定义 的字符串实现
// int64_t m_id;
std::string Column::getMemberDefine() const {
    std::stringstream ss;
    ss << TypeToString(m_dtype) << " " << GetAsMemberName(m_name) << ";" << std::endl;
    return ss.str();
}

// get型类成员函数定义 的字符串实现
// const int64_t& getId() { return m_id; }
std::string Column::getGetFunDefine() const {
    std::stringstream ss;
    ss << "const " << TypeToString(m_dtype) << "& " << GetAsGetFunName(m_name)
       << "() { return " << GetAsMemberName(m_name) << "; }" << std::endl;
    return ss.str();
}

// set型类成员函数定义 的字符串实现
// void setId(const int64_t& v);
std::string Column::getSetFunDefine() const {
    std::stringstream ss;
    ss << "void " << GetAsSetFunName(m_name) << "(const "
       << TypeToString(m_dtype) << "& v);" << std::endl;
    return ss.str();
}

}
}
