#!/bin/bash
# Build the AFL++ fuzzing harness for rapidcsv.
# Run this from the root of the rapidcsv repo.
# Requires AFL++ (afl-clang-fast must be on PATH).

set -e

HARNESS="$(dirname "$0")/fuzz_rapidcsv.cpp"

# AFL_USE_ASAN=1  — AddressSanitizer (catches heap/stack overflows, UAF)
# AFL_USE_UBSAN=1 — UndefinedBehaviorSanitizer (catches integer UB, bad casts)
# -std=c++17      — rapidcsv uses C++17 features (std::optional etc.)
# -I .            — find src/rapidcsv.h relative to repo root
AFL_USE_ASAN=1 AFL_USE_UBSAN=1 afl-clang-fast++ \
    -std=c++17 \
    -g -O1 \
    -I . \
    "$HARNESS" \
    -o fuzz_rapidcsv

echo "Built: ./fuzz_rapidcsv"
echo ""
echo "Seed the corpus:"
echo "  mkdir -p seeds"
echo "  cp tests/msft.csv seeds/msft.csv"
echo "  # (additional hand-crafted seeds are already in seeds/)"
echo ""
echo "Run:"
echo "  mkdir -p findings"
echo "  afl-fuzz -i seeds/ -o findings/ -- ./fuzz_rapidcsv"
echo ""
echo "Parallel (N cores):"
echo "  afl-fuzz -i seeds/ -o findings/ -M main    -- ./fuzz_rapidcsv &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker1 -- ./fuzz_rapidcsv &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker2 -- ./fuzz_rapidcsv &"
