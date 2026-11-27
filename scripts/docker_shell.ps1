# Open a shell in the frozen SiliconSprint environment (Windows PowerShell).
#   .\scripts\docker_shell.ps1                 # interactive shell
#   .\scripts\docker_shell.ps1 make -C tests   # run one command
# Finds ghcr.io/siliconsprint2026/cva6-simd:2026 (docker pull) or
# siliconsprint/cva6-simd:2026 (.tar / local build). Override with $env:IMAGE.
$Root = (Resolve-Path "$PSScriptRoot\..").Path
$Image = $env:IMAGE
if (-not $Image) {
  foreach ($cand in @("ghcr.io/siliconsprint2026/cva6-simd:2026", "siliconsprint/cva6-simd:2026")) {
    docker image inspect $cand *> $null
    if ($LASTEXITCODE -eq 0) { $Image = $cand; break }
  }
}
if (-not $Image) {
  Write-Error "cva6-simd image not found. Run: docker pull ghcr.io/siliconsprint2026/cva6-simd:2026 (see docs\1_docker_setup.md)"
  exit 1
}
if ($args.Count -eq 0) {
  docker run --rm -it -v "${Root}:/work" -w /work $Image bash
} else {
  docker run --rm -v "${Root}:/work" -w /work $Image @args
}
