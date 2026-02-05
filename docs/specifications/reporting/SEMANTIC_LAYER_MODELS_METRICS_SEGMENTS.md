# Semantic Layer: Models, Metrics, Segments

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the semantic layer used by reporting to provide curated datasets, reusable metrics, and reusable filters.

---

## Models

A **Model** is a curated dataset definition for reporting. It can be:

- A table
- A view
- A SQL query

Models can hide columns, rename fields, add descriptions, and promote canonical joins.

Optional persistence:

- Models may be materialized as cached datasets for faster dashboards.
- Persistence is project-configurable and can be disabled for sensitive data.
- Persisted models must be explicitly refreshed or scheduled.

Discovery:

- Models are stored in collections and appear in search results.
- Models can be pinned or marked as official for discovery.

---

## Metrics

A **Metric** is a reusable aggregate definition built on a model.

Example:

- `total_revenue = SUM(orders.total)`

Notes:

- Metrics can define a default time dimension for use in dashboards.
- Metrics are stored in collections and inherit collection permissions.

---

## Segments

A **Segment** is a reusable filter definition.

Example:

- `active_customers = customers.status = 'active'`

Notes:

- Segments appear in the filter builder and can be applied to questions and dashboards.
- Segment changes should mark dependent questions for review.
- Segments are created by admins and scoped to a table or model.

---

## Usage Rules

- Questions can reference models, metrics, and segments.
- Dashboards can filter on model fields.
- Metrics and segments are read-only definitions; they do not modify data.
- Models should be the primary source for business-facing dashboards.

---

## Governance

- Only users with Design permission can create or edit models/metrics/segments.
- Changes are versioned as project objects.

---

## Logical Schema (Example)

```json
{
  "model": {
    "id": "m_orders",
    "name": "Orders",
    "source": "table:orders",
    "fields": ["order_id", "total", "created_at"]
  },
  "metric": {
    "id": "metric_revenue",
    "name": "Total Revenue",
    "expression": "SUM(orders.total)"
  },
  "segment": {
    "id": "segment_active",
    "name": "Active Customers",
    "expression": "customers.status = 'active'"
  }
}
```

---

## ScratchBird Constraints

- All models must resolve to ScratchBird objects.
- Expressions are parsed with ScratchBirdSQL.

---

## Related Specs

- `docs/specifications/reporting/QUESTIONS_AND_QUERY_BUILDER.md`
- `docs/specifications/reporting/DASHBOARDS_AND_INTERACTIVITY.md`
