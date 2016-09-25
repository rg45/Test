#pragma once

#include <boost/range/has_range_iterator.hpp>
#include <boost/range/iterator.hpp>

namespace cqg
{
namespace RS
{
namespace ElementwiseOperations
{
namespace detail
{

template <typename, typename = void> struct RangeIterator_;

template <typename Range>
struct RangeIterator_<Range, EnableIf<boost::has_range_iterator<Range>::value>>
{
   typedef typename boost::range_iterator<Range>::type Iterator;
};

template <typename Range>
using RangeIterator = typename RangeIterator_<Range>::Iterator;

} // namespace detail
} // namespace ElementwiseOperations
} // namespace RS
} // namespace cqg
