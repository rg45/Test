#include "stdafx.h"

#include <iostream>
#include <string>
#include <tuple>
#include <functional>

#include <boost/noncopyable.hpp>

#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)

template <typename F, typename...Args>
decltype(auto) invoke(F&& f, Args&&...args)
{
   return std::forward<F>(f)(std::forward<Args>(args)...);
}

template <typename T>
void print(const std::string& name, T&& value)
{
   std::cout << name << " = " << std::forward<T>(value) << std::endl;
}

template<size_t i, typename Head, typename...Tail>
struct SelectByIndex
{
   decltype(auto) operator()(Head&& head, Tail&&...tail)
   {
      return SelectByIndex<i - 1, Tail...>()(std::forward<Tail>(tail)...);
   }
};

template<typename Head, typename...Tail>
struct SelectByIndex<0, Head, Tail...>
{
   decltype(auto) operator()(Head&& head, Tail&&...) { return std::forward<Head>(head); }
};

template<bool, typename T, typename Head, typename...Tail>
struct SelectByTypeBase;

template<typename T, typename Head, typename...Tail>
using SelectByType = SelectByTypeBase<std::is_convertible<Head, T>::value, T, Head, Tail...>;

template<bool, typename T, typename Head, typename...Tail>
struct SelectByTypeBase
{
   decltype(auto) operator()(Head&& head, Tail&&...tail)
   {
      return SelectByType<T, Tail...>()(std::forward<Tail>(tail)...);
   }
};

template<typename T, typename Head, typename...Tail>
struct SelectByTypeBase<true, T, Head, Tail...>
{
   decltype(auto) operator()(Head&& head, Tail&&...) { return std::forward<Head>(head); }
};

template <size_t i, typename...List>
decltype(auto) select(List&&...list) { return SelectByIndex<i, List...>()(std::forward<List>(list)...); }

template <typename T, typename...List>
decltype(auto) select(List&&...list) { return SelectByType<T, List...>()(std::forward<List>(list)...); }


int main()
{
   auto tuple = std::make_tuple("Pi", 3.14);
   invoke(print<decltype(std::get<double>(tuple))>, std::get<const char*>(tuple), std::get<double>(tuple));

   PRINT(sizeof(std::declval<int>()));

   double pi = asin(2.0);

   PRINT(select<1>(1, pi, "Hello!"));
   select<1>(1, pi, "Hello!") = 3.14;
   PRINT(select<1>(1, pi, "Hello!"));

   struct A : boost::noncopyable
   {
      explicit A(double value = 0) : value(value) { }
      double value;
   };

   PRINT(select<0>(A(2.71)).value);

   A a1(3.14);
   PRINT(select<std::string>(a1, "Hello, World!!!", A()));
   PRINT(select<const A&>(a1, "Hello, World!!!", A()).value);
   PRINT(select<const A&>(a1, "Hello, World!!!", A()).value = 22.4);
   PRINT(select<const A&>(a1, "Hello, World!!!", A()).value);

}
