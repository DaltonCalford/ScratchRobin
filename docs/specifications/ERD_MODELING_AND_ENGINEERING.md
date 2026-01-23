# ERD Modeling and Engineering Specification

Status: Draft
Last Updated: 2026-01-09

This specification defines ScratchRobin's ERD system: supported notations,
model types, reverse-engineering rules, DDL generation, and free-form
whiteboard mode. It is a design reference and does not imply immediate
implementation.

## Goals

- Support multiple ERD notations for large and complex schemas.
- Enable forward-engineering (diagram -> DDL) and reverse-engineering
  (catalog -> diagram).
- Maintain a round-trip mapping using UUIDs, not names.
- Provide a free-form diagram mode for exploratory design.

## Non-goals (Alpha)

- Real-time collaborative editing.
- Automatic data migration execution across environments.
- Full schema diff/merge for every dialect (initially ScratchBird-first).

## Notation Support

Required notations:
- Crow's Foot
- IDEF1X
- UML Class (ERD profile)
- Chen (optional)
- Silverston (spec provided by project owner)

Rules:
- Notation is a rendering layer on top of a shared logical model.
- Each notation has a renderer and a symbol dictionary.
- Diagram settings persist the chosen notation per diagram.

## Diagram Types

- Conceptual ERD: entities + relationships only (no physical columns).
- Logical ERD: attributes + keys + constraints (no storage options).
- Physical ERD: tables + columns + indexes + storage options.
- Free-form (whiteboard): shapes and connectors not bound to schema.

## Core Model Objects

- Entity/Table
  - uuid
  - name
  - schema/path
  - attributes/columns
  - primary key
  - indexes
  - notes/tags
- Attribute/Column
  - uuid
  - name
  - data type
  - nullability
  - default
  - domain/type reference
- Relationship
  - uuid
  - source entity
  - target entity
  - cardinality (1:1, 1:N, N:M)
  - optionality (mandatory/optional)
  - identifying vs non-identifying
  - join columns (logical/physical)
- Constraint
  - uuid
  - kind (PK, FK, UNIQUE, CHECK)
  - definition
- Index
  - uuid
  - columns
  - uniqueness
  - method/type
- View (physical model)
  - uuid
  - definition

## Storage and Project Integration

- Diagrams are stored locally by default (JSON on disk).
- Diagrams may be stored in Project schemas as first-class objects.
- Every diagram element stores:
  - element id
  - diagram id
  - object uuid (if bound)
  - layout (x, y, width, height)
  - style (color, notation-specific attributes)

## Reverse-Engineering (Database -> Diagram)

### Sources
- ScratchBird system catalogs (preferred for native).
- External engines via adapters (PostgreSQL/MySQL/Firebird).

### Rules
- Tables -> entities
- Columns -> attributes
- PK/FK/UNIQUE/CHECK -> constraints
- FK pairs -> relationships with cardinality inferred from keys
- Indexes are included in physical models
- Views can be included optionally
- System schemas are excluded by default

### Naming
- Display names follow catalog names; paths include catalog/schema.
- UUID mapping is preserved where provided by the server.

### Diff Behavior
- Existing diagrams can be refreshed and show:
  - added entities/attributes
  - removed entities/attributes
  - renamed objects (UUID match)

## Forward-Engineering (Diagram -> DDL)

### Output Modes
- Create-only (new schema)
- Alter/patch (diff-based migration)
- Preview (no execution)

### Generation Order (Physical)
1. Schemas
2. Domains / types
3. Tables
4. Primary keys / unique constraints
5. Foreign keys
6. Indexes
7. Views

### Dialect Rules
- ScratchBird native is the canonical dialect.
- External targets are dialect-limited (only supported features emitted).
- DDL generation must be aware of:
  - identifier quoting rules
  - data type mappings
  - constraint syntax differences

### Safety
- DDL is never executed implicitly; it must be reviewed and confirmed.
- DDL generation respects permissions and connection capabilities.

## Round-Trip Mapping

- All diagram elements store their object UUID when bound.
- If a UUID is missing (external engines), a stable key is derived from
  catalog + schema + object name.
- Renames are detected via UUID; names are display-only.

## Free-Form Diagram Mode

- Shapes and connectors are not bound to schema objects.
- Free-form nodes can be linked to objects later (promote to bound nodes).
- Whiteboard content is stored in the same diagram storage format.

## UI Behaviors

- Diagrams can be opened in detachable windows or tabbed within diagram
  windows only.
- Multiple diagrams can be open at once; each has independent layout state.
- Diagram canvas supports:
  - zoom, grid, snap, alignment
  - auto-layout (optional)
  - grouping/layers

## Export / Import

- Export formats: PNG, SVG, PDF
- Diagram exchange format: JSON (ScratchRobin ERD schema)
- Optional import from existing ERD tools is out of scope for Alpha

## Security

- Only show objects the user can access.
- DDL generation must re-check permissions prior to execution.

## Related Research

- Tooling evaluation: `docs/specifications/ERD_TOOLING_RESEARCH.md`
