#include "stdafx.h"

#include <Gateway/RiskServer/TestFramework/TestScenarioTools.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>

#include <boost/noncopyable.hpp>

using namespace cqg::RS::TestFramework::TestScenarioTools;

// Development/Debug utility
#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)
#define TEST(name) void name(); std::cout << "\n==================== "#name" ====================" << std::endl; name();

void foo(int value) { PRINT(value); }

namespace detail2
{
using detail::decay_t;
using detail::enable_if_t;

template <typename T, typename C>
using IsAnyCastAvailable = std::is_convertible<C, T>;

template <typename T, typename C>
struct IsValueCastAvailable
{
   struct ImplicitConvertible { operator C&& (); };
   static constexpr bool value = std::is_convertible<ImplicitConvertible, T>::value;
};

template <typename T, typename C>
struct IsReferenceCastAvailable
{
   template <typename T> using Pointer = typename std::add_pointer<decay_t<T>>::type;
   static constexpr bool value = std::is_convertible<Pointer<C>, Pointer<T>>::value
      && IsAnyCastAvailable<T, C>::value;
};

template <typename T, typename C>
struct IsExactCastAvailable
{
   template <typename U>
   static std::false_type test(U&&);
   static std::true_type test(T);
   static constexpr bool value = decltype(test(std::declval<C>()))::value;
};

enum class ContextCastPriority
{
   none = 0,
   custom = 1,
   value = 2,
   reference = 3,
   exact = 4
};

template <typename T, typename C, typename = void>
struct ContextCastImpl
{
   static constexpr auto priority = ContextCastPriority::none;
};

template <typename T, typename C>
struct ContextCastImpl<T, C, enable_if_t<
   IsAnyCastAvailable<T, C>::value &&
   !IsReferenceCastAvailable<T, C>::value>>
{
   static constexpr auto priority = IsValueCastAvailable<C,T>::value ?
      ContextCastPriority::value : ContextCastPriority::custom;

   T operator()(C&& c) const { return T(std::forward<C>(c)); }
};

template <typename T, typename C>
struct ContextCastImpl<T, C, enable_if_t<
   IsReferenceCastAvailable<T, C>::value ||
   IsExactCastAvailable<T, C>::value>>
{
   static constexpr auto priority = IsExactCastAvailable<C, T>::value ?
      ContextCastPriority::exact : ContextCastPriority::reference;

   C&& operator()(C&& c) const { return std::forward<C>(c); }
};

template <typename T, typename C>
decltype(auto) ContextCast(C&& c)
{
   static_assert(ContextCastImpl<T, C>::priority != ContextCastPriority::none, "Context cast failed: " __FUNCSIG__);

   return ContextCastImpl<T, C>()(std::forward<C>(c));
}

template <typename T, typename Head, typename Middle, typename Tail, typename = void>
struct ContextMatchImpl { };

template <typename T, typename...Head, typename Next, typename...Tail>
struct ContextMatchImpl<T, std::tuple<Head...>, std::tuple<>, std::tuple<Next, Tail...>,
   enable_if_t<ContextCastImpl<T, Next>::priority == ContextCastPriority::none>>
   : ContextMatchImpl<T, std::tuple<Head..., Next>, std::tuple<>, std::tuple<Tail...>> { };

template <typename T, typename...Head, typename Next, typename...Tail>
struct ContextMatchImpl<T, std::tuple<Head...>, std::tuple<>, std::tuple<Next, Tail...>,
   enable_if_t<ContextCastImpl<T, Next>::priority != ContextCastPriority::none>>
   : ContextMatchImpl<T, std::tuple<Head...>, std::tuple<Next>, std::tuple<Tail...>> { };

template <typename T, typename...Head, typename Current, typename...Middle, typename Next, typename...Tail>
struct ContextMatchImpl<T, std::tuple<Head...>, std::tuple<Current, Middle...>, std::tuple<Next, Tail...>,
   enable_if_t<ContextCastImpl<T, Current>::priority >= ContextCastImpl<T, Next>::priority>>
   : ContextMatchImpl<T, std::tuple<Head...>, std::tuple<Current, Middle..., Next>, std::tuple<Tail...>> { };

template <typename T, typename...Head, typename Current, typename...Middle, typename Next, typename...Tail>
struct ContextMatchImpl<T, std::tuple<Head...>, std::tuple<Current, Middle...>, std::tuple<Next, Tail...>,
   enable_if_t<ContextCastImpl<T, Current>::priority < ContextCastImpl<T, Next>::priority>>
   : ContextMatchImpl<T, std::tuple<Head..., Current, Middle...>, std::tuple<Next>, std::tuple<Tail...>> { };

template <typename T, typename...Head, typename Current, typename...Tail>
struct ContextMatchImpl < T, std::tuple<Head...>, std::tuple<Current, Tail...>, std::tuple<>>
{
   decltype(auto) operator()(Head&&..., Current&& current, Tail&&...) const
   {
      return ContextCast<T>(std::forward<Current>(current));
   }
};

} // namespace detail2

template <typename T, typename C>
void printConversionTraits()
{
   using namespace detail2;
   std::cout << Format("\"%1%\" -> \"%2%\":", GetTypeName<C>(), GetTypeName<T>()) << std::endl;
   PRINT((IsAnyCastAvailable<T, C>::value));
   PRINT((IsValueCastAvailable<T, C>::value));
   PRINT((IsReferenceCastAvailable<T, C>::value));
   PRINT((IsExactCastAvailable<T, C>::value));
};

struct B { };
struct D : B { };

int main()
{
   std::cout << std::boolalpha;

   using namespace detail2;
   int&& i = 42;
   PRINT((ContextCast<const double&>(i)));
   //printConversionTraits<double&, const double>();


//   TEST(TestContextCall);
//   TEST(TestTruncatedSignatureType);
//   TEST(TestFunctionObjectKindDetection);
//   TEST(TestContextMatch);
//   TEST(TestGetTypeName);
}

#if 10
void TestContextCall()
{
   auto l2 = [](int i, short&& r, double d, /*auto&&,*/ auto&&...context) { PRINT(i), PRINT(r); PRINT(d); PRINT(sizeof...(context)); };
   short s = 2;
   ContextCall(l2, "Hello", 42, 3.14, s);

   PRINT(detail::IsVariadicFunctionObject<decltype(l2)>::value);
   PRINT(detail::IsNonTemplatedFunctionObject<decltype(l2)>::value);
   ContextCall(l2, 43);

   auto l1 = [](double d) { PRINT(d); };
   PRINT(detail::IsVariadicFunctionObject<decltype(l1)>::value);
   PRINT(detail::IsNonTemplatedFunctionObject<decltype(l1)>::value);
   ContextCall(l1, 3.14);

   PRINT(detail::IsVariadicFunctionObject<decltype(&foo)>::value);
   PRINT(detail::IsNonTemplatedFunctionObject<decltype(&foo)>::value);
   ContextCall(foo, 42);
   ContextCall(&foo, 42);
}

void TestTruncatedSignatureType()
{
   PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 0>>()));
   PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 1>>()));
   PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 2>>()));
   PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 3>>()));
   //PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 4>>()));
}

void TestFunctionObjectKindDetection()
{
   auto l3 = [](int i, auto&& x) { PRINT(i); PRINT(x); };
   l3(42, 3.14);
   PRINT(detail::IsVariadicFunctionObject<decltype(l3)>::value);
   PRINT(detail::IsNonTemplatedFunctionObject<decltype(l3)>::value);

   auto l2 = [](int, auto&&...context) { PRINT(ContextMatch<int>(std::forward<decltype(context)>(context)...)); };
   PRINT(detail::IsVariadicFunctionObject<decltype(l2)>::value);
   PRINT(detail::IsNonTemplatedFunctionObject<decltype(l2)>::value);

   auto l1 = [](int) -> void { };
   PRINT(detail::IsVariadicFunctionObject<decltype(l1)>::value);
   PRINT(detail::IsNonTemplatedFunctionObject<decltype(l1)>::value);
}

void TestContextMatch()
{
   auto d = 3.14;
   PRINT(ContextMatch<double&>(true, d) = 2.71);
   PRINT(ContextMatch<double&&>(d, 1.23, true, d));
   PRINT(d);
   PRINT(GetTypeName<decltype(ContextMatch<const std::string&>(2.71, false, "Hello!"))>());
   PRINT(ContextMatch<std::string>("Hello, World!", 3.14, size_t(42), ""));
   PRINT(ContextGet<1>("Hello, World!", 3.14, size_t(42), ""));
   PRINT((intptr_t)ContextMatch<class C*>("Hello, World!", 3.14, size_t(42), "", nullptr));
   //ContextMatch<double&>(3.14); // error C2338: The requested type is missing from the actual parameter list
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