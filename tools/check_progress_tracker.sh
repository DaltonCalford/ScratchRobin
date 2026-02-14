#!/usr/bin/env bash
set -euo pipefail

if [[ "${ALLOW_NO_TRACKER_UPDATE:-0}" == "1" ]]; then
  echo "Skipping tracker check because ALLOW_NO_TRACKER_UPDATE=1"
  exit 0
fi

usage() {
  cat <<'EOF'
Usage:
  ./tools/check_progress_tracker.sh --staged
  ./tools/check_progress_tracker.sh --range <git-range>

Rules:
  If implementation files changed (src/, tests/, CMakeLists.txt), then both tracker files
  must also be changed:
    docs/conformance/BETA1B_IMPLEMENTATION_PROGRESS_TRACKER.csv
    docs/conformance/BETA1B_IMPLEMENTATION_STEP_LOG.csv
EOF
}

mode="${1:---staged}"
changed=""
if [[ "${mode}" == "--staged" ]]; then
  changed="$(git diff --cached --name-only)"
elif [[ "${mode}" == "--range" ]]; then
  range="${2:-}"
  if [[ -z "${range}" ]]; then
    usage
    exit 2
  fi
  changed="$(git diff --name-only "${range}")"
else
  usage
  exit 2
fi

if [[ -z "${changed}" ]]; then
  echo "No changed files detected; tracker check passed."
  exit 0
fi

impl_changed=0
while IFS= read -r path; do
  [[ -z "${path}" ]] && continue
  if [[ "${path}" == src/* ]] || [[ "${path}" == tests/* ]] || [[ "${path}" == "CMakeLists.txt" ]]; then
    impl_changed=1
    break
  fi
done <<< "${changed}"

if [[ "${impl_changed}" -eq 0 ]]; then
  echo "No implementation-surface changes detected; tracker check passed."
  exit 0
fi

tracker_file="docs/conformance/BETA1B_IMPLEMENTATION_PROGRESS_TRACKER.csv"
step_log_file="docs/conformance/BETA1B_IMPLEMENTATION_STEP_LOG.csv"

missing=0
if ! grep -Fxq "${tracker_file}" <<< "${changed}"; then
  echo "Missing tracker update: ${tracker_file}" >&2
  missing=1
fi
if ! grep -Fxq "${step_log_file}" <<< "${changed}"; then
  echo "Missing step-log update: ${step_log_file}" >&2
  missing=1
fi

if [[ "${missing}" -ne 0 ]]; then
  cat <<'EOF' >&2
Progress tracking check failed.
Run tools/update_progress_tracker.py for the queue item worked in this step,
then stage both tracker files and commit again.
EOF
  exit 3
fi

echo "Tracker check passed."
