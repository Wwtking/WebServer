#ifndef __SYLAR_HTTP_SERVLET_H__
#define __SYLAR_HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string.h>
#include "http.h"
#include "http_session.h"
#include "thread.h"

namespace sylar {
namespace http {


/**
 * @brief Servlet封装，基类
 * @details HTTP请求路径到处理类的映射，用于规范化的HTTP消息处理
 *          不同的请求路径对应不同的Servlet，处理方式不同
 *          要实现不同的Servlet都要继承基类，重新实现handle()
*/
class HttpServlet {
public:
    typedef std::shared_ptr<HttpServlet> ptr;

    /**
     * @brief 构造函数
     * @param[in] name 名称
     */
    HttpServlet(std::string name);

    /**
     * @brief 析构函数
     */
    virtual ~HttpServlet() {}

    /**
     * @brief 处理请求
     * @param[in] req HTTP请求
     * @param[in] res HTTP响应
     * @param[in] session HTTP连接
     * @return 是否处理成功
     */
    virtual int32_t handle(HttpRequest::ptr req, 
                           HttpResponse::ptr res, 
                           HttpSession::ptr session) = 0;

    /**
     * @brief 返回Servlet名称
     */
    const std::string& getName() const { return m_name; }

    /**
     * @brief 设置Servlet名称
     */
    void setName(const std::string& name) { m_name = name; }

protected:
    std::string m_name;     // 名称
};


/**
 * @brief 函数式Servlet
 * @details 提供了Function方式支持Servlet，而并非只能用继承来实现
 */
class FunctionServlet : public HttpServlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t(HttpRequest::ptr req, 
                                HttpResponse::ptr res, 
                                HttpSession::ptr session)> FuncType;

    /**
     * @brief 构造函数
     * @param[in] cb 回调函数
     */
    FunctionServlet(FuncType cb);

    /**
     * @brief 函数式的处理请求
     * @param[in] req HTTP请求
     * @param[in] res HTTP响应
     * @param[in] session HTTP连接
     * @return 是否处理成功
     */
    virtual int32_t handle(HttpRequest::ptr req, 
                           HttpResponse::ptr res, 
                           HttpSession::ptr session) override;

private:
    FuncType m_cb;      // 回调函数
};


/**
 * @brief Not Found类型的Servlet(默认返回404)
 * @details 说明没有找到该HTTP请求路径对应的处理类
 */
class NotFoundServlet : public HttpServlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    /**
     * @brief 构造函数
     */
    NotFoundServlet(const std::string& serverName);

    /**
     * @brief Not Found时的处理请求
     * @param[in] req HTTP请求
     * @param[in] res HTTP响应
     * @param[in] session HTTP连接
     * @return 是否处理成功
     */
    virtual int32_t handle(HttpRequest::ptr req, 
                           HttpResponse::ptr res, 
                           HttpSession::ptr session) override;

private:
    std::string m_name;     // 服务器名称
    std::string m_content;  // 响应消息体
};


/**
 * @brief Servlet分发器
 * @details 管理Servlet，用于指定某个请求路径该用哪个Servlet来处理
 */
class ServletDispatch : public HttpServlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex MutexType;

    /**
     * @brief 构造函数
     */
    ServletDispatch();

    /**
     * @brief 处理请求
     * @param[in] req HTTP请求
     * @param[in] res HTTP响应
     * @param[in] session HTTP连接
     * @return 是否处理成功
     */
    virtual int32_t handle(HttpRequest::ptr req, 
                           HttpResponse::ptr res, 
                           HttpSession::ptr session) override;

    /**
     * @brief 添加精准匹配servlet
     * @param[in] uri uri
     * @param[in] slt serlvet
     */
    void addServlet(const std::string& uri, HttpServlet::ptr servlet);

    /**
     * @brief 添加精准匹配servlet
     * @param[in] uri uri
     * @param[in] cb FunctionServlet回调函数
     */
    void addServlet(const std::string& uri, FunctionServlet::FuncType cb);

    /**
     * @brief 添加模糊匹配servlet
     * @param[in] uri uri 模糊匹配 /wwt*
     * @param[in] slt servlet
     */
    void addGlobServlet(const std::string& uri, HttpServlet::ptr servlet);

    /**
     * @brief 添加模糊匹配servlet
     * @param[in] uri uri 模糊匹配 /wwt*
     * @param[in] cb FunctionServlet回调函数
     */
    void addGlobServlet(const std::string& uri, FunctionServlet::FuncType cb);

    /**
     * @brief 删除精准匹配servlet
     * @param[in] uri uri
     */
    void delServlet(const std::string& uri);

    /**
     * @brief 删除模糊匹配servlet
     * @param[in] uri uri
     */
    void delGlobServlet(const std::string& uri);

    /**
     * @brief 通过uri获取精准匹配servlet
     * @param[in] uri uri
     * @return 返回对应的servlet
     */
    HttpServlet::ptr getServlet(const std::string& uri);

    /**
     * @brief 通过uri获取模糊匹配servlet
     * @param[in] uri uri
     * @return 返回对应的servlet
     */
    HttpServlet::ptr getGlobServlet(const std::string& uri);

    /**
     * @brief 通过uri获取servlet
     * @param[in] uri uri
     * @return 优先精准匹配,其次模糊匹配,最后返回默认
     */
    HttpServlet::ptr getMatchServlet(const std::string& uri);

    /**
     * @brief 返回默认servlet
     */
    HttpServlet::ptr getDefault() const { return m_default; }

    /**
     * @brief 设置默认servlet
     * @param[in] v servlet
     */
    void setDefault(HttpServlet::ptr servlet) { m_default = servlet; }

private:
    MutexType m_mutex;              // 读写互斥量
    std::unordered_map<std::string, HttpServlet::ptr> m_datas;      // 精准匹配
    std::vector<std::pair<std::string, HttpServlet::ptr> > m_globs; // 模糊匹配
    HttpServlet::ptr m_default;     // 默认servlet，所有路径都没匹配到时使用
};

}
}

#endif