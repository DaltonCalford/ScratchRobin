# ScratchRobin Project On-Disk Layout

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Overview

ScratchRobin stores project state in a single authoritative binary file at the project root, with optional human-readable exports and asset folders. This layout keeps projects portable while enabling fast load/save.

---

## Project Root Layout

Typical structure:

```
project-root/
  project.srproj
  designs/
  diagrams/
  whiteboards/
  mindmaps/
  docs/
  reports/
  tests/
  deployments/
```

### Core Files

- `project.srproj` — authoritative binary project file.
- `project.srproj.bak` — last known-good backup created during migrations or recovery.
- `project.srproj.tmp` — temporary file used for atomic save (should not persist).

### Optional Exports (Git-Friendly)

- `designs/` — per-object exports (e.g., table/view JSON or SQL); optional and regenerable.
- `docs/` — project documentation and notes.
- `tests/` — generated or hand-written tests.
- `deployments/` — deployment plans or scripts.
- `diagrams/`, `whiteboards/`, `mindmaps/` — visual assets.

---

## User-Level State (Outside Project Root)

These are not part of the project and should not be committed:

- Linux: `~/.local/state/scratchrobin/session.json`
- macOS: `~/Library/Application Support/ScratchRobin/session.json`
- Windows: `%LOCALAPPDATA%\ScratchRobin\session.json`

---

## Migration Strategy

When `project.srproj` has an older version:

1. Load using the last compatible reader.
2. Write a new `project.srproj.tmp` with the upgraded format.
3. Move current `project.srproj` to `project.srproj.bak`.
4. Atomically rename `project.srproj.tmp` to `project.srproj`.

If the file version is newer than the reader supports, open in read-only mode and prompt for an update.

---

## Save Safety

- All saves are atomic (write temp, fsync, rename).
- Partial writes should never replace the primary file.
- On startup, stale `project.srproj.tmp` files are treated as recoverable candidates.
