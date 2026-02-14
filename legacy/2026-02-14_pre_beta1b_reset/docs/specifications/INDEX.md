# Specifications Index (Developer Onboarding)

**Last Updated**: 2026-02-05

This index provides a guided path through the specifications needed to understand ScratchRobin end-to-end.

Spec map diagram:
- `docs/specifications/SPEC_MAP.md`
- `docs/specifications/SPEC_MAP_LEGEND.md`
Workflow legend:
- `docs/specifications/SPEC_MAP_LEGEND.md`
Mirror notice:
- `docs/specifications/MIRROR_NOTICE.md`

---

## 1) Core Product and Architecture

- `docs/ARCHITECTURE.md`
- `docs/TARGET_FEATURES.md`
- `docs/ROADMAP.md`

---

## 2) Project Model and Persistence

- `docs/specifications/core/PROJECT_PERSISTENCE_FORMAT.md`
- `docs/specifications/core/PROJECT_ON_DISK_LAYOUT.md`
- `docs/specifications/core/PROJECT_MIGRATION_POLICY.md`
- `docs/specifications/core/PROJECT_DEFINITION_AND_GOVERNANCE.md`
- `docs/specifications/core/OBJECT_STATE_MACHINE.md`
- `docs/specifications/core/SESSION_STATE.md`
- `docs/specifications/core/SESSION_PROJECT_INTERACTION.md`

---

## 3) SQL Surface and DDL Generation

- `docs/specifications/sql/SCRATCHROBIN_SQL_SURFACE.md`
- `docs/specifications/sql/DDL_GENERATION_RULES.md`
- `docs/specifications/sql/managers/TABLES_SQL_SURFACE.md`
- `docs/specifications/sql/managers/VIEWS_SQL_SURFACE.md`
- `docs/specifications/sql/managers/INDEXES_SQL_SURFACE.md`
- `docs/specifications/sql/managers/TRIGGERS_SQL_SURFACE.md`
- `docs/specifications/sql/managers/PROCEDURES_FUNCTIONS_SQL_SURFACE.md`
- `docs/specifications/sql/managers/SEQUENCES_SQL_SURFACE.md`
- `docs/specifications/sql/managers/SCHEMAS_SQL_SURFACE.md`
- `docs/specifications/sql/managers/DOMAINS_SQL_SURFACE.md`

---

## 4) Git and Migration Workflow

- `docs/GIT_INTEGRATION.md`
- `docs/DATABASE_GIT_PROTOCOL.md`
- `docs/specifications/git/GIT_CONFLICT_RESOLUTION.md`
- `docs/specifications/deployment/MIGRATION_GENERATION.md`

---

## 5) Deployment and Rollback

- `docs/specifications/deployment/DEPLOYMENT_PLAN_FORMAT.md`
- `docs/specifications/deployment/ROLLBACK_POLICY.md`
- `docs/specifications/core/TRANSACTION_MANAGEMENT.md`

---

## 6) UI and Manager Surfaces

### Core UI Architecture
- `docs/specifications/ui/DOCKABLE_WORKSPACE_ARCHITECTURE.md` - **Dockable/floating workspace system**
- `docs/specifications/ui/UI_WINDOW_MODEL.md` - Base window model
- `docs/specifications/ui/UI_ICON_ASSETS.md` - Icon system

### User Interaction
- `docs/specifications/ui/KEYBOARD_SHORTCUTS.md` - Complete keyboard reference
- `docs/specifications/ui/ACCESSIBILITY_AND_INPUT.md` - Accessibility features
- `docs/specifications/ui/CROSS_PLATFORM_UI_CONSTRAINTS.md` - Platform considerations
- `docs/specifications/ui/STATUS_RESPONSE_SCHEMA.md` - Status bar and responses

Manager UI specs:
- `docs/specifications/managers/SCHEMA_MANAGER_UI.md`
- `docs/specifications/managers/TABLE_DESIGNER_UI.md`
- `docs/specifications/managers/INDEX_DESIGNER_UI.md`
- `docs/specifications/managers/VIEW_MANAGER_UI.md`
- `docs/specifications/managers/TRIGGER_MANAGER_UI.md`
- `docs/specifications/managers/SEQUENCE_MANAGER_UI.md`
- `docs/specifications/managers/PROCEDURE_MANAGER_UI.md`
- `docs/specifications/managers/DOMAIN_MANAGER_UI.md`
- `docs/specifications/managers/JOB_SCHEDULER_UI.md`
- `docs/specifications/managers/DATABASE_ADMINISTRATION_SPEC.md`
- `docs/specifications/managers/BACKUP_RESTORE_UI.md`
- `docs/specifications/managers/CLUSTER_MANAGER_UI.md`
- `docs/specifications/managers/REPLICATION_MANAGER_UI.md`
- `docs/specifications/managers/ETL_MANAGER_UI.md`

---

## 6.5) Reporting and Live Analytics

- `docs/specifications/reporting/REPORTING_OVERVIEW.md`
- `docs/specifications/reporting/QUESTIONS_AND_QUERY_BUILDER.md`
- `docs/specifications/reporting/DASHBOARDS_AND_INTERACTIVITY.md`
- `docs/specifications/reporting/SEMANTIC_LAYER_MODELS_METRICS_SEGMENTS.md`
- `docs/specifications/reporting/COLLECTIONS_AND_PERMISSIONS.md`
- `docs/specifications/reporting/ALERTS_AND_SUBSCRIPTIONS.md`
- `docs/specifications/reporting/CACHING_AND_REFRESH.md`
- `docs/specifications/reporting/REPORTING_EXECUTION_ENGINE.md`
- `docs/specifications/reporting/EMBEDDING_AND_SHARING.md`
- `docs/specifications/reporting/EVENTS_AND_TIMELINES.md`
- `docs/specifications/reporting/DATA_REFERENCE.md`
- `docs/specifications/reporting/REPORTING_STORAGE_FORMAT.md`
- `docs/specifications/reporting/reporting_storage_format.json`

---

## 7) Diagramming

- `docs/specifications/diagramming/ERD_MODELING_AND_ENGINEERING.md`
- `docs/specifications/diagramming/SILVERSTON_DIAGRAM_SPECIFICATION.md`
- `docs/specifications/diagramming/DIAGRAM_SERIALIZATION_FORMAT.md`
- `docs/specifications/diagramming/DIAGRAM_ROUNDTRIP_RULES.md`
- `docs/specifications/diagramming/MIND_MAP_SPECIFICATION.md`
- `docs/specifications/diagramming/WHITEBOARD_SPECIFICATION.md`
- `docs/specifications/diagramming/DATAFLOW_DIAGRAM_SPECIFICATION.md`
- `docs/specifications/diagramming/DIAGRAM_STYLE_SYSTEM.md`
- `docs/specifications/diagramming/CROSS_DIAGRAM_EMBEDDING.md`
- `docs/specifications/diagramming/SILVERSTON_OBJECT_CATALOG.md`

---

## 8) Testing and QA

- `docs/specifications/testing/TEST_GENERATION_RULES.md`
- `docs/specifications/testing/TEST_FIXTURE_STRATEGY.md`
- `docs/specifications/testing/PERFORMANCE_TARGETS.md`
- `docs/specifications/testing/PERFORMANCE_BENCHMARKS.md`

---

## 9) Security and Audit

- `docs/specifications/core/CREDENTIAL_STORAGE_POLICY.md`
- `docs/specifications/core/AUDIT_LOGGING.md`
- `docs/specifications/core/ERROR_HANDLING.md`

---

## 10) Integrations

- `docs/ISSUE_TRACKER_SPECIFICATION.md`
- `docs/specifications/integrations/AI_INTEGRATION_SCOPE.md`

---

## 10.5) Automated Documentation

- `docs/specifications/documentation/AUTOMATED_DOCUMENTATION_SYSTEM.md`
- `docs/specifications/documentation/GENERATED_METHODS_AND_OPERATIONS_SPEC.md`

---

## 11) ScratchBirdSQL Dialect Reference

- `docs/specifications/dialect/scratchbird/parser/README.md`
- `docs/specifications/dialect/scratchbird/catalog/README.md`
- `docs/specifications/dialect/scratchbird/parser/SCRATCHBIRD_SQL_COMPLETE_BNF.md`
- `docs/specifications/dialect/scratchbird/parser/ScratchBird SQL Language Specification - Master Document.md`
- `docs/specifications/dialect/scratchbird/beta/BETA_SQL2023_IMPLEMENTATION_SPECIFICATION.md`
- `docs/specifications/dialect/scratchbird/alpha/alpha_1_05_design_synthesis.md`
- `docs/specifications/dialect/scratchbird/alpha/alpha_1_05_sql_parser_design_decisions.md`
- `docs/specifications/dialect/scratchbird/alpha/alpha_1_05_outstanding_decisions.md`
