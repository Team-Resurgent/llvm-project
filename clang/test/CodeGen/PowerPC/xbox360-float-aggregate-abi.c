// The Xbox 360 (Xenon) title ABI is ILP32-on-ppc64 and uses the ELFv2 ABI kind,
// but its MS compiler does NOT apply the ELFv2 homogeneous-floating-point-
// aggregate (HFA) rule: a by-value struct of only floats is passed/returned in
// general-purpose registers like any other small aggregate, not spread across
// the floating-point registers. clang-built titles link against precompiled MS
// XDK libraries, so clang must match that: a float-only aggregate goes through
// the normal small-aggregate path (<=8 bytes in one GPR, larger in GPR-sized
// pieces), producing exactly the same signature as a struct of ints -- never
// the [N x float] HFA coercion that a real ELFv2 target uses.

// RUN: %clang_cc1 -triple powerpc64-unknown-xbox360 -emit-llvm -o - %s | FileCheck %s --check-prefix=XBOX
// Contrast: a real ELFv2 target still passes these as homogeneous float
// aggregates in the floating-point registers.
// RUN: %clang_cc1 -triple powerpc64le-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s --check-prefix=ELFV2

struct P2 { float x, y; };       // 8 bytes  -> one GPR
struct P3 { float x, y, z; };    // 12 bytes -> two GPRs

// A two-float struct argument: one GPR, not two FPRs.
// XBOX-LABEL:  define{{.*}} void @take2(i64 %v.coerce)
// ELFV2-LABEL: define{{.*}} void @take2([2 x float] %v.coerce)
void take2(struct P2 v) {}

// A three-float struct argument: two GPRs, not three FPRs.
// XBOX-LABEL:  define{{.*}} void @take3([2 x i64] %v.coerce)
// ELFV2-LABEL: define{{.*}} void @take3([3 x float] %v.coerce)
void take3(struct P3 v) {}

// Returned the same way: in the GPRs, not the FPRs.
// XBOX-LABEL:  define{{.*}} i64 @ret2()
// ELFV2-LABEL: define{{.*}} [2 x float] @ret2()
struct P2 ret2(void) { struct P2 p = {1, 2}; return p; }

// XBOX-LABEL:  define{{.*}} { i64, i64 } @ret3()
// ELFV2-LABEL: define{{.*}} [3 x float] @ret3()
struct P3 ret3(void) { struct P3 p = {1, 2, 3}; return p; }

// The call site passes the argument in the GPR form too.
// XBOX-LABEL:  define{{.*}} void @call2()
// XBOX:        call void @take2(i64
// ELFV2-LABEL: define{{.*}} void @call2()
// ELFV2:       call void @take2([2 x float]
void call2(void) { struct P2 p = {1, 2}; take2(p); }
