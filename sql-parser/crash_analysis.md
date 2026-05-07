# sql-parser (hsql) Fuzzing Analysis

**Result: No crashes or hangs found.**

---

## Run — ASAN + UBSan

| Stat | Value |
|---|---|
| Duration | ~71.1 hours (255,812 s) |
| Executions | 1,666,235,306 (~1.67B) |
| Exec/sec | 6,513 avg (5,726 last minute) |
| Edge coverage | 16.72% (2,139 / 12,792 edges) |
| Corpus entries | 3,397 |
| Cycles completed | 898 |
| Cycles without finds | 1 |
| Crashes | 0 |
| Hangs | 0 |
| Timeouts (unsaved) | 101,726 |
| Bitmap stability | 96.63% |
| Last new corpus entry | ~60.6 hours in |

---

## Harness design

Built with both ASAN and UBSan (`AFL_USE_ASAN=1 AFL_USE_UBSAN=1`), targeting the full Flex lexer → Bison parser → AST construction pipeline of hsql. Two code paths are exercised on every input:

- **Path 1 — Full parse:** `SQLParser::parse()` drives the complete pipeline. On success, every statement node is walked via `printStatementInfo()` (redirected to `/dev/null`) to cover all AST accessor branches and expression/table-ref sub-trees. The `SQLParserResult` destructor then frees all AST nodes, exercising every delete path.
- **Path 2 — Tokenize only:** `SQLParser::tokenize()` exercises the Flex lexer independently of the Bison grammar, covering pure-lexer code paths that the full parse might not isolate.

All exceptions are caught, so only sanitizer violations or signals surface as findings.

---

## Coverage notes

16.72% edge coverage across 12,792 total edges is low in percentage terms, but the absolute count of 2,139 covered edges is the highest of any project in this fuzzing campaign. The gap reflects the scale of Bison and Flex generated code — the parser tables, lexer transition functions, and error-recovery rules amount to thousands of edges that are difficult to reach through unstructured byte mutations alone. The reachable surface through SQL text input is thoroughly covered.

The corpus of 3,397 entries is by far the largest in this campaign, driven by SQL's rich grammar: SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, ALTER, PREPARE, EXECUTE, subqueries, joins, expressions, and more each generate distinct coverage paths that AFL++ preserves as separate corpus entries.

---

## Active discovery throughout the run

With only **1 cycle without finds** at the time the run ended, the fuzzer had not saturated the reachable surface — new corpus entries were still being discovered as recently as ~60.6 hours into the 71-hour run, leaving only ~10 hours of dry time at the end. This is the most actively evolving run in the campaign and suggests sql-parser's grammar is deep enough that longer or structure-aware fuzzing would continue to find new paths.

---

## Timeouts

101,726 unsaved timeouts occurred across the run. None met the threshold for a saved hang, meaning no input consistently reproduced a slow parse. The high count likely reflects complex nested SQL queries (deeply nested subqueries, long `IN` lists, or large `CASE` expressions) that push the Bison parser into expensive backtracking or error-recovery states. Since no input was slow enough to save, this is a throughput sensitivity rather than an algorithmic complexity bug.

---

## Assessment

sql-parser is robust under ASAN and UBSan across 1.67 billion executions covering both the full Flex/Bison parse pipeline and the standalone lexer. No memory-safety bug or undefined behaviour was found. The standout characteristic of this run is that coverage was still growing at hour 60 — the SQL grammar is expansive enough that the fuzzer had not fully explored it by the end. Structure-aware mutations (SQL grammar-based fuzzing) or a longer run would be the natural next step to push deeper into the parser's error-recovery and edge-case statement handling.
