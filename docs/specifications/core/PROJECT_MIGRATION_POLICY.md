# Project Migration Policy

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how ScratchRobin upgrades older project files to newer formats while preserving data and integrity.

---

## Versioning Model

- Project file format version is `Major.Minor`.
- Major changes require explicit migration logic or read-only fallback.
- Minor changes are backward compatible by ignoring unknown fields.

---

## Migration Process

1. Detect version on load.
2. If older and compatible, perform in-memory upgrade.
3. Write new file to `project.srproj.tmp`.
4. Move old file to `project.srproj.bak`.
5. Atomically replace.

---

## Incompatible Versions

If project file is newer than supported:
- Open in read-only mode.
- Show warning and require upgrade of ScratchRobin.

---

## Example Migration

### v1.0 → v1.1
- Add `source_connection_id` in `META` snapshot.
- Loader sets empty UUID if absent.

---

## Edge Cases

- **Partial corruption**: skip corrupted chunks and warn; if required chunks missing, fail open.
- **Interrupted migration**: if `.tmp` exists, attempt recovery; if `.bak` exists, allow user restore.

---

## Implementation Notes

- Each migration should be unit-tested with sample files.
- Migration steps should be deterministic and idempotent.

