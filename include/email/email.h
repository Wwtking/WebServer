#ifndef __SYLAR_EMAIL_EMAIL_H__
#define __SYLAR_EMAIL_EMAIL_H__

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace sylar {

/**
 * @brief 邮件附件类封装
*/
class EMailEntity {
public:
    typedef std::shared_ptr<EMailEntity> ptr;

    /**
     * @brief 创建邮件附件
     * @param[in] filename 附件路径
    */
    static EMailEntity::ptr CreateAttach(const std::string& filename);

    /**
     * @brief 添加头部信息
    */
    void addHeader(const std::string& key, const std::string& val);

    /**
     * @brief 获取头部信息
    */
    std::string getHeader(const std::string& key, const std::string& def = "");

    /**
     * @brief 获取附件内容
    */
    const std::string& getContent() const { return m_content; }

    /**
     * @brief 设置附件内容
    */
    void setContent(const std::string& v) { m_content = v; }

    /**
     * @brief 字符串形式输出
    */
    std::string toString() const;

private:
    /// 邮件头部信息
    std::map<std::string, std::string> m_headers;
    /// 附件内容
    std::string m_content;
};

/**
 * @brief 邮件类封装
*/
class EMail {
public:
    typedef std::shared_ptr<EMail> ptr;

    /**
     * @brief 创建邮件
     * @param[in] from_address 发信人的邮箱
     * @param[in] from_passwd 发信人的邮箱密码
     * @param[in] title 邮件标题
     * @param[in] body 邮件内容
     * @param[in] to_address 收信人的邮箱
     * @param[in] cc_address 抄送人的邮件地址
     * @param[in] bcc_address 密送人的邮件地址
     * @return 返回邮件封装
    */
    static EMail::ptr Create(const std::string& from_address, const std::string& from_passwd
                            ,const std::string& title, const std::string& body
                            ,const std::vector<std::string>& to_address
                            ,const std::vector<std::string>& cc_address = {}
                            ,const std::vector<std::string>& bcc_address = {});

    /**
     * @brief get型获取函数
    */
    const std::string& getFromEMailAddress() const { return m_fromEMailAddress; }
    const std::string& getFromEMailPasswd() const { return m_fromEMailPasswd; }
    const std::string& getTitle() const { return m_title; }
    const std::string& getBody() const { return m_body; }
    const std::vector<std::string>& getToEMailAddress() const { return m_toEMailAddress; }
    const std::vector<std::string>& getCcEMailAddress() const { return m_ccEMailAddress; }
    const std::vector<std::string>& getBccEMailAddress() const { return m_bccEMailAddress; }

    /**
     * @brief set型设置函数
    */
    void setFromEMailAddress(const std::string& v) { m_fromEMailAddress = v; }
    void setFromEMailPasswd(const std::string& v) { m_fromEMailPasswd = v; }
    void setTitle(const std::string& v) { m_title = v; }
    void setBody(const std::string& v) { m_body = v; }
    void setToEMailAddress(const std::vector<std::string>& v) { m_toEMailAddress = v; }
    void setCcEMailAddress(const std::vector<std::string>& v) { m_ccEMailAddress = v; }
    void setBccEMailAddress(const std::vector<std::string>& v) { m_bccEMailAddress = v; }

    /**
     * @brief 添加附件
    */
    void addEntity(EMailEntity::ptr entity);
    /**
     * @brief 获取附件
    */
    const std::vector<EMailEntity::ptr>& getEntitys() const { return m_entitys; }
    
private:
    std::string m_fromEMailAddress;             // 发信人的邮箱
    std::string m_fromEMailPasswd;              // 发信人的邮箱密码
    std::string m_title;                        // 邮件标题
    std::string m_body;                         // 邮件内容
    std::vector<std::string> m_toEMailAddress;  // 收信人的邮箱
    std::vector<std::string> m_ccEMailAddress;  // 抄送（CC）人的邮件地址
    std::vector<std::string> m_bccEMailAddress; // 密送（BCC）人的邮件地址
    std::vector<EMailEntity::ptr> m_entitys;    // 附件
};

}

#endif
