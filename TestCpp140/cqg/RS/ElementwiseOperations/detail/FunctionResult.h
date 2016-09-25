#pragma once

#include "instance.h"

namespace cqg
{
namespace RS
{
namespace ElementwiseOperations
{
namespace detail
{

template <typename F, typename... Args>
using FunctionResult = decltype(instance<F&&>()(instance<Args&&>()...));

} // namespace detail
} // namespace ElementwiseOperations
} // namespace RS
} // namespace cqg
