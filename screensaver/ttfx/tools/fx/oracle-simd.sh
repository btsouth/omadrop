#!/usr/bin/env bash
# oracle-simd.sh - run tools/fx/oracle.sh for every effect with each of fx's
# SIMD kernel choices: the widest the CPU runs, TTFX_NO_AVX512=1 and
# TTFX_NO_AVX2=1 (the SSE2 and scalar paths), at most JOBS (default 4)
# oracles at a time. Pass THREADS=1 to run fx single-threaded.
#
# Usage: tools/fx/oracle-simd.sh [quick|full]
set -u

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
MODE="${1:-quick}"
JOBS="${JOBS:-4}"
LOGS="$(mktemp -d "${ORACLE_TMP:-$ROOT/target/oracle-tmp}/simd.XXXXXX")"
trap 'rm -rf "$LOGS"' EXIT
effects=$(ls "$ROOT"/src/fx/effects/*.rs | xargs -n1 basename | sed 's/\.rs$//' | grep -vx mod)

status=0
for kernel in widest no-avx512 no-avx2; do
    case "$kernel" in
    widest) env=() ;;
    no-avx512) env=(TTFX_NO_AVX512=1) ;;
    no-avx2) env=(TTFX_NO_AVX2=1) ;;
    esac
    [ "${THREADS:-}" = 1 ] && env+=(TTFX_THREADS=1)
    for e in $effects; do echo "$e"; done |
        xargs -P "$JOBS" -I{} env "${env[@]}" "$ROOT/tools/fx/oracle.sh" {} "$MODE" > "$LOGS/$kernel.log" 2>&1
    total=$(grep -c '^oracle ' "$LOGS/$kernel.log")
    bad=$(grep '^oracle ' "$LOGS/$kernel.log" | grep -vc ' 0 failed')
    grep '^FAIL' "$LOGS/$kernel.log" | head -20
    echo "$kernel: $((total - bad))/$total effects pass ($MODE)"
    [ "$bad" -eq 0 ] && [ "$total" -gt 0 ] || status=1
done
exit $status
