// __BITFIELDS_LSB_FIRST__ is defined only for big-endian targets that allocate
// bit-fields least-significant-bit first (the MS Xbox 360 convention); libc++'s
// std::string relies on it to keep its long/short flag bits overlapping.

// RUN: %clang_cc1 -E -dM -triple powerpc64-unknown-xbox360 < /dev/null \
// RUN:   | FileCheck --check-prefix=LSB %s
// RUN: %clang_cc1 -E -dM -triple powerpc-unknown-xbox360 < /dev/null \
// RUN:   | FileCheck --check-prefix=LSB %s
// RUN: %clang_cc1 -E -dM -triple powerpc64-unknown-linux-gnu < /dev/null \
// RUN:   | FileCheck --check-prefix=NONE %s
// RUN: %clang_cc1 -E -dM -triple i686-pc-windows-msvc < /dev/null \
// RUN:   | FileCheck --check-prefix=NONE %s

// LSB: #define __BIG_ENDIAN__ 1
// LSB: #define __BITFIELDS_LSB_FIRST__ 1
// NONE-NOT: __BITFIELDS_LSB_FIRST__
