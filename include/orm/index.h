#ifndef __SYLAR_ORM_INDEX_H__
#define __SYLAR_ORM_INDEX_H__

#include <memory>
#include <string>
#include <vector>
#include "tinyxml2.h"

namespace sylar {
namespace orm {

/**
 * @brief 索引封装
*/
class Index {
public:
    typedef std::shared_ptr<Index> ptr;

    // 索引类型
    enum Type {
        TYPE_NULL = 0,  // 空索引
        TYPE_PK,        // 主键索引
        TYPE_UNIQ,      // 唯一索引
        TYPE_INDEX      // 常规索引
    };

    /**
     * @brief 初始化
     * @details 解析xml元素节点node(索引)，将数据存在类中
     * @example 解析 <index name="pk" cols="id" type="pk" desc="主键"/>
    */
    bool init(const tinyxml2::XMLElement& node);

    /**
     * @brief 获取索引名称
    */
    const std::string& getName() const { return m_name;}

    /**
     * @brief 获取索引类型
    */
    const std::string& getType() const { return m_type;}

    /**
     * @brief 获取索引描述
    */
    const std::string& getDesc() const { return m_desc;}

    /**
     * @brief 获取索引对应列
    */
    const std::vector<std::string>& getCols() const { return m_cols;}

    /**
     * @brief 获取索引类型
    */
    Type getDType() const { return m_dtype;}

    /**
     * @brief 是否为主键索引
    */
    bool isPK() const { return m_type == "pk";}

    /**
     * @brief 解析字符串形式索引类型，返回Type类型
    */
    static Type ParseType(const std::string& v);

    /**
     * @brief Type索引类型转为字符串形式索引类型
    */
    static std::string TypeToString(Type v);

private:
    std::string m_name;     // 索引名称 index name
    std::string m_type;     // 索引类型 index type
    std::string m_desc;     // 索引描述 index desc
    std::vector<std::string> m_cols;    // 索引对应列 index cols
    Type m_dtype;           // 索引类型

    // <index name="pk" cols="id" type="pk" desc="主键"/>
    // <index name="name" cols="name" type="uniq" desc="关联"/>
    // <index name="email" cols="email" type="uniq" desc="关联"/>
    // <index name="status" cols="status" type="index" desc="关联"/>
};

}
}

#endif
