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

template <typename Iterator>
using IteratorReference = decltype(*instance<Iterator&&>());

} // namespace detail
} // namespace ElementwiseOperations
} // namespace RS
} // namespace cqg
