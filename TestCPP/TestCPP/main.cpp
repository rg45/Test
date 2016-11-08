#include "stdafx.h"

#include <Gateway/RiskServer/TestFramework/TestScenarioTools.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/noncopyable.hpp>

using namespace cqg::RS::TestFramework::TestScenarioTools;

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

   auto l2 = [](int, auto&&...context) { PRINT(GetConvertibleTo<int>(std::forward<decltype(context)>(context)...)); };
   PRINT(IsVariadicFunctionObject<decltype(l2)>::value);
   PRINT(IsNonTemplatedFunctionObject<decltype(l2)>::value);

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