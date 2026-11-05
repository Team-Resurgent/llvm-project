; RUN: llc -mtriple=powerpc64-unknown-xbox360 < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-unknown-linux-gnu < %s | FileCheck %s --check-prefix=ELFV2

; The Xbox 360's platform compiler is MSVC, which 16-byte aligns the START of a
; by-value aggregate larger than a doubleword in the parameter save area: it
; pads before the aggregate (the aggregate's size is only rounded to a
; doubleword), which shifts every argument that follows up by one doubleword.
; The alignment padding does NOT consume a GPR -- the aggregate still begins in
; the next available GPR.  clang titles link against the precompiled MSVC XDK
; libraries, so the layout must match.
;
; Here a 36-byte aggregate ([5 x i64]) is followed by a pointer, a length and
; two i32s.  this=r3, aggregate=r4-r8, list=r9, len=r10, and the two i32s land
; on the stack.  On the Xbox 360 the aggregate is homed at a 16-aligned offset
; so the two trailing i32s sit a doubleword higher than the generic ELFv2 ABI
; places them.

target datalayout = "E-m:e-p:32:32-Fn32-i64:64-i128:128-n32:64"

declare zeroext i32 @rrc_callee(ptr, [5 x i64], ptr, i32 zeroext, i32 signext, i32 signext)

; Callee: read the two trailing stack arguments.  The aggregate's 16-byte
; alignment pushes them to +0x5C / +0x64 of the caller's frame (read here at
; frame-size 0x70 + those: 92 and 100).  Without the fix they were at 84 / 92.
define zeroext i32 @rrc(ptr %self, [5 x i64] %addr, ptr %list, i32 zeroext %len,
                        i32 signext %a, i32 signext %b) {
entry:
  %add = add nsw i32 %b, %a
  ret i32 %add
}
; CHECK-LABEL: rrc:
; CHECK-DAG:     lwz {{[0-9]+}}, 92(1)
; CHECK-DAG:     lwz {{[0-9]+}}, 100(1)

; Caller: the aggregate goes in the dense GPRs r4-r8 (no GPR skipped for the
; alignment padding) and the two i32s are stored one doubleword higher than the
; generic ABI (88 / 96 instead of 80 / 88).
define zeroext i32 @call_rrc(ptr %self, ptr %pa, ptr %list, i32 zeroext %len,
                             i32 signext %a, i32 signext %b) {
entry:
  %p0 = load i64, ptr %pa, align 4
  %g1 = getelementptr inbounds i8, ptr %pa, i32 8
  %p1 = load i64, ptr %g1, align 4
  %g2 = getelementptr inbounds i8, ptr %pa, i32 16
  %p2 = load i64, ptr %g2, align 4
  %g3 = getelementptr inbounds i8, ptr %pa, i32 24
  %p3 = load i64, ptr %g3, align 4
  %g4 = getelementptr inbounds i8, ptr %pa, i32 32
  %p4 = load i64, ptr %g4, align 4
  %a0 = insertvalue [5 x i64] poison, i64 %p0, 0
  %a1 = insertvalue [5 x i64] %a0, i64 %p1, 1
  %a2 = insertvalue [5 x i64] %a1, i64 %p2, 2
  %a3 = insertvalue [5 x i64] %a2, i64 %p3, 3
  %a4 = insertvalue [5 x i64] %a3, i64 %p4, 4
  %call = tail call zeroext i32 @rrc_callee(ptr %self, [5 x i64] %a4, ptr %list,
                                            i32 zeroext %len, i32 signext %a,
                                            i32 signext %b)
  ret i32 %call
}
; CHECK-LABEL: call_rrc:
; CHECK-DAG:     std {{[0-9]+}}, 88(1)
; CHECK-DAG:     std {{[0-9]+}}, 96(1)
; CHECK:         bl rrc_callee

; The generic 64-bit ELFv2 ABI does not insert this padding: the aggregate is
; packed, so the trailing i32s are NOT shifted up to the Xbox 360 offsets.
; (ELFv2 is LP64 so its absolute offsets differ; it suffices that the Xbox 360
; stack offsets introduced by the 16-byte aggregate alignment never appear.)
; ELFV2-LABEL: rrc:
; ELFV2-NOT:     100(1)
; ELFV2-LABEL: call_rrc:
; ELFV2-NOT:     96(1)
