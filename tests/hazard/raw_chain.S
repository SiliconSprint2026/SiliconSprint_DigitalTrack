// Public hazard test - back-to-back read-after-write chains (Gate check G1.3).
// Every instruction consumes the result of the one immediately before it, so
// operands must come through scoreboard forwarding, not the register file.
#include "vsimd_asm.h"

TEST_BEGIN
  li a0, 0x0000000100000002
  li a1, 0x0000001000000020

  // vector -> vector -> vector
  VADD(t0, a0, a1)              // [0x11, 0x22]
  VADD(t1, t0, a1)              // [0x21, 0x42]
  VSUB(t2, t1, a0)              // [0x20, 0x40]
  CHECK(1, t2, 0x0000002000000040)

  // vector -> scalar -> vector
  VADD(t0, a0, a0)              // [2, 4]
  add  t1, t0, t0               // 64-bit add: [4, 8]
  VXOR(t2, t1, a0)              // [5, 10]
  CHECK(2, t2, 0x000000050000000A)

  // scalar -> vector with lane carry (scalar result crosses a lane boundary)
  li   t0, 0x00000000FFFFFFFF
  addi t1, t0, 0                // forwarded scalar result
  VADD(t2, t1, a0)              // lane0: 0xFFFFFFFF+2 wraps to 1; lane1: 0+1
  CHECK(3, t2, 0x0000000100000001)

  // load -> vector (load-use) and vector -> store -> load
  la   s0, scratch
  ld   t0, 0(s0)                // 0x0000000500000006
  VADD(t1, t0, t0)              // [10, 12]
  sd   t1, 8(s0)
  ld   t2, 8(s0)
  VAND(t3, t2, t1)
  CHECK(4, t3, 0x0000000A0000000C)

  // same register as both sources and destination, chained
  li   t0, 0x0000000100000001
  VADD(t0, t0, t0)              // [2, 2]
  VADD(t0, t0, t0)              // [4, 4]
  VADD(t0, t0, t0)              // [8, 8]
  CHECK(5, t0, 0x0000000800000008)

  // write-after-write: the last writer must win
  VADD(t4, a0, a0)
  VOR(t4, a1, a1)
  CHECK(6, t4, 0x0000001000000020)

  // rd = x0 must stay zero
  VADD(x0, a0, a1)
  CHECK(7, x0, 0)

  TEST_PASS

  .data
  .align 3
scratch:
  .dword 0x0000000500000006
  .dword 0
