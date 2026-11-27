// Public directed test - VSUB at SEW = 32 (Stage 1, Gate check G1.2).
#include "vsimd.h"
#include "test.h"

int main(void) {
  check64("vsub.basic",
          vsub(0x0000003200000028ULL, 0x0000000A00000005ULL), 0x0000002800000023ULL);
  check64("vsub.zero", vsub(0, 0), 0);
  check64("vsub.self", vsub(0x123456789ABCDEF0ULL, 0x123456789ABCDEF0ULL), 0);
  check64("vsub.lane0_borrow_isolated",
          vsub(0x0000000100000000ULL, 0x0000000000000001ULL), 0x00000001FFFFFFFFULL);
  check64("vsub.lane1_borrow_wraps",
          vsub(0x0000000000000005ULL, 0x0000000100000000ULL), 0xFFFFFFFF00000005ULL);
  check64("vsub.zero_minus_one", vsub(0, 0x0000000100000001ULL), 0xFFFFFFFFFFFFFFFFULL);
  check64("vsub.min_signed_minus_one",
          vsub(0x8000000080000000ULL, 0x0000000100000001ULL), 0x7FFFFFFF7FFFFFFFULL);
  check64("vsub.signed [3,-4]-[5,-6]",
          vsub(0x00000003FFFFFFFCULL, 0x00000005FFFFFFFAULL), 0xFFFFFFFE00000002ULL);
  return test_summary("vsub");
}
