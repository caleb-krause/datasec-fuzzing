#!/bin/bash
# Build the AFL++ fuzzing harness for sql-parser (hsql).
# Run this from the root of the sql-parser repo.
# Requires AFL++ (afl-clang-fast must be on PATH).
#
# The Flex/Bison-generated files (bison_parser.cpp, flex_lexer.cpp) are
# pre-generated and committed to the repo, so no flex/bison needed at build time.

set -e

REPO="$(cd "$(dirname "$0")" && pwd)"
SRC="$REPO/src"
PARSER="$SRC/parser"

# AFL_USE_ASAN=1   AddressSanitizer — catches OOB, UAF, heap corruption
# AFL_USE_UBSAN=1  UndefinedBehaviorSanitizer — integer UB, bad pointer casts
#
# -Wno-* flags silence warnings in the generated Bison/Flex files that would
# otherwise cause -Werror to abort the build.
AFL_USE_ASAN=1 AFL_USE_UBSAN=1 afl-clang-fast++ \
    -std=c++17 \
    -g -O1 \
    -I "$REPO" \
    -Wno-register \
    -Wno-unused-function \
    -Wno-unused-variable \
    -Wno-sign-compare \
    "$REPO/fuzz_sqlparser.cpp" \
    "$SRC/SQLParser.cpp" \
    "$SRC/SQLParserResult.cpp" \
    "$SRC/sql/CreateStatement.cpp" \
    "$SRC/sql/Expr.cpp" \
    "$SRC/sql/PrepareStatement.cpp" \
    "$SRC/sql/SQLStatement.cpp" \
    "$SRC/sql/statements.cpp" \
    "$SRC/util/sqlhelper.cpp" \
    "$PARSER/bison_parser.cpp" \
    "$PARSER/flex_lexer.cpp" \
    -o fuzz_sqlparser

echo "Built: ./fuzz_sqlparser"
echo ""
echo "Run:"
echo "  mkdir -p findings"
echo "  afl-fuzz -i seeds/ -o findings/ -- ./fuzz_sqlparser"
echo ""
echo "Parallel (N cores):"
echo "  afl-fuzz -i seeds/ -o findings/ -M main    -- ./fuzz_sqlparser &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker1 -- ./fuzz_sqlparser &"
echo "  afl-fuzz -i seeds/ -o findings/ -S worker2 -- ./fuzz_sqlparser &"
