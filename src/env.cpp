#include "env.h"
#include "log.h"
#include <iomanip>
#include <unistd.h>
#include <string.h>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

bool Env::init(int argc, char** argv) {
    m_program = argv[0];

    char link[1024] = {0};
    char path[1024] = {0};
    sprintf(link, "/proc/%d/exe", getpid());
    readlink(link, path, sizeof(path));     // 将读到的软链接转换为实际路径
    m_exe = path;

    int pos = m_exe.find_last_of('/');
    m_cwd = m_exe.substr(0, pos);

    pos = m_cwd.find_last_of('/');
    m_workPath = m_cwd.substr(0, pos);

    m_cwd += "/";
    m_workPath += "/";

    // 分离出各参数，并存放
    char* now_key = nullptr;
    // -config /path/to/config -file xxx -d -s
    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            if(strlen(argv[i]) > 1) {
                if(now_key) {
                    addArg(now_key, "");
                }
                now_key = argv[i] + 1;
            }
            else {
                SYLAR_LOG_ERROR(g_logger) << "invalid arg, index=" << i
                                          << " arg=" << argv[i];
                return false;
            }
        }
        else {
            if(now_key) {
                addArg(now_key, argv[i]);
                now_key = nullptr;
            }
            else {
                SYLAR_LOG_ERROR(g_logger) << "invalid arg, index=" << i
                                          << " arg=" << argv[i];
                return false;
            }
        }
    }
    if(now_key) {
        addArg(now_key, "");
    }
    return true;
}

void Env::addArg(const std::string& key, const std::string& val) {
    RWMutex::WriteLock lock(m_mutex);
    m_args[key] = val;
}

bool Env::hasArg(const std::string& key) {
    RWMutex::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    if(it != m_args.end()) {
        return true;
    }
    return false;
}

void Env::delArg(const std::string& key) {
    RWMutex::WriteLock lock(m_mutex);
    m_args.erase(key);
}

const std::string& Env::getArg(const std::string& key, const std::string& def) {
    RWMutex::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    if(it != m_args.end()) {
        return it->second;
    }
    return def;
}

void Env::printArgs() {
    RWMutex::ReadLock lock(m_mutex);
    for(auto& it : m_args) {
        SYLAR_LOG_DEBUG(g_logger) << "-" << it.first << " : " << it.second;
    }
}

void Env::addHelp(const std::string& key, const std::string& val) {
    delHelp(key);
    RWMutex::WriteLock lock(m_mutex);
    m_helps.push_back(std::make_pair(key, val));
}

void Env::delHelp(const std::string& key) {
    RWMutex::WriteLock lock(m_mutex);
    for(auto it = m_helps.begin(); it != m_helps.end(); ++it) {
        if(it->first == key) {
            m_helps.erase(it);
            break;
        }
    }
}

void Env::printHelps() {
    std::cout << "Usage: " << m_program << " [options]" << std::endl;
    RWMutex::ReadLock lock(m_mutex);
    for(auto& it : m_helps) {
        std::cout << std::setw(5) << "-" << it.first << " : " << it.second << std::endl;
    }
}

bool Env::setEnv(const std::string& name, const std::string& value) {
    return !setenv(name.c_str(), value.c_str(), 1);
}

std::string Env::getEnv(const std::string& name, const std::string& def) {
    std::string val = getenv(name.c_str());
    if(val.empty()) {
        return def;
    }
    return val;
}

std::string Env::getAbsolutePath(const std::string& path) {
    if(path.empty()) {
        return m_workPath;
    }
    if(path[0] == '/') {
        return path;
    }
    return m_workPath + path;
}

std::string Env::getDefaultConfPath() {
    return m_workPath + "conf";
}

std::string Env::getConfPath() {
    std::string get_conf = getArg("c");
    return get_conf.empty() ? getDefaultConfPath() : (getCwd() + get_conf);
}

}