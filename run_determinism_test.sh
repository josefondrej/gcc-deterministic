#!/usr/bin/env bash
#
# run_determinism_test.sh
#
# Compile the sample C program N times under a controlled, reproducible
# environment and verify that every produced object file and the final
# binary are byte-for-byte identical (compared via SHA-256).
#
# If any build differs, the script diagnoses *where* by comparing ELF
# sections (readelf / objdump) between the first divergent build and the
# reference build, and writes a report under out/.
#
set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$ROOT/src"
OUT_DIR="$ROOT/out"
RUNS="${RUNS:-10}"
CC="${CC:-gcc}"
BIN_NAME="calc"

SOURCES=(main.c vec.c symtab.c lexer.c parser.c)

# ---------------------------------------------------------------------------
# A frozen, locale- and timezone-stable environment.
#
#   SOURCE_DATE_EPOCH  freezes __DATE__/__TIME__/__TIMESTAMP__ expansion
#                      (2021-01-01T00:00:00Z) per the reproducible-builds spec.
#   TZ / LC_ALL / LANG keep any locale- or tz-dependent formatting constant.
# ---------------------------------------------------------------------------
export SOURCE_DATE_EPOCH=1609459200
export TZ=UTC
export LC_ALL=C
export LANG=C

# ---------------------------------------------------------------------------
# Compiler flags.
#
# CFLAGS pile on as many knobs as is reasonable: a strict language standard,
# optimization, debug info (so any path/timestamp leakage would actually show
# up in the binary), an extensive warning set, and hardening features.
#
# REPRO_FLAGS are the ones that specifically remove non-deterministic inputs:
#   -ffile-prefix-map  strips the absolute build path from debug info,
#                      __FILE__ and embedded directory strings.
#   -frandom-seed      pins the seed GCC uses to uniquify internal symbol
#                      names (set per-file to its name: stable yet unique).
#   -Wdate-time        warns if anything reintroduces a wall-clock dependency.
# ---------------------------------------------------------------------------
CFLAGS=(
    -std=c11
    -O2
    -g
    -pipe
    -fno-common
    -fstack-protector-strong
    -fstack-clash-protection
    -fcf-protection=full
    -fPIE
    -fasynchronous-unwind-tables
    -funwind-tables
    -grecord-gcc-switches
    -D_FORTIFY_SOURCE=2
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wcast-qual
    -Wwrite-strings
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wredundant-decls
    -Wpointer-arith
    -Wformat=2
    -Wundef
    -Wdate-time
)

REPRO_COMMON=(
    "-ffile-prefix-map=${ROOT}=."
)

LDFLAGS=(
    -pie
    -Wl,-z,relro
    -Wl,-z,now
    -lm
)

# ---------------------------------------------------------------------------
# Build one full copy of the program into the given directory.
# ---------------------------------------------------------------------------
build_once() {
    local dest="$1"
    mkdir -p "$dest"

    local objs=()
    local src obj
    for src in "${SOURCES[@]}"; do
        obj="$dest/${src%.c}.o"
        # -frandom-seed is pinned to the source file name: identical across
        # runs, distinct between translation units.
        "$CC" "${CFLAGS[@]}" "${REPRO_COMMON[@]}" \
            "-frandom-seed=${src}" \
            -c "$SRC_DIR/$src" -o "$obj"
        objs+=("$obj")
    done

    "$CC" "${CFLAGS[@]}" "${REPRO_COMMON[@]}" \
        "${objs[@]}" -o "$dest/$BIN_NAME" "${LDFLAGS[@]}"
}

# ---------------------------------------------------------------------------
# Hash every artifact in a build directory into a sorted manifest.
# ---------------------------------------------------------------------------
manifest() {
    local dir="$1"
    ( cd "$dir" && sha256sum "$BIN_NAME" ./*.o | sort -k2 )
}

# ---------------------------------------------------------------------------
# Diagnose a divergence between two build directories.
# ---------------------------------------------------------------------------
diagnose() {
    local ref="$1" bad="$2" report="$3"
    {
        echo "=========================================================="
        echo " Determinism FAILURE diagnosis"
        echo "=========================================================="
        echo "Reference build : $ref"
        echo "Divergent build : $bad"
        echo

        local f rel
        for f in "$ref/$BIN_NAME" "$ref"/*.o; do
            rel="$(basename "$f")"
            if ! cmp -s "$ref/$rel" "$bad/$rel"; then
                echo "----------------------------------------------------------"
                echo "Artifact differs: $rel"
                echo "----------------------------------------------------------"

                echo "[ELF section headers — sizes/offsets]"
                diff <(readelf -S -W "$ref/$rel" 2>/dev/null) \
                     <(readelf -S -W "$bad/$rel" 2>/dev/null) || true
                echo

                echo "[Per-section byte content diff]"
                diff <(objdump -s "$ref/$rel" 2>/dev/null) \
                     <(objdump -s "$bad/$rel" 2>/dev/null) | head -n 80 || true
                echo
            fi
        done
    } | tee "$report"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
main() {
    echo "GCC determinism test"
    echo "  compiler          : $("$CC" --version | head -n1)"
    echo "  runs              : $RUNS"
    echo "  SOURCE_DATE_EPOCH : $SOURCE_DATE_EPOCH ($(date -u -d "@$SOURCE_DATE_EPOCH" 2>/dev/null || echo frozen))"
    echo

    rm -rf "$OUT_DIR"
    mkdir -p "$OUT_DIR"

    local i ref_manifest="" all_ok=1 first_bad=""
    for (( i = 1; i <= RUNS; i++ )); do
        local dest
        dest="$(printf '%s/build_%02d' "$OUT_DIR" "$i")"
        build_once "$dest"

        local m
        m="$(manifest "$dest")"
        printf 'build %02d : %s\n' "$i" "$(echo "$m" | sha256sum | cut -c1-16)..."

        if [[ -z "$ref_manifest" ]]; then
            ref_manifest="$m"
        elif [[ "$m" != "$ref_manifest" ]]; then
            all_ok=0
            [[ -z "$first_bad" ]] && first_bad="$dest"
        fi
    done

    echo
    echo "Reference manifest (SHA-256 of each artifact):"
    echo "$ref_manifest" | sed 's/^/  /'
    echo

    if [[ "$all_ok" -eq 1 ]]; then
        echo "RESULT: PASS — all $RUNS builds are byte-for-byte identical."
        exit 0
    fi

    echo "RESULT: FAIL — builds diverged. Diagnosing..."
    echo
    diagnose "$OUT_DIR/build_01" "$first_bad" "$OUT_DIR/divergence_report.txt"
    echo
    echo "Full report written to: $OUT_DIR/divergence_report.txt"
    exit 1
}

main "$@"
