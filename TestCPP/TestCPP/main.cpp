#include "stdafx.h"

#include <iostream>
#include <string>
#include <functional>

#include <boost/noncopyable.hpp>

#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)

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

template <bool cond> using EnableIf = typename std::enable_if<cond>::type;
template <typename T> using Decay = typename std::decay<T>::type;

// template <typename T, typename = decltype(&T::operator())> struct EnableIfFunctionObject_ : std::enable_if<true>{ };
// template <typename T> using EnableIfFunctionObject = typename EnableIfFunctionObject_<T>::type;
template <typename T, typename = decltype(&T::operator())> using EnableIfFunctionObject = EnableIf<true>;
template <typename T, typename = void> struct IsFunctionObject_ : std::false_type { };
template <typename T> struct IsFunctionObject_<T, EnableIfFunctionObject<T>> : std::true_type { };
template <typename T> constexpr bool isFunctionObject() { return IsFunctionObject_<Decay<T>>::value; };
template <typename T> constexpr bool isFunctionObject(T&&) { return isFunctionObject<T>(); };

template <typename T, typename = decltype(&T::operator())> struct EnableIfVariadicFunctionObject_ { using type = void; };
template <typename T> using EnableIfVariadicFunctionObject = typename EnableIfVariadicFunctionObject_<T>::type;

template <typename T> struct SignatureOfBase_;
template <typename R, typename...Args> struct SignatureOfBase_<R(Args...)> { typedef R type(Args...); };
template <typename R, typename...Args> struct SignatureOfBase_<R(*)(Args...)> : SignatureOfBase_<R(Args...)> { };
template <typename T, typename R, typename...Args> struct SignatureOfBase_<R(T::*)(Args...)> : SignatureOfBase_<R(Args...)> { };
template <typename T, typename R, typename...Args> struct SignatureOfBase_<R(T::*)(Args...)const> : SignatureOfBase_<R(Args...)> { };

template <typename T, typename = void> struct SignatureOf_;
template <typename T>
struct SignatureOf_<T, EnableIf<isFunctionObject<T>()>> : SignatureOfBase_<decltype(&T::operator())> { };

//template <typename T> struct SignatureOf_<T, EnableIfVariadicFunctionObject<T>> : SignatureOfBase_<decltype(&T::operator()<>)> { };
template <typename T> using SignatureOf = typename SignatureOf_<T>::type;


template <typename F, typename = SignatureOf<F>>
struct SmartCall;
// {
//    template <typename...Args>
//    decltype(auto) operator()(F&& f, Args&&...args) const { return f(std::forward<Args>(args)...); }
// };

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
   PRINT(isFunctionObject(f));
   PRINT(isFunctionObject<F>());
   PRINT(sizeof(&F::operator()));
   PRINT(sizeof(EnableIf<isFunctionObject<F>()>*));
   PRINT(sizeof(typename SignatureOf_<F>::type)); // use of undefined type
   //return SmartCall<F, typename SignatureOf_<F>::type>()(std::forward<F>(f), std::forward<Args>(args)...);
}

template <typename T>
void print(const std::string& name, const T& value)
{
   std::cout << name << " = " << value << std::endl;
}

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

   auto l3 = [](const std::string& name, const A& a) {PRINT(name); PRINT(a.value);};
   PRINT(sizeof(decltype(&decltype(l3)::operator())));
   PRINT(isFunctionObject(l3));
   PRINT(isFunctionObject([](const std::string& name, const A& a) {PRINT(name); PRINT(a.value);}));
   //PRINT(sizeof(SignatureOf<decltype(&decltype(l3)::operator())>)); //OK - should be error: 'illegal sizof operand'

   smartcall([](const std::string& name/*, const A& a*/) {PRINT(name);}, a1, "Hello, World!!!", A(-1.0));
   //smartcall([](auto&&...context) { PRINT(select<const char*>(context...)); }, a1, "Hello, World!!!", A(-1.0));
   //smartcall(&print<double>, a1, "Pi", 3.14, A(-1.0));
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
