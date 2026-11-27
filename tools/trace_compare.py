#!/usr/bin/env python3
"""Run one ELF on the CVA6 RTL and on Spike (with the vsimd extension) and
compare their commit traces instruction by instruction.

  tools/trace_compare.py tests/build/hazard/raw_chain.elf
  tools/trace_compare.py --spike-ext my/libvsimd.so --keep out/ <elf>

Exit status 0 = traces match and both runs passed.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import simlib  # noqa: E402


def compare_elf(elf: Path, spike_ext: Path, workdir: Path, timeout: float):
    rtl = simlib.run_rtl(elf, workdir / "rtl", timeout=timeout)
    ref = simlib.run_spike(elf, workdir / "spike", ext=spike_ext)
    if rtl.status == "HANG":
        return "HANG", rtl, ref, "RTL did not finish (HANG)"
    if ref.status != "PASS" and ref.exit_code is None:
        return "ERROR", rtl, ref, "Spike failed to run:\n" + ref.log[-2000:]
    a = simlib.filter_trace(simlib.parse_trace(rtl.trace, "rvfi"), elf)
    b = simlib.filter_trace(simlib.parse_trace(ref.trace, "spike"), elf)
    if not b:
        return "ERROR", rtl, ref, "Spike produced no trace:\n" + ref.log[-2000:]
    cmp = simlib.compare_traces(a, b)
    if not cmp.ok:
        return "MISMATCH", rtl, ref, cmp.message
    if rtl.exit_code != ref.exit_code:
        return "MISMATCH", rtl, ref, (f"exit code differs: RTL {rtl.exit_code} "
                                      f"SPIKE {ref.exit_code}")
    status = "PASS" if rtl.status == "PASS" else "FAIL"
    return status, rtl, ref, cmp.message


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("elf", type=Path)
    ap.add_argument("--spike-ext", type=Path, default=simlib.DEFAULT_SPIKE_EXT)
    ap.add_argument("--timeout", type=float, default=300.0)
    ap.add_argument("--keep", type=Path, help="keep logs and traces in this directory")
    args = ap.parse_args()

    work = args.keep or simlib.tmpdir("tc_")
    status, rtl, ref, msg = compare_elf(args.elf, args.spike_ext, work, args.timeout)
    print(f"{status}: {args.elf.name}: {msg}")
    print(f"  RTL  : {rtl.status} exit={rtl.exit_code} cycles={rtl.cycles} trace={rtl.trace}")
    print(f"  SPIKE: {ref.status} exit={ref.exit_code} trace={ref.trace}")
    if not args.keep:
        simlib.rmtree(work)
    sys.exit(0 if status == "PASS" else 1)


if __name__ == "__main__":
    main()
