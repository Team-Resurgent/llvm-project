// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// std::move_only_function (C++23, [func.wrap.move]).
//
// Not yet implemented in upstream libc++; provided by the RXDK-360 llvm fork.
// A move-only, type-erased callable. Storage is __small_buffer: a small,
// trivially-relocatable target (a function pointer, a small trivial lambda) is
// held inline with no allocation; anything larger or non-trivially-destructible
// is heap-allocated. Because the buffer is trivially relocatable, move is just a
// buffer relocation + vtable-pointer steal; the per-target vtable only needs
// call + destroy.
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___FUNCTIONAL_MOVE_ONLY_FUNCTION_H
#define _LIBCPP___FUNCTIONAL_MOVE_ONLY_FUNCTION_H

#include <__config>
#include <__functional/invoke.h>
#include <__memory/addressof.h>
#include <__type_traits/decay.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/invoke.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_function.h>
#include <__type_traits/is_member_pointer.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_pointer.h>
#include <__utility/exchange.h>
#include <__utility/forward.h>
#include <__utility/in_place.h>
#include <__utility/move.h>
#include <__utility/small_buffer.h>
#include <cstddef>
#include <initializer_list>
#include <new>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

template <class...>
class move_only_function;

template <class _Tp>
inline constexpr bool __is_move_only_function_v = false;

template <class _Tp>
inline constexpr bool __is_in_place_type_v = false;
template <class _Tp>
inline constexpr bool __is_in_place_type_v<in_place_type_t<_Tp>> = true;

// Room for two pointers inline (a function pointer, or a lambda capturing a
// couple of trivial scalars, stays out of the heap).
using __mo_function_buffer = __small_buffer<2 * sizeof(void*), alignof(void*)>;

// One macro body per signature form. _CV is {} or const; _REF is {}, & or &&
// (the operator() ref-qualifier); _INV_REF is & or && (the value category the
// target is invoked with); _NE is the noexcept bool.
#  define _LIBCPP_MOF_SPECIALIZATION(_CV, _REF, _INV_REF, _NE)                                                          \
    template <class _ReturnT, class... _ArgTypes>                                                                       \
    class move_only_function<_ReturnT(_ArgTypes...) _CV _REF noexcept(_NE)> {                                           \
      template <class _Tp>                                                                                              \
      using __inv_quals _LIBCPP_NODEBUG = _CV _Tp _INV_REF;                                                             \
                                                                                                                       \
      using __call_t _LIBCPP_NODEBUG    = _ReturnT (*)(__mo_function_buffer&, _ArgTypes...) noexcept(_NE);              \
      using __destroy_t _LIBCPP_NODEBUG = void (*)(__mo_function_buffer&) noexcept;                                     \
      struct __vtable_t {                                                                                               \
        __call_t __call_;                                                                                               \
        __destroy_t __destroy_;                                                                                        \
      };                                                                                                                \
      template <class _Fn>                                                                                              \
      static _ReturnT __call_impl(__mo_function_buffer& __buf, _ArgTypes... __args) noexcept(_NE) {                     \
        return std::__invoke_r<_ReturnT>(                                                                               \
            static_cast<__inv_quals<_Fn>>(*__buf.template __get<_Fn>()), std::forward<_ArgTypes>(__args)...);           \
      }                                                                                                                 \
      template <class _Fn>                                                                                             \
      static void __destroy_impl(__mo_function_buffer& __buf) noexcept {                                                \
        __buf.template __get<_Fn>()->~_Fn();                                                                            \
        __buf.template __dealloc<_Fn>();                                                                                \
      }                                                                                                                 \
      template <class _Fn>                                                                                             \
      static constexpr __vtable_t __vtable_for{&__call_impl<_Fn>, &__destroy_impl<_Fn>};                                \
                                                                                                                       \
      template <class _VT>                                                                                             \
      static constexpr bool __is_callable_from =                                                                        \
          _NE ? is_nothrow_invocable_r_v<_ReturnT, __inv_quals<_VT>, _ArgTypes...>                                      \
              : is_invocable_r_v<_ReturnT, __inv_quals<_VT>, _ArgTypes...>;                                             \
                                                                                                                       \
      __mo_function_buffer __storage_;                                                                                 \
      const __vtable_t* __vtable_ = nullptr;                                                                            \
                                                                                                                       \
      _LIBCPP_HIDE_FROM_ABI void __reset() noexcept {                                                                   \
        if (__vtable_)                                                                                                  \
          __vtable_->__destroy_(__storage_);                                                                            \
        __vtable_ = nullptr;                                                                                            \
      }                                                                                                                \
      template <class _VT, class... _Args>                                                                             \
      _LIBCPP_HIDE_FROM_ABI void __emplace(_Args&&... __args) {                                                         \
        __storage_.template __construct<_VT>(std::forward<_Args>(__args)...);                                           \
        __vtable_ = std::addressof(__vtable_for<_VT>);                                                                  \
      }                                                                                                                 \
                                                                                                                       \
    public:                                                                                                             \
      using result_type = _ReturnT;                                                                                     \
                                                                                                                       \
      _LIBCPP_HIDE_FROM_ABI move_only_function() noexcept = default;                                                    \
      _LIBCPP_HIDE_FROM_ABI move_only_function(nullptr_t) noexcept {}                                                   \
      move_only_function(const move_only_function&)            = delete;                                                \
      move_only_function& operator=(const move_only_function&) = delete;                                                \
                                                                                                                       \
      _LIBCPP_HIDE_FROM_ABI move_only_function(move_only_function&& __other) noexcept                                   \
          : __storage_(std::move(__other.__storage_)), __vtable_(std::__exchange(__other.__vtable_, nullptr)) {}        \
                                                                                                                       \
      template <class _Fn,                                                                                              \
                class _VT = decay_t<_Fn>,                                                                               \
                __enable_if_t<!is_same_v<_VT, move_only_function> && !__is_in_place_type_v<_VT> &&                      \
                                  __is_callable_from<_VT> && is_constructible_v<_VT, _Fn>,                              \
                              int> = 0>                                                                                 \
      _LIBCPP_HIDE_FROM_ABI move_only_function(_Fn&& __fn) {                                                            \
        if constexpr (is_member_pointer_v<_VT> || is_function_v<remove_pointer_t<_VT>>) {                               \
          if (__fn == nullptr)                                                                                          \
            return;                                                                                                     \
        }                                                                                                              \
        __emplace<_VT>(std::forward<_Fn>(__fn));                                                                        \
      }                                                                                                                 \
                                                                                                                       \
      template <class _Tp,                                                                                              \
                class... _Args,                                                                                         \
                class _VT = decay_t<_Tp>,                                                                               \
                __enable_if_t<__is_callable_from<_VT> && is_constructible_v<_VT, _Args...>, int> = 0>                   \
      _LIBCPP_HIDE_FROM_ABI explicit move_only_function(in_place_type_t<_Tp>, _Args&&... __args) {                      \
        __emplace<_VT>(std::forward<_Args>(__args)...);                                                                 \
      }                                                                                                                 \
                                                                                                                       \
      template <class _Tp,                                                                                              \
                class _Up,                                                                                              \
                class... _Args,                                                                                         \
                class _VT = decay_t<_Tp>,                                                                               \
                __enable_if_t<__is_callable_from<_VT> && is_constructible_v<_VT, initializer_list<_Up>&, _Args...>,     \
                              int> = 0>                                                                                 \
      _LIBCPP_HIDE_FROM_ABI explicit move_only_function(                                                                \
          in_place_type_t<_Tp>, initializer_list<_Up> __il, _Args&&... __args) {                                       \
        __emplace<_VT>(__il, std::forward<_Args>(__args)...);                                                           \
      }                                                                                                                 \
                                                                                                                       \
      _LIBCPP_HIDE_FROM_ABI move_only_function& operator=(move_only_function&& __other) noexcept {                      \
        if (this != std::addressof(__other)) {                                                                          \
          __reset();                                                                                                    \
          __storage_ = std::move(__other.__storage_);                                                                   \
          __vtable_  = std::__exchange(__other.__vtable_, nullptr);                                                     \
        }                                                                                                              \
        return *this;                                                                                                   \
      }                                                                                                                \
      _LIBCPP_HIDE_FROM_ABI move_only_function& operator=(nullptr_t) noexcept {                                         \
        __reset();                                                                                                      \
        return *this;                                                                                                   \
      }                                                                                                                \
      template <class _Fn, __enable_if_t<is_constructible_v<move_only_function, _Fn>, int> = 0>                         \
      _LIBCPP_HIDE_FROM_ABI move_only_function& operator=(_Fn&& __fn) {                                                 \
        move_only_function(std::forward<_Fn>(__fn)).swap(*this);                                                        \
        return *this;                                                                                                   \
      }                                                                                                                \
                                                                                                                       \
      _LIBCPP_HIDE_FROM_ABI ~move_only_function() { __reset(); }                                                        \
                                                                                                                       \
      _LIBCPP_HIDE_FROM_ABI explicit operator bool() const noexcept { return __vtable_ != nullptr; }                   \
                                                                                                                       \
      _LIBCPP_HIDE_FROM_ABI _ReturnT operator()(_ArgTypes... __args) _CV _REF noexcept(_NE) {                          \
        return __vtable_->__call_(const_cast<__mo_function_buffer&>(__storage_), std::forward<_ArgTypes>(__args)...);   \
      }                                                                                                                \
                                                                                                                       \
      _LIBCPP_HIDE_FROM_ABI void swap(move_only_function& __other) noexcept {                                          \
        move_only_function __tmp = std::move(*this);                                                                    \
        *this                    = std::move(__other);                                                                  \
        __other                  = std::move(__tmp);                                                                   \
      }                                                                                                                \
      _LIBCPP_HIDE_FROM_ABI friend void swap(move_only_function& __x, move_only_function& __y) noexcept { __x.swap(__y); } \
      _LIBCPP_HIDE_FROM_ABI friend bool operator==(const move_only_function& __f, nullptr_t) noexcept { return !__f; }  \
    };                                                                                                                  \
    template <class _ReturnT, class... _ArgTypes>                                                                       \
    inline constexpr bool                                                                                               \
        __is_move_only_function_v<move_only_function<_ReturnT(_ArgTypes...) _CV _REF noexcept(_NE)>> = true;

#  define _LIBCPP_MOF_ALL_CV_REF(_NE)                                                                                   \
    _LIBCPP_MOF_SPECIALIZATION(, , &, _NE)                                                                              \
    _LIBCPP_MOF_SPECIALIZATION(, &, &, _NE)                                                                             \
    _LIBCPP_MOF_SPECIALIZATION(, &&, &&, _NE)                                                                           \
    _LIBCPP_MOF_SPECIALIZATION(const, , &, _NE)                                                                         \
    _LIBCPP_MOF_SPECIALIZATION(const, &, &, _NE)                                                                        \
    _LIBCPP_MOF_SPECIALIZATION(const, &&, &&, _NE)

_LIBCPP_MOF_ALL_CV_REF(false)
_LIBCPP_MOF_ALL_CV_REF(true)

#  undef _LIBCPP_MOF_ALL_CV_REF
#  undef _LIBCPP_MOF_SPECIALIZATION

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___FUNCTIONAL_MOVE_ONLY_FUNCTION_H
