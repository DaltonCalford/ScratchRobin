# Embedding and Sharing

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how reports and dashboards can be shared or embedded while preserving project governance controls.

---

## Sharing Modes

- **Private**: project-only, requires login
- **Public link**: read-only, tokenized
- **Signed embed**: server-generated signed token

---

## Parameter Controls

- Embedded dashboards can expose selected parameters.
- Parameters must be explicitly marked as safe for public use.
- Signed embeds can require locked parameters enforced by the signature.

---

## Governance Rules

- Public sharing can be disabled per project.
- Embedding requires `Admin` or `Owner` permission.
- All embeds are logged for audit.
- Public links must be treated as untrusted and are view-only.
- Public links do not allow drill-through or editing.

---

## Related Specs

- `docs/specifications/core/PROJECT_DEFINITION_AND_GOVERNANCE.md`
- `docs/specifications/reporting/COLLECTIONS_AND_PERMISSIONS.md`
