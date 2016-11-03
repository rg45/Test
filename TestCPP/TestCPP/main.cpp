#include "stdafx.h"

#include <iostream>
#include <string>
#include <tuple>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

template <typename F, typename...Args>
decltype(auto) call(F&& f, Args&&...args)
{
   return std::forward<F>(f)(std::forward<Args>(args)...);
}

template <typename T>
void print(const std::string& name, T&& value)
{
   std::cout << name << " = " << std::forward<T>(value) << std::endl;
}

class A { };
class B : public A, public boost::enable_shared_from_this<B>
{
public:
   B() { std::cout << __FUNCTION__ << std::endl; }
   ~B() { std::cout << __FUNCTION__ << std::endl; }

   boost::shared_ptr<const B> foo() const { return shared_from_this(); }
};

class C : public B, public boost::enable_shared_from_this<C>
{
public:
   C() { std::cout << __FUNCTION__ << std::endl; }
   ~C() { std::cout << __FUNCTION__ << std::endl; }
};

template<typename T>
void bar(T&& t)
{
   *t;
}

int main()
{
//    auto tuple = std::make_tuple("Pi", 3.14);
//    call(print<decltype(std::get<double>(tuple))>, std::get<const char*>(tuple), std::get<double>(tuple));

   boost::shared_ptr<const C> c = boost::make_shared<C>();
   boost::shared_ptr<const B> b = c->foo();

   bar(b);
   bar(c);

}
