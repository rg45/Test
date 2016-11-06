#include "stdafx.h"

#include <iostream>
#include <string>
#include <functional>

#include <boost/noncopyable.hpp>

#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)

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

int main()
{

   auto l3 = [](const std::string& name) { PRINT(name); };

   smartcall([](const std::string& name) {PRINT(name);}, "Hello, World!!!");
}
