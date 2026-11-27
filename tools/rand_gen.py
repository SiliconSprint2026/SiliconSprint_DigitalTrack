#!/usr/bin/env python3
"""Random instruction-stream generator for L3 verification.

Generates self-contained assembly tests mixing scalar and custom vector
instructions. Correctness is decided by trace comparison against Spike, so
the programs do not check themselves; they only need to terminate.

  tools/rand_gen.py --seed 7 -o tests/random/rand_0007.S
  tools/rand_gen.py --seeds 1-50 --outdir tests/random              # Stage 1
  tools/rand_gen.py --seeds 1-50 --outdir tests/random --stage2 \
                    --ops vadd,vsub,vand,vor,vxor,vsll,vsrl,vmul,vmac

Features
  * register values biased towards corner cases (0, -1, INT_MIN/MAX per lane,
    lane-boundary carries)
  * back-to-back dependencies, loads/stores feeding vector ops
  * forward branches over vector ops (creates mispredicted shadows)
  * --stage2: random vcfg writes (SEW 8/16/32), vstat reads/clears, and
    vector ops with vcfg.EN = 0 (must trap as illegal instruction)

TODO(team): add constraints for your own stretch instructions or corner cases.
"""
import argparse
import random
from pathlib import Path

MANDATORY = ["vadd", "vsub", "vand", "vor", "vxor"]
ENCODING = {  # name -> (funct3, funct7)
    "vadd": (0, 0), "vsub": (1, 0), "vand": (2, 0), "vor": (3, 0), "vxor": (4, 0),
    "vsll": (5, 0), "vsrl": (6, 0), "vmul": (7, 0), "vmac": (0, 1),
}
# Registers the generator may use freely (x8/s0 = data pointer, x31/t6 =
# scratch, ra/sp/gp/tp are reserved).
POOL = [5, 6, 7, 9] + list(range(10, 18)) + list(range(18, 28)) + [28, 29, 30]
SCALAR_RR = ["add", "sub", "and", "or", "xor", "sll", "srl", "sra", "slt", "sltu",
             "addw", "subw", "mul", "mulw"]
SCALAR_RI = ["addi", "andi", "ori", "xori", "slli", "srli", "srai", "addiw"]
SPECIAL = [
    0x0, 0xFFFFFFFFFFFFFFFF, 0x7FFFFFFF7FFFFFFF, 0x8000000080000000,
    0x00000000FFFFFFFF, 0xFFFFFFFF00000000, 0x7FFFFFFF80000000, 0x0000000100000001,
    0x7F7F7F7F7F7F7F7F, 0x8080808080808080, 0x7FFF7FFF7FFF7FFF, 0x8000800080008000,
    0x00FF00FF00FF00FF, 0xFF00FF00FF00FF00, 0x0101010101010101, 0x1F1F1F1F1F1F1F1F,
]


class Gen:
    def __init__(self, seed, n, ops, stage2):
        self.r = random.Random(seed)
        self.seed, self.n, self.ops, self.stage2 = seed, n, ops, stage2
        self.lines = []
        self.label = 0

    def value(self):
        r = self.r.random()
        if r < 0.45:
            return self.r.choice(SPECIAL)
        if r < 0.6:  # small lanes
            return (self.r.randrange(256) << 32) | self.r.randrange(256)
        return self.r.getrandbits(64)

    def reg(self):
        return f"x{self.r.choice(POOL)}"

    def emit(self, s):
        self.lines.append("  " + s)

    def vec(self, name=None, rd=None, rs1=None, rs2=None):
        name = name or self.r.choice(self.ops)
        f3, f7 = ENCODING[name]
        rd, rs1, rs2 = rd or self.reg(), rs1 or self.reg(), rs2 or self.reg()
        self.emit(f".insn r 0x0B, {f3}, {f7}, {rd}, {rs1}, {rs2}   # {name}")
        return rd

    def scalar(self):
        if self.r.random() < 0.6:
            op = self.r.choice(SCALAR_RR)
            self.emit(f"{op} {self.reg()}, {self.reg()}, {self.reg()}")
        else:
            op = self.r.choice(SCALAR_RI)
            if op in ("slli", "srli", "srai"):
                imm = self.r.randrange(64)
            elif op == "addiw" or op == "addi":
                imm = self.r.randrange(-2048, 2048)
            else:
                imm = self.r.randrange(-2048, 2048)
            self.emit(f"{op} {self.reg()}, {self.reg()}, {imm}")

    def mem(self):
        off = 8 * self.r.randrange(32)
        if self.r.random() < 0.5:
            self.emit(f"ld {self.reg()}, {off}(s0)")
        else:
            self.emit(f"sd {self.reg()}, {off}(s0)")

    def dep_chain(self):
        # back-to-back chain through one register
        a = self.reg()
        self.vec(rd=a)
        for _ in range(self.r.randrange(1, 4)):
            if self.r.random() < 0.7:
                self.vec(rd=a, rs1=a)
            else:
                self.emit(f"add {a}, {a}, {self.reg()}")

    def load_use(self):
        a = self.reg()
        off = 8 * self.r.randrange(32)
        self.emit(f"ld {a}, {off}(s0)")
        self.vec(rs1=a)

    def branch_shadow(self):
        self.label += 1
        lbl = f"L{self.label}"
        cond = self.r.choice(["beq", "bne", "blt", "bge", "bltu", "bgeu"])
        self.emit(f"{cond} {self.reg()}, {self.reg()}, {lbl}")
        for _ in range(self.r.randrange(1, 4)):
            self.vec() if self.r.random() < 0.7 else self.scalar()
        self.lines.append(f"{lbl}:")

    def fresh_value(self):
        self.emit(f"li {self.reg()}, 0x{self.value():016x}")

    def vcfg_write(self):
        sew = self.r.choice([0, 1, 2])  # 8, 16, 32
        self.emit(f"li t6, {1 | (sew << 1)}")
        self.emit("csrw 0x8C0, t6       # vcfg: EN=1")

    def vstat_ops(self):
        if self.r.random() < 0.5:
            self.emit(f"csrr {self.reg()}, 0x8C1   # vstat")
        else:
            self.emit("csrw 0x8C1, zero        # clear vstat")

    def disabled_op(self):
        # vcfg.EN = 0 -> the vector op must raise an illegal-instruction trap;
        # the handler skips it and counts the trap in x31 (t6) via memory.
        self.emit("csrw 0x8C0, zero      # vcfg: EN=0")
        self.vec()
        self.vcfg_write()

    def body(self):
        choices = [
            (30, self.vec), (20, self.scalar), (8, self.mem), (10, self.dep_chain),
            (6, self.load_use), (8, self.branch_shadow), (8, self.fresh_value),
        ]
        if self.stage2:
            choices += [(5, self.vcfg_write), (4, self.vstat_ops), (2, self.disabled_op)]
        total = sum(w for w, _ in choices)
        count = 0
        while count < self.n:
            x = self.r.uniform(0, total)
            for w, fn in choices:
                x -= w
                if x <= 0:
                    fn()
                    break
            count += 1

    def render(self):
        out = [
            f"// Random test, seed={self.seed}, n={self.n}, ops={','.join(self.ops)}, "
            f"stage2={int(self.stage2)}",
            "// Generated by tools/rand_gen.py - checked by trace comparison vs Spike.",
            "  .text",
            "  .globl main",
            "main:",
            "  addi sp, sp, -16",
            "  sd ra, 8(sp)",
            "  la s0, rand_data",
        ]
        if self.stage2:
            out += ["  li t6, 5", "  csrw 0x8C0, t6       # vcfg: EN=1, SEW=32", "  csrw 0x8C1, zero"]
        for rn in POOL:
            out.append(f"  li x{rn}, 0x{self.value():016x}")
        self.body()
        out += self.lines
        if self.stage2:
            out += ["  csrr t6, 0x8C1", "  csrr t6, 0x8C0"]
        out += [
            "  ld ra, 8(sp)",
            "  addi sp, sp, 16",
            "  li a0, 0",
            "  ret",
            "",
            "# Expected traps (vcfg.EN = 0) skip the faulting instruction.",
            "  .globl handle_trap",
            "handle_trap:",
            "  li t0, 2",
            "  bne a0, t0, 1f",
            "  addi a0, a1, 4",
            "  ret",
            "1:",
            "  addi a0, a0, 0x100",
            "  j exit",
            "",
            "  .data",
            "  .align 3",
            "rand_data:",
        ]
        for _ in range(32):
            out.append(f"  .dword 0x{self.value():016x}")
        return "\n".join(out) + "\n"


def parse_seeds(s):
    seeds = []
    for part in s.split(","):
        if "-" in part:
            a, b = part.split("-")
            seeds += list(range(int(a), int(b) + 1))
        else:
            seeds.append(int(part))
    return seeds


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--seed", type=int)
    ap.add_argument("--seeds", type=str, help="e.g. 1-50 or 3,7,11")
    ap.add_argument("-o", "--output", type=Path)
    ap.add_argument("--outdir", type=Path, default=Path("tests/random"))
    ap.add_argument("-n", "--num-instr", type=int, default=300)
    ap.add_argument("--ops", default=",".join(MANDATORY))
    ap.add_argument("--stage2", action="store_true")
    ap.add_argument("--prefix", default="rand")
    args = ap.parse_args()

    ops = [o.strip() for o in args.ops.split(",") if o.strip()]
    for o in ops:
        if o not in ENCODING:
            ap.error(f"unknown op {o}")
    seeds = parse_seeds(args.seeds) if args.seeds else [args.seed if args.seed is not None else 1]
    for s in seeds:
        text = Gen(s, args.num_instr, ops, args.stage2).render()
        path = args.output if (args.output and len(seeds) == 1) else \
            args.outdir / f"{args.prefix}_{s:04d}.S"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    print(f"generated {len(seeds)} test(s)")


if __name__ == "__main__":
    main()
