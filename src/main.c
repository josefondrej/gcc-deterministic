#include "vec.h"
#include "symtab.h"
#include "parser.h"

#include <stdio.h>
#include <string.h>

/* Reducer passed to vec_reduce as a function pointer. */
static double add(double acc, double x)
{
    return acc + x;
}

static double max_of(double acc, double x)
{
    return x > acc ? x : acc;
}

/* Plain recursion, independent of the parser, to exercise the call stack. */
static unsigned long long factorial(unsigned int n)
{
    if (n <= 1) {
        return 1ULL;
    }
    return (unsigned long long)n * factorial(n - 1);
}

int main(void)
{
    SymTab vars;
    symtab_init(&vars);
    symtab_set(&vars, "pi", 3.14159265358979323846);
    symtab_set(&vars, "e", 2.71828182845904523536);
    symtab_set(&vars, "r", 2.0);

    const char *expressions[] = {
        "1 + 2 * 3",
        "(1 + 2) * 3",
        "2 ^ 3 ^ 2",          /* right associative -> 512 */
        "-3 ^ 2",             /* unary binds looser -> -9 */
        "pi * r ^ 2",         /* area of a circle, r = 2  */
        "e + pi",
        "10 / (5 - 5)",       /* division by zero -> error */
        "1 + ",               /* parse error               */
    };
    size_t n = sizeof(expressions) / sizeof(expressions[0]);

    Vec results;
    vec_init(&results);

    puts("Expression evaluation");
    puts("---------------------");
    for (size_t i = 0; i < n; i++) {
        EvalResult r = eval_expression(expressions[i], &vars);
        if (r.ok) {
            printf("  %-14s = %.6f\n", expressions[i], r.value);
            vec_push(&results, r.value);
        } else {
            printf("  %-14s ! %s\n", expressions[i], r.error);
        }
    }

    double total = vec_reduce(&results, 0.0, add);
    double peak  = vec_reduce(&results, results.data ? results.data[0] : 0.0, max_of);

    puts("");
    printf("Evaluated OK   : %zu\n", vec_len(&results));
    printf("Sum of results : %.6f\n", total);
    printf("Max of results : %.6f\n", peak);

    puts("");
    puts("Factorials");
    puts("----------");
    for (unsigned int k = 1; k <= 10; k++) {
        printf("  %2u! = %llu\n", k, factorial(k));
    }

    vec_free(&results);
    symtab_free(&vars);
    return 0;
}
