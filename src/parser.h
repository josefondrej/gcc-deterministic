#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symtab.h"

typedef struct {
    int    ok;       /* 1 on success, 0 on parse/eval error */
    double value;    /* result when ok == 1                 */
    char   error[64];
} EvalResult;

/*
 * Parse and evaluate an arithmetic expression in one recursive-descent pass.
 * Grammar (lowest to highest precedence):
 *
 *   expr   := term  (('+' | '-') term)*
 *   term   := unary (('*' | '/') unary)*
 *   unary  := ('-' | '+') unary | power
 *   power  := atom ('^' unary)?         (right associative)
 *   atom   := NUMBER | IDENT | '(' expr ')'
 *
 * Identifiers are resolved against the supplied symbol table.
 */
EvalResult eval_expression(const char *src, const SymTab *vars);

#endif /* PARSER_H */
