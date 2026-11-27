# Instruction encoding (frozen)

All instructions are R-type in the **custom-0** major opcode `0x0B` (`0001011`).

```
 31      25 24   20 19   15 14  12 11    7 6       0
+----------+-------+-------+------+-------+---------+
|  funct7  |  rs2  |  rs1  |funct3|  rd   | 0001011 |
+----------+-------+-------+------+-------+---------+
```

| Instruction | funct7 | funct3 | Operation (per lane i, at current SEW) | Tier |
| --- | --- | --- | --- | --- |
| VADD | 0000000 | 000 | rd[i] = rs1[i] + rs2[i] | Mandatory |
| VSUB | 0000000 | 001 | rd[i] = rs1[i] − rs2[i] | Mandatory |
| VAND | 0000000 | 010 | rd[i] = rs1[i] & rs2[i] | Mandatory |
| VOR | 0000000 | 011 | rd[i] = rs1[i] \| rs2[i] | Mandatory |
| VXOR | 0000000 | 100 | rd[i] = rs1[i] ^ rs2[i] | Mandatory |
| VSLL | 0000000 | 101 | rd[i] = rs1[i] << rs2[i][log2(SEW)−1:0] | Stretch |
| VSRL | 0000000 | 110 | rd[i] = rs1[i] >> rs2[i][log2(SEW)−1:0] (logical) | Stretch |
| VMUL | 0000000 | 111 | rd[i] = low SEW bits of rs1[i] × rs2[i] | Stretch |
| VMAC | 0000001 | 000 | rd[i] = rd[i] + low SEW bits of rs1[i] × rs2[i] | Stretch |

Every other funct7/funct3 combination in custom-0 is **reserved** and must raise
an illegal-instruction exception. A stretch instruction you do not implement is
also treated as reserved.

## Semantics

* Lanes: SEW = 32 → 2 lanes, SEW = 16 → 4 lanes, SEW = 8 → 8 lanes. Lane 0 is
  the least significant.
* Arithmetic is modular per lane. Carries and borrows never cross lanes.
* Logical operations are bitwise, so their result does not depend on SEW.

## Vector CSRs (Stage 2)

| CSR | Address | Field | Bits | Reset | Description |
| --- | --- | --- | --- | --- | --- |
| `vcfg` | 0x8C0 | EN | 0 | 1 | Unit enable. EN = 0 → every custom-0 instruction is illegal |
| `vcfg` | 0x8C0 | SEW | 2:1 | 10 | 00 = 8, 01 = 16, 10 = 32. Writing 11 leaves the field unchanged |
| `vstat` | 0x8C1 | OV | 0 | 0 | Sticky. Set when a **committed** VADD / VSUB has signed overflow in any lane. Cleared only by software |

All other bits read as 0 and ignore writes. Reset value of `vcfg` is `0x5`.

Architectural rules (problem statement 5.4):

1. A vector instruction after a `vcfg` write (program order, no fence) uses the new value.
2. Only committed instructions set `vstat.OV`; flushed or trapping instructions never do.
3. EN = 0 or a reserved encoding → `mcause` = 2, `mepc` = PC of the instruction,
   `mtval` = instruction bits, `rd` not written.
4. `csrr vstat` sees every update from older committed vector instructions.

## Writing the instructions

C (see `tests/common/vsimd.h`):

```c
uint64_t r = vadd(a, b);           // .insn r 0x0B, 0, 0, rd, rs1, rs2
vsimd_set_sew(16);                 // csrw 0x8C0, EN | SEW16
```

Assembly (see `tests/common/vsimd_asm.h`):

```asm
VADD(t0, a0, a1)                   # .insn r 0x0B, 0, 0, t0, a0, a1
csrw 0x8C0, t1                     # vcfg
```

Example: `.insn r 0x0B, 0, 0, a0, a1, a2` assembles to `0x00C5850B`.
