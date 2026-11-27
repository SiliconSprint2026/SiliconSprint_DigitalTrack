# Verification

## Spike model extension (task F.1)

What you added to `spike-ext/vsimd.cc` and how you checked the model itself.

## Hazard tests (task F.2)

| Scenario | Test file | What it checks | Result |
| --- | --- | --- | --- |
| 1. Back-to-back RAW chains (vector / scalar / load) | | | |
| 2. Vector ops younger than a trapping instruction / in a mispredicted-branch shadow | | | |
| 3. `vcfg` write followed by dependent vector ops | | | |
| 4. Overflowing vector op that is flushed (`vstat` unchanged) | | | |
| 5. Vector op with `vcfg.EN = 0` (`mcause`, `mepc`, `rd`) | | | |
| 6. Vector op immediately before an exception | | | |

## Random campaign (task F.3)

Generator options used, number of seeds, results. Put the grader report and
logs in `submission/verification/`.

| Seeds | Ops | Stage-2 mode | Pass | Mismatch | Hang |
| --- | --- | --- | --- | --- | --- |
| | | | | | |

## Coverage table (task F.4)

Mark each cell with the test(s) that exercise it.

| Instruction | SEW | zero | all ones | max signed | min signed | signed overflow | lane-boundary carry/borrow |
| --- | --- | --- | --- | --- | --- | --- | --- |
| VADD | 8 | | | | | | |
| VADD | 16 | | | | | | |
| VADD | 32 | | | | | | |
| VSUB | 8 / 16 / 32 | | | | | | |
| VAND / VOR / VXOR | 8 / 16 / 32 | | | | | | |

## Bugs found by verification

| Bug | Found by | Fix |
| --- | --- | --- |
| | | |

## Final grader output

Paste the summary table of `python3 tools/grade.py --gate 2` run on your final commit.
