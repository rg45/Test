#include "stdafx.h"

#include "ElementwiseOperations.h"

#include <iostream>
#include <string>

namespace ElementwiseOperations
{
namespace detail
{

template <typename T> void testIsRange() { std::cout << IsRange<T> << std::endl; }
template <typename T> void testIsRange(T&&) { testIsRange<T>(); }

template <typename F, typename... Args>
FunctionResult<F> testFunction(F&& f, Args&&... args)
{
   typedef FunctionSignature<F, Args...> Signature;
   typedef std::function<Signature> Function;
   Function g = std::forward<F>(f);
   return g(std::forward<Args>(args)...);
}

void testString(const std::string& str)
{
   std::cout << str << std::endl;
}

void test()
{
   testString("Hello World!");

   std::cout << std::boolalpha;

   testIsRange("Hello!");
   testIsRange(43);
   testIsRange<const int(&&)[1]>();

   auto λ = [](auto&&... args)
   {
      auto print = [](auto&& t) { std::cout << t << ' '; return 0; };
      std::initializer_list<int>{ print(args)... };
      return sizeof...(args);
   };
   std::cout << "argCount = " << testFunction(λ, "Pi =") << std::endl;
   std::cout << "argCount = " << testFunction(λ, "Pi =", 3.14) << std::endl;
   std::cout << "argCount = " << testFunction(λ, "Pi =", 3.14, "g =") << std::endl;
   std::cout << "argCount = " << testFunction(λ, "Pi =", 3.14, "g =", 9.8) << std::endl;
}

} // namespace detail

void test()
{
   detail::test();
}

} // namespace ElementwiseOperations

void TestElementwiseOperations()
{
   ElementwiseOperations::test();
}
