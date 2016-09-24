#include "stdafx.h"

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/has_range_iterator.hpp>
#include <boost/range/iterator.hpp>
#include <boost/ref.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <string>
#include <tuple>
#include <typeinfo>

namespace ElementwiseOperations
{
namespace detail
{
template <typename T> using Decay = typename std::decay<T>::type;

template <bool condition, typename type = void>
using EnableIf = typename std::enable_if<condition, type>::type;

template <typename T> T&& instance();

template <typename, typename = void> struct RangeIterator_;

template <typename Range>
struct RangeIterator_<Range, EnableIf<boost::has_range_iterator<Range>::value>>
{
   typedef typename boost::range_iterator<Range>::type Iterator;
};

template <typename Range>
using RangeIterator = typename RangeIterator_<Range>::Iterator;

template <typename Iterator>
using IteratorReference = decltype(*instance<Iterator>());

template <typename Range>
using RangeReference = IteratorReference<RangeIterator<Range>>;

template <typename F, typename... Args>
using FunctionResult = decltype(instance<F>()(instance<Args>()...));

template <typename F, typename... Args>
using FunctionSignature = FunctionResult<F, Args...>(Args...);

template <typename T, typename = RangeIterator<T>> struct EnableIfRange_ : std::enable_if<true> { };
template <typename T> using EnableIfRange = typename EnableIfRange_<T>::type;

template <typename, typename = void> struct IsRange_ : boost::mpl::false_ { };
template <typename T> struct IsRange_<T, EnableIfRange<T>> : boost::mpl::true_ { };
template <typename T> constexpr bool IsRange = IsRange_<T>::value;

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
