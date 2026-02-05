# ScratchRobin Live Reporting Overview (ScratchBird Only)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines ScratchRobin's live reporting and analytics layer for ScratchBird databases, including query-driven reporting, dashboards, semantic modeling, and sharing. This subsystem enables in-app reporting that mirrors the convenience and end-user workflows of modern BI tools while remaining project-governed and versionable.

---

## Scope

In scope:

- Ad-hoc questions, visualizations, and saved reports
- Dashboards with filters and drill behavior
- Semantic layer (models, metrics, segments)
- Alerts and scheduled subscriptions
- Report caching and refresh policies
- Sharing and embedding
- Data reference and metadata browsing

Out of scope:

- Non-ScratchBird databases or connectors
- External BI integrations (beyond export/share)

---

## Core Principles

- ScratchBird-only connections; no multi-database federation.
- Every report is a project asset with audit history and governance controls.
- Live reporting is read-only unless explicitly authorized by project policy.
- Reports are versioned, shareable, and reproducible.

---

## Artifact Types

- **Question**: A saved query with visualization settings.
- **Dashboard**: A collection of cards with filters and layout.
- **Model**: A curated dataset or view definition.
- **Metric**: A reusable aggregate definition.
- **Segment**: A reusable filter definition.
- **Alert**: Condition-based notification for a question.
- **Subscription**: Scheduled delivery of dashboard or question.

---

## Persistence Model

Reporting assets are stored in the project as governed objects and can be exported as JSON for version control. The authoritative storage for queries and dashboards lives in the project file (`project.srproj`) with optional exports in `docs/` or `designs/`.

---

## Related Specs

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

---

## Governance

All reporting actions are governed by project roles and approval policies. See:

- `docs/specifications/core/PROJECT_DEFINITION_AND_GOVERNANCE.md`
