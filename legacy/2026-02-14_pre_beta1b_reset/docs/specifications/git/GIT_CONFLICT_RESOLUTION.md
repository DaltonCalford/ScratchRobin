# Git Conflict Resolution Policy

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how ScratchRobin detects, represents, and resolves conflicts between the project repository and the ScratchBird database repository.

---

## Conflict Types

1. **Same object edited in both repos**
2. **Object deleted in one repo, modified in the other**
3. **Rename vs. modify**
4. **Schema divergence (DDL changed outside ScratchRobin)**

---

## Detection Rules

A conflict exists when:
- Object hash differs in both repos since last sync.
- Object missing in one repo but changed in the other.
- Migration history diverges for the same version ID.

---

## Resolution Policy

### Default Strategy

- Prefer **explicit user choice** for resolution.
- System never auto-merges DDL in v1.

### Deterministic Resolution Algorithm (v1)

When a conflict is detected, the system computes a default recommendation without auto-applying it. This recommendation is shown in the UI but must be confirmed by the user.

Inputs:

- Object state in project (`EXTRACTED/NEW/MODIFIED/DELETED/APPROVED/...`)
- Last sync hash (`base`)
- Project version (`project`)
- Database version (`database`)

Decision table (recommendation only):

| Condition | Recommendation |
| --- | --- |
| Project NEW, DB missing | Keep Project |
| Project DELETED, DB unchanged from base | Keep Project |
| Project unchanged, DB changed | Keep Database |
| Both changed (hash differs from base) | Manual Merge |
| Rename vs modify (ambiguous) | Manual Merge |
| Schema divergence (external DDL) | Manual Merge |

Rules:

- If object is APPROVED in project, recommend **Keep Project** unless DB has newer approved migration.
- If object is IMPLEMENTED in project but DB differs, recommend **Keep Database** and flag for review.
- If object is CONFLICTED, require manual merge.

### Resolution Options

| Option | Action |
| --- | --- |
| Keep Project | Overwrite DB repo with project version |
| Keep Database | Overwrite project version |
| Manual Merge | Open diff view and require explicit user merge |

---

## Conflict UI Requirements

- Show three-way diff: base, project, database.
- Require explicit user confirmation before applying.
- Provide audit log entry of resolution.

---

## Edge Cases

- **Delete vs modify**: default to manual merge; deletion requires explicit confirmation.
- **Rename vs modify**: treat as conflict if rename cannot be inferred; user must map.

---

## Implementation Notes

- Conflicts should move objects to CONFLICTED state.
- Resolution returns object to MODIFIED and sets `changed_by` and `changed_at`.
