/*
 * AFL++ fuzzing harness for sql-parser (hsql) - persistent mode
 *
 * Target: hsql::SQLParser::parse(const std::string&, SQLParserResult*)
 *         This drives the full Flex lexer → Bison parser → AST build pipeline.
 *         After a successful parse the harness walks every statement node via
 *         printStatementInfo() (redirected to /dev/null) so AFL++ can reach
 *         all AST accessor and destructor code paths.
 *
 * A secondary target, SQLParser::tokenize(), is exercised on every input to
 * cover the pure-lexer code path independently of the full parser.
 *
 * Build (inside Docker, from the sql-parser repo root):
 *   ./build_fuzz.sh
 *
 * Run:
 *   mkdir -p findings
 *   afl-fuzz -i seeds/ -o findings/ -- ./fuzz_sqlparser
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include "src/SQLParser.h"
#include "src/util/sqlhelper.h"

__AFL_FUZZ_INIT();

/*
 * Redirect printStatementInfo() output to /dev/null so it exercises the AST
 * traversal code without flooding the terminal or slowing down fuzzing.
 */
static FILE* devnull = nullptr;

int main(void)
{
    devnull = fopen("/dev/null", "w");

    __AFL_INIT();

    unsigned char* buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        const size_t size = __AFL_FUZZ_TESTCASE_LEN;
        if (size == 0)
            continue;

        /* Build a std::string from the raw fuzzer bytes.
         * The parser handles embedded NULs gracefully (the lexer reads until
         * the end of the string object, not until '\0'), so no truncation. */
        const std::string sql(reinterpret_cast<const char*>(buf), size);

        /* ── Path 1: full parse ────────────────────────────────────────── */
        {
            hsql::SQLParserResult result;

            /* parse() never throws — errors are signaled via result.isValid() */
            hsql::SQLParser::parse(sql, &result);

            if (result.isValid()) {
                /* Walk every parsed statement through the AST helper.
                 * This exercises all statement-type branches and their
                 * expression / table-ref sub-trees. */
                FILE* saved = stdout;
                stdout = devnull;   /* suppress output */
                for (auto i = 0u; i < result.size(); ++i) {
                    hsql::printStatementInfo(result.getStatement(i));
                }
                stdout = saved;
            }
            /* result destructor frees all AST nodes — exercises delete paths */
        }

        /* ── Path 2: tokenize only ─────────────────────────────────────── */
        {
            std::vector<int16_t> tokens;
            hsql::SQLParser::tokenize(sql, &tokens);
            /* tokens vector goes out of scope here */
        }
    }

    if (devnull) fclose(devnull);
    return 0;
}
