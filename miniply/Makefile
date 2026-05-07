# Compiler
export AFL_CC_COMPILER=LTO

# Sanitizers
export AFL_USE_ASAN=1

harness: miniply.o ../miniply/miniply.h harness.cpp
	afl-c++ -O0 -g -I ../miniply harness.cpp -o harness miniply.o

miniply.o: ../miniply/miniply.h ../miniply/miniply.cpp
	afl-c++ -g -I ../miniply -c ../miniply/miniply.cpp -o miniply.o

clean:
	rm harness miniply.o