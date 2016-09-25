#pragma once

#include "RangeIterator.h"

#include <boost/mpl/bool.hpp>
#include <type_traits>

namespace cqg
{
namespace RS
{
namespace ElementwiseOperations
{
namespace detail
{

template <typename T, typename = RangeIterator<T>> struct EnableIfRange_ : std::enable_if<true> { };
template <typename T> using EnableIfRange = typename EnableIfRange_<T>::type;
template <typename, typename = void> struct IsRange_ : boost::mpl::false_ { };
template <typename T> struct IsRange_<T, EnableIfRange<T>> : boost::mpl::true_ { };

template <typename T> constexpr bool IsRange = IsRange_<T>::value;

} // namespace detail
} // namespace ElementwiseOperations
} // namespace RS
} // namespace cqg
