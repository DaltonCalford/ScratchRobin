#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
hook_path="${repo_root}/.git/hooks/pre-commit"

cat > "${hook_path}" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
repo_root="$(git rev-parse --show-toplevel)"
"${repo_root}/tools/check_progress_tracker.sh" --staged
EOF

chmod +x "${hook_path}"
echo "Installed pre-commit hook: ${hook_path}"
