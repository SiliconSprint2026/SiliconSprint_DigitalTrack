#!/usr/bin/env bash
# Build the CVA6 Verilator model (frozen configuration) -> cva6/work-ver/Variane_testharness
#   scripts/build_rtl.sh            # default build
#   TRACE=1 scripts/build_rtl.sh    # build with FST waveform support (slower)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
JOBS=${NUM_JOBS:-$(nproc)}
EXTRA=()
[ "${TRACE:-0}" = "1" ] && EXTRA+=(TRACE_COMPACT=1)
time make -C "$ROOT/cva6" verilate target=cv64a6_imafdc_sv39 NUM_JOBS="$JOBS" "${EXTRA[@]}"
echo "Built: $ROOT/cva6/work-ver/Variane_testharness"
