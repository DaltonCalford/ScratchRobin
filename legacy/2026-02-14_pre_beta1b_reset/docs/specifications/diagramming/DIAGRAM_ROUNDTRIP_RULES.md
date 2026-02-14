# Diagram Round-Trip Rules

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how diagram edits map to the underlying metadata model and how metadata changes update diagrams.

---

## Forward Sync (Diagram → Metadata)

- Creating a new table node creates a ProjectObject and MetadataNode.
- Adding a column updates the corresponding MetadataNode for the table.
- Deleting a node marks the ProjectObject as DELETED.
- Adding a Data View stores a project-level snapshot with its query and metadata.
- Saving a diagram registers a ProjectObject of kind `diagram` with a design file path under `designs/diagrams/`.
  - File extensions are type-specific (`.sberd`, `.sbdfd`, `.sbwb`, `.sbmind`) or `.sbdgm` for generic diagrams.

---

## Reverse Sync (Metadata → Diagram)

- If a table exists in metadata but not in diagram, user is prompted to add it.
- If a column is removed in metadata, the column is removed from the node.
- If a relationship changes, edges update automatically.
- Data View panels are restored from project snapshots and rendered in the diagram layer.

---

## Conflict Resolution

- Diagram-only changes are authoritative for layout and visual styling.
- Metadata-only changes are authoritative for structure.
- If both change the same structural element, metadata wins and diagram is reconciled.
- Data View content is always snapshot-based; refresh is an explicit user action.
- Diagram file conflicts are resolved at the ProjectObject level (diagram registry), not by regenerating diagram JSON.

---

## Edge Cases

- **Dangling edges**: remove edge if target node no longer exists.
- **Renamed objects**: update node and maintain ID if possible.
- **Stale Data View**: if underlying schema changes, mark Data View as stale and disable auto-refresh until reviewed.
- **Query reference cache**: if `query_refs` exist, use them to target invalidation before re-parsing SQL.
