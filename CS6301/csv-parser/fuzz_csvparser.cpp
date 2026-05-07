/*
 * AFL++ fuzzing harness for csv-parser - persistent mode
 *
 * Target: csv::parse(csv::string_view, CSVFormat) → CSVReader
 *         This exercises the full StreamParser pipeline:
 *           IBasicCSVParser::parse() → field splitting, quote handling,
 *           chunk boundary logic, type detection (data_type.hpp),
 *           and CSVField::get<T>() type coercion.
 *
 * The first byte of each AFL input selects a parsing configuration so a
 * single binary covers multiple code paths:
 *   bits [1:0]  delimiter         (00=comma, 01=semicolon, 10=tab, 11=pipe)
 *   bit  [2]    trim spaces/tabs
 *   bit  [3]    no header row     (header_row(-1))
 *   bit  [4]    disable quoting
 *   bits [6:5]  variable columns  (00=IGNORE, 01=KEEP, 10=KEEP_NON_EMPTY, 11=THROW)
 *   bit  [7]    case-insensitive column name lookup
 *
 * The rest of the input is raw CSV text.
 *
 * Threading note: csv-parser spawns a background worker thread per CSVReader.
 * Compiled with -DCSV_ENABLE_THREADS=0 to remove that overhead in persistent
 * mode and keep per-iteration cost low. Single-threaded parsing still exercises
 * all field parsing, type detection, and coercion code paths.
 *
 * Build (inside Docker, from the csv-parser repo root):
 *   ./build_fuzz.sh
 *
 * Run:
 *   mkdir -p findings
 *   afl-fuzz -i seeds/ -o findings/ -- ./fuzz_csvparser
 */

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include "csv.hpp"

__AFL_FUZZ_INIT();

int main(void)
{
    __AFL_INIT();

    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        const size_t size = __AFL_FUZZ_TESTCASE_LEN;

        /* Need at least config byte + one byte of CSV data. */
        if (size < 2)
            continue;

        const uint8_t cfg   = buf[0];
        const char   *data  = reinterpret_cast<const char *>(buf + 1);
        const size_t  dsize = size - 1;

        /* Decode config byte. */
        const char delims[4] = { ',', ';', '\t', '|' };
        const char delim = delims[cfg & 0x03];

        const bool trim_ws        = (cfg & 0x04) != 0;
        const bool no_header      = (cfg & 0x08) != 0;
        const bool no_quote       = (cfg & 0x10) != 0;
        const uint8_t var_bits    = (cfg >> 5) & 0x03;
        const bool case_insensitive = (cfg & 0x80) != 0;

        const csv::VariableColumnPolicy var_policies[4] = {
            csv::VariableColumnPolicy::IGNORE_ROW,
            csv::VariableColumnPolicy::KEEP,
            csv::VariableColumnPolicy::KEEP_NON_EMPTY,
            csv::VariableColumnPolicy::THROW,
        };

        try {
            csv::CSVFormat fmt;
            fmt.delimiter(delim);
            if (trim_ws)    fmt.trim({ ' ', '\t' });
            if (no_header)  fmt.no_header();
            if (no_quote)   fmt.quote(false);
            fmt.variable_columns(var_policies[var_bits]);
            if (case_insensitive)
                fmt.column_names_policy(csv::ColumnNamePolicy::CASE_INSENSITIVE);

            /* csv::parse() copies the input into an owned stringstream, so
             * the fuzzer's buffer may be mutated freely after this call. */
            auto reader = csv::parse(csv::string_view(data, dsize), fmt);

            size_t row_idx = 0;
            for (auto& row : reader) {
                if (++row_idx > 50)
                    break;

                /* Access each field by index and exercise type coercions. */
                const size_t ncols = row.size();
                const size_t col_limit = ncols < 20 ? ncols : 20;

                for (size_t c = 0; c < col_limit; ++c) {
                    auto field = row[c];

                    /* String — always succeeds. */
                    (void)field.get<std::string>();

                    /* Numeric coercions — may throw on non-numeric data. */
                    try { (void)field.get<long long>();   } catch (const std::exception&) {}
                    try { (void)field.get<double>();      } catch (const std::exception&) {}
                    try { (void)field.get<long double>(); } catch (const std::exception&) {}

                    /* Type classification helpers. */
                    (void)field.is_null();
                    (void)field.is_str();
                    (void)field.is_num();
                    (void)field.is_int();
                    (void)field.is_float();
                }
            }
        }
        catch (const std::exception&) {
            /* Expected for malformed or pathological input. */
        }
        catch (...) {}
    }

    return 0;
}
