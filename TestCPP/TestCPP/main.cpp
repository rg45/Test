#include "stdafx.h"

#include <iostream>
#include <string>
#include <functional>

#include <boost/noncopyable.hpp>

// Development/Debug utility
#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)
template <typename T> void printType() { PRINT(__FUNCSIG__); }
template <typename T> void printType(T&&) { printType<T>(); }

// Will come with C++17
template <bool cond, typename type = void> using enable_if_t = typename std::enable_if<cond, type>::type;
template <typename T> using decay_t = typename std::decay<T>::type;

template <typename T, typename Head, typename...Tail>
enable_if_t<!std::is_convertible<Head&&, T>::value, T&&> select(Head&&, Tail&&...tail)
{
   return select<T>(std::forward<Tail>(tail)...);
}

template <typename T, typename Head, typename...Tail>
enable_if_t<std::is_convertible<Head&&, T>::value, T&&> select(Head&& head, Tail&&...)
{
   return T(head);
}

template <typename T>
T&& select()
{
   static_assert(false, __FUNCSIG__": The requested type is missing from the actual parameter list");
}

int main()
{
   void test_select(); test_select();


//   auto l2 = [](double d, auto&&...context) { PRINT(d); PRINT(select<double&>(context...)); PRINT(sizeof...(context)); };
//   l2(1.23, "Hello", true, 3.14);
}

void test_select()
{
   auto d = 3.14;
   PRINT(select<double&>(true, d) = 2.71);
   PRINT(d);
}
