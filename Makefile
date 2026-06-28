# Convenience wrapper. The real determinism logic lives in
# run_determinism_test.sh so that CI and local runs share one code path.

CC      ?= gcc
SRC      = $(wildcard src/*.c)
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra

.PHONY: all test build clean

# Default target: run the full 10x determinism check.
all: test

test:
	bash run_determinism_test.sh

# A plain one-off build, handy for development.
build:
	$(CC) $(CFLAGS) $(SRC) -o calc -lm

clean:
	rm -rf out calc
