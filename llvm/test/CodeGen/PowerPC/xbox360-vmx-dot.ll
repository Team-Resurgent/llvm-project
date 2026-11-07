; RUN: llc -mtriple=powerpc64-unknown-xbox360 -verify-machineinstrs < %s | FileCheck %s

; The Xbox 360 runtime's dot-product intrinsics __vmsum4fp/__vmsum3fp emit the
; Cell/Xenon VMX128 floating-point multiply-sum instructions via inline asm.
; These mnemonics used to be rejected by the assembler, so the runtime emulated
; them with a vector multiply plus pairwise shuffled adds, which rounds the lane
; products separately and sums them in a different order than the real
; instruction (and, for the 3-lane case, could not reproduce the -0 result).
; Verify they now assemble to the real vmsum4fp128 / vmsum3fp128.

define <4 x float> @vmsum4fp(<4 x float> %a, <4 x float> %b) {
; CHECK-LABEL: vmsum4fp:
; CHECK:       vmsum4fp128 {{[0-9]+}}, {{[0-9]+}}, {{[0-9]+}}
  %r = call <4 x float> asm "vmsum4fp128 $0, $1, $2", "=v,v,v"(<4 x float> %a, <4 x float> %b)
  ret <4 x float> %r
}

define <4 x float> @vmsum3fp(<4 x float> %a, <4 x float> %b) {
; CHECK-LABEL: vmsum3fp:
; CHECK:       vmsum3fp128 {{[0-9]+}}, {{[0-9]+}}, {{[0-9]+}}
  %r = call <4 x float> asm "vmsum3fp128 $0, $1, $2", "=v,v,v"(<4 x float> %a, <4 x float> %b)
  ret <4 x float> %r
}
