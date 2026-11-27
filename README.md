# SiliconSprint 2026 · Track 1 · Digital Design: CVA6 Packed-SIMD

**IEEE NITK × BITS Hyderabad · 24-hour hardware hackathon · 27–28 November 2026 · Online**

Extend the open-source **CVA6** RISC-V processor with custom packed-SIMD (vector)
instructions, keep it correct under speculation, and prove it against the **Spike**
golden model.

> **This is a template repository.** Do **not** fork it. Create your team's own
> **private** copy with **[Use this template → Create a new repository](https://github.com/SiliconSprint2026/SiliconSprint_DigitalTrack/generate)**.
> Full instructions: [docs/2_repository_setup.md](docs/2_repository_setup.md).

---

## Before the event: 5 steps

| # | Step | Who | Time | Guide |
| --- | --- | --- | --- | --- |
| 1 | Install Docker and give it 8 GB RAM | every member | 15 min | [Docker setup §2](docs/1_docker_setup.md#2-install-docker) |
| 2 | Get the tool image (`docker pull`, or load the `.tar`) | every member | 10 min | [Docker setup §3](docs/1_docker_setup.md#3-get-the-tool-image) |
| 3 | Create your **private** team repo from this template | one member | 2 min | [Repository setup](docs/2_repository_setup.md) |
| 4 | Add teammates and `SiliconSprint2026`, register the repo | one member | 5 min | [Repository setup](docs/2_repository_setup.md) |
| 5 | Clone, build, run the setup check | every member | 15 min | [Docker setup §4](docs/1_docker_setup.md#4-setup-check) |

**You are ready when step 5 shows `G1.0 Build PASS` and `G1.4 Scalar smoke 53/53`**,
with every vector check failing. That is expected: nothing is implemented yet.

## Documentation

| Read | What |
| --- | --- |
| [Problem statement (PDF)](docs/SiliconSprint2026_Track_Problem_Statement.pdf) | The challenge, stages, marking. **The authority** |
| [docs/1_docker_setup.md](docs/1_docker_setup.md) | Tools, Docker, setup check, troubleshooting |
| [docs/2_repository_setup.md](docs/2_repository_setup.md) | Your private repo, collaborator access, registration |
| [docs/3_event_guide.md](docs/3_event_guide.md) | Schedule, Gate 1 / Gate 2, code freeze, submission, demo, rules |
| [docs/4_workflow.md](docs/4_workflow.md) | Daily commands, where each deliverable goes, FAQ |
| [docs/instruction_encoding.md](docs/instruction_encoding.md) | The instructions and CSRs you implement |
| [docs/cva6_guide.md](docs/cva6_guide.md) | Where to look inside CVA6 |
| [docs/verification.md](docs/verification.md) | How your design is checked against Spike |
| [docs/cache_track.md](docs/cache_track.md) | Bonus track: cache design exploration |

## What is in this repository

| Path | Contents | You change it? |
| --- | --- | --- |
| `cva6/` | Frozen CVA6 v5.3.0, config `cv64a6_imafdc_sv39`, CV-X-IF disabled | **Yes: your RTL** |
| `spike-ext/` | Spike golden model (mandatory instructions, SEW = 32) | **Yes: Stage 2** |
| `tests/directed/`, `tests/hazard/` | Public tests | Add your own |
| `tests/kernels/vec/` | Bonus-track vector kernels | **Yes** |
| `submission/` | Templates for every written deliverable | **Yes: fill in** |
| `tests/common/`, `tools/`, `scripts/`, `docker/` | Runtime, grader, tooling, environment | **No** |

## The challenge in one paragraph

Docker gives you a frozen set of tools (Verilator, Spike, the RISC-V compiler and the
riscv-tests). You write SystemVerilog in `cva6/` and tests in `tests/`, and build and
simulate inside the container. A grader (`tools/grade.py`) checks your core against
the Spike reference model. The organizers run the **same grader** at Gate 1 (end of
Stage 1) and Gate 2 (code freeze, with hidden tests). At code freeze you submit your
commit SHA, and the judges grade exactly that commit.

**Contact:** [SiliconSprint 2026 WhatsApp channel](https://chat.whatsapp.com/DpBT2pWoCFdBt4d3ggV3fX) · All tracks: [SiliconSprint2026](https://github.com/SiliconSprint2026) · Happy hacking!
