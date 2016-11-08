/**
* @file    TestScenarioTools.h
* @author  Sergey Romanchenko <sergeyr@cqg.com>
* @date    November, 2016
**/

#pragma once

#include <functional>
#include <type_traits>

namespace cqg
{
namespace RS
{
namespace TestFramework
{
namespace TestScenarioHelpers
{
// Will come with C++17
template <bool cond, typename type = void> using enable_if_t = typename std::enable_if<cond, type>::type;
template <typename T> using decay_t = typename std::decay<T>::type;

template <typename Method> struct MethodSignature;

template <typename R, typename...Args>
struct MethodSignature<R(*)(Args...)> { using type = R(Args...); };

template <typename F, typename R, typename...Args>
struct MethodSignature<R(F::*)(Args...)> { using type = R(Args...); };

template <typename F, typename R, typename...Args>
struct MethodSignature<R(F::*)(Args...)const> { using type = R(Args...); };

template <typename Method>
using MethodSignatureType = typename MethodSignature<Method>::type;

template <typename T, typename = decltype(&decay_t<T>::operator())>
using IfNonTemplatedFunctionOperatorAvailable = enable_if_t<true>;

template <typename T, typename = void>
struct IsNonTemplatedFunctionObject : std::false_type { };

template <typename T>
struct IsNonTemplatedFunctionObject<T, IfNonTemplatedFunctionOperatorAvailable<T>> : std::true_type { };

template <typename T, typename = decltype(&decay_t<T>::operator() < > )>
using IfVariadicFunctionOperatorAvailable = enable_if_t<true>;

template <typename T, typename = void>
struct IsVariadicFunctionObject : std::false_type { };

template <typename T>
struct IsVariadicFunctionObject<T, IfVariadicFunctionOperatorAvailable<T>> : std::true_type { };

template <typename Signature, size_t cutSize, typename NewSig, typename = void>
struct TruncatedSignatureImpl;

template <typename R, typename First, typename...Rest, size_t cutSize, typename...NewArgs>
struct TruncatedSignatureImpl<R(First, Rest...), cutSize, R(NewArgs...), enable_if_t<sizeof...(Rest) >= cutSize>>
   : TruncatedSignatureImpl<R(Rest...), cutSize, R(NewArgs..., First)> { };

template <typename R, typename...Args, size_t cutSize, typename...NewArgs>
struct TruncatedSignatureImpl<R(Args...), cutSize, R(NewArgs...), enable_if_t<sizeof...(Args) == cutSize>>
{
   using type = R(NewArgs...);
};

template <typename Signature, size_t cutSize> struct TruncatedSignature;
template <typename R, typename...Args, size_t cutSize>
struct TruncatedSignature<R(Args...), cutSize> : TruncatedSignatureImpl<R(Args...), cutSize, R()> { };

template <typename Signature, size_t cutSize>
using TruncatedSignatureType = typename TruncatedSignature<Signature, cutSize>::type;

template <typename T, typename Head, typename...Tail>
auto GetConvertibleTo(Head&&, Tail&&...tail) ->
enable_if_t<!std::is_convertible<Head, T>::value, decltype(GetConvertibleTo<T>(std::forward<Tail>(tail)...))>
{
   return GetConvertibleTo<T>(std::forward<Tail>(tail)...);
}

template <typename T, typename Head, typename...Tail>
enable_if_t<std::is_convertible<Head, T>::value, Head&&> GetConvertibleTo(Head&& head, Tail&&...)
{
   return std::forward<Head>(head);
}

template <typename T>
T GetConvertibleTo()
{
   // This point should never be hit in a well-formed program
   static_assert(false, __FUNCSIG__": The requested type is missing from the actual parameter list");
}

template <typename F, typename = void>
struct ContextCallImpl;

template <typename R, typename...Args>
struct ContextCallImpl<R(Args...)>
{
   template <typename F, typename...Context>
   R operator()(F&& f, Context&&...context) const
   {
      return f(GetConvertibleTo<Args>(std::forward<Context>(context)...)...);
   }
};

template <typename R, typename...Args>
struct ContextCallImpl<R(*)(Args...)> : ContextCallImpl<R(Args...)> { };

template <typename R, typename...Args>
struct ContextCallImpl<R(&)(Args...)> : ContextCallImpl<R(Args...)> { };

template <typename F>
struct ContextCallImpl<F, enable_if_t<IsNonTemplatedFunctionObject<F>::value>>
: ContextCallImpl<MethodSignatureType<decltype(&decay_t<F>::operator())>> { };

/// @brief Implementation class of context call for variadic function objects (including variadic lambdas)
template <typename F>
struct ContextCallImpl<F, enable_if_t<IsVariadicFunctionObject<F>::value>>
{
   template <typename Signature, typename...Context> struct Impl;
   template <typename R, typename...Args, typename...Context>
   struct Impl<R(Args...), Context...>
   {
      R operator()(F&& f, Context&&...context) const
      {
         return f(GetConvertibleTo<Args>(std::forward<Context>(context)...)..., std::forward<Context>(context)...);
      }
   };

   template <typename...Context>
   decltype(auto) operator()(F&& f, Context&&...context) const
   {
      auto op = &decay_t<F>::operator()<Context...>;
      using FullSignature = MethodSignatureType<decltype(op)>;
      using Signature = TruncatedSignatureType<FullSignature, sizeof...(Context)>;
      return Impl<Signature, Context...>()(std::forward<F>(f), std::forward<Context>(context)...);
   }
};

template <typename F, typename...Context>
decltype(auto) ContextCall(F&& f, Context&&...context)
{
   return ContextCallImpl<F>()(std::forward<F>(f), std::forward<Context>(context)...);
}
} // namespace TestScenarioHelpers
} // namespace TestFramework
} // namespace RS
} // namespace cqg
