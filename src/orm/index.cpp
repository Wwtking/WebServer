#include "index.h"
#include "log.h"
#include "util.h"
#include "hash_util.h"

namespace sylar {
namespace orm {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("orm");

// 解析字符串形式索引类型
Index::Type Index::ParseType(const std::string& v) {
#define XX(a, b) \
    if(v == b) {\
        return a; \
    }
    XX(TYPE_PK, "pk");
    XX(TYPE_UNIQ, "uniq");
    XX(TYPE_INDEX, "index");
#undef XX
    return TYPE_NULL;
}

// Type索引类型转为字符串形式索引类型
std::string Index::TypeToString(Type v) {
#define XX(a, b) \
    if(v == a) { \
        return b; \
    }
    XX(TYPE_PK, "pk");
    XX(TYPE_UNIQ, "uniq");
    XX(TYPE_INDEX, "index");
#undef XX
    return "";
}

// node为一个xml元素节点，如<index name="pk" cols="id" type="pk" desc="主键"/>
bool Index::init(const tinyxml2::XMLElement& node) {
    // 获取 XML 元素节点中的 name 属性值，并保存到 m_name
    if(!node.Attribute("name")) {
        SYLAR_LOG_ERROR(g_logger) << "index name not exists";
        return false;
    }
    m_name = node.Attribute("name");

    // 获取 XML 元素节点中的 type 属性值，并保存到 m_type
    if(!node.Attribute("type")) {
        SYLAR_LOG_ERROR(g_logger) << "index name=" << m_name << " type is null";
        return false;
    }
    m_type = node.Attribute("type");

    // 解析 m_type，结果保存到 m_dtype
    m_dtype = ParseType(m_type);
    if(m_dtype == TYPE_NULL) {
        SYLAR_LOG_ERROR(g_logger) << "index name=" << m_name << " type=" << m_type
            << " invalid (pk, index, uniq)";
        return false;
    }

    // 获取 XML 元素节点中的 cols 属性值，并保存到 m_cols
    // 索引可能联合多列(联合索引)，用 ',' 分隔每列
    if(!node.Attribute("cols")) {
        SYLAR_LOG_ERROR(g_logger) << "index name=" << m_name << " cols is null";
    }
    std::string tmp = node.Attribute("cols");
    m_cols = sylar::split(tmp, ',');

    // 获取 XML 元素节点中的 type 属性值，并保存到 m_type
    if(node.Attribute("desc")) {
        m_desc = node.Attribute("desc");
    }
    return true;
}

}
}
