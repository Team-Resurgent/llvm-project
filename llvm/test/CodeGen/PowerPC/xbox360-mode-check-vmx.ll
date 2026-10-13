; RUN: not llc -mtriple=powerpc64-unknown-xbox360 < %s -o /dev/null 2>&1 | FileCheck %s

; Xenon's VMX does not implement the AltiVec integer multiplies, so using them
; through their intrinsics is a compile-time error on the Xbox 360 rather than
; an illegal instruction on the console.

; CHECK: error: {{.*}}in function vmulouh{{.*}}Xbox 360: VMULOUH: Xenon does not implement this VMX instruction
; CHECK: error: {{.*}}in function vsumsws{{.*}}Xbox 360: VSUMSWS: Xenon does not implement this VMX instruction
; CHECK-NOT: error: {{.*}}in function vaddubm

define <4 x i32> @vmulouh(<8 x i16> %a, <8 x i16> %b) {
  %r = call <4 x i32> @llvm.ppc.altivec.vmulouh(<8 x i16> %a, <8 x i16> %b)
  ret <4 x i32> %r
}

define <4 x i32> @vsumsws(<4 x i32> %a, <4 x i32> %b) {
  %r = call <4 x i32> @llvm.ppc.altivec.vsumsws(<4 x i32> %a, <4 x i32> %b)
  ret <4 x i32> %r
}

define <16 x i8> @vaddubm(<16 x i8> %a, <16 x i8> %b) {
  %r = add <16 x i8> %a, %b
  ret <16 x i8> %r
}

declare <4 x i32> @llvm.ppc.altivec.vmulouh(<8 x i16>, <8 x i16>)
declare <4 x i32> @llvm.ppc.altivec.vsumsws(<4 x i32>, <4 x i32>)
