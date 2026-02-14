# Diagram Serialization Format

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the on-disk format for diagram files (ERD and related diagrams), enabling portability and versioning.

---

## File Extension

- `.sberd` for ERD diagrams
- `.sbdfd` for Data Flow diagrams
- `.sbwb` for Whiteboard diagrams
- `.sbmind` for Mind Map diagrams
- `.sbdgm` for generic diagrams (including Silverston and future diagram types)

---

## Format (JSON v1)

```json
{
  "version": "1.0",
  "created_at": "2026-02-05T12:00:00Z",
  "updated_at": "2026-02-05T12:10:00Z",
  "diagram_type": "erd",
  "layout": {
    "algorithm": "sugiyama",
    "zoom": 1.0,
    "viewport": {"x": 0, "y": 0, "width": 1200, "height": 800}
  },
  "nodes": [
    {
      "id": "table_users",
      "type": "table",
      "name": "users",
      "schema": "public",
      "position": {"x": 120, "y": 80},
      "size": {"width": 220, "height": 160},
      "columns": [
        {"name": "id", "type": "INT64", "pk": true},
        {"name": "email", "type": "VARCHAR(255)"}
      ]
    }
  ],
  "edges": [
    {
      "id": "fk_orders_users",
      "type": "foreign_key",
      "from": "table_orders",
      "to": "table_users",
      "columns": ["user_id"],
      "target_columns": ["id"],
      "style": {"notation": "crows_foot"}
    }
  ],
  "data_views": [
    {
      "id": "dv_recent_orders",
      "name": "Recent Orders",
      "query": "SELECT order_id, customer_id, total, created_at FROM orders ORDER BY created_at DESC",
      "query_refs": ["orders"],
      "connection_ref": "conn_prod",
      "captured_at": "2026-02-05T13:05:00Z",
      "max_rows": 10,
      "columns": [
        {"name": "order_id", "type": "UUID"},
        {"name": "customer_id", "type": "UUID"},
        {"name": "total", "type": "DECIMAL(10,2)"},
        {"name": "created_at", "type": "TIMESTAMP"}
      ],
      "rows": [
        ["b1", "c9", "120.00", "2026-02-05T12:59:00Z"],
        ["b0", "c4", "80.00", "2026-02-05T12:58:00Z"]
      ],
      "refresh": {
        "enabled": true,
        "last_refresh": "2026-02-05T13:05:00Z"
      },
      "stale": false,
      "stale_reason": null,
      "scroll_row": 0,
      "selected_row": null,
      "selected_col": null,
      "position": {"x": 640, "y": 80},
      "size": {"width": 360, "height": 200}
    }
  ]
}
```

Example (dummy data view):

```json
{
  "id": "dv_example",
  "name": "Sample Orders",
  "query": "",
  "query_refs": [],
  "connection_ref": null,
  "captured_at": null,
  "max_rows": 10,
  "mode": "dummy",
  "columns": [
    {"name": "order_id", "type": "UUID"},
    {"name": "total", "type": "DECIMAL(10,2)"}
  ],
  "rows": [
    ["b1", "120.00"],
    ["b2", "80.00"],
    ["b3", "42.50"]
  ],
  "refresh": {
    "enabled": false,
    "last_refresh": null
  },
  "stale": false,
  "stale_reason": null,
  "scroll_row": 0,
  "selected_row": null,
  "selected_col": null,
  "position": {"x": 640, "y": 80},
  "size": {"width": 320, "height": 160}
}
```

---

## Edge Cases

- Missing nodes referenced by edges must not crash load; edges are dropped with warning.
- Unknown fields are ignored for forward compatibility.
- If a `data_view` exceeds `max_rows`, only the first `max_rows` are stored and rendered.
- If `connection_ref` is missing, refresh is disabled until the user provides a connection.

---

## Implementation Notes

- Format is JSON for diff-friendly version control.
- Schema validation should be lenient to allow upgrades.
