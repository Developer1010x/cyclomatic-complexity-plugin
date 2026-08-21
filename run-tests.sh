#!/usr/bin/env bash
# Verifies the plugin against the EXPECT: annotations in tests/.
# Every expectation is derived by hand in tests/ -- if one fails, decide which
# is wrong before changing either. The annotation is the specification.
set -uo pipefail

PLUGIN="${PLUGIN:-build/CyclomaticComplexity.so}"
CLANG="${CLANG:-clang}"

if [[ ! -f "$PLUGIN" ]]; then
    echo "error: plugin not found at $PLUGIN -- run ./build.sh first" >&2
    exit 1
fi

pass=0; fail=0

check_file() {
    local src="$1"; shift
    local actual
    local stderr_file
    stderr_file=$(mktemp)
    actual=$("$CLANG" -fsyntax-only "$@" \
        -fplugin="$PLUGIN" \
        "$src" 2>"$stderr_file")

    if [[ -s "$stderr_file" ]]; then
        echo "  note  $src: clang wrote to stderr:"
        sed 's/^/          /' "$stderr_file"
    fi
    rm -f "$stderr_file"

    if [[ -z "$actual" ]]; then
        echo "  FAIL  $src produced no output at all"
        ((fail++)); return
    fi

    # Each annotation reads:  EXPECT: <function> <complexity>
    while read -r fn want; do
        [[ -z "$fn" ]] && continue
        local got
        got=$(awk -v f="$fn" '$3 == f { print $2 }' <<< "$actual" | head -1)
        if [[ -z "$got" ]]; then
            echo "  FAIL  $src: $fn not reported"
            ((fail++))
        elif [[ "$got" == "$want" ]]; then
            echo "  ok    $src: $fn = $got"
            ((pass++))
        else
            echo "  FAIL  $src: $fn expected $want, got $got"
            ((fail++))
        fi
    done < <(grep -o 'EXPECT: [A-Za-z_][A-Za-z0-9_]* [0-9]*' "$src" | sed 's/EXPECT: //')
}

echo "== C (default std) =="
check_file tests/basic.c

echo "== C with GNU extensions (-std=gnu11) =="
check_file tests/gnu_extensions.c -std=gnu11

echo "== C++ =="
check_file tests/cpp_features.cpp -std=c++17

echo
echo "passed: $pass   failed: $fail"
[[ $fail -eq 0 ]]
