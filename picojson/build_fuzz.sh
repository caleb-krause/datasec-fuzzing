#!/bin/bash
# Build the AFL++ fuzzing harness for picojson.
# Run this from the root of the picojson repo.
# Requires AFL++ (afl-clang-fast must be on PATH).

set -e

HARNESS="$(dirname "$0")/fuzz_picojson.cpp"

# -DPICOJSON_USE_INT64  enables int64_t parsing path (covered by harness)
# AFL_USE_ASAN=1        AddressSanitizer — catches OOB, UAF, heap corruption
# AFL_USE_UBSAN=1       UndefinedBehaviorSanitizer — catches integer UB, bad casts
AFL_USE_ASAN=1 AFL_USE_UBSAN=1 afl-clang-fast++ \
    -std=c++11 \
    -g -O1 \
    -DPICOJSON_USE_INT64 \
    -I . \
    "$HARNESS" \
    -o fuzz_picojson

echo "Built: ./fuzz_picojson"
echo ""
echo "Run:"
echo "  mkdir -p findings"
echo "  afl-fuzz -i seeds/ -o findings/ -- ./fuzz_picojson"
echo ""
echo "Parallel (N cores):"
echo "  afl-fuzz -i seeds/ -o findings/ -M main    -- ./fuzz_picojson &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker1 -- ./fuzz_picojson &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker2 -- ./fuzz_picojson &"
