# ERD Modeling and Engineering Specification

Status: Draft
Last Updated: 2026-01-24

This specification defines ScratchRobin's ERD system: supported notations,
model types, reverse-engineering rules, DDL generation, file formats, and UI
behaviors. It is a design reference and does not imply immediate
implementation.

## Goals

- Support multiple ERD notations for large and complex schemas.
- Enable forward-engineering (diagram -> DDL) and reverse-engineering
  (catalog -> diagram).
- Maintain a round-trip mapping using UUIDs, not names.
- Provide a free-form diagram mode for exploratory design.
- Keep diagrams usable at scale (hundreds of entities).

## Non-goals (Alpha)

- Real-time collaborative editing.
- Automatic data migration execution across environments.
- Full schema diff/merge for every dialect (initially ScratchBird-first).
- Importing proprietary ERD formats.

## 1. Rendering Modes

### Interactive Mode (Application)

- Full editing and manipulation features enabled.
- Object notes accessible via in-canvas indicators.
- Template/theme editing enabled.
- Real-time zoom and navigation.

### Export Mode (Print/Graphic)

- Static representation only.
- Notes indicators hidden by default.
- All visual elements rendered at export resolution.
- No interactive affordances (handles, highlights, tooltips).

## 2. Notation Support

Required notations:
- Crow's Foot
- IDEF1X
- UML Class (ERD profile)
- Chen (optional)
- Silverston (see `SILVERSTON_DIAGRAM_SPECIFICATION.md`)

Rules:
- Notation is a rendering layer on top of a shared logical model.
- Each notation has a renderer and a symbol dictionary.
- Diagram settings persist the chosen notation per diagram.

### 2.1 Cardinality and Optionality Symbols

| Relationship | Crow's Foot | IDEF1X | UML | Chen |
|--------------|-------------|--------|-----|------|
| 1..1 | `|---|` | solid | 1..1 | 1:1 |
| 0..1 | `O---|` | dashed optional | 0..1 | 0..1 |
| 1..N | `|---<` | solid + crow | 1..* | 1:N |
| 0..N | `O---<` | dashed + crow | 0..* | 0..N |

Notes:
- Identifying relationships render as solid lines; non-identifying as dashed.
- Optionality is encoded per notation (O, dashed, or min cardinality).

### 2.2 Symbols and Glyphs

- Primary key: key icon or PK prefix in attribute list.
- Foreign key: link icon or FK prefix in attribute list.
- Unique constraint: UQ badge or underline (notation dependent).
- Derived attribute: italic text (optional).
- Multi-value attribute: double-oval in Chen only.

## 3. Diagram Types and Metadata

Diagram types:
- Conceptual ERD: entities + relationships only (no physical columns).
- Logical ERD: attributes + keys + constraints (no storage options).
- Physical ERD: tables + columns + indexes + storage options.
- Free-form (whiteboard): shapes and connectors not bound to schema.

Required diagram metadata:
- `diagram_id` UUID
- `name`
- `model_type` (conceptual | logical | physical | freeform)
- `notation` (crowsfoot | idef1x | uml | chen | silverston)
- `created_at` (epoch ms)
- `updated_at` (epoch ms)
- `source_dialect` (scratchbird | firebird | postgres | mysql | unknown)
- `source_catalog_uuid` (optional)
- `layout_settings` (grid size, snap, zoom)

## 4. Core Model Objects

| Object | Required Fields | Notes |
|--------|------------------|------|
| Entity/Table | uuid, name, schema/path, attributes, keys, indexes | Bound to catalog table when available |
| Attribute/Column | uuid, name, data_type, nullability, default, domain/type | Logical or physical depending on model |
| Relationship | uuid, source_uuid, target_uuid, cardinality, optionality | FK-backed when available |
| Constraint | uuid, kind (PK/FK/UQ/CHECK), definition | Bound to catalog constraint |
| Index | uuid, columns, uniqueness, method/type | Physical only |
| Domain/Type | uuid, name, base_type, constraints | Logical/physical only |
| View | uuid, name, definition | Physical only |
| Sequence | uuid, name, start/step | Physical only |
| Note | uuid, content, anchor_uuid (optional) | Free-form |
| Group | uuid, name, member_uuids | Used for schema or domain grouping |

## 5. Visual Model and Styling

### 5.1 Entity/Table Boxes

- Border: 1px solid default, configurable per template.
- Header: optional fill color, contains name and schema badge.
- Body: attribute list with PK/FK/UQ markers and data types.
- Collapse: attributes hidden, name-only box.
- Minimum size: default 160x120, configurable per template.
- Resizing: fonts and row heights scale within min/max bounds.

### 5.2 Attribute/Column Rows

- Format: `[PK/FK] name : data_type [NULL|NOT NULL] [DEFAULT]`
- Data type display uses dialect mapping rules.
- Domain/type references display as `domain_name` with base type tooltip.
- Computed/derived columns render with a formula icon and italic text.

### 5.3 Relationship Lines

- Thickness: 2px default, configurable 1-4px.
- Anchor points: nearest edge centers; user can override per endpoint.
- Self-referential: loop from right edge to top edge.
- Multiple lines: allowed; labels maintain spacing.
- Routing: auto orthogonal with manual waypoints.
- Label overflow: ellipsis at 30 chars; full text on hover.
- Line hitbox: 8px either side of line.

### 5.4 Notes and Groups

- Notes: chamfer 12px; min 80x60, max 800x600.
- Notes may be anchored to an object or float freely.
- Groups: optional shaded container with label, not a schema object.

### 5.5 Selection, Hover, and Dimming

- Selection: 2px blue (#0066CC) border + 8 resize handles.
- Hover: lighten background by 10% or faint glow.
- Filtered-out objects: 50% opacity and desaturated.
- Invalid state: red border and warning icon.

### 5.6 Templates and Themes

- Template defines fonts, colors, line styles, icon set, and spacing.
- Templates can be saved, exported, and applied per diagram.
- Default templates exist per notation (crowsfoot, idef1x, uml, chen).

## 6. Layout and Navigation

### 6.1 Grid and Snap

- Snap-to-grid default: 16px.
- Alternate grid sizes: 4px, 8px, 32px.
- Show grid: optional dotted lines.
- Overlap: prevented by default; hold modifier to allow overlap.
- Align/distribute tools: left, right, top, bottom, center, equal spacing.

### 6.2 Auto-Layout

- Hierarchical layout (top-down) for relationship-heavy diagrams.
- Orthogonal layout for physical ERDs.
- Auto-layout does not override locked object positions.

### 6.3 Zoom and Pan

- Zoom range: 10% to 400%.
- Steps: 10, 25, 50, 75, 100, 150, 200, 300, 400.
- Mouse wheel zooms to cursor; trackpad supports smooth zoom.
- Pan by dragging empty canvas or spacebar + drag.
- Minimap: bottom-right, collapsible.
- Fit-to-view: Fit All and Fit Selection.
- Zoom persists per diagram.

### 6.4 Large Diagram Performance

- Virtual rendering for >500 entities.
- Lazy loading for collapsed groups.
- Progressive layout for import/reverse-engineer operations.

## 7. Editing Operations

- Undo/redo: 50 levels.
- Copy/paste: within and between diagrams; paste generates new UUIDs.
- Multi-select: Shift+click adds, Ctrl+click toggles, drag marquee selects.
- Context menus for entities, relationships, and the canvas.
- Keyboard shortcuts: delete, copy/paste, undo/redo, nudge with arrows.
- In-place rename for entity/attribute labels.

## 8. Search and Filter

- Search by name, type, schema, or notes content.
- Filter by object type (tables, views, sequences, domains, indexes).
- Results highlight: yellow; others dimmed to 50% opacity.
- Next/Previous navigation via F3 and Shift+F3.

## 9. File and Document Management

- File format: JSON with `.sberd` extension.
- Schema version stored as `schema_version`.
- Migration applied on load for older versions.
- Auto-save every 60 seconds to a temp location.
- Manual Save and Save As are always available.

## 10. Reverse-Engineering (Database -> Diagram)

### Sources
- ScratchBird system catalogs (preferred for native).
- External engines via adapters (PostgreSQL/MySQL/Firebird).

### Mapping Rules
- Tables -> entities.
- Columns -> attributes (type, nullability, default, domain reference).
- PK/FK/UNIQUE/CHECK -> constraints.
- FK pairs -> relationships with inferred cardinality.
- Indexes included in physical models.
- Views optional in physical models.
- System schemas excluded by default.

### Naming and Paths
- Display names follow catalog names; paths include catalog/schema.
- UUID mapping preserved where provided by the server.
- If UUIDs are missing, a stable key is derived from catalog path + name.

### Diff Behavior
- Existing diagrams can be refreshed and show:
  - added entities/attributes
  - removed entities/attributes
  - renamed objects (UUID match)

## 11. Forward-Engineering (Diagram -> DDL)

### Output Modes
- Create-only (new schema).
- Alter/patch (diff-based migration).
- Preview (no execution).

### Generation Order (Physical)
1. Schemas
2. Domains / types
3. Tables
4. Primary keys / unique constraints
5. Foreign keys
6. Indexes
7. Views
8. Sequences

### Dialect Rules
- ScratchBird native is canonical.
- External targets are dialect-limited (only supported features emitted).
- Generator respects identifier quoting, data type mappings, and syntax.

### Safety
- DDL is never executed implicitly; it must be reviewed and confirmed.
- Generation checks permissions and connection capabilities.

## 12. Round-Trip Mapping and Diff

- All bound elements store object UUIDs when available.
- Missing UUIDs use stable derived keys.
- Renames detected via UUID, not display name.
- Diff view marks adds/removes/renames and allows selective apply.

## 13. Capability Gating

- Backend capability flags determine visible features.
- Unsupported objects are hidden or read-only (e.g., domains, sequences).
- DDL generation warns on unsupported or lossy translations.

## 14. Export / Import

- Export formats: PNG, SVG, PDF.
- Resolution: 72, 150, 300 DPI (PNG/PDF).
- Export scope: full diagram, visible viewport, or selection.
- Optional header/footer: diagram name, date, page numbers.
- Import supports `.sberd` only during Alpha.

## 15. Security

- Only show objects the user can access.
- DDL generation re-checks permissions before execution.

## 16. Accessibility

- Keyboard navigation across entities and relationships.
- Screen reader labels announce type, name, and cardinality.
- High-contrast mode respects OS settings.
- Line patterns provide color-blind alternatives.

## 17. Integration with Projects

- Diagrams stored locally by default.
- Diagrams can be stored in Project schemas as first-class objects.
- Project storage includes diagram metadata and version history.

## Related Research

- Tooling evaluation: `ERD_TOOLING_RESEARCH.md`
- Silverston spec: `SILVERSTON_DIAGRAM_SPECIFICATION.md`
