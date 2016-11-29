#include "stdafx.h"

#include <Gateway/RiskServer/TestFramework/TestScenarioTools.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

#include <boost/noncopyable.hpp>

namespace ContextCallRemake
{
namespace detail
{
using cqg::RS::TestFramework::TestScenarioTools::ContextMatch;
using cqg::RS::TestFramework::TestScenarioTools::detail::decay_t;
using cqg::RS::TestFramework::TestScenarioTools::detail::enable_if_t;

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

template <typename T, typename TemplateArgs, typename Context, typename = void>
struct TemplatedContextCall { };

template <typename T, typename Context>
struct ContextCallImpl<T, Context, IfEnabled<TemplatedContextCall<T, std::tuple<>, Context>>>
   : TemplatedContextCall<T, std::tuple<>, Context> { };

template <typename T, typename TemplateArgs, typename = void>
struct TemplatedFunctionOperatorImpl;

template <typename T, typename...TemplateArgs>
struct TemplatedFunctionOperatorImpl<T, std::tuple<TemplateArgs...>,
   IfAvailable<decltype(&decay_t<T>::operator()<TemplateArgs...>)>>
{
   static constexpr auto get() { return &decay_t<T>::operator()<TemplateArgs...>; }
   using type = decltype(get());
};

template <typename T, typename...TemplateArgs>
using TemplatedFunctionOperator = TemplatedFunctionOperatorImpl<T, std::tuple<TemplateArgs...>>;

template <typename T, typename...TemplateArgs, typename Next, typename...Context>
struct TemplatedContextCall<T, std::tuple<TemplateArgs...>, std::tuple<Next, Context...>, enable_if_t<
   !IsEnabled<TemplatedFunctionOperator<T, TemplateArgs...>>::value>>
      : TemplatedContextCall<T, std::tuple<TemplateArgs..., Next>, std::tuple<Context...>> { };

template <typename T, typename...TemplateArgs, typename...Context>
struct TemplatedContextCall<T, std::tuple<TemplateArgs...>, std::tuple<Context...>, enable_if_t<
   IsEnabled<TemplatedFunctionOperator<T, TemplateArgs...>>::value &&
   !IsEnabled<TemplatedFunctionOperator<T, TemplateArgs..., TemplateArgs..., Context...>>::value>>
      : ContextCallFacade<TypeOf<TemplatedFunctionOperator<T, TemplateArgs...>>, TemplateArgs..., Context...> { };

template <typename T, typename...TemplateArgs, typename...Context>
struct TemplatedContextCall<T, std::tuple<TemplateArgs...>, std::tuple<Context...>, enable_if_t<
   IsEnabled<TemplatedFunctionOperator<T, TemplateArgs...>>::value &&
   IsEnabled<TemplatedFunctionOperator<T, TemplateArgs..., TemplateArgs..., Context...>>::value>>
      : TemplatedContextCall<TypeOf<TemplatedFunctionOperator<T, TemplateArgs...>>, void,
         std::tuple<TemplateArgs..., Context...>> { };

template <typename T, typename R, typename...Args, typename Context>
struct TemplatedContextCall<R(T::*)(Args...), void, Context>
   : TemplatedContextCall<R(Args...), void, Context> { };

template <typename T, typename R, typename...Args, typename Context>
struct TemplatedContextCall<R(T::*)(Args...) const, void, Context>
   : TemplatedContextCall<R(Args...), void, Context> { };

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

// Variadic call
template <typename R, typename...Args, typename...Context>
struct TemplatedContextCall<R(Args...), void, std::tuple<Context...>> : std::enable_if<true>
{
   template <typename F>
   R operator()(F&& f, Context&&...context) const
   {
      return f(ContextMatch<Args>(std::forward<Context>(context)..., nullptr)..., std::forward<Context>(context)...);
   }
};

} // namespace detail

template <typename F, typename...Context>
decltype(auto) ContextCall(F&& f, Context&&...context)
{
   using namespace detail;

   static_assert(IsEnabled<ContextCallFacade<F, Context...>>::value,
      "A callable instance of unsupported type in " __FUNCSIG__);

   return ContextCallFacade<F, Context...>()(std::forward<F>(f), std::forward<Context>(context)...);
}

void test()
{
   using namespace detail;

   PRINT_CAPTION();

   struct Aux
   {
      static void foo(const std::string& key, double value)
      {
         PRINT_CAPTION();
         std::cout << key << " = " << value << std::endl;
      }
      int operator()(const std::string& key, double value) // const
      {
         PRINT_CAPTION();
         std::cout << key << " = " << value << std::endl;
         return 1001;
      }
   };

   auto l1 = [](const std::string& key, double value)
   {
      PRINT_CAPTION();
      std::cout << key << " = " << value << std::endl;
      return 1002;
   };

   ContextCall(Aux::foo, 42, 3.14, "Pi");
   PRINT(ContextCall(Aux(), 42, 3.14, "Pi"));
   PRINT(ContextCall(l1, 42, 3.14, "Pi"));

   auto l2 = [](auto&&, const std::string& key, auto&& value, auto&&...context)
   {
      PRINT_CAPTION();
      PRINT(sizeof...(context));
      std::cout << key << " = " << value << std::endl;
      return 1003;
   };

   PRINT(ContextCall(l2, 42, 3.14, "Pi"));

}
} // namespace ContextCallRemake

