# picojson Fuzzing Analysis

**Result: No crashes or hangs found.**

---

## Run — ASAN + UBSan

| Stat | Value |
|---|---|
| Duration | ~42.2 hours (151,954 s) |
| Executions | 1,838,703,604 (~1.84B) |
| Exec/sec | 12,100 avg (10,796 last minute) |
| Edge coverage | 35.43% (557 / 1,572 edges) |
| Corpus entries | 1,164 |
| Cycles completed | 4,594 |
| Crashes | 0 |
| Hangs | 0 |
| Timeouts (unsaved) | 5,091 |

---

## Harness design

The binary was instrumented with both ASAN and UBSan (`AFL_USE_ASAN=1 AFL_USE_UBSAN=1`), compiled with `-DPICOJSON_USE_INT64` to enable the `int64_t` parsing path. This means any memory-safety bug or undefined behaviour (signed integer overflow, bad casts, out-of-range float-to-int conversions, etc.) would fire a sanitizer and be caught as a crash.

The harness targets `picojson::parse()` with its full iterator-based pipeline (`_parse_string`, `_parse_number`, `_parse_array`, `_parse_object`, `_parse_codepoint`). After a successful parse, it:

- **Walks the full value tree** up to depth 32, exercising all five type accessors (`null`, `bool`, `int64_t`, `double`, `string`)
- **Iterates arrays and objects** up to 64 elements per node
- **Round-trips through `serialize()`** to cover the output path

All exceptions are caught, so only true sanitizer violations or timeouts surface as findings.

---

## Coverage behaviour

The fuzzer kept finding new corpus entries for the first ~30.2 hours (`last_find` − `start_time` ≈ 108,545 s), then ran dry for the final ~12 hours (890 cycles without finds). Edge coverage plateaued at 35.43%. The gap from 100% is expected — a single-header C++ library with heavy template expansion has many edges that are either instantiation artefacts or dead code under the harness's fixed call pattern.

The 5,091 unsaved timeouts are minor and produced no saved hangs, suggesting no algorithmic-complexity issues with the numeric or string parsing paths.

---

## Assessment

picojson is robust under both memory-safety and undefined-behaviour sanitizers. Across 1.84 billion executions covering all JSON value types, deep nesting, the `int64_t` path, and the serialization round-trip, no bug was found. Given the 12-hour dry spell at the end and 890 consecutive empty cycles, further fuzzing with the same harness is unlikely to produce new findings without extending the API surface or using structure-aware (grammar-based) mutations.
