#ifndef __SYLAR_URI_H__
#define __SYLAR_URI_H__

#include <iostream>
#include <memory>
#include <string.h>
#include "address.h"

namespace sylar {

/*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
*/


class Uri {
public:
    typedef std::shared_ptr<Uri> ptr;

    static Uri::ptr CreateUri(const std::string& uriStr);

    Uri();
    
    const std::string& getScheme() const { return m_scheme; }
    const std::string& getUserinfo() const { return m_userinfo; }
    const std::string& getHost() const { return m_host; }
    const std::string& getPath() const;
    const std::string& getQuery() const { return m_query; }
    const std::string& getFragment() const { return m_fragment; }
    uint32_t getPort() const;

    void setScheme(const std::string& scheme) { m_scheme = scheme; }
    void setUserinfo(const std::string& userinfo) { m_userinfo = userinfo; }
    void setHost(const std::string& host) { m_host = host; }
    void setPath(const std::string& path) { m_path = path; }
    void setQuery(const std::string& query) { m_query = query; }
    void setFragment(const std::string& fragment) { m_fragment = fragment; }
    void setPort(uint32_t port) { m_port = port; }

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;

    Address::ptr getAddress() const;

private:
    bool isDefaultPort() const;

private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    uint32_t m_port;
};

}

#endif