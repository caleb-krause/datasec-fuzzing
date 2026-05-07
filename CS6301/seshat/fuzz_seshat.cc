/*
 * AFL++ fuzzing harness for seshat - persistent mode
 *
 * Target: Sample::loadSCGInk() via the Sample(char*) constructor.
 *         The SCGInk format is a plain-text stroke sequence:
 *           SCG_INK
 *           <nstrokes>
 *           <npoints>
 *           <x> <y>
 *           ...
 *
 * The harness always prepends the "SCG_INK\n" magic header to AFL's
 * payload so that loadSCGInk() never hits its early exit() and AFL
 * can stay in persistent mode. AFL then mutates the stroke data.
 *
 * Build (from the seshat source directory):
 *   ./build_fuzz.sh
 *
 * Run:
 *   mkdir -p seeds findings
 *   cp SampleMathExps/exp.scgink seeds/
 *   afl-fuzz -i seeds/ -o findings/ -- ./fuzz_seshat
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "sample.h"

__AFL_FUZZ_INIT();

#define TMP_INPUT "/tmp/.fuzz_seshat_input.scgink"

int main(void) {
    __AFL_INIT();

    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(1000)) {
        size_t size = __AFL_FUZZ_TESTCASE_LEN;
        if (size == 0)
            continue;

        /*
         * Only process inputs that start with the SCG_INK magic header.
         * This avoids the exit() call inside loadSCGInk() on bad magic,
         * which would kill persistent mode. AFL will preserve the header
         * in mutations since removing it causes no new coverage.
         */
        if (size < 8 || memcmp(buf, "SCG_INK\n", 8) != 0)
            continue;

        FILE *f = fopen(TMP_INPUT, "wb");
        if (!f)
            continue;

        fwrite(buf, 1, size, f);
        fclose(f);

        /*
         * Construct Sample — this triggers loadSCGInk() which:
         *   1. Parses nstrokes (fscanf int — negative value → huge Punto[])
         *   2. For each stroke parses npuntos then x/y pairs
         *   3. After loading: computes centroids (cx /= np → div-by-zero if np==0)
         */
        Sample *s = nullptr;
        try {
            s = new Sample((char *)TMP_INPUT);
        } catch (...) {
            continue;
        }

        /* Exercise output path for additional coverage */
        s->print();

        /*
         * compute_strokes_distances() must be called before delete because
         * ~Sample() unconditionally calls delete[] stk_dis[i], but stk_dis
         * is only allocated inside compute_strokes_distances(). Skipping it
         * causes a crash on every destruction.
         * detRefSymbol() sets RX/RY which are required as arguments.
         */
        s->detRefSymbol();
        s->compute_strokes_distances(s->RX, s->RY);

        delete s;
    }

    unlink(TMP_INPUT);
    return 0;
}
