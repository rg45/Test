#include "stdafx.h"

#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/noncopyable.hpp>

// Will come with C++17
template <bool cond, typename type = void> using enable_if_t = typename std::enable_if<cond, type>::type;
template <typename T> using decay_t = typename std::decay<T>::type;

// Development/Debug utility
#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)

template <typename T>
std::string GetTypeName()
{
   std::ostringstream output;
   output << __FUNCSIG__;
   auto&& str = output.str();
   auto begin = str.find(__FUNCTION__) + std::string(__FUNCTION__).size() + 1;
   auto end = str.find_last_of('>');
   return str.substr(begin, end - begin);
}
template <typename T> std::string GetTypeName(T&&) { return GetTypeName<T>(); }

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
   PRINT(GetTypeName(3.14));
   PRINT(GetTypeName(std::forward<double&&>(3.14)));
   PRINT(GetTypeName<decltype(3.14)>());
   PRINT(GetTypeName<decltype(std::forward<double>(3.14))>());

   double d = 3.14;
   PRINT(GetTypeName(d));
   PRINT(GetTypeName([](int){}));

   auto createLambda = [](int x) { return [x](int y, auto&&...) { return x + y; }; };

   PRINT(GetTypeName(createLambda(1)));
   PRINT(GetTypeName(createLambda(2)));
   PRINT(GetTypeName(&decltype(createLambda(2))::operator()<>));
   PRINT(GetTypeName(&decltype(createLambda(2))::operator()<double>));
   PRINT(GetTypeName(&decltype(createLambda(2))::operator()<double&>));


   //void test_select(); test_select();


//   auto l2 = [](double d, auto&&...context) { PRINT(d); PRINT(select<double&>(context...)); PRINT(sizeof...(context)); };
//   l2(1.23, "Hello", true, 3.14);
}
void test_select()
{
   auto d = 3.14;
   PRINT(select<double&>(true, d) = 2.71);
   PRINT(d);
}
