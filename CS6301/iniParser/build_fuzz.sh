#!/bin/bash
# Run this from the root of the iniparser repo (where src/ lives).
# Requires AFL++ (afl-clang-fast must be on PATH).

set -e

HARNESS="$(dirname "$0")/fuzz_iniparser.c"

# AFL_USE_ASAN=1 tells afl-clang-fast to inject ASAN automatically.
# Use afl-clang-lto instead of afl-clang-fast if your LLVM supports it
# (better coverage via link-time instrumentation).
AFL_USE_UBSAN=1 afl-clang-fast -g -O1 \
    src/iniparser.c \
    src/dictionary.c \
    "$HARNESS" \
    -I src/ \
    -o fuzz_iniparser

echo "Built: ./fuzz_iniparser"
echo ""
echo "Run:"
echo "  mkdir -p findings"
echo "  AFL_CRASH_SEED_AS_NEW_CRASHES=1 afl-fuzz -i findings/default/queue/ -o findings_ubsan/ -- ./fuzz_iniparser"
echo ""
echo "Parallel (N cores):"
echo "  AFL_CRASH_SEED_AS_NEW_CRASHES=1 afl-fuzz -i findings/default/queue/ -o findings_ubsan/ -M main   -- ./fuzz_iniparser &"
echo "  AFL_CRASH_SEED_AS_NEW_CRASHES=1 afl-fuzz -i findings/default/queue/ -o findings_ubsan/ -S slave1 -- ./fuzz_iniparser &"
