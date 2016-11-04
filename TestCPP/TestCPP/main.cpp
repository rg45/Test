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

template<size_t i, typename First, typename...Rest>
struct SelectByIndex
{
   decltype(auto) operator()(First&& first, Rest&&...rest)
   {
      return SelectByIndex<i - 1, Rest...>()(std::forward<Rest>(rest)...);
   }
};

template<typename First, typename...Rest>
struct SelectByIndex<0, First, Rest...>
{
   decltype(auto) operator()(First&& first, Rest&&...) { return std::forward<First>(first); }
};

template<bool, typename T, typename First, typename...Rest>
struct SelectByTypeBase;

template<typename T, typename First, typename...Rest>
using SelectByType = SelectByTypeBase<std::is_convertible<First, T>::value, T, First, Rest...>;

template<bool, typename T, typename First, typename...Rest>
struct SelectByTypeBase
{
   decltype(auto) operator()(First&& first, Rest&&...rest)
   {
      return SelectByType<T, Rest...>()(std::forward<Rest>(rest)...);
   }
};

template<typename T, typename First, typename...Rest>
struct SelectByTypeBase<true, T, First, Rest...>
{
   decltype(auto) operator()(First&& first, Rest&&...) { return std::forward<First>(first); }
};

template <size_t i, typename...TT>
decltype(auto) select(TT&&...tt) { return SelectByIndex<i, TT...>()(std::forward<TT>(tt)...); }

template <typename T, typename...TT>
decltype(auto) select(TT&&...tt) { return SelectByType<T, TT...>()(std::forward<TT>(tt)...); }


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
