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
struct SelectorByIndex
{
   decltype(auto) operator()(First&& first, Rest&&...rest)
   {
      return SelectorByIndex<i - 1, Rest...>()(std::forward<Rest>(rest)...);
   }
};

template<typename First, typename...Rest>
struct SelectorByIndex<0, First, Rest...>
{
   decltype(auto) operator()(First&& first, Rest&&...) { return std::forward<First>(first); }
};

template <size_t i, typename...T>
decltype(auto) select(T&&...t) { return SelectorByIndex<i, T...>()(std::forward<T>(t)...); }


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

}
