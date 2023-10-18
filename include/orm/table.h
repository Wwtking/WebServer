#ifndef __SYLAR_ORM_TABLE_H__
#define __SYLAR_ORM_TABLE_H__

#include "column.h"
#include "index.h"
#include <fstream>

namespace sylar {
namespace orm {

class Table {
public:
    typedef std::shared_ptr<Table> ptr;

    /**
     * @brief 初始化
     * @details 解析xml元素节点node(表)，将数据存在类中
     *          将表的结构解析，如表名、所有列、所有索引等
    */
    bool init(const tinyxml2::XMLElement& node);

    /**
     * @brief 根据表 .xml 文件生成 .h 和 .cpp 代码(表结构和表操作)
     * @param[in] path 输出文件路径
    */
    void gen(const std::string& path);

    /**
     * @brief 获取文件名
    */
    std::string getFilename() const;

    /**
     * @brief 获取表名
    */
    const std::string& getName() const { return m_name;}

    /**
     * @brief 获取命名空间
    */
    const std::string& getNamespace() const { return m_namespace;}

    /**
     * @brief 获取表描述
    */
    const std::string& getDesc() const { return m_desc;}

    /**
     * @brief 获取表所有列
    */
    const std::vector<Column::ptr>& getCols() const { return m_cols;}

    /**
     * @brief 获取表所有索引
    */
    const std::vector<Index::ptr>& getIdxs() const { return  m_idxs;}

private:
    /**
     * @brief 生成 xxx.h 文件内容
     * @param[in] path 输出文件路径
    */
    void gen_inc(const std::string& path);

    /**
     * @brief 生成 xxx.cpp 文件内容
     * @param[in] path 输出文件路径
    */
    void gen_src(const std::string& path);

    /**
     * @brief 定义 toJsonString 成员函数
    */
    std::string genToStringInc();

    /**
     * @brief 实现 toJsonString 成员函数
    */
    std::string genToStringSrc(const std::string& class_name);

    /**
     * @brief 实现 mysql 的 insert 函数
    */
    std::string genToInsertSQL(const std::string& class_name);

    /**
     * @brief 实现 mysql 的 update 函数
    */
    std::string genToUpdateSQL(const std::string& class_name);

    /**
     * @brief 实现 mysql 的 delete 函数
    */
    std::string genToDeleteSQL(const std::string& class_name);

    /**
     * @brief 获取主键索引对应的列
    */
    std::vector<Column::ptr> getPKs() const;

    /**
     * @brief 根据列名称获取列
    */
    Column::ptr getCol(const std::string& name) const;

    std::string genWhere() const;

    /**
     * @brief 生成 xxx.h 文件中 表操作类定义
    */
    void gen_dao_inc(std::ofstream& ofs);

    /**
     * @brief 生成 xxx.cpp 文件中 表操作类实现
    */
    void gen_dao_src(std::ofstream& ofs);

    // sql类型
    enum DBType {
        TYPE_SQLITE3 = 1,
        TYPE_MYSQL = 2
    };
    
private:
    std::string m_name;                 // 表名
    std::string m_namespace;            // 命名空间
    std::string m_desc;                 // 表描述
    std::string m_subfix = "_info";     // 名字后缀
    DBType m_type = TYPE_SQLITE3;       // 表类型
    std::string m_dbclass = "sylar::IDB";       // 创建表函数的参数作用域
    std::string m_queryclass = "sylar::IDB";    // 查询函数的参数作用域
    std::string m_updateclass = "sylar::IDB";   // 修改函数的参数作用域
    std::vector<Column::ptr> m_cols;    // 表所有列
    std::vector<Index::ptr> m_idxs;     // 表所有索引
};

}
}

#endif
