# Collections and Permissions

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how reporting assets are organized into collections and governed by permissions.

---

## Collections

A **Collection** is a folder for questions, dashboards, models, and segments.

Properties:

- Name
- Owner
- Access policy
- Optional description
- Optional status badges (Official, Verified)

---

## Access Levels

Default access levels:

- **View**: read-only
- **Curate**: create/edit within collection
- **Admin**: manage permissions and structure

Collection contents:

- Questions
- Dashboards
- Models
- Segments
- Timelines

### Official and Personal Collections

- Projects include a default top-level collection for shared analytics.
- A user may have a personal collection for drafts and experimentation.
- Official collections are curated and appear higher in search and discovery.

### Permission Matrix (Default)

| Action | Curate | View | No Access |
| --- | --- | --- | --- |
| View items | Yes | Yes | No |
| Edit item titles/descriptions | Yes | No | No |
| Move items | Yes | No | No |
| Delete items | Yes | No | No |
| Pin items | Yes | No | No |
| View timelines | Yes | Yes | No |
| Edit timelines | Yes | No | No |

---

## Permission Inheritance

- Collections inherit from parent unless explicitly overridden.
- Objects inherit collection permissions by default.
- Overrides must be explicit and tracked in audit logs.

---

## Governance Integration

- Roles and permissions are enforced via project governance policies.
- Only Owners/Admins can change collection permissions.

---

## ScratchBird Constraints

- Permissions apply to reporting assets, not to database security.
- Database access still follows ScratchBird authentication rules.

---

## Related Specs

- `docs/specifications/core/PROJECT_DEFINITION_AND_GOVERNANCE.md`
- `docs/specifications/reporting/REPORTING_OVERVIEW.md`
