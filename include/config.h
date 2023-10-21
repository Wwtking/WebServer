#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <typeinfo>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "log.h"
#include "thread.h"


namespace sylar {

// 配置变量的基类
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    //构造函数
    ConfigVarBase(const std::string& name, const std::string& description = "") 
        :m_name(name),
        m_description(description) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(),::tolower);
    }

    //虚析构函数
    virtual ~ConfigVarBase() {}

    //返回配置参数的名称
    const std::string& getName() const { return m_name; }

    //返回配置参数的描述
    const std::string& getDescription() const { return m_description; }

    //转成字符串
    virtual std::string toString() = 0;

    //从字符串初始化值
    virtual bool fromString(const std::string& val) = 0;

    //获取T的类型
    virtual std::string getType() const = 0;

private:
    std::string m_name;    //配置参数的名称
    std::string m_description;    //配置参数的描述

};


/**
 * @brief 类型转换模板类(F 源类型, T 目标类型)
 */
template<class F, class T>
class LexicalCast {
public:
    T operator()(const F& v) {
        return boost::lexical_cast<T>(v);
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::vector<T>)
 */
template<class T>
class LexicalCast<std::string, std::vector<T> > {
public:
    std::vector<T> operator()(const std::string& s) {
        typename std::vector<T> vec;
        YAML::Node node = YAML::Load(s);
        for(size_t i = 0; i < node.size(); ++i) {
            //vec.push_back(LexicalCast<std::string, T>()(node[i].Scalar()));  //仅支持简单类型
            std::stringstream ss;
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));  //支持复杂类型
        }
        return vec;
    }
};
/**
 * @brief 类型转换模板类片特化(std::vector<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(const auto& i : v) {
            //node.push_back(LexicalCast<T, std::string>()(i));    //直接将string放入node
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));  //封装成Node再放入node
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::list<T>)
 */
template<class T>
class LexicalCast<std::string, std::list<T> > {
public:
    std::list<T> operator()(const std::string& s) {
        typename std::list<T> vec;
        YAML::Node node = YAML::Load(s);
        for(size_t i = 0; i < node.size(); ++i) {
            //vec.push_back(LexicalCast<std::string, T>()(node[i].Scalar()));  //仅支持简单类型
            std::stringstream ss;
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));  //支持复杂类型
        }
        return vec;
    }
};
/**
 * @brief 类型转换模板类片特化(std::list<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(const auto& i : v) {
            //node.push_back(LexicalCast<T, std::string>()(i));    //直接将string放入node
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));  //封装成Node再放入node
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::set<T>)
 */
template<class T>
class LexicalCast<std::string, std::set<T> > {
public:
    std::set<T> operator()(const std::string& s) {
        typename std::set<T> vec;
        YAML::Node node = YAML::Load(s);
        for(size_t i = 0; i < node.size(); ++i) {
            //vec.insert(LexicalCast<std::string, T>()(node[i].Scalar()));  //仅支持简单类型
            std::stringstream ss;
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));  //支持复杂类型
        }
        return vec;
    }
};
/**
 * @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(const auto& i : v) {
            //node.push_back(LexicalCast<T, std::string>()(i));    //直接将string放入node
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));  //封装成Node再放入node
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
 */
template<class T>
class LexicalCast<std::string, std::unordered_set<T> > {
public:
    std::unordered_set<T> operator()(const std::string& s) {
        typename std::unordered_set<T> vec;
        YAML::Node node = YAML::Load(s);
        for(size_t i = 0; i < node.size(); ++i) {
            //vec.insert(LexicalCast<std::string, T>()(node[i].Scalar()));  //仅支持简单类型
            std::stringstream ss;
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));  //支持复杂类型
        }
        return vec;
    }
};
/**
 * @brief 类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for(const auto& i : v) {
            //node.push_back(LexicalCast<T, std::string>()(i));    //直接将string放入node
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));  //封装成Node再放入node
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::map<std::string, T>)
 */
template<class T>
class LexicalCast<std::string, std::map<std::string, T> > {
public:
    std::map<std::string, T> operator()(const std::string& s) {
        typename std::map<std::string, T> vec;
        YAML::Node node = YAML::Load(s);
        for(auto it = node.begin(); it != node.end(); ++it) {
            std::stringstream ss;
            ss << it->second;
            vec[it->first.Scalar()] = LexicalCast<std::string, T>()(ss.str());
        }
        return vec;
    }
};
/**
 * @brief 类型转换模板类片特化(std::map<std::string, T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for(const auto& i : v) {
            //node[i.first] = LexicalCast<T, std::string>()(i.second);
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_map<std::string, T>)
 */
template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T> > {
public:
    std::unordered_map<std::string, T> operator()(const std::string& s) {
        typename std::unordered_map<std::string, T> vec;
        YAML::Node node = YAML::Load(s);
        for(auto it = node.begin(); it != node.end(); ++it) {
            std::stringstream ss;
            ss << it->second;
            vec[it->first.Scalar()] = LexicalCast<std::string, T>()(ss.str());
        }
        return vec;
    }
};
/**
 * @brief 类型转换模板类片特化(std::unordered_map<std::string, T> 转换成 YAML String)
 */
template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for(const auto& i : v) {
            //node[i.first] = LexicalCast<T, std::string>()(i.second);
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


//FromStr: T operator()(const std::string&)
//ToStr: std::string operator()(const T&)
//配置参数模板子类,保存对应类型的参数值
template<class T, class FromStr = LexicalCast<std::string, T>
                ,class ToStr = LexicalCast<T, std::string> >
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void(const T& oldVal, const T& newVal)> on_change_cb;
    typedef RWMutex RWMutexType;

    //构造函数，其中name参数名称有效字符为[0-9a-z_.]
    ConfigVar(const std::string& name, const T& val, const std::string& description = "") 
        :ConfigVarBase(name, description),
        m_val(val) {
    }

    //获取当前参数的值
    const T& getValue() const { 
        RWMutexType::ReadLock lock(m_mutex);
        return m_val; 
    }

    //设置当前参数的值
    void setValue(const T& val) {
        {
            RWMutexType::ReadLock lock(m_mutex);
            if(val == m_val) {
                return;
            }
            for(auto& it : m_cbs) {       //遍历所有的回调函数
                it.second(m_val, val);
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        m_val = val;
    }

    //将参数的值转成字符串，当转换失败时抛出异常
    std::string toString() override {
        try {
            //return boost::lexical_cast<std::string>(m_val);
            RWMutexType::ReadLock lock(m_mutex);
            return ToStr()(m_val);
        } 
        catch (boost::bad_lexical_cast& e) {    //lexical_cast无法执行转换操作时会抛出异常bad_lexical_cast
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception "
                << e.what() << " convert: " << typeid(T).name() << " to string";
        }
        return "";
    }

    //从string转成参数的值，当转换失败抛出异常
    bool fromString(const std::string& val) override {
        try {
            //m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
            return true;
        }
        catch (boost::bad_lexical_cast& e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception "
                << e.what() << " convert: " << "string to " << typeid(T).name();
        }
        return false;
    }

    // 获取T的类型
    std::string getType() const override { return typeid(T).name(); }

    /**
     * @brief 添加变化回调函数
     * @return 返回该回调函数对应的唯一id,用于删除回调函数
    */
    uint64_t addListener(on_change_cb cb) {
        static uint64_t cb_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++cb_id;
        m_cbs[cb_id] = cb;
        return cb_id;
    }

    //根据id删除回调函数
    void delListener(uint64_t id) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(id);
    }

    //根据id获取回调函数
    on_change_cb getListener(uint64_t id) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(id);
        return it != m_cbs.end() ? it->second : nullptr;
    }

    //清理所有的回调函数
    void clearListener() {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

private:
    T m_val;    //对应类型的参数值
    std::map<uint64_t, on_change_cb> m_cbs;  //变更回调函数组,key唯一
    mutable RWMutexType m_mutex;    //读写锁
};


//ConfigVar的管理类，提供便捷的方法创建/访问ConfigVar
class Config {
public: 
    typedef std::shared_ptr<Config> ptr;
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    typedef RWMutex RWMutexType;

    /**
     * @brief 获取/创建对应参数名的配置参数
     * @details 获取参数名为name的配置参数,如果存在直接返回(name不区分大小写)
     *          如果不存在,创建参数配置并用default_value赋值
     * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
     * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
    */
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name, 
            const T& val, const std::string& description = "") {
        RWMutexType::WriteLock lock(GetMutex());
        // std::string transform_name;    //全部转换为小写后的name
        // std::transform(name.begin(), name.end(), transform_name.begin(), ::tolower);
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);   //强转失败返回NULL
            if(tmp) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            } else {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
                            << typeid(T).name() << "; real_type=" << it->second->getType() 
                            << " value=" << it->second->toString();
                return nullptr;
            }
        }
        //abcdefghikjlmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789
        if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789")
                != std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid --- " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr p(std::make_shared<ConfigVar<T> >(name, val, description));
        GetDatas()[name] = p;
        return p; 
    } 

    //查找参数名为name的配置参数，若存在返回配置参数，不存在返回nullptr
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it == GetDatas().end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
    }

    //使用YAML::Node初始化配置模块
    static void LoadFromYaml(const YAML::Node& root);

    //从Conf文件中加载YAML初始化配置
    static void LoadFromConfDir(const std::string& path, bool force = false);

    /**
     * @brief 遍历配置模块里面所有配置项
     *        根据输入的函数,去查看s_datas数据
     * @param[in] cb 配置项回调函数
     */
    static void visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
    /**
     * @brief 返回所有的配置项
     * @details 不要直接定义成静态成员变量，如 static ConfigVarMap s_datas;
     *          因为上述静态成员函数 Lookup 中用到了 s_datas，
     *          但是在 Lookup函数静态生成初始化 和 s_datas变量初始化时，
     *          不确定哪个先被初始化，若 Lookup 先被初始化，则肯定会出问题。
     *  解决方法：用静态方法返回静态变量，在 Lookup 中使用 GetDatas()，
     *          则使得 s_datas 肯定先被初始化。
     */
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;    //所有的配置项
        return s_datas;
    }

    //配置项的RWMutex
    static RWMutexType& GetMutex() {
        static RWMutexType m_mutex;
        return m_mutex;
    }
};



};



#endif