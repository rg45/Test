#include "stdafx.h"

#include <iostream>
#include <string>
#include <functional>

#include <boost/noncopyable.hpp>

#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)

template <bool cond> using EnableIf = typename std::enable_if<cond>::type;
template <typename T> using Decay = typename std::decay<T>::type;

template <typename T, typename = decltype(&T::operator())> using EnableIfFunctionObject = EnableIf<true>;
template <typename T, typename = void> struct IsFunctionObject_ : std::false_type { };
template <typename T> struct IsFunctionObject_<T, EnableIfFunctionObject<T>> : std::true_type { };

template <typename T, typename = void> struct SignatureOf_;
template <typename T>
struct SignatureOf_<T, EnableIf<IsFunctionObject_<T>::value>> { using type = int; };

template <typename T> using SignatureOf = typename SignatureOf_<T>::type;


template <typename F, typename...Args>
decltype(auto) smartcall(F&& f, Args&&...args)
{
   PRINT(IsFunctionObject_<F>::value);
   PRINT(sizeof(&F::operator()));
   PRINT(sizeof(EnableIf<IsFunctionObject_<F>::value>*));
   PRINT(sizeof(typename SignatureOf_<F>::type)); // use of undefined type
}

int main()
{
   smartcall([](const std::string& name) {PRINT(name);}, "Hello, World!!!");
}
