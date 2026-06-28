# gcc-deterministic

A small test harness that checks whether **GCC produces bit-for-bit identical
output** when the same source is compiled repeatedly under a controlled
environment.

Given identical inputs, flags, and environment, a compiler *should* be
deterministic — every object file and the final binary identical down to the
byte. In practice that can be broken by things the compiler quietly bakes into
the output: the wall-clock time (`__DATE__` / `__TIME__`), the absolute build
path (embedded in `__FILE__` and debug info), and internally generated symbol
names. This repo compiles a non-trivial program **10 times**, then compares
every artifact with SHA-256. If anything diverges, it diagnoses *where*.

## The program under test

A self-contained arithmetic expression evaluator, split across several modules
so the build exercises multiple translation units:

| Module | What it exercises |
| --- | --- |
| `src/vec.c` | A growable array (`malloc`/`realloc`), plus a `vec_reduce` that takes a **function pointer** |
| `src/symtab.c` | A singly **linked list** used as a variable symbol table |
| `src/lexer.c` | A hand-written tokenizer (`enum`, `struct`, loops) |
| `src/parser.c` | A **recursive-descent** parser/evaluator — mutually recursive functions, `setjmp`/`longjmp` error handling |
| `src/main.c` | Drives it: evaluates a list of expressions in a loop and runs a recursive `factorial` |

The grammar handles `+ - * /`, `^` (right-associative exponentiation binding
tighter than unary minus, so `-3 ^ 2 == -9`), parentheses, named variables
(`pi`, `e`, `r`), and reports division-by-zero and parse errors.

```
$ make build && ./calc
  1 + 2 * 3      = 7.000000
  2 ^ 3 ^ 2      = 512.000000
  -3 ^ 2         = -9.000000
  pi * r ^ 2     = 12.566371
  ...
```

## Running the determinism test

```sh
./run_determinism_test.sh        # 10 builds (default)
RUNS=25 ./run_determinism_test.sh # override the count
make test                         # same thing via make
```

A passing run looks like:

```
build 01 : b16d6b2e8b13dac6...
build 02 : b16d6b2e8b13dac6...
...
RESULT: PASS — all 10 builds are byte-for-byte identical.
```

If a build ever diverges, the script compares the offending build against the
reference with `readelf -S` (section layout) and `objdump -s` (per-section
bytes), printing the diff and writing it to `out/divergence_report.txt`.

## How non-determinism is removed

**Frozen environment** (set by the script):

| Variable | Purpose |
| --- | --- |
| `SOURCE_DATE_EPOCH=1609459200` | Pins `__DATE__` / `__TIME__` / `__TIMESTAMP__` to 2021-01-01T00:00:00Z (the [reproducible-builds](https://reproducible-builds.org/docs/source-date-epoch/) convention) |
| `TZ=UTC`, `LC_ALL=C`, `LANG=C` | Keep any locale/timezone-dependent formatting constant |

**Reproducibility flags:**

| Flag | Purpose |
| --- | --- |
| `-ffile-prefix-map=$ROOT=.` | Strips the absolute build path from `__FILE__`, debug info, and embedded directory strings |
| `-frandom-seed=<file>` | Pins the seed GCC uses to uniquify internal symbol names — set per source file so it is stable across runs yet distinct between units |
| `-Wdate-time` | Warns if any code reintroduces a wall-clock dependency |

The test deliberately compiles with `-g -grecord-gcc-switches` (debug info plus
the exact flag list recorded *into* the binary). That is where path/timestamp
leakage would actually surface — so the check is meaningful rather than trivially
green.

**The full compiler flag set** (see `run_determinism_test.sh`) layers on a strict
standard (`-std=c11`), optimization (`-O2`), an extensive warning set
(`-Wall -Wextra -Wpedantic -Wshadow -Wcast-qual -Wstrict-prototypes` …), and
hardening (`-fstack-protector-strong`, `-fstack-clash-protection`,
`-fcf-protection=full`, `-D_FORTIFY_SOURCE=2`, `-fPIE`/`-pie`,
`-Wl,-z,relro -Wl,-z,now`).

## CI

`.github/workflows/determinism.yml` runs the same script on every push and pull
request, and uploads `out/divergence_report.txt` as an artifact if a build ever
fails to reproduce.

## Repo layout

```
src/                     the program under test
run_determinism_test.sh  the 10x build + compare + diagnose harness
Makefile                 `make build` / `make test` / `make clean`
.github/workflows/       CI running the test
```
