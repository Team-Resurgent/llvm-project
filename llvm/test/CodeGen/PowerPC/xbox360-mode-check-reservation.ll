; RUN: not llc -mtriple=powerpc64-unknown-xbox360 < %s -o /dev/null 2>&1 | FileCheck %s
;
; Xbox 360: a reservation the compiler cannot protect (see
; xbox360-reservation.ll) is an error, not code the kernel loader rejects.

; CHECK: error: {{.*}}Xbox 360: STWCX: a store-conditional not followed by mtmsrd rT, 1
define i32 @raw_stwcx(ptr %p, i32 %v) {
  %r = call i32 @llvm.ppc.stwcx(ptr %p, i32 %v)
  ret i32 %r
}

; CHECK: error: {{.*}}Xbox 360: INLINEASM: inline assembly uses lwarx/ldarx/stwcx./stdcx.
define i32 @raw_asm(ptr %p) {
  %r = call i32 asm sideeffect "lwarx $0, 0, $1", "=r,r"(ptr %p)
  ret i32 %r
}

declare i32 @llvm.ppc.stwcx(ptr, i32)
