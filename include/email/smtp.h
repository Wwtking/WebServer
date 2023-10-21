#ifndef __SYLAR_EMAIL_SMTP_H__
#define __SYLAR_EMAIL_SMTP_H__

#include "socket_stream.h"
#include "email.h"
#include <sstream>

namespace sylar {

/**
 * @brief 发送邮件命令的结果封装
*/
struct SmtpResult {
    typedef std::shared_ptr<SmtpResult> ptr;

    enum Result {
        OK = 0,         // 成功
        IO_ERROR = -1   // 出错
    };

    // 构造函数
    SmtpResult(int r, const std::string& m)
        :result(r)
        ,msg(m) {
    }

    int result;         // 状态码
    std::string msg;    // 原因
};

/**
 * @brief 发送邮件的类封装
 * @details 提供了与SMTP服务器进行通信的功能，以便将邮件发送到目标收件人
*/
class SmtpClient : public sylar::SocketStream {
public:
    typedef std::shared_ptr<SmtpClient> ptr;

    /**
     * @brief 创建发送邮件的类封装
     * @param[in] host SMTP服务器地址
     * @param[in] port 服务器端口
     * @param[in] ssl 是否为ssl
    */
    static SmtpClient::ptr Create(const std::string& host, uint32_t port, bool ssl = false);

    /**
     * @brief 发送邮件
     * @param[in] email 要发送的邮件信息
     * @param[in] timeout_ms 超时时间
     * @param[in] debug 是否保存debug信息
     * @return 返回发送结果
    */
    SmtpResult::ptr send(EMail::ptr email, int64_t timeout_ms = 1000, bool debug = false);

    /**
     * @brief 获取debug信息
    */
    std::string getDebugInfo();

private:
    /**
     * @brief 发送cmd命令，并等待接收SMTP服务器响应
     * @param[in] cmd 发送命令
     * @param[in] debug 是否保存debug信息
     * @return 返回发送结果
    */
    SmtpResult::ptr doCmd(const std::string& cmd, bool debug);

private:
    /**
     * @brief 构造函数
    */
    SmtpClient(Socket::ptr sock);

private:
    std::string m_host;         // SMTP服务器地址
    std::stringstream m_ss;     // 存放debug信息
    bool m_authed = false;      // 是否已通过身份验证(即邮箱密码验证)
};

}

#endif
