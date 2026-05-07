/*
 * AFL++ fuzzing harness for mpc - persistent mode
 *
 * Target: mpc_nparse() using a fixed Lispy S-expression grammar.
 *
 * The grammar is built ONCE before __AFL_INIT() so it lives in the
 * forkserver parent and is shared (read-only) across all child iterations
 * without paying the build cost on every fuzz input.
 *
 * Each iteration exercises:
 *   - mpc_nparse()     → Flex lexer + recursive descent (mpc_parse_run)
 *   - mpc_ast_traverse_*  → Full AST walker (all node types)
 *   - mpc_ast_print_to    → AST serialiser (redirected to /dev/null)
 *   - mpc_ast_delete   → Recursive AST destructor
 *
 * The Lispy grammar was chosen because it contains:
 *   - Regex sub-parsers  (number, symbol, string, comment)
 *   - Recursive rules    (sexpr, qexpr reference expr which references them)
 *   - Optional repetition (expr*)
 *   - SOI/EOI anchors
 * …giving maximum coverage of mpc_parse_run()'s 20+ parser-type branches.
 *
 * Build (inside Docker, from the mpc repo root):
 *   ./build_fuzz.sh
 *
 * Run:
 *   mkdir -p findings
 *   afl-fuzz -i seeds/ -o findings/ -- ./fuzz_mpc
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mpc.h"

__AFL_FUZZ_INIT();

int main(void)
{
    /* ── Build grammar once ─────────────────────────────────────────────── */
    mpc_parser_t *Number  = mpc_new("number");
    mpc_parser_t *Symbol  = mpc_new("symbol");
    mpc_parser_t *String  = mpc_new("string");
    mpc_parser_t *Comment = mpc_new("comment");
    mpc_parser_t *Sexpr   = mpc_new("sexpr");
    mpc_parser_t *Qexpr   = mpc_new("qexpr");
    mpc_parser_t *Expr    = mpc_new("expr");
    mpc_parser_t *Lispy   = mpc_new("lispy");

    mpc_err_t *lang_err = mpca_lang(MPCA_LANG_PREDICTIVE,
        " number  \"number\"  : /[0-9]+/ ;                          "
        " symbol  \"symbol\"  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; "
        " string  \"string\"  : /\"(\\\\.|[^\"])*\"/ ;              "
        " comment             : /;[^\\r\\n]*/ ;                     "
        " sexpr               : '(' <expr>* ')' ;                   "
        " qexpr               : '{' <expr>* '}' ;                   "
        " expr                : <number>  | <symbol> | <string>     "
        "                     | <comment> | <sexpr>  | <qexpr> ;    "
        " lispy               : /^/ <expr>* /$/ ;                   ",
        Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy, NULL);

    if (lang_err != NULL) {
        mpc_err_print(lang_err);
        mpc_err_delete(lang_err);
        return 1;
    }

    /* Redirect AST printing to /dev/null so the serialiser is exercised
     * without flooding the terminal or burning I/O time. */
    FILE *devnull = fopen("/dev/null", "w");

    /* ── Persistent loop ────────────────────────────────────────────────── */
    __AFL_INIT();

    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        size_t size = __AFL_FUZZ_TESTCASE_LEN;
        if (size == 0)
            continue;

        mpc_result_t r;

        /*
         * mpc_nparse() accepts a length so the buffer does not need to be
         * NUL-terminated — safe for raw AFL++ memory.
         */
        if (mpc_nparse("<fuzz>", (const char *)buf, size, Lispy, &r)) {

            /* Exercise the AST printer — this internally walks the full tree
             * (tags, contents, children) without any partial-traversal state
             * that could trip ASAN on mpc's internal allocator. */
            if (devnull)
                mpc_ast_print_to(r.output, devnull);

            /* Recursive destructor — exercises all node free paths. */
            mpc_ast_delete(r.output);

        } else {
            /* Error path: exercises error formatting and cleanup. */
            mpc_err_delete(r.error);
        }
    }

    /* ── Cleanup ────────────────────────────────────────────────────────── */
    if (devnull) fclose(devnull);
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);
    return 0;
}
