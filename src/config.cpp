#include "config.h"
#include "env.h"
#include "util.h"
#include "log.h"
#include <list>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static void ListAllMember(const std::string& prefix, const YAML::Node& node,
                        std::list<std::pair<std::string, const YAML::Node> >& nodes) {
    if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._0123456789-")
            != std::string::npos) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    nodes.push_back(std::make_pair(prefix, node));
    if(node.IsMap()) {
        for(auto it = node.begin(); it != node.end(); ++it) {
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), 
                            it->second, nodes);
        }
    }
}

//使用YAML::Node初始化配置模块
void Config::LoadFromYaml(const YAML::Node& root) {
    std::list<std::pair<std::string, const YAML::Node> > all_nodes;
    ListAllMember("", root, all_nodes);

    for(const auto& i : all_nodes) {
        std::string name = i.first;
        if(name.empty()) {
            continue;
        }

        //std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()) {
            if(i.second.IsScalar()) {
                it->second->fromString(i.second.Scalar());  // it->second 为 ConfigVarBase::ptr 类型
            } else {
                std::stringstream ss;
                ss << i.second;
                it->second->fromString(ss.str());    //转换失败会触发异常处理
            }
        }
    }
}

// 记录文件的最近修改时间
static std::map<std::string, uint64_t> s_FileModifyTime;
static Mutex s_mutex;

//从Conf文件中加载YAML初始化配置
void Config::LoadFromConfDir(const std::string& path, bool force) {
    std::string absolute_path = EnvMgr::GetInstance()->getAbsolutePath(path);
    std::vector<std::string> files;
    FilesUtil::ListAllFiles(files, absolute_path, ".yml");

    for(auto& i : files) {
        {
            struct stat st;
            /**
             * @brief int lstat(const char *pathname, struct stat *buf);
             * @details 获取linux操作系统下文件的属性
             * @param[in] pathname 文件的绝对路径或相对路径
             * @param[out] buf 结构体指针存放文件属性
            */
            lstat(i.c_str(), &st);
            Mutex::Lock lock(s_mutex);
            if(!force && s_FileModifyTime[i] == (uint64_t)st.st_mtime) {
                continue;
            }
            s_FileModifyTime[i] = st.st_mtime;
        }
        try {
            // YAML配置
            YAML::Node root = YAML::LoadFile(i);
            LoadFromYaml(root);
            SYLAR_LOG_INFO(g_logger) << "LoadConfFile file="
                                    << i << " ok";
        }
        catch(...) {
            SYLAR_LOG_ERROR(g_logger) << "LoadConfFile file="
                                    << i << " failed";
        }
    }
}

//遍历配置模块里面所有配置项
void Config::visit(std::function<void(ConfigVarBase::ptr)> cb) {
    RWMutexType::ReadLock lock(GetMutex());
    for(const auto& i : GetDatas()) {
        cb(i.second);
    }
}


};
