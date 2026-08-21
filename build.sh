#!/usr/bin/env bash
# Configures and builds the plugin. Override the LLVM version with
#   LLVM_VERSION=17 ./build.sh
set -euo pipefail

LLVM_VERSION="${LLVM_VERSION:-18}"
LLVM_DIR="${LLVM_DIR:-/usr/lib/llvm-${LLVM_VERSION}/lib/cmake/llvm}"
CLANG_DIR="${CLANG_DIR:-/usr/lib/llvm-${LLVM_VERSION}/lib/cmake/clang}"

if [[ ! -d "$LLVM_DIR" ]]; then
    echo "error: no LLVM cmake package at $LLVM_DIR" >&2
    echo "install it with: sudo apt install llvm-${LLVM_VERSION}-dev libclang-${LLVM_VERSION}-dev" >&2
    exit 1
fi

cmake -S . -B build \
    -DLLVM_DIR="$LLVM_DIR" \
    -DClang_DIR="$CLANG_DIR" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build -j"$(nproc)"

echo
echo "built: build/CyclomaticComplexity.so"
