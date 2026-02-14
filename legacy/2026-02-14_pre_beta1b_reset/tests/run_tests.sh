#!/usr/bin/env bash
# ScratchRobin
# Copyright (c) 2025-2026 Dalton Calford
#
# Licensed under the Initial Developer's Public License Version 1.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
# https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

run_ctest() {
    local label="$1"
    shift
    local start=$SECONDS
    "$@"
    local elapsed=$((SECONDS - start))
    echo "Completed ${label} in ${elapsed}s"
}

if [[ ! -d "${BUILD_DIR}" ]]; then
    echo "Build directory not found: ${BUILD_DIR}" >&2
    echo "Run CMake configure/generate first (e.g., mkdir build && cmake ..)." >&2
    exit 1
fi

cmake --build "${BUILD_DIR}" --target scratchrobin_tests_unit scratchrobin_tests_smoke scratchrobin_tests_integration

cd "${BUILD_DIR}"

case "${1:-default}" in
    default)
        echo "Running default suite (smoke + unit, <= 15 minutes target)..."
        run_ctest "default" ctest -L "smoke|unit" --output-on-failure --timeout 900
        ;;
    smoke)
        echo "Running smoke tests..."
        run_ctest "smoke" ctest -L smoke --output-on-failure --timeout 60
        ;;
    unit)
        echo "Running unit tests..."
        run_ctest "unit" ctest -L unit --output-on-failure --timeout 300
        ;;
    integration)
        echo "Running integration tests..."
        run_ctest "integration" ctest -L integration --output-on-failure --timeout 300
        ;;
    performance)
        echo "Running performance tests..."
        run_ctest "performance" ctest -L performance --output-on-failure --timeout 600
        ;;
    quarantine)
        echo "Running quarantine tests (known issues)..."
        run_ctest "quarantine" ctest -L quarantine --output-on-failure --timeout 300 || true
        ;;
    ci)
        echo "Running CI test suite (smoke + unit + integration)..."
        run_ctest "ci" ctest -L "smoke|unit|integration" -E "quarantine" --output-on-failure --timeout 600
        ;;
    all)
        echo "Running all tests (excluding quarantine)..."
        run_ctest "all" ctest -E "quarantine" --output-on-failure --timeout 900
        ;;
    *)
        echo "Usage: $0 {default|smoke|unit|integration|performance|quarantine|ci|all}" >&2
        echo "" >&2
        echo "  default     - smoke + unit (target <= 15 minutes)" >&2
        echo "  smoke       - Fast sanity tests" >&2
        echo "  unit        - Unit tests" >&2
        echo "  integration - Integration tests (env-gated)" >&2
        echo "  performance - Performance/benchmark tests (on-demand)" >&2
        echo "  quarantine  - Known-problematic tests (manual)" >&2
        echo "  ci          - CI suite: smoke + unit + integration" >&2
        echo "  all         - Everything except quarantine" >&2
        echo "" >&2
        exit 1
        ;;
esac
