// Public directed test - VADD at SEW = 32 (Stage 1, Gate checks G1.1 / G1.2).
// RD[31:0] = RS1[31:0] + RS2[31:0], RD[63:32] = RS1[63:32] + RS2[63:32]
#include "vsimd.h"
#include "test.h"

int main(void) {
  check64("vadd.basic [10,20]+[30,40]",
          vadd(0x000000140000000AULL, 0x000000280000001EULL), 0x0000003C00000028ULL);
  check64("vadd.zero", vadd(0, 0), 0);
  check64("vadd.identity", vadd(0x123456789ABCDEF0ULL, 0), 0x123456789ABCDEF0ULL);
  check64("vadd.lane0_carry_isolated",
          vadd(0x00000000FFFFFFFFULL, 0x0000000000000001ULL), 0x0000000000000000ULL);
  check64("vadd.lane1_wraps",
          vadd(0xFFFFFFFF00000000ULL, 0x0000000100000000ULL), 0x0000000000000000ULL);
  check64("vadd.signed [-1,5]+[2,-2]",
          vadd(0xFFFFFFFF00000005ULL, 0x00000002FFFFFFFEULL), 0x0000000100000003ULL);
  check64("vadd.max_signed_overflow",
          vadd(0x7FFFFFFF7FFFFFFFULL, 0x0000000100000001ULL), 0x8000000080000000ULL);
  check64("vadd.all_ones", vadd(~0ULL, ~0ULL), 0xFFFFFFFEFFFFFFFEULL);
  check64("vadd.min_signed", vadd(0x8000000080000000ULL, 0x8000000080000000ULL), 0);
  uint64_t x = 0x0000000700000003ULL;
  check64("vadd.same_source_reg", vadd(x, x), 0x0000000E00000006ULL);
  return test_summary("vadd");
}
