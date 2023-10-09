#include "json_util.h"

namespace sylar{

// 将json格式字符串解析成Json::Value对象
// Json::Value相当于是一个map容器类：key - value(value中也可能是键值对)
bool JsonUtil::FromString(Json::Value& json, const std::string& str) {
    Json::Reader reader;
    // 调用reader的成员函数parse，把str中的数据解析到json中
    return reader.parse(str, json);
}


// 将Json::Value对象解析成json格式字符串
std::string JsonUtil::ToString(const Json::Value& json) {
    // 输出json字符串数据
    // {"key1":"value1", "key2":1, "key3":false}
    Json::FastWriter writer;
    return writer.write(json);

    // 输出有格式的json字符串数据
    // {
    //     "key1" : "value1",
    //     "key2" : 1, 
    //     "key3" : false
    // }
    // Json::StyledWriter writer;
    // return writer.write(json);
}
    
} // namespace sylar
