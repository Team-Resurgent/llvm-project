; RUN: llc -mtriple=powerpc64-unknown-xbox360 -verify-machineinstrs < %s | FileCheck %s

; The Xbox 360 runtime's partial-vector intrinsics __lvlx/__lvrx/__stvlx/__stvrx
; emit the Cell/Xenon VMX "load/store vector left/right indexed" instructions via
; inline asm. These mnemonics used to be rejected by the assembler (the runtime
; worked around it by hand-encoding them with .long). Verify they now assemble to
; the real instructions rather than an lvx+vperm read-modify-write of the whole
; aligned block, which rewrote bytes the store did not own.

define <4 x float> @lvlx_lvrx(ptr %p) {
; CHECK-LABEL: lvlx_lvrx:
; CHECK:       lvlx {{[0-9]+}}, 0, {{[0-9]+}}
; CHECK:       lvrx {{[0-9]+}}, 0, {{[0-9]+}}
  %l = call <4 x float> asm "lvlx $0, 0, $1", "=v,r"(ptr %p)
  %r = call <4 x float> asm "lvrx $0, 0, $1", "=v,r"(ptr %p)
  %s = fadd <4 x float> %l, %r
  ret <4 x float> %s
}

define void @stvlx_stvrx(<4 x float> %v, ptr %p) {
; CHECK-LABEL: stvlx_stvrx:
; CHECK:       stvlx {{[0-9]+}}, 0, {{[0-9]+}}
; CHECK:       stvrx {{[0-9]+}}, 0, {{[0-9]+}}
  call void asm sideeffect "stvlx $0, 0, $1", "v,r"(<4 x float> %v, ptr %p)
  call void asm sideeffect "stvrx $0, 0, $1", "v,r"(<4 x float> %v, ptr %p)
  ret void
}
