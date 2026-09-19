// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// views::cartesian_product (C++23, P2374) -- the n-ary cartesian product of
// ranges, yielding a tuple of references per combination.
//
// Not yet implemented in upstream libc++; provided by the RXDK-360 llvm fork.
// Implemented for forward ranges (the common case) as an odometer over a tuple
// of iterators (least-significant = last range).
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___RANGES_CARTESIAN_PRODUCT_VIEW_H
#define _LIBCPP___RANGES_CARTESIAN_PRODUCT_VIEW_H

#include <__config>
#include <__concepts/constructible.h>
#include <__iterator/iterator_traits.h>
#include <__ranges/access.h>
#include <__ranges/all.h>
#include <__ranges/concepts.h>
#include <__ranges/enable_borrowed_range.h>
#include <__ranges/size.h>
#include <__ranges/view_interface.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstddef>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

namespace ranges {

template <input_range _First, forward_range... _Vs>
  requires(view<_First> && ... && view<_Vs>)
class cartesian_product_view : public view_interface<cartesian_product_view<_First, _Vs...>> {
  tuple<_First, _Vs...> __bases_;

  class __iterator;

public:
  _LIBCPP_HIDE_FROM_ABI cartesian_product_view() = default;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit cartesian_product_view(_First __first, _Vs... __rest)
      : __bases_(std::move(__first), std::move(__rest)...) {}

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator begin() {
    using _ITuple = tuple<iterator_t<_First>, iterator_t<_Vs>...>;
    _ITuple __its = std::apply([](auto&... __b) { return _ITuple(ranges::begin(__b)...); }, __bases_);
    const bool __any_empty =
        std::apply([](auto&... __b) { return ((ranges::begin(__b) == ranges::end(__b)) || ...); }, __bases_);
    if (__any_empty)
      std::get<0>(__its) = ranges::end(std::get<0>(__bases_));
    return __iterator(this, std::move(__its));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator end() {
    using _ITuple = tuple<iterator_t<_First>, iterator_t<_Vs>...>;
    _ITuple __its = std::apply([](auto&... __b) { return _ITuple(ranges::begin(__b)...); }, __bases_);
    std::get<0>(__its) = ranges::end(std::get<0>(__bases_));
    return __iterator(this, std::move(__its));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto size()
    requires(sized_range<_First> && ... && sized_range<_Vs>)
  {
    return std::apply([](auto&... __b) { return (static_cast<size_t>(ranges::size(__b)) * ... * size_t{1}); }, __bases_);
  }
};

template <class... _Ranges>
cartesian_product_view(_Ranges&&...) -> cartesian_product_view<views::all_t<_Ranges>...>;

template <input_range _First, forward_range... _Vs>
  requires(view<_First> && ... && view<_Vs>)
class cartesian_product_view<_First, _Vs...>::__iterator {
  cartesian_product_view* __parent_ = nullptr;
  tuple<iterator_t<_First>, iterator_t<_Vs>...> __current_;

  friend cartesian_product_view;

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator(cartesian_product_view* __parent,
                                             tuple<iterator_t<_First>, iterator_t<_Vs>...> __current)
      : __parent_(__parent), __current_(std::move(__current)) {}

  template <size_t _Nth>
  _LIBCPP_HIDE_FROM_ABI constexpr void __next() {
    auto& __it = std::get<_Nth>(__current_);
    ++__it;
    if constexpr (_Nth > 0) {
      if (__it == ranges::end(std::get<_Nth>(__parent_->__bases_))) {
        __it = ranges::begin(std::get<_Nth>(__parent_->__bases_));
        __next<_Nth - 1>();
      }
    }
  }

public:
  using iterator_category = input_iterator_tag;
  using iterator_concept  = forward_iterator_tag;
  using value_type        = tuple<range_value_t<_First>, range_value_t<_Vs>...>;
  using reference         = tuple<range_reference_t<_First>, range_reference_t<_Vs>...>;
  using difference_type   = ptrdiff_t;

  _LIBCPP_HIDE_FROM_ABI __iterator() = default;

  _LIBCPP_HIDE_FROM_ABI constexpr reference operator*() const {
    return std::apply([](auto const&... __its) { return reference(*__its...); }, __current_);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr __iterator& operator++() {
    __next<sizeof...(_Vs)>();
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

namespace views {
namespace __cartesian_product {
struct __fn {
  template <class... _Ranges>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Ranges&&... __ranges) const
      -> decltype(cartesian_product_view(std::forward<_Ranges>(__ranges)...)) {
    return cartesian_product_view(std::forward<_Ranges>(__ranges)...);
  }
};
} // namespace __cartesian_product

inline namespace __cpo {
inline constexpr auto cartesian_product = __cartesian_product::__fn{};
} // namespace __cpo
} // namespace views

} // namespace ranges

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___RANGES_CARTESIAN_PRODUCT_VIEW_H
