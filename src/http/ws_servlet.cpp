#include "ws_servlet.h"

namespace sylar {
namespace http {


// WSServlet构造函数
WSServlet::WSServlet(const std::string& name) 
    :HttpServlet(name) {
}

// FunctionWSServlet构造函数
FunctionWSServlet::FunctionWSServlet(handle_cb handle
                                    , onConnect_cb onConnect
                                    , onClose_cb onClose) 
    :WSServlet("FunctionWSServlet")
    ,m_handle(handle)
    ,m_onConnect(onConnect)
    ,m_onClose(onClose) {
}

// 握手建立连接成功的回调函数
int32_t FunctionWSServlet::onConnect(HttpRequest::ptr req, 
                                    WSSession::ptr session) {
    if(m_onConnect) {
        return m_onConnect(req, session);
    }
    return 0;
}

// 双向数据传输时的处理回调函数
int32_t FunctionWSServlet::handle(HttpRequest::ptr req, 
                                WSFrameMessage::ptr msg,
                                WSSession::ptr session) {
    if(m_handle) {
        return m_handle(req, msg, session);
    }
    return 0;
}

// 挥手关闭连接成功的回调函数
int32_t FunctionWSServlet::onClose(HttpRequest::ptr req, 
                                WSSession::ptr session) {
    if(m_onClose) {
        return m_onClose(req, session);
    }
    return 0;
}

// WSServletDispatch构造函数
WSServletDispatch::WSServletDispatch() {
    m_name = "WSServletDispatch";
}

// 添加精准匹配servlet
void WSServletDispatch::addServlet(const std::string& uri
                    , FunctionWSServlet::handle_cb handle
                    , FunctionWSServlet::onConnect_cb onConnect
                    , FunctionWSServlet::onClose_cb onClose) {
    ServletDispatch::addServlet(uri, std::make_shared<FunctionWSServlet>(handle, onConnect, onClose));
}

// 添加模糊匹配servlet
void WSServletDispatch::addGlobServlet(const std::string& uri
                    , FunctionWSServlet::handle_cb handle
                    , FunctionWSServlet::onConnect_cb onConnect
                    , FunctionWSServlet::onClose_cb onClose) {
    ServletDispatch::addServlet(uri, std::make_shared<FunctionWSServlet>(handle, onConnect, onClose));
}

// 通过uri获取WSServlet
WSServlet::ptr WSServletDispatch::getMatchWSServlet(const std::string& uri) {
    auto servlet = getMatchServlet(uri);
    return std::dynamic_pointer_cast<WSServlet>(servlet);
}

}
}