// SiliconSprint 2026 - C wrappers for the custom packed-SIMD instructions.
//
// Encoding (problem statement, Section 5.1): R-type, custom-0 opcode 0x0B.
//
//   instr   funct7   funct3
//   VADD    0000000  000      mandatory
//   VSUB    0000000  001      mandatory
//   VAND    0000000  010      mandatory
//   VOR     0000000  011      mandatory
//   VXOR    0000000  100      mandatory
//   VSLL    0000000  101      stretch
//   VSRL    0000000  110      stretch
//   VMUL    0000000  111      stretch
//   VMAC    0000001  000      stretch (rd is also a source)
//
// Element width (SEW) comes from the vcfg CSR (Stage 2). At reset SEW = 32.
#ifndef VSIMD_H
#define VSIMD_H

#include <stdint.h>

#define VSIMD_DEFINE_RR(name, f3, f7)                                         \
  static inline uint64_t name(uint64_t a, uint64_t b) {                       \
    uint64_t r;                                                               \
    asm volatile(".insn r 0x0B, " #f3 ", " #f7 ", %0, %1, %2"                 \
                 : "=r"(r) : "r"(a), "r"(b));                                 \
    return r;                                                                 \
  }

VSIMD_DEFINE_RR(vadd, 0, 0)
VSIMD_DEFINE_RR(vsub, 1, 0)
VSIMD_DEFINE_RR(vand, 2, 0)
VSIMD_DEFINE_RR(vor,  3, 0)
VSIMD_DEFINE_RR(vxor, 4, 0)
VSIMD_DEFINE_RR(vsll, 5, 0)
VSIMD_DEFINE_RR(vsrl, 6, 0)
VSIMD_DEFINE_RR(vmul, 7, 0)

// VMAC: acc[i] = acc[i] + low SEW bits of (a[i] * b[i])
static inline uint64_t vmac(uint64_t acc, uint64_t a, uint64_t b) {
  asm volatile(".insn r 0x0B, 0, 1, %0, %1, %2" : "+r"(acc) : "r"(a), "r"(b));
  return acc;
}

// ---------------------------------------------------------------------------
// Vector CSRs (Stage 2, problem statement Section 5.3)
// ---------------------------------------------------------------------------
#define CSR_VCFG  0x8C0
#define CSR_VSTAT 0x8C1

#define VCFG_EN     0x1u
#define VCFG_SEW8   (0u << 1)
#define VCFG_SEW16  (1u << 1)
#define VCFG_SEW32  (2u << 1)
#define VSTAT_OV    0x1u

static inline uint64_t vcfg_read(void) {
  uint64_t v;
  asm volatile("csrr %0, 0x8C0" : "=r"(v));
  return v;
}
static inline void vcfg_write(uint64_t v) {
  asm volatile("csrw 0x8C0, %0" ::"r"(v) : "memory");
}
static inline uint64_t vstat_read(void) {
  uint64_t v;
  asm volatile("csrr %0, 0x8C1" : "=r"(v));
  return v;
}
static inline void vstat_write(uint64_t v) {
  asm volatile("csrw 0x8C1, %0" ::"r"(v) : "memory");
}
// Convenience: enable the unit with the given element width (8, 16 or 32).
static inline void vsimd_set_sew(unsigned sew) {
  vcfg_write(VCFG_EN | (sew == 8 ? VCFG_SEW8 : sew == 16 ? VCFG_SEW16 : VCFG_SEW32));
}

// ---------------------------------------------------------------------------
// Packing helpers
// ---------------------------------------------------------------------------
static inline uint64_t pack32(uint32_t hi, uint32_t lo) {
  return ((uint64_t)hi << 32) | lo;
}
static inline uint64_t pack16(uint16_t e3, uint16_t e2, uint16_t e1, uint16_t e0) {
  return ((uint64_t)e3 << 48) | ((uint64_t)e2 << 32) | ((uint64_t)e1 << 16) | e0;
}

#endif
