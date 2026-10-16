; RUN: llc -mtriple=powerpc64-unknown-xbox360 -verify-machineinstrs < %s | FileCheck %s
;
; Xbox 360: every reservation runs with external interrupts disabled, the form
; the XDK compiler emits for the Interlocked intrinsics (the Xenon CPU has bugs
; in lwarx/stwcx., and the kernel loader reports any other use as an improper
; instruction sequence):
;   mfmsr rT ; mtmsrd r13, 1 ; l[wd]arx ... st[wd]cx. ; mtmsrd rT, 1
; A path that leaves without storing (a failed compare, a min/max that keeps
; the old value) stores the loaded value back and restores the MSR too.

define i32 @add32(ptr %p, i32 %v) {
; CHECK-LABEL: add32:
; CHECK:       mfmsr [[MSR:[0-9]+]]
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  lwarx
; CHECK:       stwcx.
; CHECK-NEXT:  mtmsrd [[MSR]], 1
  %r = atomicrmw add ptr %p, i32 %v monotonic
  ret i32 %r
}

define i64 @add64(ptr %p, i64 %v) {
; CHECK-LABEL: add64:
; CHECK:       mfmsr [[MSR:[0-9]+]]
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  ldarx
; CHECK:       stdcx.
; CHECK-NEXT:  mtmsrd [[MSR]], 1
  %r = atomicrmw add ptr %p, i64 %v monotonic
  ret i64 %r
}

define i8 @add8(ptr %p, i8 %v) {
; CHECK-LABEL: add8:
; CHECK:       mfmsr [[MSR:[0-9]+]]
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  lwarx
; CHECK:       stwcx.
; CHECK-NEXT:  mtmsrd [[MSR]], 1
  %r = atomicrmw add ptr %p, i8 %v monotonic
  ret i8 %r
}

define i32 @xchg32(ptr %p, i32 %v) {
; CHECK-LABEL: xchg32:
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  lwarx
; CHECK-NEXT:  stwcx.
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
  %r = atomicrmw xchg ptr %p, i32 %v monotonic
  ret i32 %r
}

; A failed compare clears the reservation with a store of the loaded value.
define i32 @cas32(ptr %p, i32 %e, i32 %d) {
; CHECK-LABEL: cas32:
; CHECK:       mfmsr [[MSR:[0-9]+]]
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  lwarx [[OLD:[0-9]+]], 0, [[P:[0-9]+]]
; CHECK:       stwcx. 5, 0, [[P]]
; CHECK-NEXT:  mtmsrd [[MSR]], 1
; CHECK:       stwcx. [[OLD]], 0, [[P]]
; CHECK-NEXT:  mtmsrd [[MSR]], 1
  %r = cmpxchg ptr %p, i32 %e, i32 %d monotonic monotonic
  %v = extractvalue { i32, i1 } %r, 0
  ret i32 %v
}

define i64 @cas64(ptr %p, i64 %e, i64 %d) {
; CHECK-LABEL: cas64:
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  ldarx [[OLD:[0-9]+]], 0, [[P:[0-9]+]]
; CHECK:       stdcx. 5, 0, [[P]]
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
; CHECK:       stdcx. [[OLD]], 0, [[P]]
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
  %r = cmpxchg ptr %p, i64 %e, i64 %d monotonic monotonic
  %v = extractvalue { i64, i1 } %r, 0
  ret i64 %v
}

define i16 @cas16(ptr %p, i16 %e, i16 %d) {
; CHECK-LABEL: cas16:
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  lwarx [[WORD:[0-9]+]]
; CHECK:       stwcx.
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
; CHECK:       stwcx. [[WORD]]
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
  %r = cmpxchg ptr %p, i16 %e, i16 %d monotonic monotonic
  %v = extractvalue { i16, i1 } %r, 0
  ret i16 %v
}

; A max that keeps the old value leaves through the clearing store.
define i32 @max32(ptr %p, i32 %v) {
; CHECK-LABEL: max32:
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  lwarx [[OLD:[0-9]+]]
; CHECK:       stwcx. 4
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
; CHECK:       stwcx. [[OLD]]
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
  %r = atomicrmw max ptr %p, i32 %v monotonic
  ret i32 %r
}

define i8 @umin8(ptr %p, i8 %v) {
; CHECK-LABEL: umin8:
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  lwarx [[WORD:[0-9]+]]
; CHECK:       stwcx.
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
; CHECK:       stwcx. [[WORD]]
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
  %r = atomicrmw umin ptr %p, i8 %v monotonic
  ret i8 %r
}

; Orderings only add barriers outside the protected region.
define i32 @add32_seqcst(ptr %p, i32 %v) {
; CHECK-LABEL: add32_seqcst:
; CHECK:       sync
; CHECK:       mtmsrd 13, 1
; CHECK-NEXT:  lwarx
; CHECK:       stwcx.
; CHECK-NEXT:  mtmsrd {{[0-9]+}}, 1
; CHECK:       lwsync
  %r = atomicrmw add ptr %p, i32 %v seq_cst
  ret i32 %r
}
