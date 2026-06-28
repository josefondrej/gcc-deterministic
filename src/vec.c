#include "vec.h"

#include <stdlib.h>
#include <stdio.h>

void vec_init(Vec *v)
{
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
}

void vec_push(Vec *v, double value)
{
    if (v->len == v->cap) {
        size_t new_cap = v->cap == 0 ? 4 : v->cap * 2;
        double *grown = realloc(v->data, new_cap * sizeof(double));
        if (grown == NULL) {
            fprintf(stderr, "vec_push: out of memory\n");
            exit(EXIT_FAILURE);
        }
        v->data = grown;
        v->cap = new_cap;
    }
    v->data[v->len++] = value;
}

double vec_get(const Vec *v, size_t index)
{
    if (index >= v->len) {
        fprintf(stderr, "vec_get: index %zu out of range\n", index);
        exit(EXIT_FAILURE);
    }
    return v->data[index];
}

size_t vec_len(const Vec *v)
{
    return v->len;
}

double vec_reduce(const Vec *v, double seed, double (*fn)(double acc, double x))
{
    double acc = seed;
    for (size_t i = 0; i < v->len; i++) {
        acc = fn(acc, v->data[i]);
    }
    return acc;
}

void vec_free(Vec *v)
{
    free(v->data);
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
}
