#ifndef VEC_H
#define VEC_H

#include <stddef.h>

/* A small growable array of doubles. */
typedef struct {
    double *data;
    size_t  len;
    size_t  cap;
} Vec;

void   vec_init(Vec *v);
void   vec_push(Vec *v, double value);
double vec_get(const Vec *v, size_t index);
size_t vec_len(const Vec *v);

/* Apply fn to every element and return the accumulated result. */
double vec_reduce(const Vec *v, double seed, double (*fn)(double acc, double x));

void   vec_free(Vec *v);

#endif /* VEC_H */
