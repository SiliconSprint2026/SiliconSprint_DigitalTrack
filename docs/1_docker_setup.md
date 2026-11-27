# 1 · Docker setup

Do this **at home, before the event**, on every team member's laptop.

## 1. What Docker is used for (and what it is not)

The challenge needs a specific set of tools, all at **exact versions**. A different
Verilator or Spike version can change results, and the judges grade with these versions:

| Tool | Version | Used for |
| --- | --- | --- |
| Ubuntu | 24.04 | Base system |
| Verilator | 5.008 (+ CVA6 patch) | Compiles the CVA6 SystemVerilog into a fast simulator |
| Spike | riscv-isa-sim `6e18887` | RISC-V golden reference model |
| RISC-V GCC | `riscv64-unknown-elf-gcc` 13.2 | Compiles your C and assembly test programs |
| riscv-tests | `1b3f8bd` (rv64ui/um/ua) | Scalar regression tests, in `/opt/riscv-tests/isa` |
| Python 3 + matplotlib | Ubuntu 24.04 | Grader, random test generator, cache-sweep plots |

All of these come pre-built in one **Docker image**, `siliconsprint/cva6-simd:2026`.

* **Your code lives on your laptop**, in your git repository. Edit it with any editor (VS Code, etc.).
* **Docker only runs the tools.** `scripts/docker_shell.sh` starts a container with
  your repository mounted at `/work`. Builds and simulations run inside it, and their
  output files appear in your repository folder.
* Nothing you do inside the container changes the image. Close the container any
  time; your files stay on your laptop.
* The **same image** is used at Gate 1 and Gate 2. If it works in the container on
  your laptop, it works for the judges.

## 2. Install Docker Desktop

| OS | Steps |
| --- | --- |
| **Windows 10/11** | Install [Docker Desktop](https://docs.docker.com/desktop/setup/install/windows-install/) with the **WSL 2** backend (the installer offers it). Reboot. Also install Ubuntu from the Microsoft Store (`wsl --install -d Ubuntu`) for fast builds; see §5 |
| **macOS** | Install [Docker Desktop](https://docs.docker.com/desktop/setup/install/mac-install/). On Apple Silicon (M1/M2/M3) the image runs under emulation and is several times slower. Pair with a teammate on an x86 laptop if you can |
| **Linux** | Install [Docker Engine](https://docs.docker.com/engine/install/) and add yourself to the `docker` group: `sudo usermod -aG docker $USER`, then log out and back in |

**Give Docker enough memory.** Docker Desktop → Settings → Resources:
**Memory ≥ 8 GB**, CPUs ≥ 4, disk ≥ 20 GB. (On Windows with WSL 2, memory is set
in `%UserProfile%\.wslconfig`: add `[wsl2]` and `memory=8GB` on separate lines, then run `wsl --shutdown`.)
The Verilator build is killed if Docker has less than about 6 GB.

Check: `docker run --rm hello-world` prints "Hello from Docker!".

## 3. Get the tool image

### Build it yourself (about 20 minutes, needs internet)

```bash
docker build -t siliconsprint/cva6-simd:2026 docker/
```

### Check

```bash
docker images | grep cva6-simd
# ghcr.io/siliconsprint2026/cva6-simd   2026  ...  1.39GB     (option A)
# siliconsprint/cva6-simd           2026  ...  1.39GB     (options B / C)
```

`scripts/docker_shell.sh` finds the image under either name.

## 4. Setup check

Do this after your team repository exists (see [2_repository_setup.md](2_repository_setup.md)). Clone it, then:

```bash
cd ss26-digital-<team-name>
scripts/docker_shell.sh            # Windows PowerShell: .\scripts\docker_shell.ps1
```

You are now inside the container, in `/work` (= your repository). Run:

```bash
make -C tests                      # compile test programs              ~10 s
make -C spike-ext                  # build the Spike golden model        ~20 s
scripts/build_rtl.sh               # build CVA6 with Verilator           1-10 min
python3 tools/grade.py --gate 1 --no-build
```

Expected result on the unmodified kit:

```
| G1.0 | Build                     | PASS   | yes |
| G1.1 | VADD end to end           | 0/1    | yes |   <- expected: not implemented yet
| G1.2 | Mandatory set (>= 4 of 5) | 0/5    | yes |   <- expected
| G1.3 | Dependency chain          | 0/1    | yes |   <- expected
| G1.4 | Scalar smoke (rv64ui)     | 53/53  | yes |   <- must be 53/53
```

**G1.0 PASS and G1.4 53/53 means your setup is complete.** Type `exit` to leave the container.

## 5. Windows: important details

* **Line endings.** Before cloning, run `git config --global core.autocrlf false`.
  Windows-style (CRLF) line endings break the scripts (`$'\r': command not found`).
* **Speed.** Builds are much faster when the repository lives inside WSL, not on `C:\`:
  open the Ubuntu terminal, `cd ~`, clone there, and run `scripts/docker_shell.sh` from that terminal.
  You can still edit with VS Code: run `code .` from the Ubuntu terminal (WSL extension).
* **Git Bash.** If Docker paths get mangled, prefix the command with `MSYS_NO_PATHCONV=1`.

## 6. Troubleshooting

| Problem | Fix |
| --- | --- |
| `Cannot connect to the Docker daemon` | Start Docker Desktop and wait until it says "Engine running" |
| `cva6-simd image not found` | Step 3 was not done on this laptop: `docker pull ghcr.io/siliconsprint2026/cva6-simd:2026` |
| `docker pull` says `denied` or `not found` | Check the spelling; otherwise use option B (`.tar`) |
| `permission denied ... docker.sock` (Linux) | `sudo usermod -aG docker $USER`, log out and back in |
| Build stops with `Killed` or `c++: fatal error` | Docker has too little memory: raise it to 8 GB, or run `NUM_JOBS=2 scripts/build_rtl.sh` |
| `$'\r': command not found` | CRLF line endings: `git config --global core.autocrlf false`, delete the clone, clone again |
| `scripts/docker_shell.sh: Permission denied` | `bash scripts/docker_shell.sh` (or `chmod +x scripts/*.sh`) |
| Very slow build on Windows | Move the repository into the WSL file system (§5) |
| Very slow on Apple Silicon | Expected (emulation). Use an x86 laptop for builds where possible |
| G1.4 is not 53/53 on the unmodified kit | Something is wrong with your setup; contact the organizers before the event |
