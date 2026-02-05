# Dashboards and Interactivity

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines dashboard composition, filters, drill behavior, and layout rules for live reporting.

---

## Dashboard Elements

- **Cards**: Visualizations based on saved questions or SQL queries.
- **Text blocks**: Markdown notes and context.
- **Dividers**: Visual layout separators.

---

## Filters

Dashboard-level filters can be connected to one or more cards.

Supported filter types:

- Date range
- Number range
- Text
- Enum list
- Field filter (column)
- Time grouping (day, week, month, quarter)

Rules:

- Filters can target compatible fields across multiple cards.
- A filter can map to a query parameter or column in a card.
- Filters can be pinned as required parameters for viewers.
- Filters may be scoped to a subset of cards.
- Time grouping filters only apply to compatible date/time fields.

---

## Drill and Click Behavior

Card click actions can include:

- Drill-through to underlying rows
- Drill-down (dimension breakdown)
- Open details in a new question
- Link to a URL

Rules:

- Drill actions are read-only by default.
- Drill results can be saved as a new question.
- Click behavior is configurable per card (disabled, drill, or link).
- Cards can optionally update dashboard filters on click (cross-filter).

---

## Layout Rules

- Grid-based layout with snapping.
- Cards may span multiple columns/rows.
- Layouts are responsive with breakpoints.

---

## Dashboard Schema (Logical)

```json
{
  "id": "uuid",
  "name": "Sales Overview",
  "collection_id": "uuid",
  "cards": [
    {"id": "card_1", "question_id": "q1", "position": {"x": 0, "y": 0, "w": 6, "h": 4}}
  ],
  "filters": [
    {"id": "f1", "type": "date_range", "targets": ["q1:created_at"]}
  ]
}
```

---

## ScratchBird Constraints

- Dashboards render only from ScratchBird-backed questions.
- Dashboard filters must map to ScratchBird columns or parameters.

---

## Related Specs

- `docs/specifications/reporting/QUESTIONS_AND_QUERY_BUILDER.md`
- `docs/specifications/reporting/ALERTS_AND_SUBSCRIPTIONS.md`
