// Public directed test - VAND / VOR / VXOR at SEW = 32 (Gate check G1.2).
// Built three times (vand, vor, vxor) via -DOP_<name>; see tests/Makefile.
#include "vsimd.h"
#include "test.h"

#define A0 0x000000FF000000F0ULL
#define B0 0x0000000F0000003CULL
#define A1 0xF0F0F0F0AAAAAAAAULL
#define B1 0xFF00FF0055555555ULL

int main(void) {
#if defined(OP_vand)
  check64("vand.basic", vand(A0, B0), 0x0000000F00000030ULL);
  check64("vand.pattern", vand(A1, B1), 0xF000F00000000000ULL);
  check64("vand.zero", vand(~0ULL, 0), 0);
  check64("vand.ones", vand(~0ULL, 0x8000000180000001ULL), 0x8000000180000001ULL);
  return test_summary("vand");
#elif defined(OP_vor)
  check64("vor.basic", vor(A0, B0), 0x000000FF000000FCULL);
  check64("vor.pattern", vor(A1, B1), 0xFFF0FFF0FFFFFFFFULL);
  check64("vor.zero", vor(0, 0), 0);
  check64("vor.identity", vor(0x8000000180000001ULL, 0), 0x8000000180000001ULL);
  return test_summary("vor");
#elif defined(OP_vxor)
  check64("vxor.basic", vxor(A0, B0), 0x000000F0000000CCULL);
  check64("vxor.pattern", vxor(A1, B1), 0x0FF00FF0FFFFFFFFULL);
  check64("vxor.self", vxor(A1, A1), 0);
  check64("vxor.invert", vxor(0x0123456789ABCDEFULL, ~0ULL), 0xFEDCBA9876543210ULL);
  return test_summary("vxor");
#else
#error "define OP_vand, OP_vor or OP_vxor"
#endif
}
