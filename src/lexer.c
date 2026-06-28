#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void lexer_init(Lexer *lx, const char *src)
{
    lx->src = src;
    lx->pos = 0;
}

static char peek(const Lexer *lx)
{
    return lx->src[lx->pos];
}

Token lexer_next(Lexer *lx)
{
    Token tok;
    memset(&tok, 0, sizeof(tok));

    while (isspace((unsigned char)peek(lx))) {
        lx->pos++;
    }

    char c = peek(lx);
    if (c == '\0') {
        tok.type = TOK_END;
        return tok;
    }

    if (isdigit((unsigned char)c) || c == '.') {
        char *end = NULL;
        tok.type = TOK_NUMBER;
        tok.number = strtod(lx->src + lx->pos, &end);
        lx->pos += (int)(end - (lx->src + lx->pos));
        return tok;
    }

    if (isalpha((unsigned char)c) || c == '_') {
        int n = 0;
        while ((isalnum((unsigned char)peek(lx)) || peek(lx) == '_') &&
               n < (int)sizeof(tok.ident) - 1) {
            tok.ident[n++] = peek(lx);
            lx->pos++;
        }
        tok.ident[n] = '\0';
        tok.type = TOK_IDENT;
        return tok;
    }

    lx->pos++;
    switch (c) {
        case '+': tok.type = TOK_PLUS;   break;
        case '-': tok.type = TOK_MINUS;  break;
        case '*': tok.type = TOK_STAR;   break;
        case '/': tok.type = TOK_SLASH;  break;
        case '^': tok.type = TOK_CARET;  break;
        case '(': tok.type = TOK_LPAREN; break;
        case ')': tok.type = TOK_RPAREN; break;
        default:  tok.type = TOK_ERROR;  break;
    }
    return tok;
}
