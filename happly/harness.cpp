#include <cstdint>   // for uint8_t, uint16_t, uint32_t, uint64_t
#include <unistd.h>
#include "happly.h"

/* To ensure checks are not optimized out it is recommended to disable
   code optimization for the fuzzer harness main() */
#pragma clang optimize off
#pragma GCC optimize("O0")

// __AFL_FUZZ_INIT();

int main(int argc, char* argv[])
{

    // if (argc < 2)
    // {
    //     fprintf(stderr, "no file name :(\n");
    //     exit(EXIT_FAILURE);
    // }

    #ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
    #endif

    while (__AFL_LOOP(10000))
    {

        // happly::PLYData plyIn("../seeds/ant.ply", true);

        // happly::PLYData plyIn(argv[1], true);

        try
        {
            happly::PLYData plyIn(argv[1], false);
        }
        catch (std::runtime_error)
        {
            continue;
        }

    }
}