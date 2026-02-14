#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
REPORT_DIR="${2:-${BUILD_DIR}/reports}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "build directory not found: ${BUILD_DIR}" >&2
  exit 2
fi

mkdir -p "${REPORT_DIR}"

RAW_LOG="${REPORT_DIR}/ctest_verbose.log"
SUMMARY_JSON="${REPORT_DIR}/conformance_gate_summary.json"
TMP_SUMMARY="$(mktemp)"
TMP_CONFORMANCE="$(mktemp)"
trap 'rm -f "${TMP_SUMMARY}" "${TMP_CONFORMANCE}"' EXIT

set +e
ctest --test-dir "${BUILD_DIR}" -V --output-on-failure 2>&1 | tee "${RAW_LOG}"
CTEST_STATUS=${PIPESTATUS[0]}
set -e

grep 'SummaryJson:' "${RAW_LOG}" | grep -v 'ConformanceSummaryJson:' | sed 's/^.*SummaryJson: //' > "${TMP_SUMMARY}" || true
grep 'ConformanceSummaryJson:' "${RAW_LOG}" | sed 's/^.*ConformanceSummaryJson: //' > "${TMP_CONFORMANCE}" || true

{
  echo "{"
  echo "  \"generated_at_utc\": \"$(date -u +"%Y-%m-%dT%H:%M:%SZ")\","
  echo "  \"ctest_exit_code\": ${CTEST_STATUS},"
  echo "  \"summary_records\": ["
  first=1
  while IFS= read -r line; do
    [[ -z "${line}" ]] && continue
    if [[ ${first} -eq 0 ]]; then
      echo ","
    fi
    first=0
    printf "    %s" "${line}"
  done < "${TMP_SUMMARY}"
  echo
  echo "  ],"
  echo "  \"conformance_records\": ["
  first=1
  while IFS= read -r line; do
    [[ -z "${line}" ]] && continue
    if [[ ${first} -eq 0 ]]; then
      echo ","
    fi
    first=0
    printf "    %s" "${line}"
  done < "${TMP_CONFORMANCE}"
  echo
  echo "  ]"
  echo "}"
} > "${SUMMARY_JSON}"

echo "Conformance gate summary written: ${SUMMARY_JSON}"
exit "${CTEST_STATUS}"
