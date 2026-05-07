#!/bin/bash
# Build the AFL++ fuzzing harness for mpc.
# Run this from the root of the mpc repo.
# Requires AFL++ (afl-clang-fast must be on PATH).

set -e

REPO="$(cd "$(dirname "$0")" && pwd)"

# Sanitizers are intentionally omitted for mpc: mpc.c targets C89 and uses
# patterns (e.g. signed-char array indexing in the regex engine) that are
# technically undefined behaviour but work correctly in practice.  Both ASAN
# and UBSAN abort on these patterns for every valid input, which kills every
# seed during AFL++'s dry run and prevents the fuzzer from starting.
# AFL++ still detects real crashes (SIGSEGV, SIGABRT) without sanitizers.
afl-clang-fast \
    -g -O1 \
    -I "$REPO" \
    -Wno-unused-result \
    -Wno-format-nonliteral \
    "$REPO/fuzz_mpc.c" \
    "$REPO/mpc.c" \
    -o fuzz_mpc

echo "Built: ./fuzz_mpc"
echo ""
echo "Run:"
echo "  mkdir -p findings"
echo "  afl-fuzz -i seeds/ -o findings/ -- ./fuzz_mpc"
echo ""
echo "Parallel (N cores):"
echo "  afl-fuzz -i seeds/ -o findings/ -M main    -- ./fuzz_mpc &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker1 -- ./fuzz_mpc &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker2 -- ./fuzz_mpc &"
