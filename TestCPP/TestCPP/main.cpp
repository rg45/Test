#include "stdafx.h"

#include <iostream>
#include <string>
#include <functional>

#include <boost/noncopyable.hpp>

#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)

template <size_t i, typename Head, typename...Tail>
struct SelectByIndex
{
   decltype(auto) operator()(Head&&, Tail&&...tail)
   {
      return SelectByIndex<i - 1, Tail...>()(std::forward<Tail>(tail)...);
   }
};

template <typename Head, typename...Tail>
struct SelectByIndex<0, Head, Tail...>
{
   decltype(auto) operator()(Head&& head, Tail&&...) { return std::forward<Head>(head); }
};

template <bool, typename T, typename Head, typename...Tail>
struct SelectByTypeBase;

template <typename T, typename Head, typename...Tail>
using SelectByType = SelectByTypeBase<std::is_convertible<Head, T>::value, T, Head, Tail...>;

template <bool, typename T, typename Head, typename...Tail>
struct SelectByTypeBase
{
   decltype(auto) operator()(Head&&, Tail&&...tail)
   {
      return SelectByType<T, Tail...>()(std::forward<Tail>(tail)...);
   }
};

template <typename T, typename Head, typename...Tail>
struct SelectByTypeBase<true, T, Head, Tail...>
{
   decltype(auto) operator()(Head&& head, Tail&&...) { return std::forward<Head>(head); }
};

template <size_t i, typename...List>
decltype(auto) select(List&&...list) { return SelectByIndex<i, List...>()(std::forward<List>(list)...); }

template <typename T, typename...List>
decltype(auto) select(List&&...list) { return SelectByType<T, List...>()(std::forward<List>(list)...); }

template <typename T, typename type = void, typename = decltype(&F::operator())>
using EnableIfFunctionObject = typename std::enable_if<true, type>::type;

template <typename F, typename = void> struct SignatureOf_ { using type = void; };
template <typename F> using SignatureOf = typename SignatureOf_<F>::type;
template <typename R, typename...Args> struct SignatureOf_<R(Args...)> { typedef R type(Args...); };
template <typename R, typename...Args> struct SignatureOf_<R(*)(Args...)> : SignatureOf_<R(Args...)> { };
template <typename T, typename R, typename...Args> struct SignatureOf_<R(T::*)(Args...)> : SignatureOf_<R(Args...)> { };
template <typename T, typename R, typename...Args> struct SignatureOf_<R(T::*)(Args...)const> : SignatureOf_<R(Args...)> { };
template <typename F> struct SignatureOf_<F, EnableIfFunctionObject<F>> : SignatureOf_<decltype(&F::operator())> { };

template <typename F, typename = SignatureOf<F>>
struct SmartCall
{
   template <typename...Args>
   decltype(auto) operator()(F&& f, Args&&...args) const { return f(std::forward<Args>(args)...); }
};

template <typename F, typename R, typename...Args>
struct SmartCall<F, R(Args...)>
{
   template <typename...Context>
   R operator()(F&& f, Context&&...context) const
   {
      return f(select<Args>(std::forward<Context>(context)...)...);
   }
};

template <typename F, typename...Args>
decltype(auto) smartcall(F&& f, Args&&...args)
{
   return SmartCall<F>()(std::forward<F>(f), std::forward<Args>(args)...);
}

template <typename T>
void print(const std::string& name, const T& value)
{
   std::cout << name << " = " << value << std::endl;
}

int main()
{
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

   smartcall([](const std::string& name, const A& a) {PRINT(name); PRINT(a.value);}, a1, "Hello, World!!!", A(-1.0));
   smartcall([](auto&&...context) { PRINT(select<const char*>(context...)); }, a1, "Hello, World!!!", A(-1.0));
   smartcall(&print<double>, a1, "Pi", 3.14, A(-1.0));
}
