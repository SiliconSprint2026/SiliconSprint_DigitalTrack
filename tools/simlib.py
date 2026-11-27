"""SiliconSprint 2026 - shared helpers: run an ELF on Spike or on the CVA6
Verilator model, and parse/compare commit traces.

Used by grade.py, trace_compare.py and cache_sweep.py.
"""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CVA6_DIR = REPO / "cva6"
DEFAULT_TARGET = "cv64a6_imafdc_sv39"
DEFAULT_SPIKE_EXT = REPO / "spike-ext" / "build" / "libvsimd.so"
SPIKE_ISA = "rv64gc"
PREFIX = os.environ.get("RISCV_PREFIX", "riscv64-unknown-elf-")
# Cycle limit passed to the testharness (+time_out); beyond it a run is a HANG.
MAX_CYCLES = int(os.environ.get("MAX_CYCLES", "20000000"))

# CSRs whose read values legitimately differ between Spike and RTL: never compared.
#  * counters (timing)
#  * mtval (0x343): on ecall/ebreak CVA6 writes the instruction bits, Spike
#    writes 0 - both are allowed by the privileged spec. For illegal
#    instructions both write the instruction bits; directed tests check that.
COUNTER_CSRS = set(range(0xB00, 0xB20)) | set(range(0xC00, 0xC20)) | {0x343}


@dataclass
class RunResult:
    status: str            # PASS | FAIL | HANG | ERROR
    exit_code: int | None
    log: str               # stdout + stderr of the simulator
    trace: Path | None = None
    seconds: float = 0.0
    cycles: int | None = None


@dataclass
class TraceEntry:
    pc: int
    insn: int
    rd: int | None = None
    value: int | None = None
    line: str = field(default="", repr=False)


# ---------------------------------------------------------------------------
# Running simulators
# ---------------------------------------------------------------------------
def rtl_binary(ver_dir: Path | None = None) -> Path:
    return (ver_dir or (CVA6_DIR / "work-ver")) / "Variane_testharness"


def run_rtl(elf: Path, workdir: Path, timeout: float = 300.0,
            ver_dir: Path | None = None) -> RunResult:
    """Run one ELF on the CVA6 Verilator testharness inside `workdir`.
    The RVFI tracer writes trace_rvfi_hart_00.dasm into the working dir.
    An abnormal simulator termination (not a test FAIL/HANG) is retried once."""
    r = _run_rtl_once(elf, workdir, timeout, ver_dir)
    if r.status == "ERROR" and r.exit_code is not None:
        r = _run_rtl_once(elf, workdir, timeout, ver_dir)
    return r


def _run_rtl_once(elf: Path, workdir: Path, timeout: float,
                  ver_dir: Path | None) -> RunResult:
    import time
    workdir.mkdir(parents=True, exist_ok=True)
    binary = rtl_binary(ver_dir)
    if not binary.exists():
        return RunResult("ERROR", None, f"RTL model not built: {binary}")
    t0 = time.time()
    try:
        # +time_out: the RVFI tracer otherwise ends every run after 2M cycles
        p = subprocess.run([str(binary), f"+time_out={MAX_CYCLES}", str(elf.resolve())], cwd=workdir,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           timeout=timeout, text=True, errors="replace")
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode(errors="replace") if isinstance(e.stdout, bytes) else (e.stdout or "")
        return RunResult("HANG", None, out + f"\n*** HANG: no exit after {timeout:.0f}s ***\n",
                         workdir / "trace_rvfi_hart_00.dasm", time.time() - t0)
    secs = time.time() - t0
    trace = workdir / "trace_rvfi_hart_00.dasm"
    (workdir / "sim.log").write_text(p.stdout)
    m = re.search(r"after (\d+) cycles", p.stdout)
    cycles = int(m.group(1)) if m else None
    status = "PASS" if p.returncode == 0 and "SUCCESS" in p.stdout else "FAIL"
    log = p.stdout
    if "tohost = 2147483647" in log:  # RVFI tracer cycle limit reached
        status = "HANG"
        log += f"\n*** HANG: cycle limit ({MAX_CYCLES}) reached ***\n"
    elif p.returncode < 0 or ("SUCCESS" not in log and "FAILED" not in log):
        status = "ERROR"
        log += f"\n*** simulator terminated abnormally (exit {p.returncode}); " \
               f"out of memory? reduce -j ***\n"
    return RunResult(status, p.returncode, log, trace, secs, cycles)


def run_spike(elf: Path, workdir: Path, ext: Path | None = DEFAULT_SPIKE_EXT,
              timeout: float = 120.0, log_commits: bool = True) -> RunResult:
    import time
    workdir.mkdir(parents=True, exist_ok=True)
    if ext is not None and Path(ext) == DEFAULT_SPIKE_EXT and not DEFAULT_SPIKE_EXT.exists():
        subprocess.run(["make", "-C", str(REPO / "spike-ext")], stdout=subprocess.DEVNULL)
    log = workdir / "spike_commits.log"
    cmd = ["spike", f"--isa={SPIKE_ISA}"]
    if ext is not None:
        cmd += [f"--extlib={Path(ext).resolve()}", "--extension=vsimd"]
    if log_commits:
        cmd += ["--log-commits", f"--log={log}"]
    cmd += [str(elf.resolve())]
    t0 = time.time()
    try:
        p = subprocess.run(cmd, cwd=workdir, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=timeout, text=True,
                           errors="replace")
    except subprocess.TimeoutExpired:
        return RunResult("HANG", None, f"*** spike HANG after {timeout:.0f}s ***", log,
                         time.time() - t0)
    status = "PASS" if p.returncode == 0 else "FAIL"
    return RunResult(status, p.returncode, p.stdout, log if log_commits else None,
                     time.time() - t0)


# ---------------------------------------------------------------------------
# Traces
# ---------------------------------------------------------------------------
_SPIKE_RE = re.compile(r"^core\s+\d+:\s+(\d)\s+0x([0-9a-fA-F]+)\s+\(0x([0-9a-fA-F]+)\)(.*)$")
_RVFI_RE = re.compile(r"^(\d)\s+0x([0-9a-fA-F]+)\s+\(0x([0-9a-fA-F]+)\)(.*)$")
_XREG_RE = re.compile(r"\bx\s*(\d+)\s+0x([0-9a-fA-F]+)")
_ALWAYS_TRAP = {0x00000073, 0x00100073, 0x9002}  # ecall, ebreak, c.ebreak


def _norm_insn(insn: int) -> int:
    return insn & 0xFFFF if (insn & 3) != 3 else insn & 0xFFFFFFFF


def _is_counter_read(insn: int) -> bool:
    # SYSTEM opcode, funct3 != 0 => CSR instruction; csr = insn[31:20]
    return (insn & 0x7F) == 0x73 and ((insn >> 12) & 7) != 0 and (insn >> 20) in COUNTER_CSRS


def parse_trace(path: Path, fmt: str) -> list[TraceEntry]:
    regex = _SPIKE_RE if fmt == "spike" else _RVFI_RE
    out: list[TraceEntry] = []
    if not path or not Path(path).exists():
        return out
    with open(path, errors="replace") as f:
        for line in f:
            m = regex.match(line.strip())
            if not m:
                continue
            pc = int(m.group(2), 16)
            insn = _norm_insn(int(m.group(3), 16))
            # CVA6's RVFI tracer reports ecall / ebreak as retired; Spike does not
            # log trapping instructions. They always trap, so drop them.
            if fmt == "rvfi" and insn in _ALWAYS_TRAP:
                continue
            e = TraceEntry(pc, insn, line=line.rstrip())
            x = _XREG_RE.search(m.group(4))
            if x and int(x.group(1)) != 0:
                e.rd, e.value = int(x.group(1)), int(x.group(2), 16)
            if _is_counter_read(insn):
                e.value = None  # timing-dependent, never compared
            out.append(e)
    return out


def nocompare_range(elf: Path) -> tuple[int, int]:
    """Address range of .text.nocompare (HTIF polling + exit loop)."""
    syms = {}
    p = subprocess.run([PREFIX + "nm", str(elf)], stdout=subprocess.PIPE, text=True)
    for line in p.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] in ("__nocompare_start", "__nocompare_end"):
            syms[parts[2]] = int(parts[0], 16)
    return syms.get("__nocompare_start", 0), syms.get("__nocompare_end", 0)


def filter_trace(entries: list[TraceEntry], elf: Path) -> list[TraceEntry]:
    lo, hi = nocompare_range(elf)
    return [e for e in entries if e.pc >= 0x80000000 and not (lo <= e.pc < hi)]


@dataclass
class CompareResult:
    ok: bool
    compared: int
    message: str


def compare_traces(rtl: list[TraceEntry], ref: list[TraceEntry]) -> CompareResult:
    """Compare committed instructions and integer register writes in order."""
    n = min(len(rtl), len(ref))
    for i in range(n):
        a, b = rtl[i], ref[i]
        if a.pc != b.pc or a.insn != b.insn:
            return CompareResult(False, i, (
                f"MISMATCH @ instr #{i}: control flow differs\n"
                f"  RTL  : pc=0x{a.pc:x} insn=0x{a.insn:08x}\n"
                f"  SPIKE: pc=0x{b.pc:x} insn=0x{b.insn:08x}"))
        if a.value is None or b.value is None:
            continue
        if a.rd != b.rd or a.value != b.value:
            ra = f"x{a.rd}=0x{a.value:016x}" if a.rd is not None else "no write"
            rb = f"x{b.rd}=0x{b.value:016x}" if b.rd is not None else "no write"
            return CompareResult(False, i, (
                f"MISMATCH @ pc 0x{a.pc:x} (insn 0x{a.insn:08x}, instr #{i}): "
                f"RTL {ra}  SPIKE {rb}"))
        if (a.rd is None) != (b.rd is None):
            return CompareResult(False, i, (
                f"MISMATCH @ pc 0x{a.pc:x} (insn 0x{a.insn:08x}): register write "
                f"present in {'RTL' if a.rd is not None else 'SPIKE'} only"))
    if len(rtl) != len(ref):
        return CompareResult(False, n, (
            f"MISMATCH: trace length differs (RTL {len(rtl)} vs SPIKE {len(ref)} "
            f"committed instructions); first {n} match"))
    return CompareResult(True, n, f"{n} committed instructions match")


def tmpdir(prefix: str) -> Path:
    return Path(tempfile.mkdtemp(prefix=prefix))


def rmtree(p: Path) -> None:
    shutil.rmtree(p, ignore_errors=True)
