// Public hazard test - vector instructions in the shadow of a mispredicted
// branch must never become architecturally visible.
//
// The loop body alternates between taken and not-taken outcomes, so the branch
// predictor is wrong on a regular basis. On the wrong path a VADD overwrites s1;
// if the flush is not handled, s1 changes.
#include "vsimd_asm.h"

TEST_BEGIN
  li s1, 0x5A5A5A5A5A5A5A5A     // must never change
  li a0, 0x0000000100000001
  li s2, 64                     // iterations
  li s3, 0                      // iteration counter
  li s4, 0                      // committed-path accumulator

loop:
  andi t0, s3, 1
  beqz t0, even                 // alternates taken / not taken
  // ---- odd path (fall-through) ----
  VADD(s4, s4, a0)              // committed on odd iterations
  j    next
even:
  bnez t0, wrong                // never taken; predicted taken after training?
  j    next
wrong:
  VADD(s1, s1, a0)              // must never commit
  VXOR(s1, s1, s1)
next:
  addi s3, s3, 1
  bne  s3, s2, loop

  CHECK(1, s1, 0x5A5A5A5A5A5A5A5A)
  CHECK(2, s4, 0x0000002000000020)   // 32 odd iterations

  // A taken branch whose fall-through path holds a dependent vector chain.
  li   t1, 1
  li   s5, 0x1111111111111111
  bnez t1, 1f
  VADD(s5, s5, s5)
  VADD(s5, s5, s5)
1:
  CHECK(3, s5, 0x1111111111111111)

  TEST_PASS
