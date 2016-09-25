#pragma once

#include <type_traits>

namespace cqg
{
namespace RS
{
namespace ElementwiseOperations
{
namespace detail
{

template <typename T> using Decay = typename std::decay<T>::type;

} // namespace detail
} // namespace ElementwiseOperations
} // namespace RS
} // namespace cqg
