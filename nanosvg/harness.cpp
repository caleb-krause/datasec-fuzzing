// Needed by AFL
#include <unistd.h>  

// Needed by nanosvg
#include <stdio.h>
#include <string.h>
#include <math.h>
#define NANOSVG_ALL_COLOR_KEYWORDS	// Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION	// Expands implementation
#include "nanosvg.h"

/* To ensure checks are not optimized out it is recommended to disable
   code optimization for the fuzzer harness main() */
#pragma clang optimize off
#pragma GCC optimize("O0")

__AFL_FUZZ_INIT();

int main(int argc, char* argv[])
{

    #ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
    #endif

    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000))
    {
        // Get input
        int len = __AFL_FUZZ_TESTCASE_LEN;
        if (len < sizeof(float) + 2) continue; // Reject too small inputs

        // Grab the float
        float dpi;
        memcpy(&dpi, buf, sizeof(float));

        // Grab the units
        char unit[3];
        unit[0] = (char) buf[sizeof(float)];
        unit[1] = (char) buf[sizeof(float)+1];
        unit[2] = '\0';

        // Grab the SVG
        int svg_len = len - sizeof(float) - 2;
        char* svg = (char*) malloc(svg_len + 1);
        if (!svg) continue;
        memcpy(svg, buf + sizeof(float) + 2, svg_len);
        svg[svg_len] = '\0';

        // Call the library
        struct NSVGimage* image = nsvgParse(svg, unit, dpi);
    
        // Clean up
        if (image) nsvgDelete(image);
        free(svg);

        // Check for any memory leaks
        // Uncomment if using export AFL_USE_LSAN=1
        // __AFL_LEAK_CHECK();
    }
}