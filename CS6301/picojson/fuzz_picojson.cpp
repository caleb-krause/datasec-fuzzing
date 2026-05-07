/*
 * AFL++ fuzzing harness for picojson - persistent mode
 *
 * Target: picojson::parse(value&, const char*, const char*, std::string*)
 *         This is the iterator-based entry point that drives the entire
 *         parsing pipeline: _parse() → _parse_string() / _parse_number() /
 *         _parse_array() / _parse_object() / _parse_codepoint().
 *
 * After a successful parse the harness walks the value tree and round-trips
 * it through serialize() to exercise all accessor and serialization paths.
 *
 * Compiled with -DPICOJSON_USE_INT64 to also cover the int64_t parsing path.
 *
 * Build (inside Docker, from the picojson repo root):
 *   ./build_fuzz.sh
 *
 * Run:
 *   mkdir -p findings
 *   afl-fuzz -i seeds/ -o findings/ -- ./fuzz_picojson
 */

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unistd.h>

#define PICOJSON_USE_INT64
#include "picojson.h"

__AFL_FUZZ_INIT();

/*
 * Recursively visit every node in the parsed value tree.
 * This ensures AFL++ can reach all type-specific code paths
 * (bool, double, int64, string, array, object) inside get<T>().
 * Depth and breadth are capped so a deeply nested input doesn't
 * burn cycles inside a single fuzz iteration.
 */
static void walk(const picojson::value &v, int depth)
{
    if (depth > 32)
        return;

    if (v.is<picojson::null>()) {
        /* nothing to extract */
    } else if (v.is<bool>()) {
        (void)v.get<bool>();
    } else if (v.is<int64_t>()) {
        (void)v.get<int64_t>();
    } else if (v.is<double>()) {
        (void)v.get<double>();
    } else if (v.is<std::string>()) {
        (void)v.get<std::string>();
    } else if (v.is<picojson::array>()) {
        const picojson::array &arr = v.get<picojson::array>();
        const size_t lim = arr.size() < 64 ? arr.size() : 64;
        for (size_t i = 0; i < lim; ++i)
            walk(arr[i], depth + 1);
    } else if (v.is<picojson::object>()) {
        const picojson::object &obj = v.get<picojson::object>();
        size_t n = 0;
        for (picojson::object::const_iterator it = obj.begin();
             it != obj.end() && n < 64; ++it, ++n)
            walk(it->second, depth + 1);
    }
}

int main(void)
{
    __AFL_INIT();

    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        const size_t size = __AFL_FUZZ_TESTCASE_LEN;
        if (size == 0)
            continue;

        picojson::value v;
        std::string err;

        const char *data = reinterpret_cast<const char *>(buf);

        try {
            picojson::parse(v, data, data + size, &err);

            if (err.empty()) {
                /* Walk all value types to trigger accessor code paths. */
                walk(v, 0);

                /* Serialize → string to exercise the output path. */
                (void)v.serialize();
            }
        }
        catch (const std::exception &) {
            /* picojson can throw std::overflow_error on depth exceeded. */
        }
        catch (...) {}
    }

    return 0;
}
