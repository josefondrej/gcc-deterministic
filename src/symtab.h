#ifndef SYMTAB_H
#define SYMTAB_H

/* A singly linked list acting as a simple variable symbol table. */
typedef struct Symbol {
    char          *name;
    double         value;
    struct Symbol *next;
} Symbol;

typedef struct {
    Symbol *head;
    int     count;
} SymTab;

void   symtab_init(SymTab *t);

/* Insert or update a binding. */
void   symtab_set(SymTab *t, const char *name, double value);

/* Look up a binding; returns 1 and writes *out on success, 0 if absent. */
int    symtab_get(const SymTab *t, const char *name, double *out);

void   symtab_free(SymTab *t);

#endif /* SYMTAB_H */
