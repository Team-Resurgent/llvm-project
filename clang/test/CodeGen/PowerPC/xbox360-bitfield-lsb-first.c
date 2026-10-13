// The Xbox 360 allocates bit-fields least-significant-bit first within their
// storage unit even though it is big-endian (matching the platform's MS
// compiler and GPU register layouts), and loads/stores each unit as one
// big-endian integer. Constant initializers must place the bits exactly where
// those loads and stores look for them: for a 32-bit unit, unit bit 0 is in the
// unit's last byte.

// RUN: %clang_cc1 -triple powerpc64-unknown-xbox360 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc-unknown-xbox360 -emit-llvm -o - %s | FileCheck %s

struct flag_word {
  unsigned flag : 1;
  unsigned rest : 31;
};

// The shape of GPUTEXTURE_FETCH_CONSTANT's first word; the platform compiler
// lays {3, 0, 0, 0, 0, 0xABCDE} out as the word 0xABCDE003.
struct fetch_word {
  unsigned DataFormat : 6;
  unsigned Endian : 2;
  unsigned RequestSize : 2;
  unsigned Stacked : 1;
  unsigned ClampPolicy : 1;
  unsigned BaseAddress : 20;
};

struct byte_unit {
  unsigned char a : 3;
  unsigned char b : 5;
};

struct wide {
  unsigned long long lo : 40;
  unsigned long long hi : 24;
};

struct after_member {
  unsigned short tag;
  unsigned short x : 9;
  unsigned short y : 7;
};

// CHECK: @fw = global {{.*}} { i8 0, i8 0, i8 0, i8 1 }
struct flag_word fw = {1, 0};
// CHECK: @fw2 = global {{.*}} { i8 0, i8 2, i8 70, i8 -117 }
struct flag_word fw2 = {1, 0x12345};
// CHECK: @fetch = global {{.*}} { i8 -85, i8 -51, i8 -32, i8 3 }
struct fetch_word fetch = {3, 0, 0, 0, 0, 0xABCDE};
// CHECK: @bu = global {{.*}} { i8 -115 }
struct byte_unit bu = {5, 17};
// One 64-bit unit (0xFEDCBA123456789A), not 40- and 24-bit pieces.
// CHECK: @w = global {{.*}} { i8 -2, i8 -36, i8 -70, i8 18, i8 52, i8 86, i8 120, i8 -102 }
struct wide w = {0x123456789AULL, 0xFEDCBAULL};
// CHECK: @am = global {{.*}} { i16 -16657, i8 -57, i8 44 }
struct after_member am = {0xBEEF, 300, 99};

// A partly filled unit: its unused (high) bits are at the front in memory, so
// padding is laid down a whole unit at a time and the field merged into it.
struct partial { int d : 17; };
// CHECK: @pa = global {{.*}} { [1 x i8] zeroinitializer, i8 1, i8 -1, i8 -2 }
struct partial pa = {-2};
struct padded { char c; unsigned d : 17; char e; };
// CHECK: @pd = global {{.*}} { i8 7, [3 x i8] zeroinitializer, [1 x i8] zeroinitializer, i8 0, i8 0, i8 2, i8 9, [3 x i8] zeroinitializer }
struct padded pd = {7, 2, 9};
// picolibc's caseconv_entry: a full and a partly filled unit.
struct caseconv { unsigned first : 21; unsigned diff : 8; unsigned mode : 2; int delta : 17; };
// CHECK: @cc = global {{.*}} { i8 35, i8 32, i8 0, i8 65, [1 x i8] zeroinitializer, i8 0, i8 0, i8 32 }
struct caseconv cc = {0x41, 25, 1, 32};

// Records follow MSVC's bit-field rules (ms_struct), as the platform compiler
// does: a new declared type starts a new storage unit, and :0 only takes effect
// after a bit-field.
struct mixed_types { char a : 3; int b : 5; };
struct zero_width { char a; int : 0; char b; };
// CHECK: @sizes = global [3 x i32] [i32 8, i32 8, i32 2]
int sizes[] = {sizeof(struct mixed_types), sizeof(struct wide), sizeof(struct zero_width)};

// Loads read the same bits back: the flag is bit 0 of the 32-bit unit.
// CHECK-LABEL: define {{.*}} @get_flag(
// CHECK: [[W:%.*]] = load i32, ptr
// CHECK: and i32 [[W]], 1
unsigned get_flag(struct flag_word *p) { return p->flag; }

// CHECK-LABEL: define {{.*}} @get_base(
// CHECK: [[W:%.*]] = load i32, ptr
// CHECK: lshr i32 [[W]], 12
unsigned get_base(struct fetch_word *p) { return p->BaseAddress; }
