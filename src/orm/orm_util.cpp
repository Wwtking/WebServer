#include "orm_util.h"
#include "util.h"
#include "hash_util.h"

namespace sylar {
namespace orm {

/**
 * @brief 将名称v写出普通变量名格式
*/
std::string GetAsVariable(const std::string& v) {
    return sylar::ToLower(v);
}

/**
 * @brief 将名称v用'_'分割，各段用首字符大写拼接
 * @example create_time --> CreateTime
*/
std::string GetAsClassName(const std::string& v) {
    auto vs = sylar::split(v, '_');
    std::stringstream ss;
    for(auto& i : vs) {
        i[0] = toupper(i[0]);   // 大写
        ss << i;
    }
    return ss.str();
}

/**
 * @brief 将名称v写出类成员变量格式
 * @example create_time --> m_createTime
*/
std::string GetAsMemberName(const std::string& v) {
    auto class_name = GetAsClassName(v);
    class_name[0] = tolower(class_name[0]);   // 小写
    return "m_" + class_name;
}

/**
 * @brief 将名称v写出get型类成员函数名格式
 * @example create_time --> getCreateTime
*/
std::string GetAsGetFunName(const std::string& v) {
    auto class_name = GetAsClassName(v);
    return "get" + class_name;
}

/**
 * @brief 将名称v写出set型类成员函数名格式
 * @example create_time --> setCreateTime
*/
std::string GetAsSetFunName(const std::string& v) {
    auto class_name = GetAsClassName(v);
    return "set" + class_name;
}

std::string XmlToString(const tinyxml2::XMLNode& node) {
    return "";
}

/**
 * @brief 将名称v写出宏指令名称格式
 * @example blog.data.user_info.h --> __BLOG_DATA_USER_INFO_H__
*/
std::string GetAsDefineMacro(const std::string& v) {
    std::string tmp = sylar::replace(v, '.', '_');
    tmp = sylar::ToUpper(tmp);
    return "__" + tmp + "__";
}

}
}
