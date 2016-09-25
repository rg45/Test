#pragma once

#include "FunctionResult.h"

namespace cqg
{
namespace RS
{
namespace ElementwiseOperations
{
namespace detail
{

template <typename F, typename... Args>
using FunctionSignature = FunctionResult<F, Args...>(Args...);

} // namespace detail
} // namespace ElementwiseOperations
} // namespace RS
} // namespace cqg
