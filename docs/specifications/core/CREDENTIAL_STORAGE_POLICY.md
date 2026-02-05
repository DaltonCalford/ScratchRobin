# Credential Storage Policy

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how ScratchRobin stores and retrieves credentials safely.

---

## Storage Rules

- Credentials are never stored in `project.srproj`.
- Connection strings in project files must be redacted.
- Secrets are stored in OS keychain or referenced via environment variables.

---

## Redaction Rules

- Passwords are replaced with `*****` in stored connection strings.
- A `credential_ref` is stored to rehydrate at runtime.

---

## Edge Cases

- If credential is missing, prompt user and store reference.
- If user declines, connection should fail gracefully.

