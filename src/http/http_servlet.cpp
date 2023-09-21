#include "http_servlet.h"
#include "log.h"
#include <time.h>
#include <fnmatch.h>

namespace sylar {
namespace http {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");


// HttpServlet构造函数
HttpServlet::HttpServlet(std::string name) 
    :m_name(name) {
}


// FunctionServlet构造函数
FunctionServlet::FunctionServlet(FuncType cb) 
    :HttpServlet("FunctionServlet")
    ,m_cb(cb) {
}

// 函数式的处理请求
int32_t FunctionServlet::handle(HttpRequest::ptr req, 
                                HttpResponse::ptr res, 
                                HttpSession::ptr session) {
    return m_cb(req, res, session);
}


// NotFoundServlet构造函数
NotFoundServlet::NotFoundServlet(const std::string& serverName) 
    :HttpServlet("NotFoundServlet")
    ,m_name(serverName) {
    m_content = "<html><head><title>404 Not Found</title></head>"
                "<body><center><h1>404 Not Found</h1></center>"
                "<hr><center>" + m_name + "</center></body></html>";
}

// Not Found时的处理请求
int32_t NotFoundServlet::handle(HttpRequest::ptr req, 
                                HttpResponse::ptr res, 
                                HttpSession::ptr session) {
    // HTTP/1.1 404 Not Found
    // Content-Length: 202
    // Content-Type: text/html; charset=iso-8859-1
    // Date: Thu, 24 Aug 2023 07:52:21 GMT
    // Server: Apache
    res->setStatus(HttpStatus::NOT_FOUND);
    res->setHeader("Content-Type", "text/html");
    res->setHeader("Date", TimeToStr());
    res->setHeader("Server", m_name);
    res->setBody(m_content);
    return 0;
}


// Servlet分发器的构造函数
ServletDispatch::ServletDispatch()
    :HttpServlet("ServletDispatch") {
    m_default = std::make_shared<NotFoundServlet>("wwt/1.0.0");
}

// Servlet分发器的处理请求
int32_t ServletDispatch::handle(HttpRequest::ptr req, 
                                HttpResponse::ptr res, 
                                HttpSession::ptr session) {
    // 仅仅是路径决定要选择哪个servlet，后面的参数、片段标识符没关系
    HttpServlet::ptr servlet = getMatchServlet(req->getPath());
    if(servlet) {
        return servlet->handle(req, res, session);
    }
    return 0;
}

// 添加精准匹配servlet
void ServletDispatch::addServlet(const std::string& uri, HttpServlet::ptr servlet) {
    MutexType::WriteLock lock(m_mutex);
    m_datas[uri] = servlet;
}

// 添加精准匹配servlet
void ServletDispatch::addServlet(const std::string& uri, FunctionServlet::FuncType cb) {
    MutexType::WriteLock lock(m_mutex);
    m_datas[uri] = std::make_shared<FunctionServlet>(cb);
}

// 添加模糊匹配servlet
void ServletDispatch::addGlobServlet(const std::string& uri, HttpServlet::ptr servlet) {
    MutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); it++) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, servlet));
}

// 添加模糊匹配servlet
void ServletDispatch::addGlobServlet(const std::string& uri, FunctionServlet::FuncType cb) {
    MutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); it++) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, std::make_shared<FunctionServlet>(cb)));
}

// 删除精准匹配servlet
void ServletDispatch::delServlet(const std::string& uri) {
    MutexType::WriteLock lock(m_mutex);
    auto it = m_datas.find(uri);
    m_datas.erase(it);
}

// 删除模糊匹配servlet
void ServletDispatch::delGlobServlet(const std::string& uri) {
    MutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); it++) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
}

// 通过uri获取精准匹配servlet
HttpServlet::ptr ServletDispatch::getServlet(const std::string& uri) {
    MutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    if(it != m_datas.end()) {
        return m_datas[uri];
    }
    return nullptr;
}

// 通过uri获取模糊匹配servlet
HttpServlet::ptr ServletDispatch::getGlobServlet(const std::string& uri) {
    MutexType::ReadLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); it++) {
        if(it->first == uri) {
            return it->second;
        }
    }
    return nullptr;
}

// 通过uri获取最佳匹配servlet，即优先精准匹配,其次模糊匹配,最后返回默认
HttpServlet::ptr ServletDispatch::getMatchServlet(const std::string& uri) {
    MutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    if(it != m_datas.end()) {
        return it->second;
    }
    for(auto i = m_globs.begin(); i != m_globs.end(); i++) {
        /**
         * @brief int fnmatch(const char *pattern, const char *string, int flags);
         * @param[in] pattern 模式字符串，例如 "*.txt"
         * @param[in] string 要匹配的字符串，例如 "test.txt"
         * @param[in] flags 匹配标志，一般为0
         * @return 返回0表示匹配成功，返回非0表示匹配失败
        */
        if(fnmatch(i->first.c_str(), uri.c_str(), 0) == 0) {
            return i->second;
        }
    }
    return m_default;
}


}
}