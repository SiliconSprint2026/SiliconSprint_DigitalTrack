#!/usr/bin/env bash
# Open a shell in the frozen SiliconSprint environment with this repo at /work.
#   scripts/docker_shell.sh               # interactive shell
#   scripts/docker_shell.sh make -C tests # run one command
# The image is found as ghcr.io/siliconsprint2026/cva6-simd:2026 (docker pull) or
# siliconsprint/cva6-simd:2026 (docker load of the .tar / local build).
# Override with IMAGE=<name>.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [ -z "${IMAGE:-}" ]; then
  for cand in ghcr.io/siliconsprint2026/cva6-simd:2026 siliconsprint/cva6-simd:2026; do
    if docker image inspect "$cand" >/dev/null 2>&1; then IMAGE=$cand; break; fi
  done
fi
if [ -z "${IMAGE:-}" ]; then
  echo "cva6-simd image not found. Get it with:" >&2
  echo "  docker pull ghcr.io/siliconsprint2026/cva6-simd:2026" >&2
  echo "(or see docs/1_docker_setup.md for the .tar / build options)" >&2
  exit 1
fi
TTY=""; [ -t 0 ] && TTY="-it"
exec docker run --rm $TTY -v "$ROOT":/work -w /work "$IMAGE" "${@:-bash}"
