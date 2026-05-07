#!/bin/bash
# Build the AFL++ fuzzing harness for seshat.
# Run from the seshat source directory (where sample.cc / Makefile live).
# Requires: AFL++ (afl-clang-fast++ on PATH), libxerces-c-dev, boost headers.

set -e

# Always run from the directory containing this script so relative
# source paths (sample.cc, stroke.cc, rnnlib4seshat/*.cpp, etc.) resolve correctly.
cd "$(dirname "$0")"

HARNESS="./fuzz_seshat.cc"

# If Boost headers are not in the default include path, set BOOST_INC:
#   BOOST_INC="-I/path/to/boost" ./build_fuzz.sh
BOOST_INC="${BOOST_INC:-}"

AFL_USE_ASAN=1 afl-clang-fast++ -g -O1 \
    -DBOOST_TIMER_ENABLE_DEPRECATED \
    -fpermissive \
    sample.cc \
    stroke.cc \
    grammar.cc \
    production.cc \
    gparser.cc \
    meparser.cc \
    symrec.cc \
    duration.cc \
    segmentation.cc \
    sparel.cc \
    gmm.cc \
    tablecyk.cc \
    cellcyk.cc \
    hypothesis.cc \
    logspace.cc \
    symfeatures.cc \
    featureson.cc \
    online.cc \
    rnnlib4seshat/Random.cpp \
    rnnlib4seshat/DataExporter.cpp \
    rnnlib4seshat/WeightContainer.cpp \
    rnnlib4seshat/ClassificationLayer.cpp \
    rnnlib4seshat/Layer.cpp \
    rnnlib4seshat/Mdrnn.cpp \
    rnnlib4seshat/Optimiser.cpp \
    "$HARNESS" \
    -I. ${BOOST_INC} \
    -lxerces-c -lm \
    -o fuzz_seshat

echo "Built: ./fuzz_seshat"
echo ""
echo "Run:"
echo "  mkdir -p seeds findings"
echo "  cp SampleMathExps/exp.scgink seeds/"
echo "  afl-fuzz -i seeds/ -o findings/ -- ./fuzz_seshat"
echo ""
echo "Parallel (N cores):"
echo "  afl-fuzz -i seeds/ -o findings/ -M main   -- ./fuzz_seshat &"
echo "  afl-fuzz -i seeds/ -o findings/ -S slave1 -- ./fuzz_seshat &"
