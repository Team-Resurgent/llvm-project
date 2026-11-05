// The Xbox 360's platform compiler is MSVC, which never reuses a base class's
// tail padding for a derived class's members.  The Xbox 360 target keeps the
// Itanium C++ ABI but must match that layout so classes shared with the
// MS-compiled XDK/kernel libraries agree.  Contrast with a generic PowerPC
// Itanium target, which reuses the tail padding of a non-POD base.

// RUN: %clang_cc1 -triple powerpc64-unknown-xbox360 -fdump-record-layouts -fsyntax-only %s 2>/dev/null \
// RUN:            | FileCheck %s --check-prefix=XBOX --strict-whitespace
// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -fdump-record-layouts -fsyntax-only %s 2>/dev/null \
// RUN:            | FileCheck %s --check-prefix=ITANIUM --strict-whitespace

struct Base {
  int a;
  char b;      // data ends at offset 5; tail padding [5, 8)
  Base();      // user-declared ctor => non-POD for layout
};

struct Derived : Base {
  char c;      // Xbox 360 (MSVC): offset 8.  Generic Itanium: offset 5 (reused).
};

int sink = sizeof(Derived);

// XBOX-LABEL:      0 | struct Base
// XBOX:              | [sizeof=8, dsize=8, align=4,
// XBOX-NEXT:         |  nvsize=8, nvalign=4]
// XBOX-LABEL:      0 | struct Derived
// XBOX:            0 |   struct Base (base)
// XBOX:            8 |   char c
// XBOX:              | [sizeof=12, dsize=12, align=4,
// XBOX-NEXT:         |  nvsize=12, nvalign=4]

// ITANIUM-LABEL:   0 | struct Base
// ITANIUM:           | [sizeof=8, dsize=5, align=4,
// ITANIUM-NEXT:      |  nvsize=5, nvalign=4]
// ITANIUM-LABEL:   0 | struct Derived
// ITANIUM:         0 |   struct Base (base)
// ITANIUM:         5 |   char c
// ITANIUM:           | [sizeof=8, dsize=6, align=4,
// ITANIUM-NEXT:      |  nvsize=6, nvalign=4]
