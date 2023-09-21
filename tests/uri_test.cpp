#include "uri.h"

int main(int argc, char** argv) {
    // sylar::Uri::ptr uri = sylar::Uri::CreateUri("http://wwt@www.sylar.top/test/uri?id=100&name=sylar#frg");
    sylar::Uri::ptr uri = sylar::Uri::CreateUri("http://admin@www.sylar.top/test/中文/uri?id=100&name=sylar&vv=中文#frg中文");
    // sylar::Uri::ptr uri = sylar::Uri::CreateUri("http://admin@www.sylar.top");
    // sylar::Uri::ptr uri = sylar::Uri::CreateUri("http://www.sylar.top/test/uri");
    std::cout << uri->toString() << std::endl;

    auto addr = uri->getAddress();
    std::cout << addr->toString() << std::endl;

    return 0;
}