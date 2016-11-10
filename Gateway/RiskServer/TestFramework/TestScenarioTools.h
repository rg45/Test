/**
* @file    TestScenarioTools.h
* @author  Sergey Romanchenko <sergeyr@cqg.com>
* @date    November, 2016
**/

#pragma once

#include <boost/format.hpp>
#include <functional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

namespace cqg
{
namespace RS
{
namespace TestFramework
{
namespace TestScenarioTools
{
/* 
   This is a simple toolkit useful for unit test development.
   It helps to make your unit tests more compact, declarative, readable and reliable.
   The main idea is that you create methods constructing test data rather than the data itself.
   The methods are any available callable instances: free functions, user defined function objects and lambdas.
   Variadic function objects and lambdas are also supported.
   Ideally you don't need to create any local objects in your UT methods at all.

   The 'Context' is a basic concept used in the public interface.
   The contexts are just variadic data packs containing actual parameters for your custom methods invocations.
   The same data pack can be used for invocation of methods with different formal parameter lists.

   See real examples of usage in existing Risk Server UT methods.

   A simple synthetic example is below:

      // Specific methods for test scenario description.
      // Could be implemented as any callable instances: objects, lambdas or free functions:
      auto exchange       = [](...) {...}     // creates or acquires an exchnage
      auto contract       = [](...) {...}     // creates or acquires a contract
      auto account        = [](...) {...}     // creates or acquires an account
      auto testCase       = [](...) {...}     // creates a test case
      auto doCalculations = [](...) {...}     // does a part of business logic that is a subject of the testing

      auto staticRiskData1 = tst::Aggregate(
         exchange(1,
            commodity(11,
               contract(111, InstrumentFuture),
               contract(112, InstrumentFuture)),
            commodity(12,
               contract(121, InstrumentFuture),
               contract(122, InstrumentFuture))),
         exchange(2,
            commodity(21,
               contract(211, InstrumentFuture),
               contract(212, InstrumentFuture)),
            commodity(22,
               contract(221, InstrumentFuture),
               contract(222, InstrumentFuture))));

         auto riskData1 = tst::Aggregate(
            staticRiskData1,
            account(100, master(AccountGroupRelation::BorrowFromMaster),
               position(ContractID(121), Quantity(10),
                  order(1, quantity(10)))),
            account(101,
               position(ContractID(121), Quantity(10),
                  order(2, quantity(10)))));

         testCase(1, "The test case description",
            riskData1(
               ...optional modifications...),
            doCalculations(AccountID(100),
               [](TestData&, const OrderMap&, Context&&...)
               {
                  UT_ASSERT_...
                  UT_ASSERT_...
                  UT_ASSERT_...
               });

         testCase(2, "The test case description",
            riskData1(
               ...optional modifications...),
            ...);
*/

/// @brief Aggregates an arbitrary set of callable instances into a single callable object
/// accepting variadic parameter list (so called "Context") containing actual parameter for the aggregated objects.
/// The aggregated objects may have differ formal parameter lists.
template <typename...F>
decltype(auto) Aggregate(F&&...);

/// @brief Invokes any callable object with parameters taken from the context.
/// The target method can have a fixed parameter list, variadic parameter lists or both at the time.
/// If variadic parameter set is present, whole the Context is passed to it in its original order.
/// Fixed part of actual parameter are also being retrieved from the Context
/// exercising matching by types of the formal parameters.
template <typename F, typename...Context>
decltype(auto) ContextCall(F&&, Context&&...);

/// @brief Captures the Context data and creates a callable object accepting variadic list of callable objects
/// and applying the ContextCall method for each of them
template <typename...Context>
decltype(auto) ContextCallForEach(Context&&...context);

/// @brief Matches object of the Context by the index
template <size_t index, typename...Context>
decltype(auto) ContextGet(Context&&...);

/// @brief Matches a first appropriate object of the Context that could be used for constructing of an object
/// of the requested type. Raises a compile time error if not found.
template <typename T, typename...Context>
decltype(auto) ContextMatch(Context&&...);

/// @brief Make a string representation of input data using the format string.
/// Produces exactly the same effect as boost::str(boost::format() % ...);
template <typename...Args>
std::string Format(const std::string& formatString, Args&&...);

/// @brief Returns string representation of the type.
template <typename T>
std::string GetTypeName();

/// @brief Returns string representation of type of the object.
template <typename T>
std::string GetTypeName(T&&);

namespace detail
{
using std::forward;
using std::tuple;

// C++17
template <bool cond, typename type = void>
using enable_if_t = typename std::enable_if<cond, type>::type;

// C++17
template <typename T>
using decay_t = typename std::decay<T>::type;

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

template <typename T, typename NoMatches, typename Inexact, typename NoExact, typename Tail, typename = void>
struct ContextMatchImpl;

template <typename T, typename...Context>
using ContextMatchFacade = ContextMatchImpl<T, tuple<>, tuple<>, tuple<>, tuple<Context...>>;

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

template <typename F, typename = void>
struct ContextCallImpl
{
   template <typename...Context>
   decltype(auto) operator()(F&&, Context&&...) const
   {
      // This point should never be hit in a well-formed program
      static_assert(false, "A callable instance of unsupported type was detected: " __FUNCSIG__);
   }
};

template <typename R, typename...Args>
struct ContextCallImpl<R(Args...)>
{
   template <typename F, typename...Context>
   R operator()(F&& f, Context&&...context) const
   {
      return f(ContextMatch<Args>(forward<Context>(context)...)...);
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
         return f(ContextMatch<Args>(forward<Context>(context)...)..., forward<Context>(context)...);
      }
   };

   template <typename...Context>
   decltype(auto) operator()(F&& f, Context&&...context) const
   {
      auto op = &decay_t<F>::operator()<Context...>; op;
      using FullSignature = MethodSignatureType<decltype(op)>;
      using Signature = TruncatedSignatureType<FullSignature, sizeof...(Context)>;
      return Impl<Signature, Context...>()(forward<F>(f), forward<Context>(context)...);
   }
};

} // namespace detail

template <typename...F>
decltype(auto) Aggregate(F&&...f)
{
   using namespace detail;
   return [&](auto&&...arg)
   {
      ContextCallForEach(forward<decltype(arg)>(arg)...)(forward<decltype(f)>(f)...);
   };
};

template <typename F, typename...Context>
decltype(auto) ContextCall(F&& f, Context&&...context)
{
   using namespace detail;
   return ContextCallImpl<F>()(forward<F>(f), forward<Context>(context)...);
}

template <typename...Context>
decltype(auto) ContextCallForEach(Context&&...context)
{
   using namespace detail;
   return [&](auto&&...f)
   {
      auto callWrapper = [&](auto&& f)
      {
         ContextCall(forward<decltype(f)>(f), forward<decltype(context)>(context)...);
         return 0;
      };
      std::initializer_list<int> { callWrapper(f)... };
   };
}

template <size_t index, typename...Context>
decltype(auto) ContextGet(Context&&...context)
{
   static_assert(index < sizeof...(Context), "Actual parameter index is out of range: " __FUNCSIG__);
   using namespace detail;
   return std::get<index>(tuple<Context&&...>(forward<Context>(context)...));
}

template <typename T, typename...Context>
decltype(auto) ContextMatch(Context&&...context)
{
   using namespace detail;
   return ContextMatchFacade<T, Context...>()(forward<Context>(context)...);
}

template <typename Data>
void Format(std::ostream& output, Data&& data)
{
   output << data;
}

template <typename Data, typename Arg, typename...Args>
void Format(std::ostream& output, Data&& data, Arg&& arg, Args&&...args)
{
   Format(output, data % arg, forward<Args>(args)...);
}

template <typename...Args>
std::string Format(const std::string& formatString, Args&&...args)
{
   using namespace detail;
   std::ostringstream output;
   Format(output, boost::format(formatString), forward<Args>(args)...);
   return output.str();
}

template <typename T>
std::string GetTypeName()
{
   using namespace detail;
   std::ostringstream output;
   output << __FUNCSIG__;
   auto&& str = output.str();
   auto begin = str.find(__FUNCTION__) + std::string(__FUNCTION__).size() + 1;
   auto end = str.find_last_of('>');
   return str.substr(begin, end - begin);
}

template <typename T>
std::string GetTypeName(T&&)
{
   return GetTypeName<T>();
}

} // namespace TestScenarioTools
} // namespace TestFramework
} // namespace RS
} // namespace cqg
