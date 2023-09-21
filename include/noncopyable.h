#ifndef __SYLAR_NONCOPYABLE_H__
#define __SYLAR_NONCOPYABLE_H__

namespace sylar {

/**
 * @brief 对象无法拷贝、赋值
 * @details 所以继承该类的派生类也无法用拷贝和赋值，
 *          因为派生类在执行拷贝、赋值操作时会调用基类的拷贝、赋值操作
 */
class Noncopyable {
protected:
    //将函数声明为"=default"函数，编译器将为其自动生成默认的函数定义体，提高代码的执行效率
    //默认构造函数
    Noncopyable() = default;  

    //默认析构函数
    ~Noncopyable() = default;

private:
    //拷贝构造函数(禁用)
    Noncopyable(const Noncopyable&) = delete;

    //赋值函数(禁用)
    Noncopyable& operator=(const Noncopyable&) = delete;
};

}

#endif
