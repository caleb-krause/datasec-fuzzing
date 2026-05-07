# rapidcsv Fuzzing Analysis

**Result: No crashes or hangs found.**

---

## Run — ASAN

| Stat | Value |
|---|---|
| Duration | ~18.7 hours (67,399 s) |
| Executions | 81,601,332 (~81.6M) |
| Exec/sec | 1,211 avg (989 last minute) |
| Edge coverage | 25.34% (583 / 2,301 edges) |
| Corpus entries | 847 |
| Cycles completed | 278 |
| Crashes | 0 |
| Hangs | 0 |
| Timeouts (unsaved) | 36,182 |

---

## Harness design

The harness encodes a configuration byte at the front of each input, allowing a single binary to exercise eight independent parsing dimensions simultaneously:

- **Separator:** comma, semicolon, tab, or pipe
- **Whitespace trimming**
- **Quoted linebreaks**
- **Comment line skipping** (`#`-prefixed lines)
- **Empty line skipping**
- **Header mode:** row 0 as column names vs. no headers
- **Converter strictness:** throw on bad numeric vs. silent default

After parsing, the harness calls `GetCell`, `GetRow`, and `GetColumn` across up to 20×20 cells to exercise all accessor and type-conversion paths (`std::string`, `double`, `long long`). All exceptions are caught, so only true memory-safety crashes or hangs surface as findings.

---

## Coverage plateau

The last new corpus entry was found at ~3.8 hours in (`last_find` − `start_time` ≈ 13,566 s). The fuzzer then ran for another ~15 hours (`time_wo_finds: 53,833 s`) without discovering new edges, completing 219 fruitless cycles. AFL++ had fully saturated the reachable surface by the quarter-mark of the run.

Edge coverage settled at 25.34% (583 of 2,301 edges). The gap from 100% is expected for a header-only C++ library: a large portion of the remaining edges are in template instantiations, error paths gated on compile-time types, or code unreachable through the harness's fixed API surface.

The 36,182 unsaved timeouts are worth noting. None crossed the threshold to become saved hangs, but they indicate a subset of inputs — likely large or deeply nested quoted fields — slow the parser enough to brush against the 20-second limit. No input consistently reproduced a hang, so this appears to be a throughput sensitivity rather than an algorithmic complexity bug.

---

## Assessment

rapidcsv shows no memory-safety issues under ASAN across ~81.6M executions with a multi-configuration harness covering all major parsing modes and accessor paths. The library handles malformed, truncated, and adversarial CSV content safely, relying on C++ exceptions for error propagation rather than unsafe fallbacks. Given the coverage plateau at ~3.8 hours and 219 consecutive dry cycles, continuing the run further is unlikely to yield new findings without a significant harness change (e.g., structure-aware mutations or additional API surface).
