/*
 * AFL++ fuzzing harness for rapidcsv - persistent mode
 *
 * Target: rapidcsv::Document stream constructor → ParseCsv() state machine.
 *
 * The first byte of each AFL input selects a parsing configuration so that
 * a single binary exercises multiple code paths:
 *   bits [1:0]  separator  (00=comma, 01=semicolon, 10=tab, 11=pipe)
 *   bit  [2]    trim whitespace
 *   bit  [3]    quoted linebreaks
 *   bit  [4]    skip comment lines
 *   bit  [5]    skip empty lines
 *   bit  [6]    no column headers (LabelParams(-1,-1))
 *   bit  [7]    strict converter (throw on bad numeric instead of default)
 *
 * The rest of the input is treated as raw CSV text.
 *
 * Build (inside Docker, from the rapidcsv repo root):
 *   ./build_fuzz.sh
 *
 * Run:
 *   mkdir -p findings
 *   afl-fuzz -i seeds/ -o findings/ -- ./fuzz_rapidcsv
 */

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "src/rapidcsv.h"

__AFL_FUZZ_INIT();

/* Accessors to call after a successful Document parse.
 * Bounded to avoid spending many cycles on huge synthetic tables. */
static void exercise_doc(rapidcsv::Document& doc)
{
    const size_t nrows = doc.GetRowCount();
    const size_t ncols = doc.GetColumnCount();

    const size_t max_rows = (nrows < 20) ? nrows : 20;
    const size_t max_cols = (ncols < 20) ? ncols : 20;

    /* Cell-level access: string, double, long long */
    for (size_t r = 0; r < max_rows; ++r) {
        for (size_t c = 0; c < max_cols; ++c) {
            (void)doc.GetCell<std::string>(c, r);
            (void)doc.GetCell<double>(c, r);
            (void)doc.GetCell<long long>(c, r);
        }
    }

    /* Full-row and full-column access */
    for (size_t r = 0; r < max_rows; ++r)
        (void)doc.GetRow<std::string>(r);

    for (size_t c = 0; c < max_cols; ++c)
        (void)doc.GetColumn<std::string>(c);
}

int main(void)
{
    __AFL_INIT();

    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        const size_t size = __AFL_FUZZ_TESTCASE_LEN;

        /* Need at least the config byte and one character of CSV data. */
        if (size < 2)
            continue;

        const uint8_t cfg    = buf[0];
        const char   *data   = reinterpret_cast<const char *>(buf + 1);
        const size_t  dsize  = size - 1;

        /* Decode config byte */
        const char sep_chars[4] = { ',', ';', '\t', '|' };
        const char   sep             = sep_chars[cfg & 0x03];
        const bool   trim            = (cfg & 0x04) != 0;
        const bool   quoted_lf       = (cfg & 0x08) != 0;
        const bool   skip_comments   = (cfg & 0x10) != 0;
        const bool   skip_empty      = (cfg & 0x20) != 0;
        const int    col_name_idx    = (cfg & 0x40) ? -1 : 0;
        const bool   has_default_cv  = (cfg & 0x80) != 0; /* true = no throw on bad numeric */

        try {
            std::string csv_str(data, dsize);
            std::istringstream ss(csv_str);

            rapidcsv::Document doc(
                ss,
                rapidcsv::LabelParams(col_name_idx, -1),
                rapidcsv::SeparatorParams(sep, trim,
                                          rapidcsv::sPlatformHasCR,
                                          quoted_lf),
                rapidcsv::ConverterParams(has_default_cv),
                rapidcsv::LineReaderParams(skip_comments, '#', skip_empty)
            );

            exercise_doc(doc);
        }
        catch (const std::exception&) {
            /* Expected for malformed input — not a crash. */
        }
        catch (...) {
            /* Catch non-standard exceptions too. */
        }
    }

    return 0;
}
