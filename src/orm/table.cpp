#include "table.h"
#include "log.h"
#include "util.h"
#include "hash_util.h"
#include "orm_util.h"
#include <set>

namespace sylar {
namespace orm {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("orm");

// 获取文件名
std::string Table::getFilename() const {
    return sylar::ToLower(m_name + m_subfix);
}

// node为一个xml元素节点，如<table name="user" namespace="test.orm" desc="用户表">
bool Table::init(const tinyxml2::XMLElement& node) {
    // 获取 XML 元素节点中的 name 属性值，并保存到 m_name
    if(!node.Attribute("name")) {
        SYLAR_LOG_ERROR(g_logger) << "table name is null";
        return false;
    }
    m_name = node.Attribute("name");

    // 获取 XML 元素节点中的 namespace 属性值，并保存到 m_namespace
    if(!node.Attribute("namespace")) {
        SYLAR_LOG_ERROR(g_logger) << "table namespace is null";
        return false;
    }
    m_namespace = node.Attribute("namespace");

    // 获取 XML 元素节点中的 desc 属性值，并保存到 m_desc
    if(node.Attribute("desc")) {
        m_desc = node.Attribute("desc");
    }

    // 获取 node 中名为 columns 的子元素
    const tinyxml2::XMLElement* cols = node.FirstChildElement("columns");
    if(!cols) {
        SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " columns is null";
        return false;
    }

    // 获取 cols 中名为 column 的子元素
    const tinyxml2::XMLElement* col = cols->FirstChildElement("column");
    if(!col) {
        SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " column is null";
        return false;
    }

    // 遍历 columns 中每一列
    std::set<std::string> col_names;    // 存放所有列名称
    int index = 0;
    do {
        Column::ptr col_ptr(new Column);
        // 解析 col
        if(!col_ptr->init(*col)) {
            SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " init column error";
            return false;
        }
        // 存放列名
        if(col_names.insert(col_ptr->getName()).second == false) {
            SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " column name="
                << col_ptr->getName() << " exists";
            return false;
        }
        // 更新列索引
        col_ptr->m_index = index++;
        // 存放列
        m_cols.push_back(col_ptr);
        // 下一列
        col = col->NextSiblingElement("column");
    } while(col);

    // 获取 node 中名为 indexs 的子元素
    const tinyxml2::XMLElement* idxs = node.FirstChildElement("indexs");
    if(!idxs) {
        SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " indexs is null";
        return false;
    }

    // 获取 idxs 中名为 index 的子元素
    const tinyxml2::XMLElement* idx = idxs->FirstChildElement("index");
    if(!idx) {
        SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " index is null";
        return false;
    }

    // 遍历 indexs 中每一列
    std::set<std::string> idx_names;    // 存放所有列名称
    bool has_pk = false;
    do {
        Index::ptr idx_ptr(new Index);
        // 解析 idx
        if(!idx_ptr->init(*idx)) {
            SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " index init error";
            return false;
        }
        // 存放列名
        if(idx_names.insert(idx_ptr->getName()).second == false) {
            SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " index name="
                << idx_ptr->getName() << " exists";
            return false;
        }
        // 是否为主键
        if(idx_ptr->isPK()) {
            if(has_pk) {
                SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " more than one pk"; 
                return false;
            }
            has_pk = true;
        }
        // 判断索引是否对应上列中的列名
        auto& cnames = idx_ptr->getCols();
        for(auto& x : cnames) {
            if(col_names.count(x) == 0) {
                // 上面列名没有索引中对应的列名，报错
                SYLAR_LOG_ERROR(g_logger) << "table name=" << m_name << " idx="
                    << idx_ptr->getName() << " col=" << x << " not exists";
                return false;
            }
        }
        // 存放索引
        m_idxs.push_back(idx_ptr);
        // 下一个索引
        idx = idx->NextSiblingElement("index");
    } while(idx);
    return true;
}

// 根据表 .xml 文件生成 .h 和 .cpp 代码(表结构和表操作)
void Table::gen(const std::string& path) {
    // 创建orm输出文件路径
    // blog.data --> blog/data
    // path/blog/data
    std::string p = path + "/" + sylar::replace(m_namespace, ".", "/");
    sylar::FilesUtil::Mkdir(p);

    // 输出xxx.h文件
    gen_inc(p);
    // 输出xxx.cpp文件
    gen_src(p);
}

// 生成 xxx.h 文件内容
void Table::gen_inc(const std::string& path) {
    // 输出.h文件名
    std::string filename = path + "/" + m_name + m_subfix + ".h";
    // 输出.h文件中表结构类名
    std::string class_name = m_name + m_subfix;
    // 输出.h文件中表操作类名
    std::string class_name_dao = m_name + m_subfix + "_dao";

    std::ofstream ofs(filename);
    ofs << "#ifndef " << GetAsDefineMacro(m_namespace + "." + class_name + ".h") << std::endl;
    ofs << "#define " << GetAsDefineMacro(m_namespace + "." + class_name + ".h") << std::endl;
    ofs << std::endl;

    // 用到的头文件
    std::set<std::string> sincs = {"vector", "json/json.h"};
    for(auto& i : sincs) {
        ofs << "#include <" << i << ">" << std::endl;
    }
    // std::set<std::string> incs = {"sylar/db/db.h", "sylar/util.h"};
    std::set<std::string> incs = {"db.h", "util.h"};
    for(auto& i : incs) {
        ofs << "#include \"" << i << "\"" << std::endl;
    }
    ofs << std::endl;
    ofs << std::endl;

    // 命名空间
    std::vector<std::string> ns = sylar::split(m_namespace, '.');
    for(auto it = ns.begin(); it != ns.end(); ++it) {
        ofs << "namespace " << *it << " {" << std::endl;
    }
    ofs << std::endl;

    // 表结构类定义 begin
    ofs << "class " << GetAsClassName(class_name_dao) << ";" << std::endl;
    ofs << "class " << GetAsClassName(class_name) << " {" << std::endl;
    ofs << "friend class " << GetAsClassName(class_name_dao) << ";" << std::endl;
    ofs << "public:" << std::endl;
    ofs << "    typedef std::shared_ptr<" << GetAsClassName(class_name) << "> ptr;" << std::endl;
    ofs << std::endl;
    // 构造函数
    ofs << "    " << GetAsClassName(class_name) << "();" << std::endl;
    ofs << std::endl;

    // 将表中所有列按列类型顺序排列(为了把类成员变量排序)
    auto cols = m_cols;
    std::sort(cols.begin(), cols.end(), [](const Column::ptr& a, const Column::ptr& b){
        if(a->getDType() != b->getDType()) {
            return a->getDType() < b->getDType();
        } else {
            return a->getIndex() < b->getIndex();
        }
    });

    // 生成 get型类成员函数 和 set型类成员函数
    for(auto& i : m_cols) {
        ofs << "    " << i->getGetFunDefine();
        ofs << "    " << i->getSetFunDefine();
        ofs << std::endl;
    }

    // 生成 ToString 成员函数
    ofs << "    " << genToStringInc() << std::endl;
    //ofs << "    std::string toInsertSQL() const;" << std::endl;
    //ofs << "    std::string toUpdateSQL() const;" << std::endl;
    //ofs << "    std::string toDeleteSQL() const;" << std::endl;
    ofs << std::endl;
    
    // 生成类成员变量(按照上述排序顺序)
    ofs << "private:" << std::endl;
    for(auto& i : cols) {
        ofs << "    " << i->getMemberDefine();
    }
    //ofs << "    uint64_t _flags = 0;" << std::endl;
    ofs << "};" << std::endl;
    ofs << std::endl;
    // 表结构类定义 end

    // 表操作类定义 begin
    ofs << std::endl;
    gen_dao_inc(ofs);
    ofs << std::endl;
    // 表操作类定义 end

    // 收尾
    for(auto it = ns.rbegin(); it != ns.rend(); ++it) {
        ofs << "} //namespace " << *it << std::endl;
    }
    ofs << "#endif //" << GetAsDefineMacro(m_namespace + "." + class_name + ".h") << std::endl;
}

// 定义 toJsonString 成员函数
std::string Table::genToStringInc() {
    std::stringstream ss;
    ss << "std::string toJsonString() const;";
    return ss.str();
}

// 实现 toJsonString 成员函数
std::string Table::genToStringSrc(const std::string& class_name) {
    std::stringstream ss;
    ss << "std::string " << GetAsClassName(class_name) << "::toJsonString() const {" << std::endl;
    ss << "    Json::Value jvalue;" << std::endl;
    for(auto it = m_cols.begin(); it != m_cols.end(); ++it) {
        ss  << "    jvalue[\"" << (*it)->getName() << "\"] = ";
        if((*it)->getDType() == Column::TYPE_UINT64
                || (*it)->getDType() == Column::TYPE_INT64) {
            // jvalue["id"] = std::to_string(m_id);
            ss << "std::to_string(" << GetAsMemberName((*it)->getName()) << ")"
               << ";" << std::endl;
        } else if((*it)->getDType() == Column::TYPE_TIMESTAMP) {
            // jvalue["login_time"] = sylar::TimeToStr(m_loginTime);
            ss << "sylar::TimeToStr(" << GetAsMemberName((*it)->getName()) << ")"
               << ";" << std::endl;
        } else {
            // jvalue["account"] = m_account;
            ss << GetAsMemberName((*it)->getName())
               << ";" << std::endl;
        }
    }
    ss << "    return sylar::JsonUtil::ToString(jvalue);" << std::endl;
    ss << "}" << std::endl;
    return ss.str();
}

// 生成 xxx.cpp 文件内容
void Table::gen_src(const std::string& path) {
    // 表结构类名
    std::string class_name = m_name + m_subfix;
    // 输出.cpp文件名
    std::string filename = path + "/" + class_name + ".cpp";

    std::ofstream ofs(filename);
    // 用到的头文件
    ofs << "#include \"" << class_name + ".h\"" << std::endl;
    ofs << "#include \"log.h\"" << std::endl;
    ofs << "#include \"json_util.h\"" << std::endl;
    ofs << std::endl;

    // 命名空间
    std::vector<std::string> ns = sylar::split(m_namespace, '.');
    for(auto it = ns.begin(); it != ns.end(); ++it) {
        ofs << "namespace " << *it << " {" << std::endl;
    }
    ofs << std::endl;

    ofs << "static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME(\"orm\");" << std::endl;
    ofs << std::endl;

    // 表结构类实现 begin
    // 构造函数
    ofs << GetAsClassName(class_name) << "::" << GetAsClassName(class_name) << "()" << std::endl;
    ofs << "    :";
    // 将表中所有列按列类型顺序排列(为了把类成员变量排序)
    auto cols = m_cols;
    std::sort(cols.begin(), cols.end(), [](const Column::ptr& a, const Column::ptr& b){
        if(a->getDType() != b->getDType()) {
            return a->getDType() < b->getDType();
        } else {
            return a->getIndex() < b->getIndex();
        }
    });
    // 初始化列表
    for(auto it = cols.begin(); it != cols.end(); ++it) {
        if(it != cols.begin()) {
            ofs << std::endl << "    ,";
        }
        ofs << GetAsMemberName((*it)->getName()) << "("
            << (*it)->getDefaultValueString() << ")";
    }
    ofs << " {" << std::endl;
    ofs << "}" << std::endl;
    ofs << std::endl;

    // toJsonString 成员函数
    ofs << genToStringSrc(class_name) << std::endl;
    //ofs << genToInsertSQL(class_name) << std::endl;
    //ofs << genToUpdateSQL(class_name) << std::endl;
    //ofs << genToDeleteSQL(class_name) << std::endl;

    // set型类成员函数实现
    for(size_t i = 0; i < m_cols.size(); ++i) {
        ofs << m_cols[i]->getSetFunImpl(class_name, i) << std::endl;
    }
    // 表结构类实现 end

    // 表操作类实现 begin
    ofs << std::endl;
    gen_dao_src(ofs);
    ofs << std::endl;
    // 表操作类实现 end

    // 收尾
    for(auto it = ns.rbegin(); it != ns.rend(); ++it) {
        ofs << "} //namespace " << *it << std::endl;
    }
}

std::string Table::genToInsertSQL(const std::string& class_name) {
    std::stringstream ss;
    ss << "std::string " << 
       GetAsClassName(class_name)
       << "::toInsertSQL() const {" << std::endl;
    ss << "    std::stringstream ss;" << std::endl;
    ss << "    ss << \"insert into " << m_name
       << "(";
    bool is_first = true;
    for(size_t i = 0; i < m_cols.size(); ++i) {
        if(m_cols[i]->isAutoIncrement()) {
            continue;
        }
        if(!is_first) {
            ss << ",";
        }
        ss << m_cols[i]->getName();
        is_first = false;
    }
    ss << ") values (\"" << std::endl;
    is_first = true;
    for(size_t i = 0; i < m_cols.size(); ++i) {
        if(m_cols[i]->isAutoIncrement()) {
            continue;
        }
        if(!is_first) {
            ss << "    ss << \",\";" << std::endl;
        }
        if(m_cols[i]->getDType() == Column::TYPE_STRING) {
            ss << "    ss << \"'\" << sylar::replace("
               << GetAsMemberName(m_cols[i]->getName())
               << ", \"'\", \"''\") << \"'\";" << std::endl;
        } else {
            ss << "    ss << " << GetAsMemberName(m_cols[i]->getName())
               << ";" << std::endl;
        }
        is_first = true;
    }
    ss << "    ss << \")\";" << std::endl;
    ss << "    return ss.str();" << std::endl;
    ss << "}" << std::endl;
    return ss.str();
}

std::string Table::genToUpdateSQL(const std::string& class_name) {
    std::stringstream ss;
    ss << "std::string " << 
       GetAsClassName(class_name)
       << "::toUpdateSQL() const {" << std::endl;
    ss << "    std::stringstream ss;" << std::endl;
    ss << "    bool is_first = true;" << std::endl;
    ss << "    ss << \"update " << m_name
       << " set \";" << std::endl;
    for(size_t i = 0; i < m_cols.size(); ++i) {
        ss << "    if(_flags & " << (1ul << i) << "ul) {" << std::endl;
        ss << "        if(!is_first) {" << std::endl;
        ss << "            ss << \",\";" << std::endl;
        ss << "        }" << std::endl;
        ss << "        ss << \" " << m_cols[i]->getName()
           << " = ";
        if(m_cols[i]->getDType() == Column::TYPE_STRING) {
            ss << "'\" << sylar::replace("
               << GetAsMemberName(m_cols[i]->getName())
               << ", \"'\", \"''\") << \"'\";" << std::endl;
        } else {
            ss << "\" << " << GetAsMemberName(m_cols[i]->getName())
               << ";" << std::endl;
        }
        ss << "        is_first = false;" << std::endl;
        ss << "    }" << std::endl;
    }
    ss << genWhere();
    ss << "    return ss.str();" << std::endl;
    ss << "}" << std::endl;
    return ss.str();
}

std::string Table::genToDeleteSQL(const std::string& class_name) {
    std::stringstream ss;
    ss << "std::string " << 
       GetAsClassName(class_name)
       << "::toDeleteSQL() const {" << std::endl;
    ss << "    std::stringstream ss;" << std::endl;
    ss << "    ss << \"delete from " << m_name << "\";" << std::endl;
    ss << genWhere();
    ss << "    return ss.str();" << std::endl;
    ss << "}" << std::endl;
    return ss.str();

}

std::string Table::genWhere() const {
    std::stringstream ss;
    ss << "    ss << \" where ";
    auto pks = getPKs();

    for(size_t i = 0; i < pks.size(); ++i) {
        if(i) {
            ss << "    ss << \" and ";
        }
        ss << pks[i]->getName() << " = ";
        if(pks[i]->getDType() == Column::TYPE_STRING) {
            ss << "'\" << sylar::replace("
               << GetAsMemberName(m_cols[i]->getName())
               << ", \"'\", \"''\") << \"'\";" << std::endl;
        } else {
            ss << "\" << " << GetAsMemberName(m_cols[i]->getName()) << ";" << std::endl;
        }
    }
    return ss.str();
}

// 获取主键索引对应的列
std::vector<Column::ptr> Table::getPKs() const {
    std::vector<Column::ptr> cols;
    for(auto& i : m_idxs) {
        if(i->isPK()) {
            for(auto& n : i->getCols()) {
                cols.push_back(getCol(n));
            }
        }
    }
    return cols;
}

// 根据列名称获取列
Column::ptr Table::getCol(const std::string& name) const {
    for(auto& i : m_cols) {
        if(i->getName() == name) {
            return i;
        }
    }
    return nullptr;
}

// 生成 xxx.h 文件中 表操作类定义
void Table::gen_dao_inc(std::ofstream& ofs) {
    // 表结构类名
    std::string class_name = m_name + m_subfix;
    // 表操作类名
    std::string class_name_dao = class_name + "_dao";

    // 表操作类定义
    ofs << "class " << GetAsClassName(class_name_dao) << " {" << std::endl;
    ofs << "public:" << std::endl;
    ofs << "    typedef std::shared_ptr<" << GetAsClassName(class_name_dao) << "> ptr;" << std::endl;
    // Update 函数定义
    ofs << "    static int Update(" << GetAsClassName(class_name)
        << "::ptr info, " << m_updateclass << "::ptr conn);" << std::endl;
    // Insert 函数定义
    ofs << "    static int Insert(" << GetAsClassName(class_name)
        << "::ptr info, " << m_updateclass << "::ptr conn);" << std::endl;
    // InsertOrUpdate 函数定义
    ofs << "    static int InsertOrUpdate(" << GetAsClassName(class_name)
        << "::ptr info, " << m_updateclass << "::ptr conn);" << std::endl;
    // Delete 函数定义 
    ofs << "    static int Delete(" << GetAsClassName(class_name)
        << "::ptr info, " << m_updateclass << "::ptr conn);" << std::endl;
    
    // Delete 函数定义 
    auto vs = getPKs();
    ofs << "    static int Delete(";
    for(auto& i : vs) {
        ofs << "const " << i->getDTypeString() << "& "
            << GetAsVariable(i->getName()) << ", ";
    }
    ofs << m_updateclass << "::ptr conn);" << std::endl;

    // DeleteById 函数定义
    // DeleteByAccount 函数定义
    // DeleteByEmail 函数定义
    // DeleteByName 函数定义
    for(auto& i : m_idxs) {
        if(i->getDType() == Index::TYPE_UNIQ
            || i->getDType() == Index::TYPE_PK
            || i->getDType() == Index::TYPE_INDEX) {
            ofs << "    static int Delete";
            // 制作函数名
            std::string tmp = "by";
            for(auto& c : i->getCols()) {
                tmp += "_" + c;
            }
            ofs << GetAsClassName(tmp) << "(";
            // 制作函数参数
            for(auto& c : i->getCols()) {
                auto d = getCol(c);
                ofs << " const " << d->getDTypeString() << "& "
                    << GetAsVariable(d->getName()) << ", ";
            }
            ofs << m_updateclass << "::ptr conn);" << std::endl;
        }
    }

    // QueryAll 函数定义
    ofs << "    static int QueryAll(std::vector<"
        << GetAsClassName(class_name) << "::ptr>& results, " << m_queryclass << "::ptr conn);" << std::endl;
    
    // Query 函数定义
    ofs << "    static " << GetAsClassName(class_name) << "::ptr Query(";
    for(auto& i : vs) {
        ofs << " const " << i->getDTypeString() << "& "
            << GetAsVariable(i->getName()) << ", ";
    }
    ofs << m_queryclass << "::ptr conn);" << std::endl;

    // QueryByAccount 函数定义
    // QueryByEmail 函数定义
    // QueryByName 函数定义
    for(auto& i : m_idxs) {
        if(i->getDType() == Index::TYPE_UNIQ) {
            ofs << "    static " << GetAsClassName(class_name) << "::ptr Query";
            // 制作函数名
            std::string tmp = "by";
            for(auto& c : i->getCols()) {
                tmp += "_" + c;
            }
            ofs << GetAsClassName(tmp) << "(";
            // 制作函数参数
            for(auto& c : i->getCols()) {
                auto d = getCol(c);
                ofs << " const " << d->getDTypeString() << "& "
                    << GetAsVariable(d->getName()) << ", ";
            }
            ofs << m_queryclass << "::ptr conn);" << std::endl;
        } else if(i->getDType() == Index::TYPE_INDEX) {
            ofs << "    static int Query";
            // 制作函数名
            std::string tmp = "by";
            for(auto& c : i->getCols()) {
                tmp += "_" + c;
            }
            ofs << GetAsClassName(tmp) << "(";
            // 制作函数参数
            ofs << "std::vector<" << GetAsClassName(class_name) << "::ptr>& results, ";
            for(auto& c : i->getCols()) {
                auto d = getCol(c);
                ofs << " const " << d->getDTypeString() << "& "
                    << GetAsVariable(d->getName()) << ", ";
            }
            ofs << m_queryclass << "::ptr conn);" << std::endl;
        }
    }

    // CreateTableSQLite3 函数定义
    ofs << "    static int CreateTableSQLite3(" << m_dbclass << "::ptr info);" << std::endl;
    
    // CreateTableMySQL 函数定义
    ofs << "    static int CreateTableMySQL(" << m_dbclass << "::ptr info);" << std::endl;
    ofs << "};" << std::endl;
}

template<class V, class T>
bool is_exists(const V& v, const T& t) {
    for(auto& i : v) {
        if(i == t) {
            return true;
        }
    }
    return false;
}

// 生成 xxx.cpp 文件中 表操作类实现
void Table::gen_dao_src(std::ofstream& ofs) {
    // 表结构类名
    std::string class_name = m_name + m_subfix;
    // 表操作类名
    std::string class_name_dao = class_name + "_dao";

    // 1、Update 函数实现
    ofs << "int " << GetAsClassName(class_name_dao)
        << "::Update(" << GetAsClassName(class_name)
        << "::ptr info, " << m_updateclass << "::ptr conn) {" << std::endl;
    // sql语句
    ofs << "    std::string sql = \"update " << m_name << " set";
    auto pks = getPKs();
    bool is_first = true;
    for(auto& i : m_cols) {
        // 跳过有主键索引的列，因为要放到where后面
        if(is_exists(pks, i)) {
            continue;
        }
        if(!is_first) {
            ofs << ",";
        }
        ofs << " " << i->getName() << " = ?";
        is_first = false;
    }
    // where后面放条件，即主键索引对应列
    ofs << " where";
    is_first = true;
    for(auto& i : pks) {
        if(!is_first) {
            // 联合索引
            ofs << " and";
        }
        ofs << " " << i->getName() << " = ?";
    }
    ofs << "\";" << std::endl;
    // prepare
#define CHECK_STMT(v) \
    ofs << "    auto stmt = conn->prepare(sql);" << std::endl; \
    ofs << "    if(!stmt) {" << std::endl; \
    ofs << "        SYLAR_LOG_ERROR(g_logger) << \"stmt=\" << sql" << std::endl << \
        "                 << \" errno=\"" \
        " << conn->getErrno() << \" errstr=\" << conn->getErrStr();" << std::endl \
        << "        return " v ";" << std::endl; \
    ofs << "    }" << std::endl;
    CHECK_STMT("conn->getErrno()");
    // bind
    is_first = true;
    int idx = 1;
    for(auto& i : m_cols) {
        // 跳过有主键索引的列
        if(is_exists(pks, i)) {
            continue;
        }
        ofs << "    stmt->" << i->getBindString() << "(" << idx << ", ";
        ofs << "info->" << GetAsMemberName(i->getName());
        ofs << ");" << std::endl;
        ++idx;
    }
    for(auto& i : pks) {
        ofs << "    stmt->" << i->getBindString() << "(" << idx << ", ";
        ofs << "info->" << GetAsMemberName(i->getName()) << ");" << std::endl;
        ++idx;
    }
    // execute
    ofs << "    return stmt->execute();" << std::endl;
    ofs << "}" << std::endl << std::endl;

    // 2、Insert 函数实现
    ofs << "int " << GetAsClassName(class_name_dao)
        << "::Insert(" << GetAsClassName(class_name)
        << "::ptr info, " << m_updateclass << "::ptr conn) {" << std::endl;
    // sql语句
    ofs << "    std::string sql = \"insert into " << m_name << " (";
    is_first = true;
    Column::ptr auto_inc;
    for(auto& i : m_cols) {
        // 忽略自增的列，因为插入数据会自动更新，不需要插入该列
        if(i->isAutoIncrement()) {
            auto_inc = i;
            continue;
        }
        if(!is_first) {
            ofs << ", ";
        }
        ofs << i->getName();
        is_first = false;
    }
    ofs << ") values (";
    is_first = true;
    for(auto& i : m_cols) {
        if(i->isAutoIncrement()) {
            continue;
        }
        if(!is_first) {
            ofs << ", ";
        }
        ofs << "?";
        is_first = false;
    }
    ofs << ")\";" << std::endl;
    // prepare
    CHECK_STMT("conn->getErrno()");
    // bind
    idx = 1;
    for(auto& i : m_cols) {
        if(i->isAutoIncrement()) {
            continue;
        }
        ofs << "    stmt->" << i->getBindString() << "(" << idx << ", ";
        ofs << "info->" << GetAsMemberName(i->getName());
        ofs << ");" << std::endl;
        ++idx;
    }
    // execute
    ofs << "    int rt = stmt->execute();" << std::endl;
    if(auto_inc) {
        ofs << "    if(rt == 0) {" << std::endl;
        ofs << "        info->" << GetAsMemberName(auto_inc->getName())
            << " = conn->getLastInsertId();" << std::endl
            << "    }" << std::endl;
    }
    ofs << "    return rt;" << std::endl;
    ofs << "}" << std::endl << std::endl;

    // 3、InsertOrUpdate 函数实现
    ofs << "int " << GetAsClassName(class_name_dao)
        << "::InsertOrUpdate(" << GetAsClassName(class_name)
        << "::ptr info, " << m_updateclass << "::ptr conn) {" << std::endl;
    // 判断是Insert还是Update
    for(auto& i : m_cols) {
        if(i->isAutoIncrement()) {
            auto_inc = i;
            break;
        }
    }
    if(auto_inc) {
        ofs << "    if(info->" << GetAsMemberName(auto_inc->getName()) << " == 0) {" << std::endl;
        ofs << "        return Insert(info, conn);" << std::endl;
        ofs << "    }" << std::endl;
    }
    // sql语句
    ofs << "    std::string sql = \"replace into " << m_name << " (";
    is_first = true;
    for(auto& i : m_cols) {
        if(!is_first) {
            ofs << ", ";
        }
        ofs << i->getName();
        is_first = false;
    }
    ofs << ") values (";
    is_first = true;
    for(auto& i : m_cols) {
        (void)i;
        if(!is_first) {
            ofs << ", ";
        }
        ofs << "?";
        is_first = false;
    }
    ofs << ")\";" << std::endl;
    // prepare
    CHECK_STMT("conn->getErrno()");
    // bind
    idx = 1;
    for(auto& i : m_cols) {
        ofs << "    stmt->" << i->getBindString() << "(" << idx << ", ";
        ofs << "info->" << GetAsMemberName(i->getName()) << ");" << std::endl;
        ++idx;
    }
    // execute
    ofs << "    return stmt->execute();" << std::endl;
    ofs << "}" << std::endl << std::endl;

    // 4、Delete 函数实现
    ofs << "int " << GetAsClassName(class_name_dao)
        << "::Delete(" << GetAsClassName(class_name)
        << "::ptr info, " << m_updateclass << "::ptr conn) {" << std::endl;
    // sql语句
    ofs << "    std::string sql = \"delete from " << m_name << " where";
    is_first = true;
    for(auto& i : pks) {
        if(!is_first) {
            ofs << " and";
        }
        ofs << " " << i->getName() << " = ?";
        is_first = false;
    }
    ofs << "\";" << std::endl;
    // prepare
    CHECK_STMT("conn->getErrno()");
    // bind
    idx = 1;
    for(auto& i : pks) {
        ofs << "    stmt->" << i->getBindString() << "(" << idx << ", ";
        ofs << "info->" << GetAsMemberName(i->getName()) << ");" << std::endl;
        ++idx;
    }
    // execute
    ofs << "    return stmt->execute();" << std::endl;
    ofs << "}" << std::endl << std::endl;

    // 5、DeleteById 函数实现
    // 6、DeleteByAccount 函数实现
    // 7、DeleteByEmail 函数实现
    // 8、DeleteByName 函数实现
    for(auto& i : m_idxs) {
        if(i->getDType() == Index::TYPE_UNIQ
            || i->getDType() == Index::TYPE_PK
            || i->getDType() == Index::TYPE_INDEX) {
            ofs << "int " << GetAsClassName(class_name_dao) << "::Delete";
            // 制作函数名
            std::string tmp = "by";
            for(auto& c : i->getCols()) {
                tmp += "_" + c;
            }
            ofs << GetAsClassName(tmp) << "(";
            // 制作函数参数
            for(auto& c : i->getCols()) {
                auto d = getCol(c);
                ofs << " const " << d->getDTypeString() << "& "
                    << GetAsVariable(d->getName()) << ", ";
            }
            ofs << m_updateclass << "::ptr conn) {" << std::endl;
            // sql语句
            ofs << "    std::string sql = \"delete from " << m_name << " where";
            is_first = true;
            for(auto& x : i->getCols()) {
                if(!is_first) {
                    ofs << " and";
                }
                ofs << " " << x << " = ?";
                is_first = false;
            }
            ofs << "\";" << std::endl;
            // prepare
            CHECK_STMT("conn->getErrno()");
            // bind
            idx = 1;
            for(auto& x : i->getCols()) {
                ofs << "    stmt->" << getCol(x)->getBindString() << "(" << idx << ", ";
                ofs << GetAsVariable(x) << ");" << std::endl;
            }
            // execute
            ofs << "    return stmt->execute();" << std::endl;
            ofs << "}" << std::endl << std::endl;
        }
    }

    // 9、QueryAll 函数实现
    ofs << "int " << GetAsClassName(class_name_dao) << "::QueryAll(std::vector<"
        << GetAsClassName(class_name) << "::ptr>& results, "
        << m_queryclass << "::ptr conn) {" << std::endl;
    // sql语句
    ofs << "    std::string sql = \"select ";
    is_first = true;
    for(auto& i : m_cols) {
        if(!is_first) {
            ofs << ", ";
        }
        ofs << i->getName();
        is_first = false;
    }
    ofs << " from " << m_name << "\";" << std::endl;
    // prepare
    CHECK_STMT("conn->getErrno()");
    // query
    ofs << "    auto rt = stmt->query();" << std::endl;
    ofs << "    if(!rt) {" << std::endl;
    ofs << "        return stmt->getErrno();" << std::endl;
    ofs << "    }" << std::endl;
    // step
    ofs << "    while (rt->next()) {" << std::endl;
    ofs << "        " << GetAsClassName(class_name) << "::ptr v(new "
        << GetAsClassName(class_name) << ");" << std::endl;
#define PARSE_OBJECT(prefix) \
    for(size_t i = 0; i < m_cols.size(); ++i) { \
        ofs << prefix "v->" << GetAsMemberName(m_cols[i]->getName()) << " = "; \
        ofs << "rt->" << m_cols[i]->getGetString() << "(" << (i) << ");" << std::endl; \
    }
    PARSE_OBJECT("        ");
    ofs << "        results.push_back(v);" << std::endl;
    ofs << "    }" << std::endl;
    ofs << "    return 0;" << std::endl;
    ofs << "}" << std::endl << std::endl;

    // 10、Query 函数实现
    ofs << GetAsClassName(class_name) << "::ptr "
        << GetAsClassName(class_name_dao) << "::Query(";
    for(auto& i : pks) {
        ofs << " const " << i->getDTypeString() << "& "
            << GetAsVariable(i->getName()) << ", ";
    }
    ofs << m_queryclass << "::ptr conn) {" << std::endl;
    // sql语句
    ofs << "    std::string sql = \"select ";
    is_first = true;
    for(auto& i : m_cols) {
        if(!is_first) {
            ofs << ", ";
        }
        ofs << i->getName();
        is_first = false;
    }
    ofs << " from " << m_name << " where";
    is_first = true;
    for(auto& i : pks) {
        if(!is_first) {
            ofs << " and";
        }
        ofs << " " << i->getName() << " = ?";
        is_first = false;
    }
    ofs << "\";" << std::endl;
    // prepare
    CHECK_STMT("nullptr");
    // bind
    idx = 1;
    for(auto& i : pks) {
        ofs << "    stmt->" << i->getBindString() << "(" << idx << ", ";
        ofs << GetAsVariable(i->getName()) << ");" << std::endl;
        ++idx;
    }
    // query
    ofs << "    auto rt = stmt->query();" << std::endl;
    ofs << "    if(!rt) {" << std::endl;
    ofs << "        return nullptr;" << std::endl;
    ofs << "    }" << std::endl;
    // step
    ofs << "    if(!rt->next()) {" << std::endl;
    ofs << "        return nullptr;" << std::endl;
    ofs << "    }" << std::endl;
    ofs << "    " << GetAsClassName(class_name) << "::ptr v(new "
        << GetAsClassName(class_name) << ");" << std::endl;
    PARSE_OBJECT("    ");
    ofs << "    return v;" << std::endl;
    ofs << "}" << std::endl << std::endl;

    // 11、QueryByAccount 函数实现
    // 12、QueryByEmail 函数实现
    // 13、QueryByName 函数实现
    for(auto& i : m_idxs) {
        if(i->getDType() == Index::TYPE_UNIQ) {
            // 唯一索引
            ofs << "" << GetAsClassName(class_name) << "::ptr "
                << GetAsClassName(class_name_dao) << "::Query";
            // 制作函数名
            std::string tmp = "by";
            for(auto& c : i->getCols()) {
                tmp += "_" + c;
            }
            // 制作函数参数
            ofs << GetAsClassName(tmp) << "(";
            for(auto& c : i->getCols()) {
                auto d = getCol(c);
                ofs << " const " << d->getDTypeString() << "& "
                    << GetAsVariable(d->getName()) << ", ";
            }
            ofs << m_queryclass << "::ptr conn) {" << std::endl;
            // sql语句
            ofs << "    std::string sql = \"select ";
            is_first = true;
            for(auto& i : m_cols) {
                if(!is_first) {
                    ofs << ", ";
                }
                ofs << i->getName();
                is_first = false;
            }
            ofs << " from " << m_name << " where";
            is_first = true;
            for(auto& x : i->getCols()) {
                if(!is_first) {
                    ofs << " and";
                }
                ofs << " " << x << " = ?";
                is_first = false;
            }
            ofs << "\";" << std::endl;
            // prepare
            CHECK_STMT("nullptr");
            // bind
            idx = 1;
            for(auto& x : i->getCols()) {
                ofs << "    stmt->" << getCol(x)->getBindString() << "(" << idx << ", ";
                ofs << GetAsVariable(x) << ");" << std::endl;
                ++idx;
            }
            // query
            ofs << "    auto rt = stmt->query();" << std::endl;
            ofs << "    if(!rt) {" << std::endl;
            ofs << "        return nullptr;" << std::endl;
            ofs << "    }" << std::endl;
            // step
            ofs << "    if(!rt->next()) {" << std::endl;
            ofs << "        return nullptr;" << std::endl;
            ofs << "    }" << std::endl;
            ofs << "    " << GetAsClassName(class_name) << "::ptr v(new "
                << GetAsClassName(class_name) << ");" << std::endl;
            PARSE_OBJECT("    ");
            ofs << "    return v;" << std::endl;
            ofs << "}" << std::endl << std::endl;
        } else if(i->getDType() == Index::TYPE_INDEX) {
            // 普通索引
            ofs << "int " << GetAsClassName(class_name_dao) << "::Query";
            // 制作函数名
            std::string tmp = "by";
            for(auto& c : i->getCols()) {
                tmp += "_" + c;
            }
            ofs << GetAsClassName(tmp) << "(";
            // 制作函数参数
            ofs << "std::vector<" << GetAsClassName(class_name) << "::ptr>& results, ";
            for(auto& c : i->getCols()) {
                auto d = getCol(c);
                ofs << " const " << d->getDTypeString() << "& "
                    << GetAsVariable(d->getName()) << ", ";
            }
            ofs << m_queryclass << "::ptr conn) {" << std::endl;
            // sql语句
            ofs << "    std::string sql = \"select ";
            is_first = true;
            for(auto& i : m_cols) {
                if(!is_first) {
                    ofs << ", ";
                }
                ofs << i->getName();
                is_first = false;
            }
            ofs << " from " << m_name << " where";
            is_first = true;
            for(auto& x : i->getCols()) {
                if(!is_first) {
                    ofs << " and";
                }
                ofs << " " << x << " = ?";
                is_first = false;
            }
            ofs << "\";" << std::endl;
            // prepare
            CHECK_STMT("conn->getErrno()");
            // bind
            idx = 1;
            for(auto& x : i->getCols()) {
                ofs << "    stmt->" << getCol(x)->getBindString() << "(" << idx << ", ";
                ofs << GetAsVariable(x) << ");" << std::endl;
                ++idx;
            }
            // query
            ofs << "    auto rt = stmt->query();" << std::endl;
            ofs << "    if(!rt) {" << std::endl;
            ofs << "        return 0;" << std::endl;
            ofs << "    }" << std::endl;
            // step
            ofs << "    while (rt->next()) {" << std::endl;
            ofs << "        " << GetAsClassName(class_name) << "::ptr v(new "
                << GetAsClassName(class_name) << ");" << std::endl;
            PARSE_OBJECT("        ");
            ofs << "        results.push_back(v);" << std::endl;
            ofs << "    };" << std::endl;
            ofs << "    return 0;" << std::endl;
            ofs << "}" << std::endl << std::endl;
        }
    }

    // 14、CreateTableSQLite3 函数实现
    ofs << "int " << GetAsClassName(class_name_dao) << "::CreateTableSQLite3(" << m_dbclass << "::ptr conn) {" << std::endl;
    // 执行sql语句
    ofs << "    return conn->execute(\"CREATE TABLE " << m_name << "(\"" << std::endl;
    is_first = true;
    bool has_auto_increment = false;
    // 创建表结构的sql语句 begin
    for(auto& i : m_cols) {
        if(!is_first) {
            ofs << ",\"" << std::endl;
        }
        ofs << "            \"" << i->getName() << " " << i->getSQLite3TypeString();
        if(i->isAutoIncrement()) {
            ofs << " PRIMARY KEY AUTOINCREMENT";
            has_auto_increment = true;
        } else {
            ofs << " NOT NULL DEFAULT " << i->getSQLite3Default();
        }
        is_first = false;
    }
    // 如果表中列全都没有自增属性，则将所有主键设为自增
    if(!has_auto_increment) {
        ofs << ", PRIMARY KEY(";
        is_first = true;
        for(auto& i : pks) {
            if(!is_first) {
                ofs << ", ";
            }
            ofs << i->getName();
        }
        ofs << ")";
    }
    ofs << ");\"" << std::endl;
    // 创建表结构的sql语句 end
    // 创建表索引的sql语句 begin
    for(auto& i : m_idxs) {
        // 跳过主键索引
        if(i->getDType() == Index::TYPE_PK) {
            continue;
        }
        ofs << "            \"CREATE";
        // 唯一索引
        if(i->getDType() == Index::TYPE_UNIQ) {
            ofs << " UNIQUE";
        }
        ofs << " INDEX " << m_name;
        // 制作索引名称
        for(auto& x : i->getCols()) {
            ofs << "_" << x;
        }
        ofs << " ON " << m_name
            << "(";
        is_first = true;
        for(auto& x : i->getCols()) {
            if(!is_first) {
                ofs << ",";
            }
            ofs << x;
            is_first = false;
        }
        ofs << ");\"" << std::endl;
    }
    // 创建表索引的sql语句 end
    ofs << "            );" << std::endl;
    ofs << "}" << std::endl << std::endl;

    // 15、CreateTableMySQL 函数实现
    ofs << "int " << GetAsClassName(class_name_dao) << "::CreateTableMySQL(" << m_dbclass << "::ptr conn) {" << std::endl;
    // 执行sql语句
    ofs << "    return conn->execute(\"CREATE TABLE " << m_name << "(\"" << std::endl;
    is_first = true;
    // 创建表结构的sql语句 begin
    for(auto& i : m_cols) {
        if(!is_first) {
            ofs << ",\"" << std::endl;
        }
        ofs << "            \"`" << i->getName() << "` " << i->getMySQLTypeString();
        if(i->isAutoIncrement()) {
            ofs << " AUTO_INCREMENT";
            has_auto_increment = true;
        } else {
            ofs << " NOT NULL DEFAULT " << i->getSQLite3Default();
        }
        if(!i->getUpdate().empty()) {
            ofs << " ON UPDATE " << i->getUpdate() << " ";
        }
        if(!i->getDesc().empty()) {
            ofs << " COMMENT '" << i->getDesc() << "'";
        }
        is_first = false;
    }
    // 设置主键
    ofs << ",\"" << std::endl << "            \"PRIMARY KEY(";
    is_first = true;
    for(auto& i : pks) {
        if(!is_first) {
            ofs << ", ";
        }
        ofs << "`" << i->getName() << "`";
    }
    ofs << ")";
    // 创建表结构的sql语句 end
    // 创建表索引的sql语句 begin
    for(auto& i : m_idxs) {
        // 跳过主键索引
        if(i->getDType() == Index::TYPE_PK) {
            continue;
        }
        ofs << ",\"" << std::endl;
        // 唯一索引
        if(i->getDType() == Index::TYPE_UNIQ) {
            ofs << "            \"UNIQUE ";
        } else {
            ofs << "            \"";
        }
        ofs << "KEY `" << m_name;
        // 制作索引名称
        for(auto& x : i->getCols()) {
            ofs << "_" << x;
        }
        ofs << "` (";
        is_first = true;
        for(auto& x : i->getCols()) {
            if(!is_first) {
                ofs << ",";
            }
            ofs << "`" << x << "`";
            is_first = false;
        }
        ofs << ")";
    }
    ofs << ")";
    // 表注释
    if(!m_desc.empty()) {
        ofs << " COMMENT='" << m_desc << "'";
    }
    // 创建表索引的sql语句 end
    ofs << "\");" << std::endl;
    ofs << "}";
}

}
}
