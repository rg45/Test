#include "stdafx.h"

#include "tests.h"
#include <Gateway/RiskServer/TestFramework/TestScenarioTools.h>

#include <boost/noncopyable.hpp>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

using cqg::RS::TestFramework::TestScenarioTools::GetTypeName;
using cqg::RS::TestFramework::TestScenarioTools::detail::enable_if_t;

template <typename Signature, typename NewSig, size_t tailSize, typename = void>
struct SplitSignatureImpl;

template <typename R, typename Next, typename...Tail, typename...Head, size_t tailSize>
struct SplitSignatureImpl<R(Next, Tail...), R(Head...), tailSize, enable_if_t<sizeof...(Tail) >= tailSize>>
   : SplitSignatureImpl<R(Tail...), R(Head..., Next), tailSize> { };

template <typename R, typename...Tail, typename...Head, size_t tailSize>
struct SplitSignatureImpl<R(Tail...), R(Head...), tailSize, enable_if_t<sizeof...(Tail) == tailSize>>
{
   using type = R(Head...);
};

template <typename Signature, size_t tailSize> struct SplitSignature;
template <typename R, typename...Args, size_t tailSize>
struct SplitSignature<R(Args...), tailSize> : SplitSignatureImpl<R(Args...), R(), tailSize> { };

template <typename Signature, size_t tailSize>
using SplitSignatureType = typename SplitSignature<Signature, tailSize>::type;

int main()
{
   using namespace tests;
   std::cout << std::boolalpha;

   PRINT((GetTypeName<SplitSignatureType<void(int, double, float, char, bool), 2>>()));

//   ContextCallRemake::test();

//    TestContextCall();
//    TestTruncatedSignatureType();
//    TestFunctionObjectKindDetection();
//    TestContextMatch();
//    TestGetTypeName();
}

namespace tests
{
using namespace cqg::RS::TestFramework::TestScenarioTools;

void foo(int value) { PRINT(value); }

void TestContextCall()
{
   PRINT_CAPTION();

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
   PRINT_CAPTION();

   PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 0>>()));
   PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 1>>()));
   PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 2>>()));
   PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 3>>()));
//   PRINT((GetTypeName<detail::TruncatedSignatureType<int(double, short, bool), 4>>()));
}

void TestFunctionObjectKindDetection()
{
   PRINT_CAPTION();

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

template <typename T, typename C>
void printConversionTraits()
{
   PRINT_CAPTION();

   using namespace detail2;
   std::cout << Format("\"%1%\" -> \"%2%\":", GetTypeName<C>(), GetTypeName<T>()) << std::endl;
   PRINT((IsAnyCastAvailable<T, C>::value));
   PRINT((IsValueCastAvailable<T, C>::value));
   PRINT((IsReferenceCastAvailable<T, C>::value));
   PRINT((IsExactCastAvailable<T, C>::value));
};


void TestContextMatch()
{
   PRINT_CAPTION();

   using namespace detail;
   {
      struct B { virtual std::string foo() const { return __FUNCSIG__; } };
      struct D : B { virtual std::string foo() const { return __FUNCSIG__; } };
      D d; B b;
      auto&& x = ContextMatch<const B&>(d, b);
      PRINT(x.foo());
      PRINT(int(ContextCastImpl<B*, B&>::priority));
   }
   {
      const char* x = ContextMatch<const char*>(95., true, 0., 'R', 'U', 0., 42, "Hi!", short(1));
      if (x) PRINT(x); else PRINT(intptr_t(x));
   }
   {
      const char* x = ContextMatch<const char*>(nullptr);
      if (x) PRINT(x); else PRINT(intptr_t(x));
      PRINT(GetTypeName<decltype(x)>());
   }
//   {
//      int&& i = 42;
//      PRINT((ContextCast<const double&>(i)));
//      //printConversionTraits<double&, const double>();
//   }
   struct A { using type_ = A; };
   PRINT(IsEnabled<A>::value);
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
}

void TestGetTypeName()
{
   PRINT_CAPTION();

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
} // namespace tests