# Reporting Storage Format (ScratchBird Only)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the consolidated JSON storage format for reporting artifacts used by ScratchRobin. These schemas are the canonical serialized form for export, version control, and interchange. The authoritative storage remains in the project file (`project.srproj`), but this format governs any exported JSON representation.

Schema bundle:

- `docs/specifications/reporting/reporting_storage_format.json`

---

## Global Conventions

- All objects include `id`, `name`, `created_at`, `updated_at`, and `collection_id` where applicable.
- All timestamps are ISO-8601 UTC.
- `connection_ref` always points to a ScratchBird-only connection.
- `sql` must be ScratchBirdSQL.

---

## 1. Question Schema

```json
{
  "$schema": "https://scratchrobin.local/schemas/reporting/question.json",
  "id": "uuid",
  "name": "New Signups",
  "collection_id": "uuid",
  "description": "Optional description",
  "sql_mode": false,
  "query": {
    "type": "builder",
    "source": "table:users",
    "filters": [],
    "aggregations": [],
    "breakouts": []
  },
  "sql": null,
  "parameters": [],
  "visualization": {
    "type": "bar",
    "options": {}
  },
  "last_run": {
    "at": "2026-02-05T12:30:00Z",
    "row_count": 1200
  },
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## 2. Dashboard Schema

```json
{
  "$schema": "https://scratchrobin.local/schemas/reporting/dashboard.json",
  "id": "uuid",
  "name": "Sales Overview",
  "collection_id": "uuid",
  "description": "Optional description",
  "cards": [
    {
      "id": "card_1",
      "question_id": "q1",
      "position": {"x": 0, "y": 0, "w": 6, "h": 4},
      "visualization_override": null
    }
  ],
  "filters": [
    {
      "id": "f1",
      "type": "date_range",
      "targets": ["q1:created_at"],
      "required": true
    }
  ],
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## 3. Collection Schema

```json
{
  "$schema": "https://scratchrobin.local/schemas/reporting/collection.json",
  "id": "uuid",
  "name": "Project Analytics",
  "parent_id": null,
  "description": "Default analytics collection",
  "status": "official",
  "permissions": {
    "view": ["role:viewer", "role:designer"],
    "curate": ["role:designer"],
    "admin": ["role:owner"]
  },
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## 4. Model Schema

```json
{
  "$schema": "https://scratchrobin.local/schemas/reporting/model.json",
  "id": "uuid",
  "name": "Orders Model",
  "collection_id": "uuid",
  "source": "table:orders",
  "fields": [
    {"name": "order_id", "type": "UUID", "visible": true},
    {"name": "total", "type": "DECIMAL(10,2)", "visible": true}
  ],
  "joins": [],
  "description": "Curated Orders dataset",
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## 5. Alert Schema

```json
{
  "$schema": "https://scratchrobin.local/schemas/reporting/alert.json",
  "id": "uuid",
  "question_id": "q1",
  "name": "Revenue Alert",
  "condition": {
    "operator": "<",
    "value": 10000
  },
  "schedule": "hourly",
  "channels": ["email"],
  "only_on_change": true,
  "enabled": true,
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## 6. Subscription Schema

```json
{
  "$schema": "https://scratchrobin.local/schemas/reporting/subscription.json",
  "id": "uuid",
  "name": "Daily Sales Digest",
  "target_type": "dashboard",
  "target_id": "dash_001",
  "schedule": "daily",
  "filters": {"date_range": "last_7_days"},
  "channels": ["email"],
  "include_csv": false,
  "enabled": true,
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## 7. Timeline Schema

```json
{
  "$schema": "https://scratchrobin.local/schemas/reporting/timeline.json",
  "id": "uuid",
  "name": "Production Events",
  "collection_id": "uuid",
  "description": "Operational events for dashboards",
  "events": [
    {
      "id": "event_001",
      "type": "deployment",
      "timestamp": "2026-02-05T10:00:00Z",
      "title": "Deploy v1.2",
      "description": "Deployed billing schema updates"
    }
  ],
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## 8. Segment Schema

```json
{
  "$schema": "https://scratchrobin.local/schemas/reporting/segment.json",
  "id": "uuid",
  "name": "Active Customers",
  "collection_id": "uuid",
  "expression": "customers.status = 'active'",
  "scope": "table:customers",
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## 9. Metric Schema

```json
{
  "$schema": "https://scratchrobin.local/schemas/reporting/metric.json",
  "id": "uuid",
  "name": "Total Revenue",
  "collection_id": "uuid",
  "expression": "SUM(orders.total)",
  "model_ref": "model_orders",
  "default_time_dimension": "orders.created_at",
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:20:00Z"
}
```

---

## Related Specs

- `docs/specifications/reporting/REPORTING_OVERVIEW.md`
- `docs/specifications/reporting/QUESTIONS_AND_QUERY_BUILDER.md`
- `docs/specifications/reporting/DASHBOARDS_AND_INTERACTIVITY.md`
- `docs/specifications/reporting/COLLECTIONS_AND_PERMISSIONS.md`
- `docs/specifications/reporting/SEMANTIC_LAYER_MODELS_METRICS_SEGMENTS.md`
- `docs/specifications/reporting/ALERTS_AND_SUBSCRIPTIONS.md`
