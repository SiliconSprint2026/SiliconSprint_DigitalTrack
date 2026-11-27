# 4 · Workflow, deliverables and FAQ

All commands run inside the container: `scripts/docker_shell.sh` (Windows: `.\scripts\docker_shell.ps1`).

## 1. Daily commands


| Task | Command |
| --- | --- |
| Rebuild RTL after an edit | `scripts/build_rtl.sh` (incremental: 1–3 min) |
| Build all tests | `make -C tests` |
| Rebuild the Spike model | `make -C spike-ext` |
| Run one test on RTL | `scripts/run_test.sh rtl tests/build/directed/vadd.elf` |
| Run one test on Spike | `scripts/run_test.sh spike tests/build/directed/vadd.elf` |
| Compare RTL with Spike | `scripts/run_test.sh cmp tests/build/hazard/raw_chain.elf` |
| Waveforms | `TRACE=1 scripts/build_rtl.sh`, then `scripts/run_test.sh wave <elf>` → `run_out/<test>/wave.fst` (GTKWave) |
| Random tests | `python3 tools/rand_gen.py --seeds 1-50 --outdir tests/random` then `make -C tests` |
| Gate dry runs | `python3 tools/grade.py --gate 1 --no-build` / `--gate 2 --no-build` |
| Bonus sweep | `python3 tools/cache_sweep.py --configs base,d16w4_wt,d16w4_wb,d8w2_wt` |

The RTL commit trace for each run is `run_out/<test>/trace_rvfi_hart_00.dasm`:
one line per committed instruction, with the register it wrote.

Tips:

* Get **VADD** working end to end first. Nothing else counts until it does.
* Use `-j` with care: each simulation needs about 0.5 GB of RAM.
* Write a test for a bug **before** you fix it, then keep the test.

---

## 2. Where your work goes

| Deliverable (problem statement §10) | Location |
| --- | --- |
| RTL implementation | `cva6/core/` (new files must be added to `cva6/core/Flist.cva6`) |
| Extended Spike model | `spike-ext/vsimd.cc` |
| Your directed tests | `tests/directed/*.c` or `*.S` |
| Your hazard tests (≥ 6 scenarios, task F.2) | `tests/hazard/*.S` or `*.c` |
| Random-campaign results | `submission/verification/` (grader report + logs) |
| Coverage table (task F.4) | `submission/VERIFICATION.md` |
| Instruction documentation | `submission/INSTRUCTIONS.md` |
| Architecture documentation + block diagram | `submission/DESIGN.md` (+ image in `submission/`) |
| Bonus: vector kernels | `tests/kernels/vec/*_vec.c` |
| Bonus: cache report, CSV, plots | `submission/CACHE_REPORT.md`, `submission/bonus/` |
| Team details, claimed stretch goals, references | `submission/TEAM.md` |
| Demo slides (optional) | `submission/slides.pdf` |

Every file in `submission/` is a template with the headings we expect.
Replace the guidance text with your content.

---

## 3. FAQ and troubleshooting

| Symptom | Fix |
| --- | --- |
| Verilator build killed / `c++: fatal error: Killed` | Give Docker 8 GB RAM, or `NUM_JOBS=2 scripts/build_rtl.sh` |
| `%Error-PINMISSING` | A new port is not connected at every instantiation (check `cva6.sv`) |
| My new `.sv` file is ignored | Add it to `cva6/core/Flist.cva6` |
| `[TRAP] unexpected trap mcause=0x2` | Illegal instruction: the decoder does not recognise the encoding (expected on the unmodified core) |
| Test never ends / grader says `HANG` | An instruction never writes back or the core deadlocks. Check the last lines of `trace_rvfi_hart_00.dasm` |
| `MISMATCH @ pc …` | The first instruction where RTL and Spike disagree. Look at that PC in `tests/build/<dir>/<test>.dump` |
| `Unable to load extlib` | `make -C spike-ext` |
| Custom CSR is illegal on Spike | Register it from `get_instructions()` as well as `get_csrs()` (see the comment in `spike-ext/vsimd.cc`) |
| Scripts fail with `$'\r': command not found` | CRLF line endings: `git config core.autocrlf false`, re-checkout or re-unzip |
| Docker paths mangled in Git Bash | Prefix commands with `MSYS_NO_PATHCONV=1` |

More: [1_docker_setup.md](1_docker_setup.md), [cva6_guide.md](cva6_guide.md), [verification.md](verification.md), [cache_track.md](cache_track.md).

**Can we use our own laptop tools instead of Docker?** Only if you reproduce
the exact versions in `docker/Dockerfile`. Gates run in the Docker image, so
anything that works only outside it does not count.

**Is VMUL / VMAC required?** No, they are stretch goals. Unimplemented stretch
encodings must raise an illegal-instruction exception, like any reserved encoding.

**Do we need to implement the bonus track?** No. It adds up to +8 marks and
needs a passed Gate 1.

