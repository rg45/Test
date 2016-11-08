#include "stdafx.h"

#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/noncopyable.hpp>

// Development/Debug utility
#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)
#define TEST(name) void name(); std::cout << "\n==================== "#name" ====================" << std::endl; name();

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

template <typename F, typename...VarArgs>
struct TemplatedFunctionObjectSignature
{
   static constexpr decltype(&decay_t<F>::operator()<VarArgs...>) getMethod()
   {
      return &decay_t<F>::operator()<VarArgs...> ;
   }
   using type = MethodSignatureType<decltype(getMethod())>;
};

template <typename F, typename...VarArgs>
using TemplatedFunctionObjectSignarureType = typename TemplatedFunctionObjectSignature<F, VarArgs...>::type;

template <typename T, typename Head, typename...Tail>
auto GetConvertibleTo(Head&&, Tail&&...tail) ->
enable_if_t<!std::is_convertible<Head, T>::value,
   decltype(GetConvertibleTo<T>(std::forward<Tail>(tail)...))>
{
   return GetConvertibleTo<T>(std::forward<Tail>(tail)...);
}

template <typename T, typename Head, typename...Tail>
enable_if_t<std::is_convertible<Head, T>::value, Head&&> GetConvertibleTo(Head&& head, Tail&&...)
{
   return std::forward<Head>(head);
}

template <typename T>
T&& GetConvertibleTo()
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
      using FullSignature = TemplatedFunctionObjectSignarureType<F, Context...>;
      using Signature = TruncatedSignatureType<FullSignature, sizeof...(Context)>;
      return Impl<Signature, Context...>()(std::forward<F>(f), std::forward<Context>(context)...);
   }
};

template <typename F, typename...Context>
decltype(auto) ContextCall(F&& f, Context&&...context)
{
   return ContextCallImpl<F>()(std::forward<F>(f), std::forward<Context>(context)...);
}

void foo(int value) { PRINT(value); }

int main()
{
   std::cout << std::boolalpha;

   TEST(TestContextCall);
   //TEST(TestTruncatedSignatureType);
   //TEST(TestFunctionObjectKindDetection);
   //TEST(TestGetConvertibleTo);
   //TEST(TestGetTypeName);
}

void TestContextCall()
{
   auto l2 = [](int i, short&& r, double d, auto&&...context) { PRINT(i), PRINT(r); PRINT(d); PRINT(sizeof...(context)); };
   short s = 2;
   ContextCall(l2, "Hello", 42, 3.14, s);

   PRINT(IsVariadicFunctionObject<decltype(l2)>::value);
   PRINT(IsNonTemplatedFunctionObject<decltype(l2)>::value);
   ContextCall(l2, 43);

   auto l1 = [](double d) { PRINT(d); };
   PRINT(IsVariadicFunctionObject<decltype(l1)>::value);
   PRINT(IsNonTemplatedFunctionObject<decltype(l1)>::value);
   ContextCall(l1, 3.14);

   PRINT(IsVariadicFunctionObject<decltype(&foo)>::value);
   PRINT(IsNonTemplatedFunctionObject<decltype(&foo)>::value);
   ContextCall(foo, 42);
   ContextCall(&foo, 42);
}

#if 10
void TestTruncatedSignatureType()
{
   PRINT((GetTypeName<TruncatedSignatureType<int(double, short, bool), 0>>()));
   PRINT((GetTypeName<TruncatedSignatureType<int(double, short, bool), 1>>()));
   PRINT((GetTypeName<TruncatedSignatureType<int(double, short, bool), 2>>()));
   PRINT((GetTypeName<TruncatedSignatureType<int(double, short, bool), 3>>()));
   //PRINT((GetTypeName<TruncatedSignatureType<int(double, short, bool), 4>>()));
}

void TestFunctionObjectKindDetection()
{
   auto l3 = [](int i, auto&& x) { PRINT(i); PRINT(x); };
   l3(42, 3.14);
   PRINT(IsVariadicFunctionObject<decltype(l3)>::value);
   PRINT(IsNonTemplatedFunctionObject<decltype(l3)>::value);
   PRINT((GetTypeName<TemplatedFunctionObjectSignarureType<decltype(l3), double>>()));

   auto l2 = [](int, auto&&...context) { PRINT(GetConvertibleTo<int>(std::forward<decltype(context)>(context)...)); };
   PRINT(IsVariadicFunctionObject<decltype(l2)>::value);
   PRINT(IsNonTemplatedFunctionObject<decltype(l2)>::value);
   PRINT((GetTypeName<TemplatedFunctionObjectSignarureType<decltype(l2), int>>()));

   auto l1 = [](int) -> void { };
   PRINT(IsVariadicFunctionObject<decltype(l1)>::value);
   PRINT(IsNonTemplatedFunctionObject<decltype(l1)>::value);
}

void TestGetConvertibleTo()
{
   auto d = 3.14;
   PRINT(GetConvertibleTo<double&>(true, d) = 2.71);
   PRINT(GetConvertibleTo<double&&>(d, 1.23, true, d));
   PRINT(d);
   PRINT(GetTypeName<decltype(GetConvertibleTo<const std::string&>(2.71, false, "Hello!"))>());
}

void TestGetTypeName()
{
   PRINT(GetTypeName(3.14));
   PRINT(GetTypeName(std::forward<double&&>(3.14)));
   PRINT(GetTypeName<decltype(3.14)>());
   PRINT(GetTypeName<decltype(std::forward<double>(3.14))>());

   double d = 3.14;
   PRINT(GetTypeName(d));
   PRINT(GetTypeName([](int) {}));

   auto createLambda = [](int x) { return [x](int y, auto&&...) { return x + y; }; };

   PRINT(GetTypeName(createLambda(1)));
   PRINT(GetTypeName(createLambda(2)));
   PRINT(GetTypeName(&decltype(createLambda(2))::operator() < > ));
   PRINT(GetTypeName(&decltype(createLambda(2))::operator() < double > ));
   PRINT(GetTypeName(&decltype(createLambda(2))::operator() < double& > ));
}
#endif