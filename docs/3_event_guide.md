# 3 · Event guide

The problem statement PDF is the authority. This guide summarises how the 24 hours
run in practice.

## Schedule (tentative, IST, fully online)

| Time | Event |
| --- | --- |
| 27 Nov, 5:00 PM | **Kickoff**: problem statement, CVA6 / Spike / grader walkthrough. **Stage 1 begins** |
| 27 Nov, 10:00–11:00 PM | **Review 1**: progress review. Show where your instructions are decoded and how your VALU will connect |
| 28 Nov, 9:00 AM | **Review 2 = Gate 1 screening**: grader run plus a 5-minute viva. **Stage 2 begins** |
| 28 Nov, 1:00 PM | **Review 3 (optional)**: Gate 1 retry slot (4-mark penalty) and Stage 2 progress check |
| **28 Nov, 5:00 PM** | **Finale**: code freeze (submit your final commit SHA), then final demonstrations. Gate 2 hidden tests run on the frozen commits |
| After the finale | Evaluation and awards |

All reviews, vivas and demonstrations happen over video call; slot times and links are
posted on the [SiliconSprint 2026 WhatsApp channel](https://chat.whatsapp.com/DpBT2pWoCFdBt4d3ggV3fX). Review 1 also feeds the "Understanding of CVA6
microarchitecture" mark. You may start Stage 2 work at any time, but it is scored only
after you pass Gate 1.

## Stage 1: Core integration (27 Nov 5:00 PM – 28 Nov 9:00 AM)

Get the five mandatory instructions (VADD, VSUB, VAND, VOR, VXOR, 2 × 32-bit lanes)
working end to end through a **dedicated VALU functional unit**. **VADD first:**
nothing else counts until VADD works.

### Gate 1 (Review 2, 28 Nov, 9:00 AM)

The organizers run, on your pushed `main`:

```bash
python3 tools/grade.py --gate 1
```

| Check | Requirement |
| --- | --- |
| G1.0 Build | Verilator build succeeds |
| G1.1 VADD | All public VADD tests pass |
| G1.2 Mandatory set | At least 4 of the 5 instructions fully pass |
| G1.3 Dependency chain | `tests/hazard/raw_chain` passes and matches Spike |
| G1.4 Scalar smoke | 53/53 rv64ui tests pass |
| G1.5 Viva (5 min) | Show your decoder change, VALU unit type and writeback path; explain the scoreboard |

* **Pass:** Stage 2 and bonus marks are unlocked.
* **Fail:** one retry at Review 3 (1:00 PM) with a 4-mark penalty.
* **Fail at retry:** you may keep improving Stage 1, but Stage 2 and bonus marks are not awarded and you are not eligible for awards.

Run Gate 1 yourself as often as you like. It is the same script.

## Stage 2: State, verification, exploration (28 Nov 9:00 AM – 5:00 PM)

* `vcfg` / `vstat` CSRs (0x8C0 / 0x8C1), element widths 8 / 16 / 32, the four architectural rules
* Extend the Spike model, write hazard tests (at least 6 scenarios), run the random campaign, report coverage
* Optional: stretch instructions (VSLL/VSRL, pipelined VMUL, VMAC) and the cache bonus track

## Code freeze and submission (28 Nov, 5:00 PM IST)

1. Commit and **push** your final work. Fill in `submission/TEAM.md` (claimed features) and the other `submission/*.md` files.
2. Get the full commit SHA: `git rev-parse HEAD` (40 characters).
3. Submit it before **5:05 PM** through the submission form (link shared on the [SiliconSprint 2026 WhatsApp channel](https://chat.whatsapp.com/DpBT2pWoCFdBt4d3ggV3fX)).
4. Stop pushing.

The judges grade **exactly that commit**. A missing SHA means the last commit
pushed before 5:00 PM is graded.

Before you submit, check that a **fresh clone** of your commit builds:

```bash
git clone <your repo> /tmp/check && cd /tmp/check && git checkout <SHA>
scripts/docker_shell.sh scripts/build_rtl.sh
```

### Gate 2 (after code freeze, 28 Nov, 5:00 PM)

The organizers run the grader on your submitted commit with **hidden tests**,
their **complete Spike model**, and **fresh random seeds** for the instructions
you claim in `submission/TEAM.md`.

| Check | Requirement for awards |
| --- | --- |
| G2.0 Build | Builds |
| G2.1 Directed (hidden, all SEW) | ≥ 90 % pass, no hangs |
| G2.2 Hazards (hidden) | Scored |
| G2.3 Random vs Spike (fresh seeds) | Scored |
| G2.4 Scalar regression | 85/85 |
| G2.5 Stretch (hidden) | Bonus only |

Dry run: `python3 tools/grade.py --gate 2 --seeds 50 --random-ops vadd,vsub,vand,vor,vxor`

## Final demonstration (Finale, 28 Nov, 5:00 PM)

Will be informed later

Members must be able to answer questions about the implementation.

## Marking (summary)

| Part | Marks |
| --- | --- |
| Stage 1: CVA6 understanding 8, decode 8, VALU datapath 12, pipeline integration 12 | 40 |
| Stage 2: CSRs and element widths 15, your verification work 15, Gate 2 hidden-test results 15 | 45 |
| Final: documentation 7, demonstration 8 | 15 |
| **Bonus** (capped at +15): cache track +8, VSLL/VSRL +1, pipelined VMUL +2, VMAC +2, extra verification +2 | +15 |

## Rules (summary; full list in the problem statement)

1. Work on the provided kit; do not change the CVA6 version or configuration.
2. Implement the instructions in a dedicated VALU unit, not inside the existing ALU. No CV-X-IF.
3. Do not modify `tests/common/`, `tools/`, `docker/`, cache RTL or the memory latency.
4. Use external material only with attribution in `submission/TEAM.md`.
5. No code sharing between teams; repositories stay private until the awards.
6. **AI tools are allowed.** Disclose transparently where and how you used them, in `submission/TEAM.md`.
7. Every member must be able to explain the implementation.

## Help during the event

* Mentors: ask on the [SiliconSprint 2026 WhatsApp channel](https://chat.whatsapp.com/DpBT2pWoCFdBt4d3ggV3fX); mentor names and call links are going to be posted there.
* Clarifications are announced to all teams on the same channel.
