# Compiler
export AFL_CC_COMPILER=LTO
# export AFL_CC_COMPILER=LLVM

# Sanitizers
export AFL_USE_ASAN=1
export ASAN_OPTIONS=allocator_may_return_null=1 
# export AFL_USE_MSAN=1
# export AFL_USE_UBSAN=1
# export AFL_USE_CFISAN=1
# export AFL_USE_TSAN=1
# export AFL_USE_LSAN=1
# export AFL_HARDEN=1

# Deny list. We don't need to instrument functions that aren't called
# export AFL_LLVM_DENYLIST=denylist.txt

afl-c++ -O0 -g harness.cpp -o harness
