// The Xbox 360's platform compiler is MSVC, which declares the XDK's XMVECTOR
// with __declspec(intrin_type): it is register-resident, passed and returned in
// a vector register like a bare __vector128.  RXDK-360 defines __vector4 as a
// 16-byte union wrapping a native 128-bit vector (so element access such as
// v.vf / v.v[i] works), which clang's generic PPC64 ELFv2 ABI would otherwise
// pass in a GPR pair and return via sret, corrupting every by-value XMVECTOR
// exchanged with the MS-compiled XDK.  On the Xbox 360 target such a union is
// passed and returned in a vector register; generic PowerPC ELFv2 is unchanged.

// RUN: %clang_cc1 -triple powerpc64-unknown-xbox360 -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=XBOX
// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=ELFV2

typedef float __vf4 __attribute__((__vector_size__(16)));
typedef union __attribute__((__aligned__(16))) {
  __vf4 vf;
  float v[4];
  unsigned u[4];
} __vector4;

// A __vector4 returned by value, two __vector4 arguments, and an intervening
// integer argument (which must not consume a vector register slot).
__vector4 pick(__vector4 a, int sel, __vector4 b) {
  (void)a;
  (void)sel;
  return b;
}

// On the Xbox 360 the union is coerced to a 128-bit vector for both the return
// value and the vector arguments (the i32 stays a GPR argument).
// XBOX: define{{.*}} <4 x float> @pick(<4 x float> {{[^,]*}}, i32 {{[^,]*}}, <4 x float> {{[^,]*}})

// On generic PowerPC ELFv2 the union is returned via sret and passed as an
// integer aggregate ([1 x i128]) in GPRs -- never a vector register.
// ELFV2: define{{.*}} void @pick(ptr {{.*}}sret{{.*}}, [1 x i128] {{[^,]*}}, i32 {{[^,]*}}, [1 x i128] {{[^,]*}})
