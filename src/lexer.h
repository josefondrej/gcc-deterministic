#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_NUMBER,
    TOK_IDENT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_CARET,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_END,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    double    number;     /* valid when type == TOK_NUMBER */
    char      ident[32];  /* valid when type == TOK_IDENT  */
} Token;

typedef struct {
    const char *src;
    int         pos;
} Lexer;

void  lexer_init(Lexer *lx, const char *src);
Token lexer_next(Lexer *lx);

#endif /* LEXER_H */
