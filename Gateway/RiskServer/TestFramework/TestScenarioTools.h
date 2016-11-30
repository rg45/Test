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
      auto exchange       = [](...) {...}     // creates or acquires an exchange
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
/// The callable object can declare fixed, template and variadic formal parameters.
/// Template parameters are matched by order, in contrast with fixed parameters that are matched by type. 
/// If variadic parameter list is declared, whole the Context is passed to it in its original order.
/// Fixed part of actual parameter are also being retrieved from the Context
/// exercising matching by types of the formal parameters.
template <typename F, typename...Context>
decltype(auto) ContextCall(F&&, Context&&...);

/// @brief Captures the Context data and creates a callable object accepting variadic list of callable objects
/// and applying the ContextCall method for each of them
template <typename...Context>
decltype(auto) ContextForEach(Context&&...);

/// @brief Matches object of the Context by the index
template <size_t index, typename...Context>
decltype(auto) ContextGet(Context&&...);

/// @brief Matches a first appropriate object of the Context that could be used for constructing of an object
/// of the requested type.
/// Returns a reference to an object of the context.
/// Raises a compile time error if not found.
/// Please note, ContextMatch only performs matching but applies no conversions!
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
// C++17
template <bool cond, typename type = void>
using enable_if_t = typename std::enable_if<cond, type>::type;

// C++17
template <typename T>
using decay_t = typename std::decay<T>::type;

template <typename T>
using TypeOf = typename decay_t<T>::type;

template <typename>
using IfAvailable = TypeOf<std::enable_if<true>>;

template <typename T, typename = void>
struct IsEnabled : std::false_type { };

template <typename T>
struct IsEnabled<T, IfAvailable<TypeOf<T>>> : std::true_type { };

template <typename T>
using IfEnabled = enable_if_t<IsEnabled<T>::value>;

template <typename T, typename C>
using IsAnyCastAvailable = std::is_convertible<C, T>;

template <typename T, typename C>
struct IsValueCastAvailable
{
   struct ImplicitConvertible { operator C && (); };
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
   pointer = 3,
   reference = 4,
   exactPointer = 5,
   exact = 6
};

template <typename T, typename C, typename = void>
struct ContextCastImpl
{
   static constexpr auto priority = ContextCastPriority::none;
};

template <typename T, typename C>
struct ContextCastImpl<T, C, enable_if_t<IsAnyCastAvailable<T, C>::value>>
   : std::enable_if<true>
{
   static constexpr auto priority =
      IsExactCastAvailable<T, C>::value ? ContextCastPriority::exact :
      IsReferenceCastAvailable<T, C>::value ? ContextCastPriority::reference :
      IsValueCastAvailable<T, C>::value ? ContextCastPriority::value :
      IsAnyCastAvailable<T, C>::value ? ContextCastPriority::custom :
      ContextCastPriority::none;

   C&& operator()(C&& c) const { return std::forward<C>(c); }
};

template <typename T, typename C>
struct ContextCastImpl<T*, C, enable_if_t<
   !IsAnyCastAvailable<T*, C>::value &&
   ContextCastImpl<T&, C>::priority >= ContextCastPriority::reference>>
   : std::enable_if<true>
{
   static constexpr auto priority = ContextCastPriority(int(ContextCastImpl<T&, C>::priority) - 2);

   T* operator()(C&& c) const
   {
      auto&& named = ContextCastImpl<T&, C>()(std::forward<C>(c));
      return &named;
   }
};

template <typename T, typename Head, typename Middle, typename Tail, typename = void>
struct ContextMatchImpl : std::enable_if<false> { };

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
struct ContextMatchImpl < T, std::tuple<Head...>, std::tuple<Current, Middle...>, std::tuple<Next, Tail...>,
   enable_if_t<ContextCastImpl<T, Current>::priority < ContextCastImpl<T, Next>::priority>>
   : ContextMatchImpl<T, std::tuple<Head..., Current, Middle...>, std::tuple<Next>, std::tuple<Tail...>> { };

template <typename T, typename...Head, typename Matched, typename...Tail>
struct ContextMatchImpl <T, std::tuple<Head...>, std::tuple<Matched, Tail...>, std::tuple<>>
   : std::enable_if<true>
{
   decltype(auto) operator()(Head&&..., Matched&& matched, Tail&&...) const
   {
      return ContextCastImpl<T, Matched>()(std::forward<Matched>(matched));
   }
};

template <typename T, typename...Context>
using ContextMatchFacade = ContextMatchImpl<T, std::tuple<>, std::tuple<>, std::tuple<Context...>>;

template <typename, typename = void>
struct SignatureOf;

template <typename T>
using SignatureType = TypeOf<SignatureOf<decay_t<T>>>;

template <typename T>
struct SignatureOf<T, IfAvailable<typename T::Signature>> { using type = typename T::Signature; };

template <typename R, typename...Args>
struct SignatureOf<R(Args...)> { using type = R(Args...); };

template <typename R, typename...Args>
struct SignatureOf<R(*)(Args...)> { using type = R(Args...); };

template <typename T, typename R, typename...Args>
struct SignatureOf<R(T::*)(Args...)> { using type = R(Args...); };

template <typename T, typename R, typename...Args>
struct SignatureOf<R(T::*)(Args...) const> { using type = R(Args...); };

template <typename Tail, typename Head, size_t tailSize, typename = void>
struct SplitSignatureImpl;

template <typename R, typename Next, typename...Tail, typename...Head, size_t tailSize>
struct SplitSignatureImpl<R(Next, Tail...), R(Head...), tailSize, enable_if_t<sizeof...(Tail) >= tailSize>>
   : SplitSignatureImpl<R(Tail...), R(Head..., Next), tailSize> { };

template <typename R, typename...Tail, typename...Head, size_t tailSize>
struct SplitSignatureImpl<R(Tail...), R(Head...), tailSize, enable_if_t<sizeof...(Tail) == tailSize>>
{
   using type = R(Head...);
};

template <typename T, size_t tailSize>
struct SplitSignature : SplitSignature<SignatureType<T>, tailSize> { };

template <typename R, typename...Args, size_t tailSize>
struct SplitSignature<R(Args...), tailSize> : SplitSignatureImpl<R(Args...), R(), tailSize> { };

template <typename Signature, size_t tailSize>
using SplitSignatureType = TypeOf<SplitSignature<Signature, tailSize>>;

template <typename T, typename Context, typename = void>
struct ContextCallImpl;

template <typename F, typename...Context>
using ContextCallFacade = ContextCallImpl<decay_t<F>, std::tuple<Context...>>;

template <typename R, typename...Args, typename Context>
struct ContextCallImpl<R(*)(Args...), Context> : ContextCallImpl<R(Args...), Context> { };

template <typename T, typename Context>
struct ContextCallImpl<T, Context, IfEnabled<ContextCallImpl<decltype(&decay_t<T>::operator()), Context>>>
   : ContextCallImpl<decltype(&decay_t<T>::operator()), Context> { };

template <typename T, typename R, typename...Args, typename Context>
struct ContextCallImpl<R(T::*)(Args...), Context> : ContextCallImpl<R(Args...), Context> { };

template <typename T, typename R, typename...Args, typename Context>
struct ContextCallImpl<R(T::*)(Args...) const, Context> : ContextCallImpl<R(Args...), Context> { };

// Non-variadic call
template <typename R, typename...Args, typename...Context>
struct ContextCallImpl<R(Args...), std::tuple<Context...>> : std::enable_if<true>
{
   template <typename F>
   R operator()(F&& f, Context&&...context) const
   {
      return f(ContextMatch<Args>(std::forward<Context>(context)..., nullptr)...);
   }
};

template <typename T, typename TemplateArgs, typename Context, typename = void>
struct TemplatedContextCall { };

template <typename T, typename Context>
struct ContextCallImpl<T, Context, IfEnabled<TemplatedContextCall<T, std::tuple<>, Context>>>
   : TemplatedContextCall<T, std::tuple<>, Context> { };

template <typename T, typename TemplateArgs, typename = void>
struct IsTemplatedFunctionOperatorAvailableImpl : std::false_type { };

template <typename T, typename...TemplateArgs>
struct IsTemplatedFunctionOperatorAvailableImpl<T, std::tuple<TemplateArgs...>,
   IfAvailable<decltype(&decay_t<T>::operator()<TemplateArgs...> )>> : std::true_type { };

template <typename T, typename...TemplateArgs>
using IsTemplatedFunctionOperatorAvailable = IsTemplatedFunctionOperatorAvailableImpl<T, std::tuple<TemplateArgs...>>;

template <typename T, typename...TemplateArgs>
struct TemplatedFunctionOperator
{
   static constexpr auto get() { return &decay_t<T>::operator()<TemplateArgs...>; }
   using type = decltype(get());
   using Signature = SignatureType<type>;
};

template <typename T, typename...TemplateArgs, typename Next, typename...Context>
struct TemplatedContextCall<T, std::tuple<TemplateArgs...>, std::tuple<Next, Context...>, enable_if_t<
   !IsTemplatedFunctionOperatorAvailable<T, TemplateArgs...>::value>>
      : TemplatedContextCall<T, std::tuple<TemplateArgs..., Next>, std::tuple<Context...>> { };

template <typename T, typename...TemplateArgs, typename...Context>
struct TemplatedContextCall<T, std::tuple<TemplateArgs...>, std::tuple<Context...>, enable_if_t<
   IsTemplatedFunctionOperatorAvailable<T, TemplateArgs...>::value &&
   !IsTemplatedFunctionOperatorAvailable<T, TemplateArgs..., TemplateArgs..., Context...>::value>>
      : ContextCallFacade<TypeOf<TemplatedFunctionOperator<T, TemplateArgs...>>, TemplateArgs..., Context...> { };

template <typename T, typename...Context>
struct VariadicContextCall;

template <typename T, typename...TemplateArgs, typename...Context>
struct TemplatedContextCall<T, std::tuple<TemplateArgs...>, std::tuple<Context...>, enable_if_t<
   IsTemplatedFunctionOperatorAvailable<T, TemplateArgs...>::value &&
   IsTemplatedFunctionOperatorAvailable<T, TemplateArgs..., TemplateArgs..., Context...>::value>>
      : VariadicContextCall<TemplatedFunctionOperator<T, TemplateArgs..., TemplateArgs..., Context...>, TemplateArgs..., Context...> { };

template <typename T, typename...Context>
struct VariadicContextCall : VariadicContextCall<SplitSignatureType<T, sizeof...(Context)>, Context...> { };

// Variadic call
template <typename R, typename...Args, typename...Context>
struct VariadicContextCall<R(Args...), Context...> : std::enable_if<true>
{
   template <typename F>
   R operator()(F&& f, Context&&...context) const
   {
      return f(ContextMatch<Args>(std::forward<Context>(context)..., nullptr)..., std::forward<Context>(context)...);
   }
};

} // namespace detail

template <typename...F>
decltype(auto) Aggregate(F&&...f)
{
   using namespace detail;
   return [&](auto&&...arg)
   {
      ContextForEach(std::forward<decltype(arg)>(arg)...)(std::forward<decltype(f)>(f)...);
   };
};

template <typename F, typename...Context>
decltype(auto) ContextCall(F&& f, Context&&...context)
{
   using namespace detail;

   static_assert(IsEnabled<ContextCallFacade<F, Context...>>::value,
      "A callable instance of unsupported type in " __FUNCSIG__);

   return ContextCallFacade<F, Context...>()(std::forward<F>(f), std::forward<Context>(context)...);
}

template <typename...Context>
decltype(auto) ContextForEach(Context&&...context)
{
   using namespace detail;
   return [&](auto&&...f)
   {
      auto callWrapper = [&](auto&& f)
      {
         ContextCall(std::forward<decltype(f)>(f), std::forward<decltype(context)>(context)...);
         return 0;
      };
      std::initializer_list<int> { callWrapper(std::forward<decltype(f)>(f))... };
   };
}

template <size_t index, typename...Context>
decltype(auto) ContextGet(Context&&...context)
{
   using namespace detail;
   static_assert(index < sizeof...(Context), "Actual parameter index is out of range: " __FUNCSIG__);
   return std::get<index>(std::tuple<Context&&...>(std::forward<Context>(context)...));
}

template <typename T, typename...Context>
decltype(auto) ContextMatch(Context&&...context)
{
   using namespace detail;

   static_assert(IsEnabled<ContextMatchFacade<T, Context...>>::value, "Context match failed: " __FUNCSIG__);

   return ContextMatchFacade<T, Context...>()(std::forward<Context>(context)...);
}

template <typename Data>
void Format(std::ostream& output, Data&& data)
{
   output << data;
}

template <typename Data, typename Arg, typename...Args>
void Format(std::ostream& output, Data&& data, Arg&& arg, Args&&...args)
{
   Format(output, data % arg, std::forward<Args>(args)...);
}

template <typename...Args>
std::string Format(const std::string& formatString, Args&&...args)
{
   using namespace detail;
   std::ostringstream output;
   Format(output, boost::format(formatString), std::forward<Args>(args)...);
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
