// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// views::chunk (C++23, P2442) -- split a range into subranges of size n.
//
// Not yet implemented in upstream libc++; provided by the RXDK-360 llvm fork.
// This implements the forward_range form (the common case: vector/array/string/
// ...). The input_range-only form is a documented later refinement.
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___RANGES_CHUNK_VIEW_H
#define _LIBCPP___RANGES_CHUNK_VIEW_H

#include <__config>
#include <__assert>
#include <__concepts/constructible.h>
#include <__functional/bind_back.h>
#include <__iterator/advance.h>
#include <__iterator/default_sentinel.h>
#include <__iterator/distance.h>
#include <__iterator/iterator_traits.h>
#include <__ranges/access.h>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/enable_borrowed_range.h>
#include <__ranges/range_adaptor.h>
#include <__ranges/subrange.h>
#include <__ranges/take_view.h>
#include <__ranges/view_interface.h>
#include <__type_traits/maybe_const.h>
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
class chunk_view : public view_interface<chunk_view<_View>> {
  _View __base_                       = _View();
  range_difference_t<_View> __n_      = 0;

  class __iterator;

public:
  _LIBCPP_HIDE_FROM_ABI chunk_view()
    requires default_initializable<_View>
  = default;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit chunk_view(_View __base, range_difference_t<_View> __n)
      : __base_(std::move(__base)), __n_(__n) {
    _LIBCPP_ASSERT_ARGUMENT_WITHIN_DOMAIN(__n > 0, "chunk size must be greater than 0");
  }

  _LIBCPP_HIDE_FROM_ABI constexpr _View base() const&
    requires copy_constructible<_View>
  {
    return __base_;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr _View base() && { return std::move(__base_); }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator begin() { return __iterator(this, ranges::begin(__base_)); }

  _LIBCPP_HIDE_FROM_ABI constexpr auto end() {
    if constexpr (common_range<_View> && sized_range<_View>)
      return __iterator(this, ranges::end(__base_));
    else
      return default_sentinel;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto size()
    requires sized_range<_View>
  {
    return std::__to_unsigned_like((ranges::distance(__base_) + __n_ - 1) / __n_);
  }
};

template <class _Range>
chunk_view(_Range&&, range_difference_t<_Range>) -> chunk_view<views::all_t<_Range>>;

template <view _View>
  requires forward_range<_View>
class chunk_view<_View>::__iterator {
  chunk_view* __parent_               = nullptr;
  iterator_t<_View> __current_        = iterator_t<_View>();
  range_difference_t<_View> __n_      = 0;
  range_difference_t<_View> __missing_ = 0;

  friend chunk_view;

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(chunk_view* __parent, iterator_t<_View> __current)
      : __parent_(__parent), __current_(std::move(__current)), __n_(__parent->__n_) {}

public:
  using iterator_category = input_iterator_tag;
  using iterator_concept  = forward_iterator_tag;
  using value_type        = decltype(views::take(subrange(__current_, ranges::end(__parent_->__base_)), __n_));
  using difference_type   = range_difference_t<_View>;

  _LIBCPP_HIDE_FROM_ABI __iterator() = default;

  _LIBCPP_HIDE_FROM_ABI constexpr value_type operator*() const {
    _LIBCPP_ASSERT_VALID_ELEMENT_ACCESS(__current_ != ranges::end(__parent_->__base_), "dereferencing past-the-end");
    return views::take(subrange(__current_, ranges::end(__parent_->__base_)), __n_);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator++() {
    __missing_ = ranges::advance(__current_, __n_, ranges::end(__parent_->__base_));
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
  _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const __iterator& __x, default_sentinel_t) {
    return __x.__current_ == ranges::end(__x.__parent_->__base_);
  }
};

template <class _Tp>
inline constexpr bool enable_borrowed_range<chunk_view<_Tp>> = false;

namespace views {
namespace __chunk_view {
struct __fn {
  template <viewable_range _Range>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Range&& __range, range_difference_t<_Range> __n) const
      -> decltype(chunk_view(std::forward<_Range>(__range), __n)) {
    return chunk_view(std::forward<_Range>(__range), __n);
  }

  template <class _Np>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Np&& __n) const {
    return __pipeable(std::__bind_back(*this, std::forward<_Np>(__n)));
  }
};
} // namespace __chunk_view

inline namespace __cpo {
inline constexpr auto chunk = __chunk_view::__fn{};
} // namespace __cpo
} // namespace views

} // namespace ranges

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___RANGES_CHUNK_VIEW_H
