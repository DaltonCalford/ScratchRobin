#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-"$ROOT_DIR/build"}"
TMP_DIR="${TMPDIR:-"$BUILD_DIR/tmp"}"

mkdir -p "$BUILD_DIR"
mkdir -p "$TMP_DIR"

cleanup() {
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT

export TMPDIR="$TMP_DIR"

ctest --test-dir "$BUILD_DIR" --output-on-failure ${CTEST_ARGS:-}
