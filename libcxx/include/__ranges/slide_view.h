// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// views::slide (C++23, P2442) -- all size-n sliding windows of a range.
//
// Not yet implemented in upstream libc++; provided by the RXDK-360 llvm fork.
// forward_range form (the common case).
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___RANGES_SLIDE_VIEW_H
#define _LIBCPP___RANGES_SLIDE_VIEW_H

#include <__config>
#include <__assert>
#include <__concepts/constructible.h>
#include <__functional/bind_back.h>
#include <__iterator/advance.h>
#include <__iterator/default_sentinel.h>
#include <__iterator/distance.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/next.h>
#include <__ranges/access.h>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/enable_borrowed_range.h>
#include <__ranges/range_adaptor.h>
#include <__ranges/subrange.h>
#include <__ranges/view_interface.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <__utility/to_underlying.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

namespace ranges {

template <view _View>
  requires forward_range<_View>
class slide_view : public view_interface<slide_view<_View>> {
  _View __base_                  = _View();
  range_difference_t<_View> __n_ = 0;

  class __iterator;
  class __sentinel;

public:
  _LIBCPP_HIDE_FROM_ABI slide_view()
    requires default_initializable<_View>
  = default;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit slide_view(_View __base, range_difference_t<_View> __n)
      : __base_(std::move(__base)), __n_(__n) {
    _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(__n > 0, "slide window size must be greater than 0");
  }

  _LIBCPP_HIDE_FROM_ABI constexpr _View base() const&
    requires copy_constructible<_View>
  {
    return __base_;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr _View base() && { return std::move(__base_); }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator begin() {
    auto __first     = ranges::begin(__base_);
    auto __last_ele  = ranges::next(__first, __n_ - 1, ranges::end(__base_));
    return __iterator(std::move(__first), std::move(__last_ele), __n_);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto end() {
    if constexpr (common_range<_View>) {
      auto __last = ranges::end(__base_);
      // window start for the past-the-end position is (last - (n-1)); clamp.
      auto __first = ranges::begin(__base_);
      auto __start = ranges::next(__first, ranges::distance(__first, __last) - __n_ + 1);
      return __iterator(std::move(__start), std::move(__last), __n_);
    } else {
      return __sentinel(ranges::end(__base_));
    }
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto size()
    requires sized_range<_View>
  {
    auto __sz = ranges::distance(__base_) - __n_ + 1;
    if (__sz < 0)
      __sz = 0;
    return std::__to_unsigned_like(__sz);
  }
};

template <class _Range>
slide_view(_Range&&, range_difference_t<_Range>) -> slide_view<views::all_t<_Range>>;

template <view _View>
  requires forward_range<_View>
class slide_view<_View>::__iterator {
  iterator_t<_View> __current_       = iterator_t<_View>();
  iterator_t<_View> __last_ele_      = iterator_t<_View>();
  range_difference_t<_View> __n_     = 0;

  friend slide_view;
  friend __sentinel;

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(iterator_t<_View> __current, iterator_t<_View> __last_ele,
                                             range_difference_t<_View> __n)
      : __current_(std::move(__current)), __last_ele_(std::move(__last_ele)), __n_(__n) {}

public:
  using iterator_category = input_iterator_tag;
  using iterator_concept  = forward_iterator_tag;
  using value_type        = decltype(subrange(__current_, ranges::next(__last_ele_)));
  using difference_type   = range_difference_t<_View>;

  _LIBCPP_HIDE_FROM_ABI __iterator() = default;

  _LIBCPP_HIDE_FROM_ABI constexpr value_type operator*() const {
    return subrange(__current_, ranges::next(__last_ele_));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator++() {
    ++__current_;
    ++__last_ele_;
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr __iterator operator++(int) {
    auto __tmp = *this;
    ++*this;
    return __tmp;
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator& __x, const __iterator& __y) {
    return __x.__current_ == __y.__current_;
  }
};

template <view _View>
  requires forward_range<_View>
class slide_view<_View>::__sentinel {
  sentinel_t<_View> __end_ = sentinel_t<_View>();
  friend slide_view;
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __sentinel(sentinel_t<_View> __end) : __end_(std::move(__end)) {}

public:
  _LIBCPP_HIDE_FROM_ABI __sentinel() = default;
  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator& __x, const __sentinel& __y) {
    return __x.__last_ele_ == __y.__end_;
  }
};

template <class _Tp>
inline constexpr bool enable_borrowed_range<slide_view<_Tp>> = enable_borrowed_range<_Tp>;

namespace views {
namespace __slide_view {
struct __fn {
  template <viewable_range _Range>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range, range_difference_t<_Range> __n) const
      -> decltype(slide_view(std::forward<_Range>(__range), __n)) {
    return slide_view(std::forward<_Range>(__range), __n);
  }

  template <class _Np>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Np&& __n) const {
    return __pipeable(std::__bind_back(*this, std::forward<_Np>(__n)));
  }
};
} // namespace __slide_view

inline namespace __cpo {
inline constexpr auto slide = __slide_view::__fn{};
} // namespace __cpo
} // namespace views

} // namespace ranges

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___RANGES_SLIDE_VIEW_H
