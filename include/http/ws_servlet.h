#ifndef __SYLAR_WS_SERVLET_H__
#define __SYLAR_WS_SERVLET_H__

#include "http_servlet.h"
#include "ws_session.h"

namespace sylar {
namespace http {


class WSServlet : public HttpServlet {
public:
    typedef std::shared_ptr<WSServlet> ptr;

    /**
     * @brief 构造函数
     * @param[in] name 名称
     */
    WSServlet(const std::string& name);

    /**
     * @brief 析构函数
     */
    virtual ~WSServlet() {}

    /**
     * @brief 处理请求
     * @param[in] req HTTP请求
     * @param[in] res HTTP响应
     * @param[in] session HTTP连接
     * @return 是否处理成功
     */
    virtual int32_t handle(HttpRequest::ptr req, 
                           HttpResponse::ptr res, 
                           HttpSession::ptr session) override {
        return 0;
    }

    /**
     * @brief 握手建立连接成功的回调函数
     * @param[in] req HTTP请求
     * @param[in] session WebSocket连接
     * @return 是否处理成功
     */
    virtual int32_t onConnect(HttpRequest::ptr req, 
                              WSSession::ptr session) = 0;

    /**
     * @brief 双向数据传输时的处理回调函数
     * @param[in] req HTTP请求
     * @param[in] msg 接收到的WS消息体封装
     * @param[in] session WebSocket连接
     * @return 是否处理成功
     */
    virtual int32_t handle(HttpRequest::ptr req, 
                           WSFrameMessage::ptr msg,
                           WSSession::ptr session) = 0;

    /**
     * @brief 挥手关闭连接成功的回调函数
     * @param[in] req HTTP请求
     * @param[in] session WebSocket连接
     * @return 是否处理成功
     */
    virtual int32_t onClose(HttpRequest::ptr req, 
                            WSSession::ptr session) = 0;

    /**
     * @brief 返回WSServlet名称
     */
    const std::string& getName() const { return m_name; }

    /**
     * @brief 设置WSServlet名称
     */
    void setName(const std::string& name) { m_name = name; }

};


class FunctionWSServlet : public WSServlet {
public:
    typedef std::shared_ptr<FunctionWSServlet> ptr;
    typedef std::function<int32_t(HttpRequest::ptr req, 
                                  WSFrameMessage::ptr msg,
                                  WSSession::ptr session)> handle_cb;
    typedef std::function<int32_t(HttpRequest::ptr req, 
                                  WSSession::ptr session)> onConnect_cb;
    typedef std::function<int32_t(HttpRequest::ptr req, 
                                  WSSession::ptr session)> onClose_cb;
    
    /**
     * @brief 构造函数
     */
    FunctionWSServlet(handle_cb handle
                    , onConnect_cb onConnect = nullptr
                    , onClose_cb onClose = nullptr);

    /**
     * @brief 握手建立连接成功的回调函数(函数式)
     * @param[in] req HTTP请求
     * @param[in] session WebSocket连接
     * @return 是否处理成功
     */
    virtual int32_t onConnect(HttpRequest::ptr req, 
                              WSSession::ptr session) override;

    /**
     * @brief 双向数据传输时的处理回调函数(函数式)
     * @param[in] req HTTP请求
     * @param[in] msg 接收到的WS消息体封装
     * @param[in] session WebSocket连接
     * @return 是否处理成功
     */
    virtual int32_t handle(HttpRequest::ptr req, 
                           WSFrameMessage::ptr msg,
                           WSSession::ptr session) override;

    /**
     * @brief 挥手关闭连接成功的回调函数(函数式)
     * @param[in] req HTTP请求
     * @param[in] session WebSocket连接
     * @return 是否处理成功
     */
    virtual int32_t onClose(HttpRequest::ptr req, 
                            WSSession::ptr session) override;

protected:
    handle_cb m_handle;         // 数据处理回调
    onConnect_cb m_onConnect;   // 握手建立连接成功的回调
    onClose_cb m_onClose;       // 挥手关闭连接成功的回调
};


class WSServletDispatch : public ServletDispatch {
public:
    typedef std::shared_ptr<WSServletDispatch> ptr;

    /**
     * @brief 构造函数
     */
    WSServletDispatch();

    /**
     * @brief 添加精准匹配servlet
     * @param[in] uri uri
     * @param[in] cb FunctionWSServlet回调函数
     */
    void addServlet(const std::string& uri
                  , FunctionWSServlet::handle_cb handle
                  , FunctionWSServlet::onConnect_cb onConnect = nullptr
                  , FunctionWSServlet::onClose_cb onClose = nullptr);

    /**
     * @brief 添加模糊匹配servlet
     * @param[in] uri uri 模糊匹配 /wwt*
     * @param[in] cb FunctionWSServlet回调函数
     */
    void addGlobServlet(const std::string& uri
                      , FunctionWSServlet::handle_cb handle
                      , FunctionWSServlet::onConnect_cb onConnect = nullptr
                      , FunctionWSServlet::onClose_cb onClose = nullptr);

    /**
     * @brief 通过uri获取WSServlet
     * @param[in] uri uri
     * @return 优先精准匹配,其次模糊匹配,最后返回默认
     */
    WSServlet::ptr getMatchWSServlet(const std::string& uri);
};

}
}

#endif