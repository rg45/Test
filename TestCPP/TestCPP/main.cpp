#include "stdafx.h"

#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/noncopyable.hpp>

// Development/Debug utility
#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)
#define TEST(name) void name(); name();

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
   static_assert(false, __FUNCSIG__": The requested type is missing from the actual parameter list");
}

template <typename T, typename = decltype(&decay_t<T>::operator())>
using IfNonTemplatedFunctionOperatorAvailable = enable_if_t<true>;

template <typename T, typename = void>
struct IsNonTemplatedFunctionObject : std::false_type { };

template <typename T>
struct IsNonTemplatedFunctionObject<T, IfNonTemplatedFunctionOperatorAvailable<T>> : std::true_type { };

template <typename Method> struct MethodSignature;

template <typename R, typename...Args>
struct MethodSignature<R(*)(Args...)> { using type = R(Args...); };

template <typename F, typename R, typename...Args>
struct MethodSignature<R(F::*)(Args...)> { using type = R(Args...); };

template <typename F, typename R, typename...Args>
struct MethodSignature<R(F::*)(Args...)const> { using type = R(Args...); };

template <typename Method>
using MethodSignatureType = typename MethodSignature<Method>::type;

template <typename F, typename = void>
struct ContextSignature;
//{
//   template <typename...Context>
//   using type = MethodSignatureType<decltype(&decay_t<F>::operator()<Context...>)>;
//};

template <typename F>
struct ContextSignature<F, enable_if_t<IsNonTemplatedFunctionObject<F>::value>>
{
   template <typename...Context>
   using foo = void;

   template <typename...Context>
   using type = MethodSignatureType<decltype(&decay_t<F>::operator())>;
};

//template <typename F, typename...Context>
//using ContextSignatureType = typename ContextSignature<decay_t<F>>::type<Context...>;

template <typename Signature> struct CallInContextImpl;

template <typename R, typename...Args>
struct CallInContextImpl<R(Args...)>
{
   template <typename F, typename...Context>
   R operator()(F&& f, Context&&...context) const
   {
      return f(GetConvertibleTo<Args>(context)...);
   }
};

template <typename F, typename...Context>
struct Foo
{
   //using ContextSignatureT = typename ContextSignature<F>::type<Context...>;
};

template <typename F, typename...Context>
decltype(auto) CallInContext(F&& f, Context&&...context)
{
   //PRINT(GetTypeName<typename Foo<F, Context...>::ContextSignatureT>());
//   using Impl = CallInContextImpl<ContextSignatureType<F, Context...>>;
//   return Impl()(std::forward<F>(f), std::forward<Context>(context)...);
}

void foo(int value) { }

//template <typename F>
//struct VariadicFunctionObjectSignarureType = typename VariadicFunctionObjectSignarure<F>::type;

int main()
{
   auto l1 = [](int) -> void { };
   CallInContext(l1, 3.14);

   //TEST(TestGetConvertibleTo);
   //TEST(TestGetTypeName);
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
