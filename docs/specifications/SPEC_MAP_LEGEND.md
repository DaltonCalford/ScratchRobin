# Spec Map Legend

**Last Updated**: 2026-02-05

This legend explains each workflow in the Spec Map and the specification cluster it references.

---

## Project Setup

Covers project creation, persistence, and recovery.
- Core specs: `core/PROJECT_PERSISTENCE_FORMAT.md`, `core/PROJECT_ON_DISK_LAYOUT.md`, `core/PROJECT_MIGRATION_POLICY.md`

---

## Design Changes

Covers state transitions and SQL generation.
- Core: `core/OBJECT_STATE_MACHINE.md`
- SQL: `sql/SCRATCHROBIN_SQL_SURFACE.md`, `sql/DDL_GENERATION_RULES.md`
- Per-manager SQL surface in `sql/managers/`

---

## Git Sync

Covers bidirectional synchronization and conflict policy.
- `git/GIT_CONFLICT_RESOLUTION.md`
- `deployment/MIGRATION_GENERATION.md`

---

## Deploy

Covers deployment plans, execution, and rollback.
- `deployment/DEPLOYMENT_PLAN_FORMAT.md`
- `deployment/ROLLBACK_POLICY.md`
- `core/TRANSACTION_MANAGEMENT.md`

---

## Diagramming

Covers diagram creation, storage, and sync.
- `diagramming/ERD_MODELING_AND_ENGINEERING.md`
- `diagramming/DIAGRAM_SERIALIZATION_FORMAT.md`
- `diagramming/DIAGRAM_ROUNDTRIP_RULES.md`

---

## Testing & QA

Covers automated tests, fixtures, and performance targets.
- `testing/TEST_GENERATION_RULES.md`
- `testing/TEST_FIXTURE_STRATEGY.md`
- `testing/PERFORMANCE_TARGETS.md`

---

## Security

Covers credentials and auditability.
- `core/CREDENTIAL_STORAGE_POLICY.md`
- `core/AUDIT_LOGGING.md`

---

## Dialect Reference

Authoritative ScratchBirdSQL grammar and dialect expansions.
- `dialect/scratchbird/parser/`
- `dialect/scratchbird/alpha/`
- `dialect/scratchbird/beta/`
- `dialect/scratchbird/catalog/`

