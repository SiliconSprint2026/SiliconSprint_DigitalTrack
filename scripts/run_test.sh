#!/usr/bin/env bash
# Run one test ELF on the RTL, on Spike, or compare both.
#   scripts/run_test.sh rtl   tests/build/directed/vadd.elf
#   scripts/run_test.sh spike tests/build/directed/vadd.elf
#   scripts/run_test.sh cmp   tests/build/hazard/raw_chain.elf
#   scripts/run_test.sh wave  tests/build/directed/vadd.elf   # needs TRACE=1 build
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODE=${1:?mode: rtl|spike|cmp|wave}
ELF=$(realpath "${2:?path to ELF}")
EXT=${SPIKE_EXT:-$ROOT/spike-ext/build/libvsimd.so}
OUT=${OUT:-$ROOT/run_out/$(basename "$ELF" .elf)}
mkdir -p "$OUT"
case "$MODE" in
  rtl)
    cd "$OUT" && "$ROOT/cva6/work-ver/Variane_testharness" +time_out=${MAX_CYCLES:-20000000} "$ELF"
    echo "Commit trace: $OUT/trace_rvfi_hart_00.dasm" ;;
  wave)
    cd "$OUT" && "$ROOT/cva6/work-ver/Variane_testharness" +time_out=${MAX_CYCLES:-20000000} -f "$OUT/wave.fst" "$ELF"
    echo "Waveform: $OUT/wave.fst (open with gtkwave)" ;;
  spike)
    [ -f "$EXT" ] || make -C "$ROOT/spike-ext"
    spike --isa=rv64gc --extlib="$EXT" --extension=vsimd --log-commits \
          --log="$OUT/spike_commits.log" "$ELF"
    echo "Commit log: $OUT/spike_commits.log" ;;
  cmp)
    [ -f "$EXT" ] || make -C "$ROOT/spike-ext"
    python3 "$ROOT/tools/trace_compare.py" --spike-ext "$EXT" --keep "$OUT" "$ELF" ;;
  *) echo "unknown mode $MODE" >&2; exit 2 ;;
esac
