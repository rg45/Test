#include "stdafx.h"

#include <Gateway/RiskServer/TestFramework/TestScenarioTools.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>

#include <boost/noncopyable.hpp>

using namespace cqg::RS::TestFramework::TestScenarioTools;
using namespace cqg::RS::TestFramework::TestScenarioTools::detail;

// Development/Debug utility
#define PRINT(ex) (std::cout << #ex" = " << (ex) << std::endl)
#define TEST(name) void name(); std::cout << "\n==================== "#name" ====================" << std::endl; name();

void foo(int value) { PRINT(value); }

template <typename T, typename...Context> struct IsAnyCastAvailable;
template <typename T> struct IsAnyCastAvailable<T> : std::false_type { };
template <typename T, typename Context> struct IsAnyCastAvailable<T, Context>
: std::is_constructible<T, Context&&> { };

template <typename T, typename...Context> struct IsExactCastAvailable;
template <typename T> struct IsExactCastAvailable<T> : std::false_type { };
template <typename T, typename Context> struct IsExactCastAvailable<T, Context> : std::integral_constant<bool,
   IsAnyCastAvailable<T, Context>::value &&
   std::is_same<decay_t<T>, decay_t<Context>>::value> { };

template <typename T, typename...Context> struct IsInexactCastAvailable;
template <typename T> struct IsInexactCastAvailable<T> : std::false_type { };
template <typename T, typename Context> struct IsInexactCastAvailable<T, Context>
: std::integral_constant<bool,
   IsAnyCastAvailable<T, Context>::value &&
   !std::is_same<decay_t<T>, decay_t<Context>>::value> { };

using std::tuple;

template <typename T, typename NoMatches, typename Inexact, typename NoExact, typename Tail, typename = void>
struct ContextMatchImpl;

template <typename T, typename...NoMatches, typename Next, typename...Tail>
struct ContextMatchImpl<T, tuple<NoMatches...>, tuple<>, tuple<>, tuple<Next, Tail...>,
enable_if_t<!IsAnyCastAvailable<T, Next>::value>>
: ContextMatchImpl<T, tuple<NoMatches..., Next>, tuple<>, tuple<>, tuple<Tail...>> { };

template <typename T, typename...NoMatches, typename Next, typename...Tail>
struct ContextMatchImpl<T, tuple<NoMatches...>, tuple<>, tuple<>, tuple<Next, Tail...>,
enable_if_t<IsInexactCastAvailable<T, Next>::value>>
: ContextMatchImpl<T, tuple<NoMatches...>, tuple<Next>, tuple<>, tuple<Tail...>> { };

template <typename T, typename...NoMatches, typename Inexact, typename...NoExact, typename Next, typename...Tail>
struct ContextMatchImpl<T, tuple<NoMatches...>, tuple<Inexact>, tuple<NoExact...>, tuple<Next, Tail...>,
enable_if_t<!IsExactCastAvailable<T, Next>::value>>
: ContextMatchImpl<T, tuple<NoMatches...>, tuple<Inexact>, tuple<NoExact..., Next>, tuple<Tail...>> { };

// Exact type match
template <typename T, typename...NoMatches, typename...Inexact, typename...NoExact, typename Exact, typename...Tail>
struct ContextMatchImpl<T, tuple<NoMatches...>, tuple<Inexact...>, tuple<NoExact...>, tuple<Exact, Tail...>,
enable_if_t<IsExactCastAvailable<T, Exact>::value>>
{
   Exact&& operator()(NoMatches&&..., Inexact&&..., NoExact&&..., Exact&& exact, Tail&&...) const
   {
      return forward<Exact>(exact);
   }
};

// Inexact type match
template <typename T, typename...NoMatches, typename Inexact, typename...NoExact>
struct ContextMatchImpl<T, tuple<NoMatches...>, tuple<Inexact>, tuple<NoExact...>, tuple<>>
{
   Inexact&& operator()(NoMatches&&..., Inexact&& inexact, NoExact&&...) const
   {
      return forward<Inexact>(inexact);
   }
};

// No matches
template <typename T, typename...NoMatches>
struct ContextMatchImpl<T, tuple<NoMatches...>, tuple<>, tuple<>, tuple<>>
{
   T&& operator()(NoMatches&&...) const
   {
      // This point should never be hit in a well-formed program
      static_assert(false, "The requested type is missing from the actual parameter list: " __FUNCSIG__);
   }
};

template <typename T, typename...Context>
decltype(auto) ContextMatch2(Context&&...context)
{
   return ContextMatchImpl<T, tuple<>, tuple<>, tuple<>, tuple<Context...>>()(forward<Context>(context)...);
}

int main()
{
   std::cout << std::boolalpha;

   PRINT(ContextMatch2<std::string>("Hello, World!", 3.14, size_t(42), ""));

//    TEST(TestContextCall);
//    TEST(TestTruncatedSignatureType);
//    TEST(TestFunctionObjectKindDetection);
//    TEST(TestMatch);
//    TEST(TestGetTypeName);
}

void TestContextCall()
{
   auto l2 = [](int i, short&& r, double d, auto&&...context) { PRINT(i), PRINT(r); PRINT(d); PRINT(sizeof...(context)); };
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

#if 10
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

void TestMatch()
{
   auto d = 3.14;
   PRINT(ContextMatch<double&>(true, d) = 2.71);
   PRINT(ContextMatch<double&&>(d, 1.23, true, d));
   PRINT(d);
   PRINT(GetTypeName<decltype(ContextMatch<const std::string&>(2.71, false, "Hello!"))>());
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