# SiliconSprint 2026 – Digital Design Track: Problem Statement

## 1. Summary

Teams extend CVA6, an open-source 64-bit RISC-V application core, with custom packed-SIMD instructions: one instruction processes several data elements in parallel in the existing 64-bit integer registers. The work runs in two stages. Each stage ends in a screening gate where the organizers run the same grader the teams have.

**Stage 1, core integration:** the five mandatory instructions work end to end through a dedicated vector execution unit (VALU), without breaking any scalar behaviour.

**Stage 2, architectural state, verification and exploration:** configurable element widths, two vector CSRs that stay correct under speculation, proof of correctness against the Spike golden model, and optional stretch goals and a cache bonus track.

| Item | Detail |
|---|---|
| Format | 24 hours, online, 27 Nov 5:00 PM to 28 Nov 5:00 PM IST |
| Platform | CVA6 v5.3.0 (OpenHW Group), RV64, frozen configuration `cv64a6_imafdc_sv39` |
| Language | SystemVerilog for RTL; C and RISC-V assembly for tests; C++ for the Spike model |
| Tooling | One Docker image: Verilator 5.008, Spike, RISC-V GCC 13, riscv-tests, Python grader |
| Reference model | Spike ISA simulator with a custom extension (provided, teams extend it) |
| Scoring | 100 marks, plus up to 15 bonus marks |
| Hardware needed | None. Everything runs in simulation |
| Starter kit | `SiliconSprint_DigitalTrack` template repository |

---

## 2. Objectives and Design Principles

This track tests whether a team understands a real processor well enough to extend it and prove the extension correct. RTL that compiles is not enough.

### What we assess

1. Whether a team can navigate a large open-source core and find where instructions are decoded, issued, executed, written back and committed.
2. Whether a team can integrate a new functional unit into an in-order-issue, out-of-order-completion, in-order-commit pipeline.
3. Whether a team can keep architectural state precise under speculation and exceptions.
4. Whether a team can verify its own design: write its own tests, extend a golden model, and use the provided verification infrastructure.
5. Whether the team understands its own design well enough to defend it live.

### Design principles

- **A functional prototype, not full RVV.** Complete RISC-V Vector (RVV 1.0) compliance is not expected. Correctness and clean integration come before performance.
- **Existing registers, no new register file.** The instructions use CVA6's 64-bit integer registers.
- **A dedicated unit.** The instructions are implemented in a separate VALU functional unit with its own functional-unit type, not inside CVA6's existing integer ALU. The CV-X-IF coprocessor interface is disabled and must not be used.
- **Scalar behaviour must be preserved.** Every existing instruction must keep working.
- **The team's own work.** Open-source material and AI tools are allowed with transparent disclosure.

---

## 3. Technical Scope

### 3.1 The baseline

Every team starts from the same frozen kit: CVA6 with a working Verilator build, the Spike reference model with a vector extension skeleton, public tests, a random test generator, a trace-comparison tool and the grader used at both gates.

### 3.2 Lane semantics

Each 64-bit source register holds independent elements. An instruction operates on all of them at once and returns a 64-bit result of independent elements. Results are truncated to SEW bits per lane. Carry and borrow never cross lanes.

**Example, SEW = 32:** `VADD` with RS1 = [10, 20] and RS2 = [30, 40] gives RD = [40, 60].

```
RD[31:0]  = RS1[31:0]  + RS2[31:0]
RD[63:32] = RS1[63:32] + RS2[63:32]
```

### 3.3 Instructions

At reset `vcfg` selects SEW = 32, so the Stage 1 instructions are exactly VADD2.W, VSUB2.W and so on.

| Item | Specification |
|---|---|
| Core | CVA6 v5.3.0, RV64, 6-stage pipeline: in-order issue, out-of-order completion, in-order commit |
| Data width | 64 bits |
| Element width (SEW) | 32 bits in Stage 1. 8, 16 or 32 bits in Stage 2, selected by the `vcfg` CSR |
| Elements per operation | 2 (SEW 32), 4 (SEW 16), 8 (SEW 8) |
| Operand storage | Existing 64-bit integer registers |
| Instruction space | custom-0 major opcode (`0x0B`), R-type |
| Execution model | Single-cycle for the mandatory set; multi-cycle pipelined units allowed for stretch goals |

```
 31      25 24   20 19   15 14  12 11    7 6       0
+----------+-------+-------+------+-------+---------+
|  funct7  |  rs2  |  rs1  |funct3|  rd   | 0001011 |
+----------+-------+-------+------+-------+---------+
```

| Instruction | funct7 | funct3 | Operation per lane | Tier |
|---|---|---|---|---|
| VADD | `0000000` | `000` | rd = rs1 + rs2 | Mandatory |
| VSUB | `0000000` | `001` | rd = rs1 − rs2 | Mandatory |
| VAND | `0000000` | `010` | rd = rs1 AND rs2 | Mandatory |
| VOR  | `0000000` | `011` | rd = rs1 OR rs2 | Mandatory |
| VXOR | `0000000` | `100` | rd = rs1 XOR rs2 | Mandatory |
| VSLL | `0000000` | `101` | logical left shift by the low log₂(SEW) bits of the rs2 lane | Stretch |
| VSRL | `0000000` | `110` | logical right shift by the low log₂(SEW) bits of the rs2 lane | Stretch |
| VMUL | `0000000` | `111` | low SEW bits of rs1 × rs2 | Stretch |
| VMAC | `0000001` | `000` | rd = rd + low SEW bits of rs1 × rs2 | Stretch |

Every other funct7/funct3 combination in custom-0, and every stretch instruction a team does not implement, is **reserved** and must raise an illegal-instruction exception. Use `.insn r 0x0B, <funct3>, <funct7>, rd, rs1, rs2` in assembly; the kit's `vsimd.h` and `vsimd_asm.h` wrap all of them.

### 3.4 Vector CSRs (Stage 2)

| CSR | Address | Field | Bits | Reset | Behaviour |
|---|---|---|---|---|---|
| `vcfg` | `0x8C0` | EN | 0 | 1 | When 0, every custom-0 instruction raises an illegal-instruction exception |
| `vcfg` | `0x8C0` | SEW | 2:1 | 10 | 00 = 8, 01 = 16, 10 = 32. Writing `11` leaves the field unchanged |
| `vstat` | `0x8C1` | OV | 0 | 0 | Sticky. Set when a **committed** VADD or VSUB has signed overflow in any lane. Cleared only by software |

All other bits read as zero and ignore writes. Reset value of `vcfg` is `0x5` (EN=1, SEW=32).

### 3.5 Architectural rules (Stage 2)

1. **Configuration ordering:** a vector instruction after a `vcfg` write in program order uses the new value. Software inserts no fence.
2. **Precise status:** only **committed** instructions set `vstat.OV`. Instructions that are flushed or that trap never change it.
3. **Precise exceptions:** EN = 0 or a reserved encoding traps with `mcause` = 2, `mepc` = the instruction's PC and `mtval` = the instruction bits. `rd` is not written.
4. **Visibility:** a `csrr` of `vstat` sees every update from older committed vector instructions.

---

## 4. Stage 1: Core Integration (40 marks)

Implement the five mandatory instructions at SEW = 32, end to end, in a dedicated VALU. No optimization is required.

**Milestone rule:** VADD must work correctly through the whole processor before a team moves on to the other instructions.

### 4.1 Required functional blocks

| Block | Requirement |
|---|---|
| Instruction decode | Recognize custom-0, decode the operation, route it to the VALU, trap on reserved encodings, leave every existing instruction unaffected |
| Operand handling | Treat each 64-bit operand as independent lanes; take operands from the register file and from scoreboard forwarding |
| VALU | Separate module with its own functional-unit type; independent lanes; combined 64-bit result; datapath ready for SEW 8/16 in Stage 2 |
| Pipeline integration | Instantiated in the execute stage; issue logic handles its busy state and structural hazards; writeback through a new port or an arbitrated shared port (justify the choice); a flushed instruction never writes back |

### 4.2 Milestones and marks

| Milestone | What we check | Marks |
|---|---|---|
| M1. Understanding | At Review 1 and the Gate 1 viva: show where the instructions are decoded, where the VALU sits, and explain the scoreboard's role | 8 |
| M2. Decode | Decoder change, reserved encodings trap, public directed tests G1.2 | 8 |
| M3. VALU datapath | VADD and the mandatory set pass (G1.1, G1.2); lanes are independent; datapath scales to SEW 8/16 | 12 |
| M4. Pipeline integration | Separate functional unit, hazard handling, writeback, flush behaviour; dependency chain (G1.3) and scalar smoke (G1.4) pass | 12 |

### 4.3 Gate 1 (Review 2, 28 Nov, 9:00 AM)

The organizers run `python3 tools/grade.py --gate 1` on the team's repository and hold a 5-minute viva over video call.

**Pass:** Stage 2 and bonus marks are unlocked.

| Check | Content | Pass criterion |
|---|---|---|
| G1.0 Build | Verilator build of the team's CVA6 | Builds (required) |
| G1.1 VADD | Public VADD directed tests | All pass (required) |
| G1.2 Mandatory set | Public directed tests for VSUB, VAND, VOR, VXOR | At least 4 of the 5 instructions fully pass (required) |
| G1.3 Dependency chain | Back-to-back vector → vector → scalar chain, self-checked and compared with Spike | Passes (required) |
| G1.4 Scalar smoke | All 53 `rv64ui` riscv-tests | 100% (required) |
| G1.5 Viva | Decoder change, VALU unit type, writeback path, scoreboard | Rated satisfactory (required) |

**Fail:** one retry at Review 3 (28 Nov, 1:00 PM) with a 4-mark penalty.

**Fail at the retry:** the team may keep improving Stage 1 for the final evaluation, but Stage 2 and bonus marks are not awarded and the team is not eligible for awards.

---

## 5. Stage 2: State, Verification and Exploration (45 marks)

Stage 2 makes the extension architecturally complete and proves it correct. Teams may start Stage 2 work at any time, but it is scored only after Gate 1 is passed.

### 5.1 Configurable width and vector CSRs (15 marks)

- Implement `vcfg` and `vstat` exactly as in Section 3.4.
- Support SEW = 8, 16 and 32 for every implemented instruction.
- Meet the four architectural rules in Section 3.5.
- Document, for each rule, the mechanism you chose and why (for example: flush the pipeline after a `vcfg` write; carry the overflow flag through the scoreboard to commit).

### 5.2 Verification work (15 marks)

The team's own verification, described in detail in Section 6: extending the Spike model (5), hazard tests (4), the random campaign (3) and the coverage table (3).

### 5.3 Gate 2 hidden-test results (15 marks)

Scored automatically at Gate 2 (Section 6.4): hidden directed tests (6), hidden hazard tests (5), random tests against the organizers' complete Spike model (4), each scaled by pass rate.

### 5.4 Stretch goals (bonus)

| Goal | Requirement |
|---|---|
| VSLL / VSRL | Per-lane logical shifts at every SEW |
| VMUL | A pipelined multi-cycle unit (2–3 stages); handles writeback contention and kills in-flight operations on a flush |
| VMAC | Reads `rd` as a third source; needs a third register-file read port and scoreboard tracking of `rd` as a source |
| Extra verification | For example lockstep comparison with Spike, functional coverage, or a unit testbench for the VALU |

### 5.5 Bonus track: memory-system exploration (bonus)

Vector instructions cut the number of instructions, not the data moved, so the cache matters more for vector code. Using the provided kernels and sweep tool:

1. Write vector versions of the kernels in `tests/kernels/vec/`.
2. Measure scalar and vector versions on at least four pre-validated L1 cache configurations (cycles, instructions, miss rates).
3. Choose one configuration within the **40 KiB** L1 storage budget (simulated memory latency fixed at **40 cycles**) and justify it.
4. Explain how the vector extension changes which configuration is best.

Only cache **parameters** may change; cache RTL must not be modified. The track is graded on reasoning, not on the lowest cycle count. Details: `docs/cache_track.md` in the kit.

---

## 6. Verification: What You Test, What We Provide, What We Run

Verification is a deliverable, not an afterthought. Teams must (a) write and run their own tests, and (b) run the provided infrastructure on their design throughout the event. At the gates, the organizers run the same infrastructure with hidden tests on top.

### 6.1 Three layers

| Layer | Who | What |
|---|---|---|
| Provided | Organizers, in the kit | Public directed tests (mandatory set, SEW 32), two example hazard tests, Spike model for the mandatory set at SEW 32, random test generator, RTL-vs-Spike trace comparison, the grader, 85 scalar riscv-tests, performance kernels |
| Your own | Each team | Spike model extension, your own directed tests (SEW 8/16, stretch instructions), at least six hazard scenarios, a random campaign, a coverage table, plus running the provided grader yourself |
| Hidden | Organizers, at Gate 2 | Unseen directed and hazard tests, a complete Spike model, fresh random seeds, full scalar regression, all on a clean checkout of your submitted commit |

### 6.2 How a test is judged

Two independent checks:

1. **Self-check.** A test program compares results with expected values and prints one line per check: `[PASS] name expected=0x… actual=0x…`. The program exits with 0 only if every check passed.
2. **Trace comparison with Spike.** The same program runs on your RTL and on Spike. Both produce a commit trace (PC, instruction, register written). The first differing instruction is reported as a `MISMATCH`; a run that does not finish is a `HANG`. Timer and counter reads are excluded, because Spike has no timing.

A design can pass its self-checks and still fail trace comparison. For example, a wrong value in a register the test never checks. Both count.

### 6.3 Your verification tasks (15 marks)

| Task | Minimum requirement | Marks |
|---|---|---|
| V1. Extend the Spike model | Add SEW 8/16, `vcfg`/`vstat`, the EN = 0 trap, the overflow flag, and every stretch instruction you implement to `spike-ext/vsimd.cc`. It is your reference: if it is wrong, your comparisons are wrong | 5 |
| V2. Hazard tests | At least these six scenarios, self-checking, passing trace comparison: **(1)** back-to-back read-after-write chains mixing vector, scalar and load instructions; **(2)** vector instructions right after a trapping instruction (`ecall`, `ebreak`, illegal instruction) that the handler skips: they must leave no trace; **(3)** a `vcfg` write followed immediately by dependent vector instructions; **(4)** an overflowing vector instruction that is flushed: `vstat` unchanged; **(5)** a vector instruction with EN = 0: check `mcause`, `mepc`, `rd`; **(6)** a vector instruction immediately before an exception | 4 |
| V3. Random campaign | At least 50 seeds from `tools/rand_gen.py` in Stage 2 mode with every instruction you implement, all compared with Spike. Record seeds, pass, mismatch and hang counts, and the bugs you found | 3 |
| V4. Coverage table | For each instruction × each SEW: which tests cover zero, all ones, maximum signed, minimum signed, signed overflow, and carry or borrow at a lane boundary | 3 |

Also write your own directed tests for SEW 8 and 16 and for every stretch instruction. They feed V4, and the hidden tests at Gate 2 cover the same ground.

### 6.4 What the organizers run

**Gate 1** runs the public tests only (Section 4.3).

**Gate 2** runs on a clean checkout of your submitted commit:

| Check | Content | Requirement |
|---|---|---|
| G2.0 Build | Verilator build of the submitted commit | Builds (required for awards) |
| G2.1 Hidden directed | Every implemented instruction at SEW 8, 16 and 32, with edge cases | At least 90% pass and no hangs (required for awards); 6 marks scaled by pass rate |
| G2.2 Hidden hazards | The scenarios of V2 plus unseen variants | 5 marks scaled by pass rate |
| G2.3 Random vs Spike | 100 fresh seeds, instructions you claim in `submission/TEAM.md` | 4 marks scaled by pass rate |
| G2.4 Scalar regression | All 85 `rv64ui`, `rv64um` and `rv64ua` riscv-tests | 100% (required for awards) |
| G2.5 Stretch | Hidden tests for each stretch instruction you claim | Bonus only |

### 6.5 Workflow we expect

```bash
scripts/docker_shell.sh                          # enter the frozen environment
scripts/build_rtl.sh                             # rebuild after every RTL change
make -C tests && make -C spike-ext               # build tests and your Spike model
scripts/run_test.sh cmp tests/build/hazard/raw_chain.elf      # one test: RTL vs Spike
python3 tools/rand_gen.py --seeds 1-50 --outdir tests/random --stage2 \
        --ops vadd,vsub,vand,vor,vxor
python3 tools/grade.py --gate 1 --no-build       # before Review 2
python3 tools/grade.py --gate 2 --no-build --seeds 50 \
        --random-ops vadd,vsub,vand,vor,vxor     # before the finale
```

Run the grader early and often. It is the exact script used at both gates; the only differences at Gate 2 are the hidden tests, the complete Spike model and fresh seeds.

### 6.6 Verification evidence to submit

- Your tests in `tests/directed/` and `tests/hazard/`, your Spike model in `spike-ext/`.
- `submission/VERIFICATION.md`: the V2 scenario table, the V3 campaign results, the V4 coverage table, and the bugs you found and fixed.
- `submission/verification/`: the grader reports and logs from your final commit.

---

## 7. Evaluation

Marked out of 100, plus up to 15 bonus marks. About half of the marks come from automated checks; the rest from reviews, the viva, the documents and the demo.

| Stage | Component | Marks | How it is measured |
|---|---|---|---|
| Stage 1 | M1. Understanding of CVA6 microarchitecture | 8 | Review 1 and Gate 1 viva, final Q&A |
| Stage 1 | M2. Instruction decode | 8 | Code review, reserved encodings trap, G1.2 |
| Stage 1 | M3. VALU datapath | 12 | G1.1 / G1.2, lane independence, readiness for SEW 8/16 |
| Stage 1 | M4. Pipeline integration | 12 | Separate functional unit, hazards, writeback, flush, G1.3 / G1.4 |
| **Stage 1** | | **40** | |
| Stage 2 | Configurable width and vector CSRs | 15 | The four architectural rules, design rationale |
| Stage 2 | Your verification work (V1–V4) | 15 | Spike model 5, hazard tests 4, random campaign 3, coverage 3 |
| Stage 2 | Gate 2 hidden-test results | 15 | G2.1 (6), G2.2 (5), G2.3 (4), scaled by pass rate |
| **Stage 2** | | **45** | |
| Final | Architecture and documentation | 7 | Block diagram, execution flow, design decisions |
| Final | Demonstration and presentation | 8 | Live demo with Spike comparison, answers to judges |
| **Final** | | **15** | |
| **Total** | | **100** | |

### Bonus (capped at +15)

| Item | Max | Condition |
|---|---|---|
| Memory-system exploration | +8 | Measurements 30%, analysis 40%, justification under the budget 20%, presentation 10% |
| VSLL / VSRL | +1 | Pass the hidden G2.5 tests |
| Pipelined VMUL | +2 | Pass G2.5, including flush and writeback-contention cases |
| VMAC | +2 | Pass G2.5, including read-after-write on `rd` |
| Additional verification | +2 | Lockstep with Spike, functional coverage, or a VALU unit testbench |

### Gate rules

- **Gate 1 not passed at the retry:** Stage 2 and bonus marks are zero, and the team is not eligible for awards.
- **Gate 1 passed at the retry:** 4 marks are deducted.
- **A Gate 2 "required for awards" criterion missed:** the team is fully scored but not eligible for awards.
- **Tie-break:** higher G2.1 + G2.2 pass rate, then the judges' decision.

---

## 8. Schedule (online, IST, tentative)

| Time | Event |
|---|---|
| 27 Nov, 5:00 PM | Kickoff: problem statement, CVA6 / Spike / grader walkthrough. Stage 1 begins |
| 27 Nov, 10:00–11:00 PM | Review 1: show where your instructions are decoded and how your VALU will connect (counts toward M1) |
| 28 Nov, 9:00 AM | Review 2 = Gate 1 (grader plus 5-minute viva). Stage 2 begins |
| 28 Nov, 1:00 PM | Review 3 (optional): Gate 1 retry (4-mark penalty), Stage 2 progress check |
| 28 Nov, 5:00 PM | Finale: code freeze, then final demonstrations. Gate 2 runs on the frozen commits |

All reviews, vivas and demonstrations are held over video call. Slots, links and any changes are posted on the WhatsApp channel. Create your repository, add `SiliconSprint2026` as a collaborator and register before Review 1.

---

## 9. Submission Deliverables

Each team works in its own private repository created from the template, and submits the full commit SHA of its final commit at code freeze (28 Nov, 5:00 PM IST). The judges grade exactly that commit, on a clean checkout.

| Deliverable | Where |
|---|---|
| RTL: decoder changes, the VALU, its integration, the vector CSRs | `cva6/core/` (new files added to `cva6/core/Flist.cva6`) |
| Extended Spike model | `spike-ext/vsimd.cc` |
| Your tests | `tests/directed/`, `tests/hazard/` |
| Instruction documentation: encoding, semantics, CSRs, exceptions, limitations | `submission/INSTRUCTIONS.md` |
| Architecture documentation: block diagram, execution flow decode → commit, design decisions | `submission/DESIGN.md` |
| Verification report: V2 table, V3 results, V4 coverage, bugs found; grader reports and logs | `submission/VERIFICATION.md`, `submission/verification/` |
| Team details, claimed stretch goals, attribution and AI-use disclosure | `submission/TEAM.md` |
| Bonus: vector kernels, cache report, CSV and plots | `tests/kernels/vec/`, `submission/CACHE_REPORT.md`, `submission/bonus/` |
| Final demonstration | Live, over video call at the finale |

> **Check before you submit:** a fresh clone of your commit must build with `scripts/build_rtl.sh`, and `submission/TEAM.md` must list exactly the stretch instructions you claim.

---

## 10. Rules, Integrity Checks and Design Defense

| Allowed | Not allowed |
|---|---|
| Modify decode, issue, execute, writeback, commit and CSR logic in CVA6 | Break existing scalar behaviour |
| Add a dedicated VALU and new writeback ports | Implement the instructions inside the existing integer ALU, or use CV-X-IF |
| Add your own tests, benchmarks and random-generator constraints | Modify `tests/common/`, `tools/` (except constraints in `rand_gen.py`), `docker/`, cache RTL or the memory latency |
| Change cache parameters in the bonus track | Replace the CVA6 baseline or its configuration |
| Use open-source material and AI tools with disclosure | Hard-code test results or detect test programs |
| | Share code between teams, or make your repository public before the awards |

**Hidden tests.** Gate 2 runs unseen directed and hazard tests and fresh random seeds against the organizers' complete Spike model, on a clean checkout of your submitted commit.

**Attribution and AI disclosure.** List every reused source and every AI tool in `submission/TEAM.md`: where it was used and how. Declared reuse earns no innovation credit. Undeclared reuse can lead to disqualification.

**Design defense.** Every team presents its design and answers questions on its own implementation. A judge may open a waveform at an instruction of their choosing and ask any team member to walk through it cycle by cycle, from decode to commit. Every member must be able to explain every part of what the team submits.

**Repositories.** Create your repository with **Use this template** and set it to **Private**. Forks are public and are not accepted. The repository must stay private until the awards.

---

## 11. Getting Started

1. Open the starter kit: `SiliconSprint_DigitalTrack`. Its README is the starting point.
2. Install Docker and get the tool image: `docker pull ghcr.io/siliconsprint2026/cva6-simd:2026` (see `docs/1_docker_setup.md`).
3. Create your private repository with **Use this template**, add your teammates and `SiliconSprint2026`, and register.
4. Run the setup check: the unmodified kit must show **G1.0 Build PASS** and **G1.4 Scalar smoke 53/53**, with every vector check failing.
5. Read `docs/instruction_encoding.md`, `docs/cva6_guide.md` and `docs/verification.md`.

Questions and announcements: **SiliconSprint 2026 WhatsApp channel.**

---

> **The challenge:** make CVA6 process several data elements with one instruction, keep it correct under speculation, and prove it against a golden model.
>
> ***Build it. Integrate it. Verify it. Defend it.***
