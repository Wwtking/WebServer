#include "uri.h"
#include <sstream>


namespace sylar {

%%{
    # See RFC 3986: http://www.ietf.org/rfc/rfc3986.txt

    machine uri_parser;

    # 用于mark标记
    action marku {
        mark = fpc;
    }

    # mark标记解析起始位置,fpc解析末尾的位置,fpc-mark就是实际scheme的长度
    action save_scheme {
        // std::cout << "scheme=" << std::string(mark, fpc - mark) << std::endl;
        uri->setScheme(std::string(mark, fpc - mark));
        mark = NULL;
    }

    action save_userinfo {
        // std::cout << "userinfo=" << std::string(mark, fpc - mark) << std::endl;
        if(mark != fpc) {
            uri->setUserinfo(std::string(mark, fpc - mark));
        }
        mark = NULL;
    }

    action save_host {
        // std::cout << "host=" << std::string(mark, fpc - mark) << std::endl;
        uri->setHost(std::string(mark, fpc - mark));
        mark = NULL;
    }

    action save_port {
        // std::cout << "port=" << std::string(mark, fpc - mark) << std::endl;
        if(mark != fpc) {
            uri->setPort(atoi(mark));
        }
        mark = NULL;
    }

    action save_path {
        // std::cout << "path=" << std::string(mark, fpc - mark) << std::endl;
        if(mark != fpc) {
            uri->setPath(std::string(mark, fpc - mark));
        }
        mark = NULL;
    }

    action save_query {
        // std::cout << "query=" << std::string(mark, fpc - mark) << std::endl;
        if(mark != fpc) {
            uri->setQuery(std::string(mark, fpc - mark));
        }
        mark = NULL;
    }

    action save_fragment {
        // std::cout << "fragment=" << std::string(mark, fpc - mark) << std::endl;
        if(mark != fpc) {
            uri->setFragment(std::string(mark, fpc - mark));
        }
        mark = NULL;
    }

    gen_delims = ":" | "/" | "?" | "#" | "[" | "]" | "@";
    sub_delims = "!" | "$" | "&" | "'" | "(" | ")"
                    | "*" | "+" | "," | ";" | "=";
    reserved = gen_delims | sub_delims;
    unreserved = alpha | digit | "_" | "." | "_" | "~";
    pct_encoded = "%" xdigit xdigit;

    dec_octet   = digit             # 0-9
                | [1-9] digit       # 10-99
                | "1" digit{2}      # 100-199
                | 2 [0-4] digit     # 200-249
                | "25" [0-5];       # 250-255
    IPv4address = dec_octet "." dec_octet "." dec_octet "." dec_octet;
    h16 = xdigit{1,4};
    ls32 = (h16 ":" h16) | IPv4address;
    IPv6address = (                         (h16 ":"){6} ls32) |
                  (                    "::" (h16 ":"){5} ls32) |
                  ((             h16)? "::" (h16 ":"){4} ls32) |
                  (((h16 ":"){1} h16)? "::" (h16 ":"){3} ls32) |
                  (((h16 ":"){2} h16)? "::" (h16 ":"){2} ls32) |
                  (((h16 ":"){3} h16)? "::" (h16 ":"){1} ls32) |
                  (((h16 ":"){4} h16)? "::"              ls32) |
                  (((h16 ":"){5} h16)? "::"              h16 ) |
                  (((h16 ":"){6} h16)? "::"                  );
    
    IPvFuture = "v" xdigit+ "." (unreserved | sub_delims | ":")+;
    IP_literal = "[" (IPv6address | IPvFuture) "]";

    # 解析scheme
    scheme = (alpha (alpha | digit | "+" | "_" | ".")*) >marku %save_scheme;

    # 解析userinfo
    userinfo = ((unreserved | pct_encoded | sub_delims | ":")*) >marku %save_userinfo;

    reg_name = (unreserved | pct_encoded | sub_delims)*;
    # 解析host
    host = (IP_literal | IPv4address | reg_name) >marku %save_host;

    # 解析port
    port = (digit*) >marku %save_port;

    authority = (userinfo  "@")? host (":" port)?;

    pchar = ((any -- ascii) | unreserved | pct_encoded | sub_delims | ":" | "@");
    segment = pchar*;
    segment_nz = pchar+;
    segment_nz_nc = (unreserved | pct_encoded | sub_delims | "@")+;

    path_abempty  = ("/" segment)*;
    path_absolute = "/" (segment_nz ("/" segment)*)?;
    path_noscheme = segment_nz_nc ("/" segment)*;
    path_rootless = segment_nz ("/" segment)*;
    path_empty    = "";

    path = path_abempty | path_absolute | path_noscheme | path_rootless | path_empty;

    # 解析query
    query = ((pchar | "/" | "?")*) >marku %save_query;

    # 解析fragment
    fragment = ((pchar | "/" | "?")*) >marku %save_fragment;

    # 解析path
    hier_part = ("//" authority path_abempty >marku %save_path)
                | path_absolute
                | path_rootless
                | path_empty;
    
    # 解析uri
    URI = scheme ":" hier_part ("?" query)? ("#" fragment)?;
    
    relative_part = ("//" authority path_abempty)
                    | path_absolute
                    | path_noscheme
                    | path_empty;
    relative_ref  = relative_part ("?" query)? ("#" fragment)?;

    URI_reference = URI | relative_ref;

    main := URI_reference;

    write data;
}%%

Uri::ptr Uri::CreateUri(const std::string& uriStr) {
    Uri::ptr uri = std::make_shared<Uri>();
    int cs = 0;             // 解析过程的状态码
    const char* mark = 0;   // 解析过程的标记

    %% write init;

    const char* p = uriStr.c_str();     // 状态机处理字符串的起始地址
    const char* pe = p + uriStr.size(); // 状态机处理字符串的结束地址
    const char* eof = pe;               // 字符串末尾

    %% write exec;

    if(cs == uri_parser_error) {    // 解析出错
        return nullptr;
    }
    else if(cs >= uri_parser_first_final) { // 解析完成
        return uri;
    }

    return nullptr;
}

Uri::Uri() 
    :m_port(0) {
}

const std::string& Uri::getPath() const {
    static std::string _path_str = "/";
    return m_path.empty() ? _path_str : m_path;
}

uint32_t Uri::getPort() const {
    if(m_port) {
        return m_port;
    }
    if(m_scheme == "http" || m_scheme == "ws") {
        return 80;
    }
    else if(m_scheme == "https" || m_scheme == "wss") {
        return 443;
    }
    return m_port;
}

std::ostream& Uri::dump(std::ostream& os) const {
    os << m_scheme << "://" 
        << m_userinfo 
        << (m_userinfo.empty() ? "" : "@")
        << m_host
        << (isDefaultPort() ? "" : (":" + std::to_string(m_port)))
        << getPath()
        << (m_query.empty() ? "" : "?")
        << m_query
        << (m_fragment.empty() ? "" : "#")
        << m_fragment;
    return os;
}

std::string Uri::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

Address::ptr Uri::getAddress() const {
    auto addr = Address::LookupAnyIPAddress(m_host);
    if(addr) {
        addr->setPort(getPort());
    }
    return addr;
}

bool Uri::isDefaultPort() const {
    if(m_port == 0) {
        return true;
    }
    if(m_scheme == "http" || m_scheme == "ws") {
        return m_port == 80;
    }
    else if(m_scheme == "https" || m_scheme == "wss") {
        return m_port == 443;
    }
    return false;
}

}