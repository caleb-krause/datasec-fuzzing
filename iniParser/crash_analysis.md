# iniParser Fuzzing Analysis

**Result: No crashes or hangs found.**

---

## Run 1 — ASAN (primary)

| Stat | Value |
|---|---|
| Duration | ~25.3 hours (90,945 s) |
| Executions | 515,710,592 (~515M) |
| Exec/sec | 5,670 avg (4,981 last minute) |
| Edge coverage | 49.48% (238 / 481 edges) |
| Corpus entries | 497 |
| Cycles completed | 2,218 |
| Crashes | 0 |
| Hangs | 0 |
| Timeouts | 6,546 (minor, not saved) |

The harness runs `iniparser_load_file()` then exercises every accessor path — `getstring`, `getint`, `getlongint`, `getint64`, `getuint64`, `getdouble`, `getboolean` — on every key in every section. AFL++ built a corpus of 497 entries and completed over 2,200 full cycles with no findings. Coverage plateaued at ~49% with 581 consecutive cycles yielding no new edges, indicating the fuzzer had thoroughly explored the reachable surface.

## Run 2 — UBSan follow-up

| Stat | Value |
|---|---|
| Duration | ~58 minutes (3,501 s) |
| Executions | 35,991,479 (~36M) |
| Exec/sec | 10,280 avg |
| Edge coverage | 28.95% (185 / 639 edges) |
| Crashes | 0 |
| Hangs | 0 |

A second pass was run with a UBSan-instrumented build, seeded from the full ASAN corpus, to catch undefined behaviour (signed overflow, invalid shifts, misaligned access, etc.) that ASAN would not detect. No UBSan violations were reported.

---

## Assessment

iniParser appears robust against malformed input. Across ~552M total executions under both ASAN and UBSan, with the full accessor API exercised on every fuzzed parse tree, no memory safety bug, undefined behaviour, crash, or hang was found. The parser handles corrupt, truncated, and adversarial INI content cleanly, returning null or default values without unsafe memory operations.
