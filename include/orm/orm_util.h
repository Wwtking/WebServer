#ifndef __SYLAR_ORM_UTIL_H__
#define __SYLAR_ORM_UTIL_H__

#include <tinyxml2.h>
#include <string>

namespace sylar {
namespace orm {

/**
 * @brief 将名称v写出普通变量名格式
*/
std::string GetAsVariable(const std::string& v);

/**
 * @brief 将名称v用'_'分割，各段用首字符大写拼接
 * @example create_time --> CreateTime
*/
std::string GetAsClassName(const std::string& v);

/**
 * @brief 将名称v写出类成员变量格式
 * @example create_time --> m_createTime
*/
std::string GetAsMemberName(const std::string& v);

/**
 * @brief 将名称v写出get型类成员函数名格式
 * @example create_time --> getCreateTime
*/
std::string GetAsGetFunName(const std::string& v);

/**
 * @brief 将名称v写出set型类成员函数名格式
 * @example create_time --> setCreateTime
*/
std::string GetAsSetFunName(const std::string& v);

std::string XmlToString(const tinyxml2::XMLNode& node);

/**
 * @brief 将名称v写出宏指令名称格式
 * @example blog.data.user_info.h --> __BLOG_DATA_USER_INFO_H__
*/
std::string GetAsDefineMacro(const std::string& v);

}
}

#endif
