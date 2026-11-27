// SiliconSprint 2026 - assembler macros for hand-scheduled tests (.S files).
// Include with: #include "vsimd_asm.h" (files are run through the C preprocessor).
#ifndef VSIMD_ASM_H
#define VSIMD_ASM_H

#define VADD(rd, rs1, rs2) .insn r 0x0B, 0, 0, rd, rs1, rs2
#define VSUB(rd, rs1, rs2) .insn r 0x0B, 1, 0, rd, rs1, rs2
#define VAND(rd, rs1, rs2) .insn r 0x0B, 2, 0, rd, rs1, rs2
#define VOR(rd, rs1, rs2)  .insn r 0x0B, 3, 0, rd, rs1, rs2
#define VXOR(rd, rs1, rs2) .insn r 0x0B, 4, 0, rd, rs1, rs2
#define VSLL(rd, rs1, rs2) .insn r 0x0B, 5, 0, rd, rs1, rs2
#define VSRL(rd, rs1, rs2) .insn r 0x0B, 6, 0, rd, rs1, rs2
#define VMUL(rd, rs1, rs2) .insn r 0x0B, 7, 0, rd, rs1, rs2
#define VMAC(rd, rs1, rs2) .insn r 0x0B, 0, 1, rd, rs1, rs2

#define CSR_VCFG  0x8C0
#define CSR_VSTAT 0x8C1

// Self-checking: if reg != expected, exit with code `testnum`.
// Uses t6 as scratch. Every check must have a unique testnum (1..1023).
#define CHECK(testnum, reg, expected) \
  li t6, expected;                     \
  beq reg, t6, 9f;                     \
  li a0, testnum;                      \
  j exit;                              \
9:

// Standard test entry: `main` is called by crt.S; return 0 = pass.
#define TEST_BEGIN \
  .text;           \
  .globl main;     \
main:

#define TEST_PASS \
  li a0, 0;       \
  ret

#endif
