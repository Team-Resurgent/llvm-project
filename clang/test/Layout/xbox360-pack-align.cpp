// The Xbox 360's platform compiler is MSVC, where #pragma pack does NOT reduce
// a member's explicit __declspec(align(N)): the effective member alignment is
// max(pack, declspec-align).  clang's generic record layout uses min (pack caps
// the explicit alignment), which under a pshpackN.h (N < 16) would shrink an
// XMVECTOR (align 16) member -- making e.g. NUI_SKELETON_DATA 0x1B8 instead of
// the MS-compiled XDK's 0x1C0.  The Xbox 360 target links against those MS-built
// libraries, so it must match MSVC.  Generic PowerPC Itanium is unchanged.

// RUN: %clang_cc1 -triple powerpc64-unknown-xbox360 -fms-extensions -fdump-record-layouts -fsyntax-only %s 2>/dev/null \
// RUN:            | FileCheck %s --check-prefix=XBOX --strict-whitespace
// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -fms-extensions -fdump-record-layouts -fsyntax-only %s 2>/dev/null \
// RUN:            | FileCheck %s --check-prefix=ITANIUM --strict-whitespace

typedef float __vf4 __attribute__((__vector_size__(16)));
typedef union __declspec(align(16)) { __vf4 vf; float v[4]; } XMVECTOR;

#pragma pack(push, 8)
struct S {
  unsigned head;    // offset 0
  XMVECTOR vec;     // Xbox 360: offset 16 (align kept).  Itanium: offset 8.
};
#pragma pack(pop)

int sink = sizeof(struct S);

// Xbox 360: #pragma pack(8) does not cap the align(16) member.
// XBOX-LABEL:      0 | struct S
// XBOX:            0 |   unsigned int head
// XBOX:           16 |   XMVECTOR vec
// XBOX:              | [sizeof=32, dsize=32, align=16,

// Generic Itanium: #pragma pack(8) caps the member to 8.
// ITANIUM-LABEL:   0 | struct S
// ITANIUM:         0 |   unsigned int head
// ITANIUM:         8 |   XMVECTOR vec
// ITANIUM:           | [sizeof=24, dsize=24, align=8,
