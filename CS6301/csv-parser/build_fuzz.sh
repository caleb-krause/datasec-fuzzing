#!/bin/bash
# Build the AFL++ fuzzing harness for csv-parser.
# Run this from the root of the csv-parser repo.
# Requires AFL++ (afl-clang-fast must be on PATH).

set -e

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
HARNESS="$REPO_ROOT/fuzz_csvparser.cpp"
INTERNAL="$REPO_ROOT/include/internal"

# -DCSV_ENABLE_THREADS=0  disables background worker threads so each
#                          CSVReader iteration doesn't pay thread create/join
#                          overhead — much faster in persistent mode.
# AFL_USE_ASAN=1           AddressSanitizer: heap/stack overflows, UAF
# AFL_USE_UBSAN=1          UndefinedBehaviorSanitizer: integer UB, bad casts
AFL_USE_ASAN=1 AFL_USE_UBSAN=1 afl-clang-fast++ \
    -std=c++17 \
    -g -O1 \
    -DCSV_ENABLE_THREADS=0 \
    -I "$REPO_ROOT/include" \
    "$HARNESS" \
    "$INTERNAL/basic_csv_parser.cpp" \
    "$INTERNAL/col_names.cpp" \
    "$INTERNAL/csv_format.cpp" \
    "$INTERNAL/csv_reader.cpp" \
    "$INTERNAL/csv_reader_iterator.cpp" \
    "$INTERNAL/csv_row.cpp" \
    "$INTERNAL/csv_row_json.cpp" \
    "$INTERNAL/csv_stat.cpp" \
    "$INTERNAL/csv_utility.cpp" \
    "$INTERNAL/raw_csv_data.cpp" \
    -o fuzz_csvparser

echo "Built: ./fuzz_csvparser"
echo ""
echo "Run:"
echo "  mkdir -p findings"
echo "  afl-fuzz -i seeds/ -o findings/ -- ./fuzz_csvparser"
echo ""
echo "Parallel (N cores):"
echo "  afl-fuzz -i seeds/ -o findings/ -M main    -- ./fuzz_csvparser &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker1 -- ./fuzz_csvparser &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker2 -- ./fuzz_csvparser &"
