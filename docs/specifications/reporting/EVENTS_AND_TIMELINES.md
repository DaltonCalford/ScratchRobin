# Events and Timelines

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines a lightweight event/timeline system for annotating dashboards and charts with operational context.

---

## Event Types

- Deployment
- Incident
- Data correction
- Feature release
- Custom

---

## Timeline Rules

- Timelines can be attached to dashboards or questions.
- Events are stored as project metadata and are versioned.
- Events are view-only by default; editing requires `Docs` permission.

---

## Display Rules

- Timeline markers appear as overlays on time-series charts.
- Hover shows event title and description.
- Timelines are scoped to collections and inherit collection permissions.

---

## Related Specs

- `docs/specifications/reporting/DASHBOARDS_AND_INTERACTIVITY.md`
- `docs/specifications/core/PROJECT_DEFINITION_AND_GOVERNANCE.md`
