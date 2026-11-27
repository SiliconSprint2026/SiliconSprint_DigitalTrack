# Verification guide

Spike is the reference for every architectural result. The grader
(`tools/grade.py`) is exactly what the organizers run at both gates; at Gate 2
they add hidden tests and their own complete Spike model.

## Levels

| Level | Where | How it is checked | You add |
| --- | --- | --- | --- |
| L1 directed | `tests/directed/` | Self-checking: `main()` returns 0, every check prints `[PASS]`/`[FAIL] … expected=… actual=…` | SEW 8/16 tests, stretch instructions |
| L2 hazards | `tests/hazard/` | Self-checking **and** trace comparison with Spike | At least the six scenarios of task F.2 |
| L3 random | `tools/rand_gen.py` → `tests/random/` | Trace comparison with Spike | Constraints for your stretch instructions / corner cases |
| L4 regression | `/opt/riscv-tests/isa` | riscv-tests pass/fail | – |
| Kernels | `tests/kernels/` | Vector result must equal the scalar result | Vector versions in `tests/kernels/vec/` |

## How trace comparison works

`tools/trace_compare.py <elf>` runs the ELF on the RTL and on Spike and compares
the **committed instruction stream**: PC, instruction bits, and the integer
register written, in order.

* RTL trace: `trace_rvfi_hart_00.dasm` (written by the testharness RVFI tracer).
* Spike trace: `spike --log-commits`.
* Ignored on purpose:
  * code in `.text.nocompare` (HTIF console polling and the exit loop, whose
    iteration counts differ between platforms),
  * values read from `mcycle`, `minstret`, `mhpmcounter*`, `cycle`, `instret`
    (timing-dependent; the instruction itself is still compared),
  * CSR-write and memory annotations (their effects show up in later register writes).
* Result: `PASS`, `FAIL` (self-check failed but traces match), `MISMATCH`
  (first differing instruction is printed), or `HANG` (no exit before the timeout).

```bash
scripts/run_test.sh cmp tests/build/hazard/raw_chain.elf
# PASS: raw_chain.elf: 412 committed instructions match
```

## Where speculation actually happens in CVA6

CVA6 issues one instruction per cycle and resolves branches one cycle after
issue, so wrong-path instructions after a mispredicted branch are normally
flushed before they execute. The bigger speculation window is **behind a
trapping instruction**: exceptions (`ecall`, `ebreak`, illegal instructions,
faulting loads) are taken at commit, and younger instructions keep issuing and
executing until then. A good hazard test therefore places vector instructions
right after a trapping instruction and has the handler skip them: they must
leave no trace (no `rd` write, no `vstat` change).

Do not rely on loads from "unmapped" low addresses to fault: CVA6 and Spike
have different memory maps below `0x8000_0000`. Use `ecall`, `ebreak` or an
illegal encoding instead.

## Writing tests

* C tests: include `vsimd.h` and `test.h`, call `check64(name, actual, expected)`,
  finish with `return test_summary("name");`.
* Assembly tests: `#include "vsimd_asm.h"`, use `TEST_BEGIN`, `CHECK(n, reg, value)`
  and `TEST_PASS`. `CHECK` uses `t6` as scratch, so do not keep results in `t6`.
* Expected traps: define your own `handle_trap(mcause, mepc, mtval, regs)` and
  return the PC to resume at (usually `mepc + 4`). Registers are restored from
  `regs[]` when the handler returns, so write to `regs[i]` to change `x<i>`.
* Do not read timers or counters to make pass/fail decisions.

## Random tests

```bash
python3 tools/rand_gen.py --seeds 1-50 --outdir tests/random            # Stage 1 designs
python3 tools/rand_gen.py --seeds 1-50 --outdir tests/random --stage2 \
        --ops vadd,vsub,vand,vor,vxor,vsll,vsrl                         # Stage 2
make -C tests
for e in tests/build/random/*.elf; do python3 tools/trace_compare.py $e; done
```

The generator mixes vector ops with scalar ALU ops, loads and stores, back-to-back
dependencies, forward branches with vector ops in their shadow, and in Stage 2
mode random `vcfg` writes, `vstat` reads and clears, and vector ops with EN = 0
(which must trap).

## Running the grader yourself

```bash
python3 tools/grade.py --gate 1 --no-build
python3 tools/grade.py --gate 2 --no-build --seeds 50 --random-ops vadd,vsub,vand,vor,vxor
```

Results: `grade_out/report.md` and `grade_out/report.json`, per-test logs in
`grade_out/runs/`.

## Coverage table (deliverable F.4)

Report, for each implemented instruction × SEW (8, 16, 32), which data classes
your tests exercise: zero, all ones, maximum signed, minimum signed, signed
overflow, carry/borrow at a lane boundary.
