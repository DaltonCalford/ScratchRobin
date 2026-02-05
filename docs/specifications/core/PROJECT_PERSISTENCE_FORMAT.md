# ScratchRobin Project Persistence Format (Binary)

**Status**: Draft (Requested)  
**Last Updated**: 2026-02-05  
**Primary Implementation Targets**: `Project::SaveProjectFile`, `Project::LoadProjectFile`

---

## Quick Reference

| Item | Value |
| --- | --- |
| File | `project.srproj` |
| Magic | `SRPJ` |
| Header Size (v1) | 44 bytes |
| TOC Entry Size (v1) | 40 bytes |
| Endianness (v1) | Little (`1`) |
| Required Chunks | `PROJ`, `OBJS` |
| Optional Chunks | `META`, `GITS`, `STBL`, `EXTR` |
| String Encoding | UTF-8 (`uvarint length` + bytes) |
| Integer Encoding | LE fixed width, `uvarint` inside chunks |
| UUID Encoding | 16 raw bytes |

---

## Overview

ScratchRobin projects are persisted to a compact binary file to ensure fast load/save and efficient storage. The file is designed for forward compatibility via a chunked container format and versioned schemas.

ScratchRobin SQL content stored in this format uses **ScratchBirdSQL** as the authoritative dialect. See ScratchBird grammar specifications in:
- `ScratchBird/docs/specifications/parser/SCRATCHBIRD_SQL_COMPLETE_BNF.md`
- `ScratchBird/docs/specifications/parser/ScratchBird SQL Language Specification - Master Document.md`

---

## Goals

- Compact on disk, fast to read/write.
- Forward compatible and resilient to partial corruption.
- Supports hybrid schema sources (internal model + live DB introspection snapshot).
- Enables future compression/encryption without breaking the base format.

---

## File Naming and Location

- **Project file name**: `project.srproj`
- **Location**: Project root directory (same folder as `designs/`, `docs/`, `tests/`, etc.)

The `.srproj` file is authoritative. Any per-object files in `designs/` are optional exports for Git or interoperability.

---

## Container Format

### Header (Fixed Size, 44 bytes)

| Field | Size | Type | Notes |
| --- | --- | --- | --- |
| Magic | 4 | bytes | `SRPJ` |
| Version Major | 2 | uint16 LE | Start at `1` |
| Version Minor | 2 | uint16 LE | Start at `0` |
| Endianness | 1 | uint8 | `1` = little, `2` = big (v1 only little) |
| Flags | 1 | uint8 | Bit 0: file encrypted, Bit 1: chunk compression, Bit 2: string table present |
| Header Size | 2 | uint16 LE | For future expansion |
| TOC Entry Size | 2 | uint16 LE | Size of a TOC entry (v1 = 40) |
| Reserved | 2 | bytes | Zero in v1 |
| File Length | 8 | uint64 LE | Total file size |
| Header CRC32 | 4 | uint32 LE | CRC of header bytes excluding this field |
| TOC Offset | 8 | uint64 LE | Byte offset to chunk table of contents |
| TOC Count | 4 | uint32 LE | Number of chunks |
| Reserved2 | 4 | bytes | Zero in v1 |

### Chunk Table of Contents (TOC)

Each TOC entry is fixed-size (40 bytes):

| Field | Size | Type | Notes |
| --- | --- | --- | --- |
| Chunk ID | 4 | bytes | FourCC (e.g., `PROJ`) |
| Offset | 8 | uint64 LE | Offset from start of file |
| Length | 8 | uint64 LE | Chunk data length |
| Uncompressed Length | 8 | uint64 LE | If compressed, else 0 |
| CRC32 | 4 | uint32 LE | CRC of chunk data |
| Chunk Flags | 2 | uint16 LE | Bit 0: compressed, Bit 1: uses string table |
| Reserved | 6 | bytes | Zero in v1 |

Chunks may appear in any order. Unknown chunks are skipped.

---

## Encoding Rules

- **Integer encoding**: little-endian fixed width (u8/u16/u32/u64). For compactness inside chunks, use **uvarint** (LEB128) where noted.
- **Strings**: UTF-8. Encoded as `uvarint length` + bytes.
- **Booleans**: `u8` (0 or 1).
- **Time**: `int64` UNIX epoch seconds (UTC) unless specified.
- **UUID**: 16 raw bytes (same ordering as `UUID::data`).
- **Length-prefixed records**: Records that may evolve (ProjectObject, MetadataNode) start with `uvarint record_length` so newer fields can be skipped.
- **String table usage**: If `STBL` is present and a chunk sets `Chunk Flags` bit 1, strings in that chunk are stored as `uvarint string_id` (0 = empty string). Otherwise they are raw strings.

---

## Chunk Inventory (v1)

| Chunk | Purpose | Required |
| --- | --- | --- |
| `PROJ` | Project header + config | Yes |
| `OBJS` | Project objects | Yes |
| `META` | Metadata snapshots (design + introspected) | Optional (required for hybrid) |
| `GITS` | Git sync state | Optional |
| `STBL` | String table (for de-dup) | Optional |
| `EXTR` | Extensible extras (UI or plugin data) | Optional |
| `RPTG` | Reporting assets (questions, dashboards, models, alerts, etc.) | Optional |
| `DVWS` | Diagram data views (query snapshots) | Optional |

---

## Chunk Definitions

### `PROJ` (Project Header + Config)

Fields (in order):
1. `project_id` (UUID)
2. `name` (string)
3. `description` (string)
4. `version` (string)
5. `database_type` (string) - `scratchbird` in v1
6. `created_at` (int64, optional; 0 if unknown)
7. `updated_at` (int64, optional; 0 if unknown)
8. `paths` (struct)
   - `designs_path` (string)
   - `diagrams_path` (string)
   - `whiteboards_path` (string)
   - `mindmaps_path` (string)
   - `docs_path` (string)
   - `tests_path` (string)
   - `deployments_path` (string)
   - `reports_path` (string)
9. `connections` (array)
   - `connection_id` (UUID, optional; zeroed if unknown)
   - `name` (string)
   - `backend_type` (string)
   - `connection_string` (string, may be redacted)
   - `credential_ref` (string, optional)
   - `is_source` (bool)
   - `is_target` (bool)
   - `git_branch` (string)
   - `requires_approval` (bool)
   - `is_git_enabled` (bool)
   - `git_repo_url` (string)
10. `git_config` (struct)
   - `enabled` (bool)
   - `repo_url` (string)
   - `default_branch` (string)
   - `workflow` (string)
   - `sync_mode` (string)
   - `auto_sync_branches` (array of string)
   - `protected_branches` (array of string)
   - `require_conventional_commits` (bool)
   - `auto_sync_messages` (bool)
11. `governance` (struct)
   - `owners` (array of string)
   - `stewards` (array of string)
   - `environments` (array)
     - `id` (string)
     - `name` (string)
     - `approval_required` (bool)
     - `min_reviewers` (uvarint)
     - `allowed_roles` (array of string)
   - `compliance_tags` (array of string)
   - `review_policy` (struct)
     - `min_reviewers` (uvarint)
     - `required_roles` (array of string)
     - `approval_window_hours` (uvarint)
   - `ai_policy` (struct)
     - `enabled` (bool)
     - `requires_review` (bool)
     - `allowed_scopes` (array of string)
     - `prohibited_scopes` (array of string)
   - `audit_policy` (struct)
     - `log_level` (string)
     - `retain_days` (uvarint)
     - `export_target` (string)

**Credential handling**:
- If `connection_string` contains secrets, they must be **redacted** and accompanied by a `credential_ref` (e.g., keychain entry, env var reference).
- If no redaction is needed, `credential_ref` may be empty.

---

## Project Governance Serialization

Governance metadata is stored inside the `PROJ` chunk as the `governance` struct. This data is authoritative for role gates, approvals, and policy enforcement at runtime.

### Governance Struct Encoding (v1)

All fields are encoded using existing rules:

- strings: `uvarint length` + UTF-8 bytes
- arrays: `uvarint count` + repeated elements
- bool: `u8` (0/1)

Order of fields in `governance` (v1):

1. `owners` (array of string)
2. `stewards` (array of string)
3. `environments` (array of struct)
4. `compliance_tags` (array of string)
5. `review_policy` (struct)
6. `ai_policy` (struct)
7. `audit_policy` (struct)

### Environment Struct (v1)

1. `id` (string)
2. `name` (string)
3. `approval_required` (bool)
4. `min_reviewers` (uvarint)
5. `allowed_roles` (array of string)

### Review Policy Struct (v1)

1. `min_reviewers` (uvarint)
2. `required_roles` (array of string)
3. `approval_window_hours` (uvarint)

### AI Policy Struct (v1)

1. `enabled` (bool)
2. `requires_review` (bool)
3. `allowed_scopes` (array of string)
4. `prohibited_scopes` (array of string)

### Audit Policy Struct (v1)

1. `log_level` (string)
2. `retain_days` (uvarint)
3. `export_target` (string)

### `OBJS` (Project Objects)

Format:
- `uvarint count`
- Repeated object record:
  - `record_length` (uvarint, bytes following in this record)
  - `object_id` (UUID)
  - `kind` (string)
  - `name` (string)
  - `path` (string)
  - `schema_name` (string)
  - `design_state` (struct)
    - `state` (u8 enum, see ObjectState mapping)
    - `changed_by` (string)
    - `changed_at` (int64)
    - `reason` (string)
    - `review_comment` (string)
  - `has_source` (bool)
  - `source_snapshot` (MetadataNode, if `has_source` true)
  - `current_design` (MetadataNode)
  - `comments` (array)
    - `author` (string)
    - `timestamp` (int64)
    - `text` (string)
    - `resolved` (bool)
  - `change_history` (array)
    - `field` (string)
    - `old_value` (string)
    - `new_value` (string)
    - `timestamp` (int64)
    - `author` (string)
  - `design_file_path` (string)

**ObjectState mapping (v1):** 0=EXTRACTED, 1=NEW, 2=MODIFIED, 3=DELETED, 4=PENDING, 5=APPROVED, 6=REJECTED, 7=IMPLEMENTED, 8=CONFLICTED

### `META` (Metadata Snapshots)

Contains two optional snapshots:
- `design_snapshot` (internal model at last save)
- `introspection_snapshot` (last DB extraction)

Format:
- `u8 flags` (bit 0: design snapshot, bit 1: introspection snapshot, bit 2: source connection info present)
- `source_connection_id` (UUID, optional if flag bit 2 set)
- `source_connection_name` (string, optional if flag bit 2 set)
- For each present snapshot:
  - `timestamp` (int64)
  - `node_count` (uvarint)
  - repeated `MetadataNode`

### `MetadataNode` Encoding

Fields (in order):
1. `record_length` (uvarint, bytes following in this record)
2. `id` (uvarint)
3. `type` (u8 enum, see MetadataType mapping)
4. `label` (string)
5. `kind` (string)
6. `catalog` (string)
7. `path` (string)
8. `ddl` (string) - ScratchBirdSQL
9. `dependencies` (array of string)
10. `child_count` (uvarint)
11. `children` (recursive MetadataNode)
12. `name` (string)
13. `schema` (string)
14. `parent_id` (uvarint)
15. `row_count` (int64)

**MetadataType mapping (v1):** 0=Schema, 1=Table, 2=View, 3=Procedure, 4=Function, 5=Trigger, 6=Index, 7=Column, 8=Constraint

### `GITS` (Git Sync State)

Fields (in order):
1. `last_sync` (int64)
2. `project_repo` (struct)
   - `head_commit` (string)
   - `branch` (string)
   - `dirty_files` (array of string)
3. `database_repo` (struct)
   - `head_commit` (string)
   - `branch` (string)
   - `dirty_files` (array of string)
4. `mappings` (array)
   - `project_file` (string)
   - `db_object` (string)
   - `last_sync_commit` (string)
   - `sync_status` (string)
5. `pending` (struct)
   - `project_to_db` (array of string)
   - `db_to_project` (array of string)
   - `conflicts` (array of string)

### `RPTG` (Reporting Assets)

Stores reporting artifacts serialized as JSON documents using:

- `docs/specifications/reporting/REPORTING_STORAGE_FORMAT.md`
- `docs/specifications/reporting/reporting_storage_format.json`

Format:

- `uvarint count`
- repeated:
  - `object_id` (UUID)
  - `object_type` (string)  # question | dashboard | collection | model | metric | segment | alert | subscription | timeline
  - `json_length` (uvarint)
  - `json_payload` (bytes, UTF-8 JSON)

### `DVWS` (Diagram Data Views)

Stores Data View panels attached to diagrams, including query snapshots.

Format:

- `uvarint count`
- repeated:
  - `data_view_id` (UUID)
  - `diagram_id` (UUID)
  - `json_length` (uvarint)
  - `json_payload` (bytes, UTF-8 JSON)

The JSON payload for `DVWS` must conform to the `data_view` schema defined in:

- `docs/specifications/diagramming/SILVERSTON_DIAGRAM_SPECIFICATION.md`
- `docs/specifications/diagramming/DIAGRAM_SERIALIZATION_FORMAT.md`

### `STBL` (String Table, Optional)

- `uvarint string_count`
- Repeated `string` entries

If present, chunks may store string IDs instead of raw strings. This is a v1 optional optimization, enabled per chunk via `Chunk Flags` bit 1 in the TOC entry.

### `EXTR` (Extras)

Freeform extension records for plugins or UI state that is not part of the core project model. Each record:
- `ext_id` (string)
- `payload_length` (uvarint)
- `payload` (bytes)

---

## Versioning Rules

- **Major version** changes for incompatible structure changes.
- **Minor version** changes for additive changes (new chunks or new fields with defaults).
- Readers must ignore unknown chunks and unknown fields when possible.
- Writers should preserve unknown chunks during rewrite (copy-through when loading and saving).
- For records with `record_length`, readers must skip any trailing bytes beyond known fields.

---

## Hybrid Schema Source Behavior

When hybrid mode is enabled:
- `current_design` fields are authoritative for edits.
- `introspection_snapshot` is used to reconcile differences and detect drift.
- Sync operations should record the snapshot timestamp and source connection info in `META` for auditability.

---

## Integrity and Recovery

- Each chunk is CRC-protected.
- If a chunk fails CRC, the loader should skip it and warn the user.
- If `PROJ` or `OBJS` is missing or invalid, the project is considered corrupted and should not open.

---

## Implementation Notes (v1)

- Compression/encryption flags are reserved but not required for v1.
- For compactness, `uvarint` is recommended for all counts and IDs.
- Keep `design_file_path` empty for objects that are fully embedded in the project file.
