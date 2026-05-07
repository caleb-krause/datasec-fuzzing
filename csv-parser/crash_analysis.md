# csv-parser Fuzzing Analysis

**Result: 13 crashes and 43 hangs found.**

---

## Run — ASAN + UBSan

| Stat | Value |
|---|---|
| Duration | ~43.9 hours (158,140 s) |
| Executions | 107,532,172 (~107.5M) |
| Exec/sec | 680 avg (664 last minute) |
| Edge coverage | 10.27% (854 / 8,315 edges) |
| Corpus entries | 665 |
| Cycles completed | 1,121 |
| Crashes | 13 (all SIGILL) |
| Hangs | 43 |
| Timeouts (unsaved) | 220,005 |
| First crash | ~40 s into the run |
| Last crash | ~16.8 hours into the run |
| First hang | ~28 s into the run |
| Last hang | ~1.5 hours into the run |

---

## Harness design

The binary was instrumented with both ASAN and UBSan (`AFL_USE_ASAN=1 AFL_USE_UBSAN=1`) and built with `-DCSV_ENABLE_THREADS=0` to disable background worker threads, keeping per-iteration cost low in persistent mode.

A config byte at the front of each input selects the parsing mode — delimiter (comma, semicolon, tab, pipe), whitespace trimming, header presence, quoting, variable-column policy, and case-sensitive column lookup — so a single binary covers multiple code paths simultaneously.

After parsing, the harness iterates up to 50 rows × 20 columns and calls `get<std::string>()`, `get<long long>()`, `get<double>()`, `get<long double>()`, and the five type-classification helpers (`is_null`, `is_str`, `is_num`, `is_int`, `is_float`) on every field.

---

## Bug 1 — UBSan integer overflow / float-to-int UB in numeric type detection (SIGILL)

**Crashes:** `id:000000` – `id:000012` (all 13 crashes, signal 4 = SIGILL)

All 13 crashes trigger UBSan traps (UBSan emits a `ud2` illegal instruction, received as SIGILL) rather than ASAN memory errors. The crash inputs all contain extreme numeric values in one of two forms:

**Form A — Huge float exponent:**
```
1.5e100000000000000000000000000000
2.998e8223372036854775807
1.5e111111111111111111111111111
9e+337203685...
```

**Form B — Near-boundary integer:**
```
9223372036854775807          ← INT64_MAX
922337203685477592337203685477-907
```

Both forms appear across nearly all 13 crashes, often in the same input. The UBSan violation fires inside csv-parser's numeric type detection and coercion pipeline — most likely:

- **Signed integer overflow** when classifying whether a parsed value fits in an `int64_t`. For a value at or near `INT64_MAX`, arithmetic like `parsed_value + 1` to check an upper bound overflows signed 64-bit integer — undefined behaviour caught by UBSan.
- **Float-to-integer conversion UB** when `get<long long>()` is called on a field whose value is too large to represent as `int64_t` (e.g., `1.5e10^30` → `+Inf` as double → casting infinity to `long long` is UB in C++).

Crashes appeared within 40 seconds of starting, meaning the seed corpus already contained INT64_MAX values and AFL only needed a small mutation (extending `1.5e10` to a huge exponent) to trigger the bug. The last crash was found at ~16.8 hours; the bug cluster is clearly concentrated early and did not require deep evolution to reach.

**Fix:** Validate the result of `strtoll`/`strtod` against `errno` and `LLONG_MAX`/`LLONG_MIN` before performing any further arithmetic; guard all float-to-integer casts with an explicit range check.

---

## Bug 2 — Algorithmic complexity hang on extreme exponents (timeout)

**Hangs:** `id:000000` – `id:000042` (43 hangs)

The 43 saved hangs, alongside 220,005 unsaved timeouts, indicate a severe performance bug in the numeric parsing path. Hang inputs share the same extreme-exponent pattern:

```
922337203E854775807        ← exponent ≈ 10^18 digits long
2.998e8223372036854775807
```

When `strtod` (or csv-parser's internal equivalent) encounters a float literal with a multi-digit exponent in the billions-to-quintillions range, some glibc `strtod` implementations fall into a slow path that iterates character-by-character through the exponent and performs repeated extended-precision arithmetic, resulting in near-infinite CPU time. AFL's 20-second timeout fires consistently on these inputs.

Hangs appeared within 28 seconds of the start — even faster than crashes — and the last saved hang was at ~1.5 hours. All 43 distinct hang inputs are variants of the same root cause; the large `total_tmout` count (220,005) shows AFL encountered this slow path hundreds of thousands of times without being able to escape it.

**Fix:** Add a pre-validation step that rejects exponent strings longer than a safe threshold (e.g., > 6 digits) before passing the value to `strtod`, or clamp parsed exponents to the `double` representable range before conversion.

---

## Coverage notes

Edge coverage is low at 10.27% (854 of 8,315 edges). csv-parser is a large library with significant dead code in the mmap-based path (only the stream path is exercised), worker-thread code (disabled by `-DCSV_ENABLE_THREADS=0`), and CSV statistics/JSON output paths not called by the harness. The reachable surface through `csv::parse()` + field iteration is well saturated — the fuzzer went 88 consecutive cycles without new finds before the run ended.

---

## Summary

| ID(s) | Signal | Bug | Root cause |
|---|---|---|---|
| 000000 – 000012 | SIGILL (4) | UBSan: integer overflow / float-to-int UB | Numeric type detection arithmetic overflows on INT64_MAX values or infinity cast to `long long` |
| hangs 000000 – 000042 | — | Algorithmic complexity | `strtod` slow path on float literals with astronomically large exponents |

**2 distinct root-cause bugs.** Both are triggered by the same class of extreme numeric inputs — huge exponents and near-INT64_MAX integer strings — that the seed corpus was already close to producing.
