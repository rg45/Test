#include "stdafx.h"

#include <iostream>
#include <string>
#include <functional>

#include <boost/noncopyable.hpp>

#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)

// Will come with C++17
template <bool cond, typename type = void> using enable_if_t = typename std::enable_if<cond, type>::type;
template <typename T> using decay_t = typename std::decay<T>::type;

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

template <typename T, typename...List>
decltype(auto) select(List&&...list) { return SelectByType<T, List...>()(std::forward<List>(list)...); }

template <typename T, typename = decltype(&F::operator())> using EnableIfFunctionObject = enable_if_t<true>;
template <typename T, typename = void> struct IsFunctionObject : std::false_type { };
template <typename T> struct IsFunctionObject<T, EnableIfFunctionObject<T>> : std::true_type { };

template <typename T, typename = decltype(&F::operator()<>)> using EnableIfVariadicFunctionObject = enable_if_t<true>;
template <typename T, typename = void> struct IsVariadicFunctionObject : std::false_type { };
template <typename T> struct IsVariadicFunctionObject<T, EnableIfVariadicFunctionObject<T>> : std::true_type { };
template <typename F> decltype(&F::operator()<>) variadicOperator() { return &F::operator()<>; }

template <typename F> struct SignatureOfBase_;
template <typename T, typename R, typename...Args> struct SignatureOfBase_<R(T::*)(Args...)> { using type = R(Args...); };
template <typename T, typename R, typename...Args> struct SignatureOfBase_<R(T::*)(Args...)const> { using type = R(Args...); };

template <typename F, typename = void> struct SignatureOf_;// { using type = void; };
template <typename R, typename...Args> struct SignatureOf_<R(*)(Args...)> { using type = R(Args...); };
template <typename F> struct SignatureOf_<F, enable_if_t<IsFunctionObject<F>::value>> : SignatureOfBase_<decltype(&F::operator())> { };
template <typename F> struct SignatureOf_<F, enable_if_t<IsVariadicFunctionObject<F>::value>> : SignatureOfBase_<decltype(variadicOperator<F>())> { };

template <typename F> using SignatureOf = typename SignatureOf_<F>::type;

template <typename F, typename = SignatureOf<F>> struct SmartCall$;

template <typename F, typename R, typename...Args>
struct SmartCall$<F, R(Args...)>
{
   template <typename...Context>
   R operator()(F&& f, Context&&...context) const
   {
      return f(select<Args>(std::forward<Context>(context)...)...);
   }
};

template <typename F, typename...Args>
decltype(auto) smartcall$(F&& f, Args&&...args)
{
   PRINT(IsVariadicFunctionObject<F>::value);
   return SmartCall$<F>()(std::forward<F>(f), std::forward<Args>(args)...);
}

template <typename T>
void print(const std::string& name, const T& value)
{
   std::cout << name << " = " << value << std::endl;
}



template <typename T>
void printType() { PRINT(__FUNCSIG__); }

template <typename T>
void printType(T&&) { printType<T>(); }

int main()
{
   struct A : boost::noncopyable
   {
      explicit A(double value = 0) : value(value) { }
      double value;
   };

   A a1(3.14);
   PRINT(select<std::string>(a1, "Hello, World!!!", A()));
   PRINT(select<const A&>(a1, "Hello, World!!!", A()).value);
   PRINT(select<const A&>(a1, "Hello, World!!!", A()).value = 22.4);
   PRINT(select<const A&>(a1, "Hello, World!!!", A()).value);

   smartcall$([](const std::string& name, const A& a) {PRINT(name); PRINT(a.value);}, a1, "Hello, World!!!", A(-1.0));
   smartcall$([](auto&&...context) { PRINT(__FUNCSIG__);/*PRINT(select<const char*>(context...));*/ }, a1, "Hello, World!!!", A(-1.0));
   smartcall$([](auto&&...) { PRINT(__FUNCSIG__); });
   smartcall$(&print<double>, a1, "Pi", 3.14, A(-1.0));
   auto l1 = [](auto&& arg) { PRINT(arg); };
   l1(777);
   using L1 = decltype(l1);
   auto op1 = &L1::operator()<int>;
   (l1.*op1)(555);

   auto l2 = [](auto&&...arg) { PRINT(sizeof...(arg)); };
   l2(1, 2, 3);
   using L2 = decltype(l2);
   auto op2 = &L2::operator()<>;
   (l2.*op2)();
}
