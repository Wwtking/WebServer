#include "smtp.h"
#include "log.h"
#include <set>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// 构造函数
SmtpClient::SmtpClient(Socket::ptr sock)
    :sylar::SocketStream(sock) {
}

// 创建发送邮件的类封装
SmtpClient::ptr SmtpClient::Create(const std::string& host, uint32_t port, bool ssl) {
    // SMTP服务器地址封装
    sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress(host);
    if(!addr) {
        SYLAR_LOG_ERROR(g_logger) << "invalid smtp server: " << host << ":" << port
            << " ssl=" << ssl;
        return nullptr;
    }
    addr->setPort(port);    // 绑定端口

    // 创建socket封装
    Socket::ptr sock;
    if(ssl) {
        sock = sylar::SSLSocket::CreatTcpSSLSocket(addr);
    } else {
        sock = sylar::Socket::CreatTcpSocket(addr);
    }

    // socket连接
    if(!sock->connect(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "connect smtp server: " << host << ":" << port
            << " ssl=" << ssl << " fail";
        return nullptr;
    }

    // 读取数据
    // 客户端首次连接到SMTP服务器时，通常会收到服务器发送的欢迎消息
    // 例如 "220 smtp.example.com ESMTP Postfix"
    std::string buf;
    buf.resize(1024);
    SmtpClient::ptr rt(new SmtpClient(sock));
    int len = rt->read(&buf[0], buf.size());
    if(len <= 0) {
        return nullptr;
    }
    buf.resize(len);
    // SYLAR_LOG_ERROR(g_logger) << "len=" << len << " buf=" << buf;

    // 收到状态码220表示连接成功
    if(sylar::TypeUtil::Atoi(buf) != 220) {
        return nullptr;
    }
    rt->m_host = host;
    return rt;
}

// 发送cmd命令，并等待接收SMTP服务器响应
SmtpResult::ptr SmtpClient::doCmd(const std::string& cmd, bool debug) {
    // 发送cmd命令
    if(writeFixSize(cmd.c_str(), cmd.size()) <= 0) {
        return std::make_shared<SmtpResult>(SmtpResult::IO_ERROR, "write io error");
    }

    // 接收回应数据
    std::string buf;
    buf.resize(4096);
    auto len = read(&buf[0], buf.size());
    if(len <= 0) {
        return std::make_shared<SmtpResult>(SmtpResult::IO_ERROR, "read io error");
    }
    buf.resize(len);

    // 保存debug信息
    if(debug) {
        m_ss << "C: " << cmd;
        m_ss << "S: " << buf;
    }

    // SMTP服务器状态码：
    // 2xx: 表示操作成功完成
    // 3xx: 表示需要进一步操作来完成请求
    // 4xx: 表示请求存在问题，但是可以尝试重新发送
    // 5xx: 表示请求存在问题，无法成功处理
    int code = sylar::TypeUtil::Atoi(buf);
    if(code >= 400) {
        return std::make_shared<SmtpResult>(code, sylar::replace(
                        buf.substr(buf.find(' ') + 1), "\r\n", ""));
    }
    return nullptr;
}

SmtpResult::ptr SmtpClient::send(EMail::ptr email, int64_t timeout_ms, bool debug) {
    SmtpResult::ptr result;
#define DO_CMD() \
    result = doCmd(cmd, debug); \
    if(result) {\
        return result; \
    }

    // 设置发送/接收超时时间
    Socket::ptr sock = getSocket();
    if(sock) {
        sock->setRecvTimeout(timeout_ms);
        sock->setSendTimeout(timeout_ms);
    }

    // 客户端向SMTP服务器打招呼，用于建立连接并标识自己
    std::string cmd = "HELO " + m_host + "\r\n";
    DO_CMD();

    // 未通过身份验证
    if(!m_authed && !email->getFromEMailAddress().empty()) {
        // 使用用户名和密码来进行SMTP身份验证
        cmd = "AUTH LOGIN\r\n";
        DO_CMD();
        // 发送用户名
        auto pos = email->getFromEMailAddress().find('@');
        cmd = sylar::base64encode(email->getFromEMailAddress().substr(0, pos)) + "\r\n";
        DO_CMD();
        // 发送密码
        cmd = sylar::base64encode(email->getFromEMailPasswd()) + "\r\n";
        DO_CMD();

        m_authed = true;
    }

    // 指定发件人地址，用于设置邮件的寄件人
    cmd = "MAIL FROM: <" + email->getFromEMailAddress() + ">\r\n";
    DO_CMD();

    // 存放收信人、抄送人、密送人的邮箱
    std::set<std::string> targets;
#define XX(fun) \
    for(auto& i : email->fun()) { \
        targets.insert(i); \
    } 
    XX(getToEMailAddress);
    XX(getCcEMailAddress);
    XX(getBccEMailAddress);
#undef XX

    // 指定收件人地址，用于设置邮件的收件人
    for(auto& i : targets) {
        cmd = "RCPT TO: <" + i + ">\r\n";
        DO_CMD();
    }

    // 开始发送邮件的内容，在该命令后，客户端可以输入邮件的头部信息和正文内容
    cmd = "DATA\r\n";
    DO_CMD();

    auto& entitys = email->getEntitys();

    /**
     * 发送邮件命令的格式:
        From: <发件人邮箱>
        To: <收件人邮箱1>,<收件人邮箱2>,...
        Cc: <抄送人邮箱1>,<抄送人邮箱2>,...
        Subject: 标题
        Content-Type: multipart/mixed;boundary=...
        MIME-Version: 1.0
        /r/n
        --boundary
        Content-Type: text/html;charset="utf-8"
        /r/n
        body部分
        \r\n
        --boundary
        附件 ---> headers: xxx  content: xxx
        \r\n
        --boundary--
        \r\n.\r\n
    */
    std::stringstream ss;
    ss << "From: <" << email->getFromEMailAddress() << ">\r\n"
       << "To: ";
#define XX(fun) \
        do { \
            auto& v = email->fun(); \
            for(size_t i = 0; i < v.size(); ++i) { \
                if(i) { \
                    ss << ","; \
                } \
                ss << "<" << v[i] << ">"; \
            } \
            if(!v.empty()) { \
                ss << "\r\n"; \
            } \
        } while(0);
    // 指定收件人邮箱
    XX(getToEMailAddress);
    // 指定抄送人邮箱
    if(!email->getCcEMailAddress().empty()) {
        ss << "Cc: ";
        XX(getCcEMailAddress);
    }
    // 指定邮件标题
    ss << "Subject: " << email->getTitle() << "\r\n";
    // 表示邮件的内容是由多个部分组成的，每个部分可能包含不同类型的数据，如文本、图片、附件等
    // 则boundary则用于分隔不同的部分，以便SMTP服务器能够正确地解析邮件并将其传递给接收方
    std::string boundary;
    if(!entitys.empty()) {
        boundary = sylar::random_string(16);
        ss << "Content-Type: multipart/mixed;boundary="
           << boundary << "\r\n";
    }
    ss << "MIME-Version: 1.0\r\n";
    // 用boundary分隔
    if(!boundary.empty()) {
        ss << "\r\n--" << boundary << "\r\n";
    }
    // 指定邮件内容
    ss << "Content-Type: text/html;charset=\"utf-8\"\r\n"
       << "\r\n"
       << email->getBody() << "\r\n";
    // 用boundary分隔，传输附件
    for(auto& i : entitys) {
        ss << "\r\n--" << boundary << "\r\n";
        ss << i->toString();
    }
    if(!boundary.empty()) {
        ss << "\r\n--" << boundary << "--\r\n";
    }
    ss << "\r\n.\r\n";

    // 发送邮件
    cmd = ss.str();
    DO_CMD();
#undef XX
#undef DO_CMD
    return std::make_shared<SmtpResult>(0, "ok");
}

// 获取debug信息
std::string SmtpClient::getDebugInfo() {
    return m_ss.str();
}

}
