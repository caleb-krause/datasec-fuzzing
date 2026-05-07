# mpc Fuzzing Analysis

**Result: No crashes or hangs found.**

---

## Run — No sanitizers (AFL++ signal detection only)

| Stat | Value |
|---|---|
| Duration | ~29.0 hours cumulative (104,406 s) |
| Executions | 281,481,439 (~281.5M) |
| Exec/sec | 2,696 avg (3,138 last minute) |
| Edge coverage | 23.12% (348 / 1,505 edges) |
| Corpus entries | 458 |
| Cycles completed | 1,367 |
| Cycles without finds | 18 |
| Crashes | 0 |
| Hangs | 0 |
| Timeouts (unsaved) | 13,948 |
| Bitmap stability | 78.74% |

---

## Sanitizer note

ASAN and UBSan are intentionally absent from this build. mpc is written in C89 and uses patterns — notably signed-char array indexing in its regex engine — that are technically undefined behaviour but correct in practice. Both sanitizers abort on these patterns for every valid input, which kills every seed during AFL++'s calibration dry-run and prevents the fuzzer from starting at all. The binary is therefore instrumented for coverage only; real crashes (SIGSEGV, SIGABRT, SIGFPE) are still caught by AFL++ through signal detection, but subtler memory-safety and UB bugs that would need a sanitizer to surface are undetectable in this configuration.

---

## Harness design

The Lispy S-expression grammar is constructed once before `__AFL_INIT()` so it lives in the forkserver parent and is shared across all iterations without rebuild cost. The grammar was chosen specifically to maximise coverage of mpc's parser-type dispatch:

- **Regex sub-parsers** — `number`, `symbol`, `string`, `comment`
- **Recursive rules** — `sexpr` and `qexpr` reference `expr`, which references them back
- **Optional repetition** — `expr*` in `lispy`, `sexpr`, and `qexpr`
- **SOI/EOI anchors** — `lispy` rule anchors to start and end of input

Each iteration calls `mpc_nparse()` (length-bounded, safe for raw AFL++ memory), then exercises the full success path — `mpc_ast_print_to` serialises the AST to `/dev/null`, and `mpc_ast_delete` recursively frees all nodes — or the error path via `mpc_err_delete`.

---

## Stability and variability

The bitmap stability of 78.74% is notably low compared to the other projects in this fuzzing campaign (typically 99%+). AFL++ flagged 74 variable bitmap bytes (`var_byte_count: 74`), meaning roughly one in five coverage edges fires non-deterministically across runs of the same input.

This is likely caused by mpc's internal allocator producing heap addresses that vary between persistent-mode iterations and flow into branch conditions (e.g. pointer comparisons, hash table slots). AFL++ handles variable edges conservatively — treating them as less reliable signal — which limits the effectiveness of coverage-guided mutations on those paths. It also means the true code coverage is somewhat higher than the 23.12% figure suggests, since some edges are being suppressed as unstable.

---

## Coverage plateau behaviour

With only 18 cycles without new finds at the time of the last stats update, the fuzzer was still actively discovering new corpus entries — the last find was logged just 6 minutes into the most recent session. The run was accumulating corpus across multiple resumed sessions (cumulative run_time of 29 hours vs. a much shorter wall-clock window for the final session). Coverage is still growing, and the fuzzer has not saturated the reachable surface.

---

## Assessment

mpc shows no crashes or hangs across ~281.5M executions of its full parse-print-delete pipeline. The absence of sanitizers is the main caveat: memory-safety bugs that don't cause a signal (heap overreads, use-after-free that happens to read valid memory, integer overflow that doesn't segfault) would not be caught here. A follow-up run with ASAN enabled — after patching or suppressing the known false-positive UB in the regex engine — would give stronger assurance. The low bitmap stability is worth investigating: resolving the non-determinism (e.g. by seeding the allocator or eliminating pointer-value-dependent branches) would improve AFL++'s ability to guide mutations and likely increase effective coverage.
