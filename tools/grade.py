#!/usr/bin/env python3
"""SiliconSprint 2026 grader - the exact script used at Gate 1 and Gate 2.

  tools/grade.py --gate 1                 # Stage 1 screening (public tests)
  tools/grade.py --gate 2                 # Stage 2 screening (public tests)
  tools/grade.py --gate 2 --no-build -j 8 --seeds 50

Organizers additionally pass their private material:
  tools/grade.py --gate 2 --hidden /path/hidden --spike-ext /path/full/libvsimd.so \
                 --seeds 100 --seed-base 9000

Results: grade_out/report.md, grade_out/report.json, per-test logs in
grade_out/runs/. Exit status 0 = all REQUIRED criteria of the gate met.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import simlib  # noqa: E402
from trace_compare import compare_elf  # noqa: E402

REPO = simlib.REPO
TESTS = REPO / "tests"
MANDATORY = ["vadd", "vsub", "vand", "vor", "vxor"]
# rv64ui tests that the UNMODIFIED CVA6 baseline does not pass in this
# testharness (misaligned data access is not supported in hardware).
BASELINE_EXCLUDE = {"rv64ui-p-ma_data"}


# ---------------------------------------------------------------------------
def sh(cmd, log: Path, cwd=None, env=None) -> int:
    log.parent.mkdir(parents=True, exist_ok=True)
    with open(log, "w") as f:
        f.write("$ " + " ".join(map(str, cmd)) + "\n")
        f.flush()
        p = subprocess.run(cmd, cwd=cwd, stdout=f, stderr=subprocess.STDOUT, env=env)
    return p.returncode


def riscv_tests(suites) -> list[Path]:
    d = Path(os.environ.get("RISCV_TESTS_DIR", "/opt/riscv-tests/isa"))
    out = []
    for s in suites:
        out += sorted(p for p in d.glob(f"{s}-p-*") if p.suffix == "" and p.name not in BASELINE_EXCLUDE)
    return out


class Grader:
    def __init__(self, args):
        self.a = args
        self.out = Path(args.out).resolve()
        self.runs = self.out / "runs"
        self.results: dict[str, dict] = {}
        self.details: list[str] = []

    # ----------------------------------------------------------------- build
    def build_rtl(self) -> bool:
        if self.a.no_build and simlib.rtl_binary().exists():
            return True
        env = dict(os.environ)
        rc = sh(["make", "-C", str(simlib.CVA6_DIR), "verilate",
                 f"target={self.a.target}", f"NUM_JOBS={self.a.jobs}"],
                self.out / "build_rtl.log", env=env)
        return rc == 0 and simlib.rtl_binary().exists()

    def build_spike_ext(self) -> Path | None:
        if self.a.spike_ext:
            return Path(self.a.spike_ext).resolve()
        rc = sh(["make", "-C", str(REPO / "spike-ext")], self.out / "build_spike_ext.log")
        return simlib.DEFAULT_SPIKE_EXT if rc == 0 else None

    def build_tests(self, extra_dirs: list[Path]) -> bool:
        rc = sh(["make", "-C", str(TESTS), f"-j{self.a.jobs}", "-k",
                 "EXTRA_DIRS=" + " ".join(str(d) for d in extra_dirs)],
                self.out / "build_tests.log")
        return rc == 0

    def gen_random(self, stage2: bool) -> Path:
        d = self.out / "random"
        for f in d.glob("*.S"):
            f.unlink()
        lo = self.a.seed_base
        hi = lo + self.a.seeds - 1
        cmd = [sys.executable, str(REPO / "tools" / "rand_gen.py"), "--seeds", f"{lo}-{hi}",
               "--outdir", str(d), "--ops", self.a.random_ops, "-n", str(self.a.random_len)]
        if stage2:
            cmd.append("--stage2")
        sh(cmd, self.out / "gen_random.log")
        return d

    # ------------------------------------------------------------------- run
    def run_selfcheck(self, elf: Path, tag: str):
        r = simlib.run_rtl(elf, self.runs / tag / elf.stem, timeout=self.a.timeout)
        msg = r.log.strip().splitlines()[-1] if r.log.strip() else ""
        fails = [l for l in r.log.splitlines() if l.startswith("[FAIL]") or l.startswith("[TRAP]")]
        if fails:
            msg = fails[0]
        return elf, r.status, msg

    def run_compare(self, elf: Path, tag: str, require_selfcheck: bool):
        st, rtl, ref, msg = compare_elf(elf, self.spike_ext, self.runs / tag / elf.stem,
                                        self.a.timeout)
        if st == "PASS" and require_selfcheck and rtl.status != "PASS":
            st = "FAIL"
        return elf, st, msg

    def run_many(self, fn, elfs, tag, *extra):
        with ThreadPoolExecutor(max_workers=self.a.jobs) as ex:
            return list(ex.map(lambda e: fn(e, tag, *extra), elfs))

    def record(self, key, title, results, required=None, score=None, note=""):
        passed = sum(1 for _, s, _ in results if s == "PASS")
        hangs = sum(1 for _, s, _ in results if s == "HANG")
        total = len(results)
        entry = {"title": title, "passed": passed, "total": total, "hangs": hangs,
                 "required": required, "note": note,
                 "tests": {e.name: {"status": s, "message": m} for e, s, m in results}}
        if score is not None and total:
            entry["score"] = round(score * passed / total, 2)
            entry["max_score"] = score
        self.results[key] = entry
        for e, s, m in results:
            if s != "PASS":
                self.details.append(f"{key} {e.name}: {s}  {m}")
        return entry

    # ----------------------------------------------------------------- gates
    def gate1(self):
        self.results["G1.0"] = {"title": "Build", "ok": self.built, "required": True}
        if not self.built:
            return
        d = TESTS / "build" / "directed"
        res = self.run_many(self.run_selfcheck, [d / f"{n}.elf" for n in MANDATORY], "g1_directed")
        by = {e.stem: s for e, s, _ in res}
        self.record("G1.1", "VADD end to end", [r for r in res if r[0].stem == "vadd"], required=True)
        e = self.record("G1.2", "Mandatory set (>= 4 of 5)", res, required=True)
        e["ok"] = by.get("vadd") == "PASS" and e["passed"] >= 4
        h = self.run_many(self.run_compare, [TESTS / "build" / "hazard" / "raw_chain.elf"],
                          "g1_hazard", True)
        self.record("G1.3", "Dependency chain (self-check + Spike)", h, required=True)
        smoke = riscv_tests(["rv64ui"])
        self.record("G1.4", "Scalar smoke (rv64ui)", self.run_many(self.run_selfcheck, smoke, "g1_smoke"),
                    required=True)
        self.results["G1.5"] = {"title": "Viva", "ok": None, "required": True,
                                "note": "MANUAL - rated by a mentor"}

    def gate2(self):
        self.results["G2.0"] = {"title": "Build", "ok": self.built, "required": True}
        if not self.built:
            return
        b = TESTS / "build"
        directed = sorted((b / "directed").glob("*.elf")) + \
            sorted((b / "extra").glob("directed__*.elf"))
        r = self.run_many(self.run_selfcheck, directed, "g2_directed")
        e = self.record("G2.1", "Directed (all SEW)", r, required=True, score=6)
        e["ok"] = e["total"] > 0 and e["passed"] / e["total"] >= 0.9 and e["hangs"] == 0
        hazards = sorted((b / "hazard").glob("*.elf")) + sorted((b / "extra").glob("hazard__*.elf"))
        self.record("G2.2", "Hazards (self-check + Spike)",
                    self.run_many(self.run_compare, hazards, "g2_hazard", True), score=5)
        rnd = sorted((b / "extra").glob("random__*.elf"))
        self.record("G2.3", f"Random vs Spike ({len(rnd)} seeds)",
                    self.run_many(self.run_compare, rnd, "g2_random", False), score=4)
        reg = riscv_tests(["rv64ui", "rv64um", "rv64ua"])
        self.record("G2.4", "Scalar regression (rv64ui/um/ua)",
                    self.run_many(self.run_selfcheck, reg, "g2_regress"), required=True)
        stretch = sorted((b / "extra").glob("stretch__*.elf"))
        if stretch:
            self.record("G2.5", "Stretch (hidden)",
                        self.run_many(self.run_compare, stretch, "g2_stretch", True))

    # ---------------------------------------------------------------- report
    def finish(self, elapsed):
        lines = [f"# SiliconSprint 2026 - Gate {self.a.gate} report", "",
                 f"Elapsed: {elapsed:.0f} s", "",
                 "| Check | Content | Result | Required | Score |",
                 "| --- | --- | --- | --- | --- |"]
        all_required_ok = True
        for k, v in self.results.items():
            if "passed" in v:
                ok = v.get("ok", v["passed"] == v["total"] and v["total"] > 0)
                v["ok"] = ok
                res = f"{v['passed']}/{v['total']}" + (f" ({v['hangs']} HANG)" if v["hangs"] else "")
            else:
                ok = v.get("ok")
                res = "PASS" if ok else ("MANUAL" if ok is None else "FAIL")
            if v.get("required") and ok is False:
                all_required_ok = False
            score = f"{v['score']}/{v['max_score']}" if "score" in v else ""
            lines.append(f"| {k} | {v['title']} | {res} | {'yes' if v.get('required') else ''} | {score} |")
        lines += ["", f"**Required criteria met: {'YES' if all_required_ok else 'NO'}**", ""]
        if self.details:
            lines += ["## Failures", "", "```"] + self.details[:200] + ["```"]
        report = "\n".join(lines) + "\n"
        (self.out / "report.md").write_text(report)
        (self.out / "report.json").write_text(json.dumps(
            {"gate": self.a.gate, "required_ok": all_required_ok, "results": self.results}, indent=2))
        print(report)
        print(f"Logs: {self.out}")
        return all_required_ok

    def run(self):
        t0 = time.time()
        self.out.mkdir(parents=True, exist_ok=True)
        print(f"[grade] building RTL ({self.a.target}) ...", flush=True)
        self.built = self.build_rtl()
        self.spike_ext = self.build_spike_ext()
        extra = []
        hidden = Path(self.a.hidden).resolve() if self.a.hidden else None
        if hidden:
            extra += [hidden / s for s in ("directed", "hazard", "stretch") if (hidden / s).is_dir()]
        if self.a.gate == 2:
            extra.append(self.gen_random(stage2=not self.a.random_stage1))
        print("[grade] building tests ...", flush=True)
        self.build_tests(extra)
        print("[grade] running ...", flush=True)
        self.gate1() if self.a.gate == 1 else self.gate2()
        return self.finish(time.time() - t0)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", type=int, choices=[1, 2], required=True)
    ap.add_argument("--target", default=simlib.DEFAULT_TARGET)
    ap.add_argument("--no-build", action="store_true", help="reuse an existing RTL build")
    ap.add_argument("-j", "--jobs", type=int, default=int(os.environ.get("NUM_JOBS", "8")))
    ap.add_argument("--timeout", type=float, default=300.0, help="per-test wall clock (HANG)")
    ap.add_argument("--spike-ext", help="Spike model .so (default: spike-ext/build/libvsimd.so)")
    ap.add_argument("--hidden", help="organizer hidden-test root (directed/ hazard/ stretch/)")
    ap.add_argument("--seeds", type=int, default=50)
    ap.add_argument("--seed-base", type=int, default=1)
    ap.add_argument("--random-ops", default="vadd,vsub,vand,vor,vxor")
    ap.add_argument("--random-len", type=int, default=300)
    ap.add_argument("--random-stage1", action="store_true",
                    help="random tests without vcfg/vstat (Stage 1 designs)")
    ap.add_argument("--out", default=str(REPO / "grade_out"))
    args = ap.parse_args()
    ok = Grader(args).run()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
