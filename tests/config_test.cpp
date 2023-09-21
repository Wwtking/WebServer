#include <iostream>
#include <yaml-cpp/yaml.h>
#include <vector>
#include "config.h"
#include "log.h"
#include "env.h"

//NodeType：enum value { Undefined, Null, Scalar, Sequence, Map };
//遍历yaml内容
void printf_yaml(YAML::Node node, int level) {
    switch(node.Type()) {
        case YAML::NodeType::Null:
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') 
            << "NULL - " << node.Type() << " - " << level;
            break;
        case YAML::NodeType::Scalar:
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') 
                << node.Scalar() << " - " << node.Type() << " - " << level;
            break;
        case YAML::NodeType::Sequence:
            for(size_t i = 0; i < node.size(); ++i) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') 
                    << i << " - " << node[i].Type() << " - " << level;
                printf_yaml(node[i], level + 1);
            }
        break;
        case YAML::NodeType::Map:
            for(auto it = node.begin(); it != node.end(); ++it) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') 
                    << it->first << " - " << it->second.Type() << " - " << level;
                printf_yaml(it->second, level + 1);
            }
            break;
        default:
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "NodeType invalid : " << node.Type();
            break;
    }
}

/**
 * @brief 测试内置数据类型
 * @details int  float  vector  list  set  unordered_set  map<string,T>  unordered_map<string,T>
*/
void yaml_test01() {
    //约定(约定优于配置)
    sylar::ConfigVar<int>::ptr g_int_value_config = 
        sylar::Config::Lookup("system.port", (int)8080, "system port");
    sylar::ConfigVar<float>::ptr g_float_value_config = 
        sylar::Config::Lookup("system.value", (float)10.2f, "system value");
    sylar::ConfigVar<std::vector<int> >::ptr g_vector_int_config = 
        sylar::Config::Lookup("system.vector_int", std::vector<int>{1, 2}, "system vector_int");
    sylar::ConfigVar<std::list<int> >::ptr g_list_int_config = 
        sylar::Config::Lookup("system.list_int", std::list<int>{3, 4}, "system list_int");
    sylar::ConfigVar<std::set<int> >::ptr g_set_int_config = 
        sylar::Config::Lookup("system.set_int", std::set<int>{5, 6}, "system set_int");
    sylar::ConfigVar<std::unordered_set<int> >::ptr g_uset_int_config = 
        sylar::Config::Lookup("system.uset_int", std::unordered_set<int>{7, 8}, "system uset_int");
    sylar::ConfigVar<std::map<std::string, int> >::ptr g_map_str_int_config = 
        sylar::Config::Lookup("system.map_str_int", std::map<std::string, int>{{"k", 9}}, "system map_str_int");
    sylar::ConfigVar<std::unordered_map<std::string, int> >::ptr g_umap_str_int_config = 
        sylar::Config::Lookup("system.umap_str_int", std::unordered_map<std::string, int>{{"k", 10}}, "system umap_str_int");

    // 回调函数测试
    // g_int_value_config 中的 m_cbs 和 g_float_value_config 中的 m_cbs 没有任何关联
    // 所以在调用 setValue 函数时，各自遍历各自的 m_cbs
    g_int_value_config->addListener([](const int& oldInfo, const int& newInfo) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << oldInfo 
                                        << " new_value=" << newInfo;
    });
    // g_int_value_config->addListener([](const int& oldInfo, const int& newInfo) {
    //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << 0 
    //                                     << " new_value=" << 1;
    // });
    g_float_value_config->addListener([](const float& oldInfo, const float& newInfo) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << oldInfo 
                                        << " new_value=" << newInfo;
    });

//定义宏函数来测试
#define FUNC(g_ptr, name, prefix) \
    { \
        for(const auto& i : g_ptr->getValue()) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ":" << i; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ":" << g_ptr->toString(); \
    }

#define FUNC2(g_ptr, name, prefix) \
    { \
        for(const auto& i : g_ptr->getValue()) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ":{" << i.first \
                            << " - " << i.second << "}"; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ":" << g_ptr->toString(); \
    }

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();
    FUNC(g_vector_int_config, vector_int, before);
    FUNC(g_list_int_config, list_int, before);
    FUNC(g_set_int_config, set_int, before);
    FUNC(g_uset_int_config, uset_int, before);
    FUNC2(g_map_str_int_config, map_str_int, before);
    FUNC2(g_umap_str_int_config, umap_str_int, before);

    //配置(约定优于配置)
    YAML::Node root = YAML::LoadFile("/home/wwt/sylar/bin/conf/test.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();
    FUNC(g_vector_int_config, vector_int, after);
    FUNC(g_list_int_config, list_int, after);
    FUNC(g_set_int_config, set_int, after);
    FUNC(g_uset_int_config, uset_int, after);
    FUNC2(g_map_str_int_config, map_str_int, after);
    FUNC2(g_umap_str_int_config, umap_str_int, after);

#undef FUNC
#undef FUNC2

    //printf_yaml(root, 0);
    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}



//自定义数据类型Person
class Person {
public:
    std::string m_name = "";
    int m_age = 0;
    bool m_gender = 0;
public:
    std::string toString() const {
        std::stringstream ss;
        ss << "Person:" << " name=" << m_name
            << " age=" << m_age
            << " gender=" << m_gender;
        return ss.str();
    }

    bool operator==(const Person& p) const {
        return (m_name == p.m_name && 
                m_age == p.m_age &&
                m_gender == p.m_gender);
    }
};

/**
 * @details 自定义类型需要实现 sylar::LexicalCast 片特化
 *          实现后,就可以支持Config解析自定义类型
 *          自定义类型可以和常规stl容器一起嵌套使用,无论嵌套多少层,都可以递归地解析出来
*/
namespace sylar {

//类型转换模板类片特化(YAML String 转换成 Person)
template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& s) {
        Person p ;
        YAML::Node node = YAML::Load(s);
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_gender = node["gender"].as<bool>();
        return p;
    }
};
//类型转换模板类片特化(Person 转换成 YAML String)
template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node(YAML::NodeType::Map);
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["gender"] = p.m_gender;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

};

/**
 * @brief 测试自定义数据类型
 * @details Person  map<string, Person>  
*/
void yaml_test02() {
    //约定(约定优于配置)
    sylar::ConfigVar<Person>::ptr g_person_config = 
        sylar::Config::Lookup("class.person", Person(), "class person");
    sylar::ConfigVar<std::map<std::string, Person> >::ptr g_map_person_config = 
        sylar::Config::Lookup("class.map", std::map<std::string, Person>(), "class map person");
    sylar::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_map_vec_person_config = 
        sylar::Config::Lookup("class.map_vec", std::map<std::string, std::vector<Person> >(), "class map vector");

    //回调函数测试
    g_person_config->addListener([](const Person& oldInfo, const Person& newInfo) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << oldInfo.toString() 
                                        << " new_value=" << newInfo.toString();
    });

//定义宏函数来测试
#define FUNC(g_ptr, prefix) \
    { \
        for(const auto& it : g_ptr->getValue()) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ": [" << it.first << " - " \
                                            << it.second.toString() << "]"; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ": size=" <<g_ptr->getValue().size(); \
    }

#define FUNC2(g_ptr, prefix) \
    { \
        for(const auto& it : g_ptr->getValue()) { \
            for(const auto& i : it.second) { \
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ": [" << it.first << " - " \
                                            << i.toString() << "]"; \
            } \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ": " << it.first \
                                            << " vector_size=" << it.second.size(); \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ": map_size=" << g_ptr->getValue().size(); \
    }

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person_config->toString();
    FUNC(g_map_person_config, before);
    FUNC2(g_map_vec_person_config, before);
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_map_vec_person_config->toString();

    //配置(约定优于配置)
    YAML::Node root = YAML::LoadFile("/home/wwt/sylar/bin/conf/test.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person_config->toString();
    FUNC(g_map_person_config, after);
    FUNC2(g_map_vec_person_config, after);
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_map_vec_person_config->toString();

#undef  FUNC
#undef  FUNC2

}

void yaml_log_test() {
    static sylar::Logger::ptr logger = SYLAR_LOG_NAME("system");
    SYLAR_LOG_INFO(logger) << "hello yaml";
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;

    YAML::Node root = YAML::LoadFile("/home/wwt/sylar/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);
    
    SYLAR_LOG_INFO(logger) << "hello yaml";
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;

    //(1)若logger中appenders中的formatter未在yaml中说明，则其会默认用logger中的formatter
    //   之后要是logger中的formatter修改，appenders中的formatter也会随之修改
    //(2)若logger中appenders中的formatter在yaml中说明，则其和logger中的formatter就无关
    //   修改logger中的formatter，appenders中的formatter不变
    logger->setFormatter("%d - %m%n");
    SYLAR_LOG_INFO(logger) << "hello yaml";
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
}

void yaml_file_test(int argc, char** argv) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::Config::LoadFromConfDir("conf");
    sleep(5);
    sylar::Config::LoadFromConfDir("conf");
}

int main(int argc, char** argv) {

    //yaml_log_test();
    //yaml_test01();
    //yaml_test02();
    yaml_file_test(argc, argv);

    // visit测试
    // sylar::Config::visit([](sylar::ConfigVarBase::ptr var) {
    //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "name=" << var->getName()
    //                                 << " value=" << var->toString();
    // });
}

