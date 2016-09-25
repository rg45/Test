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

template <bool condition, typename type = void>
using EnableIf = typename std::enable_if<condition, type>::type;

} // namespace detail
} // namespace ElementwiseOperations
} // namespace RS
} // namespace cqg
