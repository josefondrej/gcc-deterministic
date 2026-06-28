#include "symtab.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char *dup_str(const char *s)
{
    size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    if (copy == NULL) {
        fprintf(stderr, "symtab: out of memory\n");
        exit(EXIT_FAILURE);
    }
    memcpy(copy, s, n);
    return copy;
}

void symtab_init(SymTab *t)
{
    t->head = NULL;
    t->count = 0;
}

void symtab_set(SymTab *t, const char *name, double value)
{
    for (Symbol *s = t->head; s != NULL; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            s->value = value;
            return;
        }
    }

    Symbol *fresh = malloc(sizeof(Symbol));
    if (fresh == NULL) {
        fprintf(stderr, "symtab: out of memory\n");
        exit(EXIT_FAILURE);
    }
    fresh->name = dup_str(name);
    fresh->value = value;
    fresh->next = t->head;
    t->head = fresh;
    t->count++;
}

int symtab_get(const SymTab *t, const char *name, double *out)
{
    for (Symbol *s = t->head; s != NULL; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            *out = s->value;
            return 1;
        }
    }
    return 0;
}

void symtab_free(SymTab *t)
{
    Symbol *s = t->head;
    while (s != NULL) {
        Symbol *next = s->next;
        free(s->name);
        free(s);
        s = next;
    }
    t->head = NULL;
    t->count = 0;
}
