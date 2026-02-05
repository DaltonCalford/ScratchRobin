# Questions and Query Builder (ScratchBird Only)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how users create ad-hoc questions, edit queries, and save questions for reuse in dashboards and alerts.

---

## Question Lifecycle

1. Create a question from a table, model, or SQL query.
2. Configure visualization and filters.
3. Save to a collection.
4. Optionally add to a dashboard or create alerts.

Notes:

- Saving directly into a dashboard creates a dashboard-scoped question (not reusable elsewhere).
- Saving to a collection creates a reusable question object.

---

## Query Builder Modes

- **Simple mode**: Table/model selection, filter builder, group-by, aggregates.
- **Native SQL**: ScratchBirdSQL editor with full syntax support.

Rules:

- Simple mode must always be convertible to ScratchBirdSQL.
- Native SQL questions are marked `sql_mode: true` and are not converted back to the visual builder.
- Query builder should surface field metadata from the semantic layer (models, segments, metrics).

---

## Parameterization

Questions can define parameters used by dashboards or manual runs.

Supported parameter types:

- Text
- Number
- Date/Time
- Enum (list)
- Field filter (links to a specific column)

---

## Visualization Defaults

Default visualization is chosen based on the query shape:

- Single number -> KPI
- One dimension + one aggregate -> Bar/Line
- Multiple dimensions -> Table

Users can override the visualization.

---

## Validation Rules

- Queries must be read-only by default.
- Statements with side-effects require explicit admin policy override.
- Row limits default to 10,000 for ad-hoc runs (configurable per project).
- Saved questions preserve query history and last successful run metadata.

---

## Saved Question Schema (Logical)

```json
{
  "id": "uuid",
  "name": "New Signups",
  "collection_id": "uuid",
  "sql_mode": false,
  "query": {"type": "builder", "table": "users"},
  "visualization": {"type": "bar"},
  "parameters": [],
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## ScratchBird Constraints

- Only ScratchBird connections are supported.
- SQL dialect is ScratchBirdSQL.
- Stored queries are validated using the ScratchBird parser before save.

---

## Related Specs

- `docs/specifications/reporting/DASHBOARDS_AND_INTERACTIVITY.md`
- `docs/specifications/reporting/SEMANTIC_LAYER_MODELS_METRICS_SEGMENTS.md`
- `docs/specifications/diagramming/DIAGRAM_SERIALIZATION_FORMAT.md`
