#ifndef __SYLAR_JSON_UTIL_H__
#define __SYLAR_JSON_UTIL_H__

#include <string>
#include <json/json.h>

namespace sylar {

class JsonUtil {
public:
    /**
     * @brief 将json格式字符串解析成Json::Value对象
     * @param[out] json 解析后输出的Json::Value对象
     * @param[in] str 要解析的json格式字符串
     * @return 返回是否解析成功
    */
    static bool FromString(Json::Value& json, const std::string& str);

    /**
     * @brief 将Json::Value对象解析成json格式字符串
     * @param[in] json 要解析的Json::Value对象
     * @return 解析后的json格式字符串
    */
    static std::string ToString(const Json::Value& json);

};

}

#endif