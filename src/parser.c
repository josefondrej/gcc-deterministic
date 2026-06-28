#include "parser.h"

#include <math.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>

/* Parser state threaded through the recursive descent. */
typedef struct {
    Lexer         lx;
    Token         cur;
    const SymTab *vars;
    jmp_buf       on_error;
    char          message[64];
} Parser;

static void advance(Parser *p)
{
    p->cur = lexer_next(&p->lx);
}

static void fail(Parser *p, const char *msg)
{
    snprintf(p->message, sizeof(p->message), "%s", msg);
    longjmp(p->on_error, 1);
}

/* Forward declarations: the grammar is mutually recursive. */
static double parse_expr(Parser *p);
static double parse_term(Parser *p);
static double parse_unary(Parser *p);
static double parse_power(Parser *p);
static double parse_atom(Parser *p);

static double parse_expr(Parser *p)
{
    double acc = parse_term(p);
    for (;;) {
        if (p->cur.type == TOK_PLUS) {
            advance(p);
            acc += parse_term(p);
        } else if (p->cur.type == TOK_MINUS) {
            advance(p);
            acc -= parse_term(p);
        } else {
            return acc;
        }
    }
}

static double parse_term(Parser *p)
{
    double acc = parse_unary(p);
    for (;;) {
        if (p->cur.type == TOK_STAR) {
            advance(p);
            acc *= parse_unary(p);
        } else if (p->cur.type == TOK_SLASH) {
            advance(p);
            double rhs = parse_unary(p);
            if (rhs == 0.0) {
                fail(p, "division by zero");
            }
            acc /= rhs;
        } else {
            return acc;
        }
    }
}

static double parse_unary(Parser *p)
{
    if (p->cur.type == TOK_MINUS) {
        advance(p);
        return -parse_unary(p);
    }
    if (p->cur.type == TOK_PLUS) {
        advance(p);
        return parse_unary(p);
    }
    return parse_power(p);
}

static double parse_power(Parser *p)
{
    double base = parse_atom(p);
    if (p->cur.type == TOK_CARET) {
        advance(p);
        /*
         * Right associative, and the exponent is a unary expression so that
         * "2 ^ -3" parses. Because '^' is reached only from inside parse_unary,
         * exponentiation binds tighter than a leading sign: "-3 ^ 2" == -(3^2).
         */
        double exponent = parse_unary(p);
        return pow(base, exponent);
    }
    return base;
}

static double parse_atom(Parser *p)
{
    switch (p->cur.type) {
        case TOK_NUMBER: {
            double v = p->cur.number;
            advance(p);
            return v;
        }
        case TOK_IDENT: {
            double v = 0.0;
            if (!symtab_get(p->vars, p->cur.ident, &v)) {
                fail(p, "unknown identifier");
            }
            advance(p);
            return v;
        }
        case TOK_LPAREN: {
            advance(p);
            double v = parse_expr(p);
            if (p->cur.type != TOK_RPAREN) {
                fail(p, "expected ')'");
            }
            advance(p);
            return v;
        }
        default:
            fail(p, "unexpected token");
    }
    return 0.0; /* unreachable */
}

EvalResult eval_expression(const char *src, const SymTab *vars)
{
    EvalResult result;
    memset(&result, 0, sizeof(result));

    Parser p;
    lexer_init(&p.lx, src);
    p.vars = vars;
    p.message[0] = '\0';

    if (setjmp(p.on_error) != 0) {
        result.ok = 0;
        snprintf(result.error, sizeof(result.error), "%s", p.message);
        return result;
    }

    advance(&p);
    double value = parse_expr(&p);

    if (p.cur.type != TOK_END) {
        fail(&p, "trailing characters");
    }

    result.ok = 1;
    result.value = value;
    return result;
}
