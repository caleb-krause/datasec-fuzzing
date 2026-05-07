/*
 * AFL++ fuzzing harness for iniparser - persistent mode
 *
 * Build (inside Docker, from repo root):
 *   AFL_USE_ASAN=1 afl-clang-fast -g -O1 \
 *     src/iniparser.c src/dictionary.c fuzz_iniparser.c \
 *     -I src/ -o fuzz_iniparser
 *
 * Run:
 *   mkdir -p findings
 *   afl-fuzz -i seeds/ -o findings/ -- ./fuzz_iniparser
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iniparser.h"

/* Required before main() — sets up AFL++ shared-memory test case delivery. */
__AFL_FUZZ_INIT();

static int silent_errback(const char *fmt, ...) {
    (void)fmt;
    return 0;
}

int main(void) {
    /* One-time init: tells AFL++ we support persistent + shmem mode. */
    __AFL_INIT();

    /*
     * __AFL_FUZZ_TESTCASE_BUF / _LEN are provided by AFL++ via shared memory.
     * No stdin reads, no disk I/O — much faster than fork mode.
     */
    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        size_t size = __AFL_FUZZ_TESTCASE_LEN;
        if (size == 0)
            continue;

        FILE *f = fmemopen(buf, size, "r");
        if (f == NULL)
            continue;

        iniparser_set_error_callback(silent_errback);

        dictionary *d = iniparser_load_file(f, "fuzz.ini");
        fclose(f);

        if (d == NULL)
            continue;

        /*
         * Exercise all accessor/conversion paths so AFL++ can reach
         * strtol, strtod, boolean parsing, etc.
         */
        int nsec = iniparser_getnsec(d);
        for (int i = 0; i < nsec; i++) {
            const char *secname = iniparser_getsecname(d, i);
            if (secname == NULL)
                continue;

            int nkeys = iniparser_getsecnkeys(d, secname);
            if (nkeys > 0) {
                const char **keys = calloc((size_t)nkeys, sizeof(char *));
                if (keys) {
                    iniparser_getseckeys(d, secname, keys);
                    for (int k = 0; k < nkeys; k++) {
                        if (keys[k] == NULL)
                            continue;
                        iniparser_getstring(d, keys[k], NULL);
                        iniparser_getint(d, keys[k], -1);
                        iniparser_getlongint(d, keys[k], -1L);
                        iniparser_getint64(d, keys[k], -1);
                        iniparser_getuint64(d, keys[k], 0);
                        iniparser_getdouble(d, keys[k], 0.0);
                        iniparser_getboolean(d, keys[k], -1);
                    }
                    free(keys);
                }
            }
        }

        iniparser_freedict(d);
    }

    return 0;
}
