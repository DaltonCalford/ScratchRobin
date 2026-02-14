# Silverston Diagram Specification

## Table of Contents

- [Main Specification](#overview)
  - [Overview](#overview)
  - [Rendering Modes](#rendering-modes)
  - [Element Types](#element-types)
  - [Objects](#1-objects)
  - [Notes](#2-notes)
  - [Dependency Lines](#3-dependency-lines-usesdepends-upon)
  - [Diagram-Level Features](#4-diagram-level-features)
  - [Configuration Schema](#5-configuration-schema)
  - [Interaction Behaviors](#6-interaction-behaviors)
  - [Workflow Example: Party Actor Modeling](#8-workflow-example-party-actor-modeling)
  - [Replicated Schema Groups (Divisions)](#9-replicated-schema-groups-divisions)
  - [Executable Objects and Diagram‑Within‑Diagram](#10-executable-objects-and-diagramwithin-diagram)
  - [Topology Superset (Location/Ownership Layer)](#11-topology-superset-locationownership-layer)
  - [Progressive Detail and Resolution Profiles](#12-progressive-detail-and-resolution-profiles)
  - [ERD Relationship Model and Editing Rules](#13-erd-relationship-model-and-editing-rules)
  - [Diagram File Format (JSON Schema)](#14-diagram-file-format-json-schema)
  - [Template Inheritance, Overrides, and Validation](#15-template-inheritance-overrides-and-validation)
- [Annex A: Silverston Conventions Reference](#annex-a-silverston-conventions-reference)
  - [Embedded Figures (SVG)](#embedded-figures-svg)
## Overview

Silverston diagrams are used to visualize system and data structures, allowing users to represent complex hierarchical relationships between infrastructure components, database objects, and their dependencies. The diagram format supports zooming to navigate from high-level cluster views down to individual field definitions.

---

## Rendering Modes

Silverston diagrams operate in two distinct modes with different capabilities:

### Interactive Mode (Application)

When used within a program, diagrams support full interactivity:

- All editing and manipulation features enabled
- Object notes accessible via **(...)** indicator (see Section 1.8)
- Object Template Editor available for creating/modifying templates
- User-defined templates persist between sessions
- Real-time zoom and navigation

### Export Mode (Print/Graphic)

When exported as a document or graphic:

- Static representation only
- Object notes **(...)** indicator is **hidden**
- All visual elements rendered at export resolution
- No interactive features
- Suitable for documentation, printing, and sharing

---

## Element Types

Silverston diagrams consist of three primary element types:

1. **Objects** - Hierarchical containers representing system/data components
2. **Notes** - Annotation elements for documentation
3. **Dependency Lines** - Connection lines showing relationships and data flow

---

## 1. Objects

Objects are the primary building blocks of Silverston diagrams. They are rectangular containers that can hold other objects in a nested hierarchy.

### 1.1 Visual Properties

| Property | Required | Description |
|----------|----------|-------------|
| Background Color | Optional | Solid color fill; standard state is opaque. Ghostly states reduce opacity. |
| Border Styling | Required | 1px solid default; configurable per template. No rounded corners. |
| Selection State | Required | 2px blue (#0066CC) border with 8 square resize handles. |
| Hover State | Optional | Lighten background by ~10% or add faint glow. |
| Ghostly Rendering | Conditional | 50% opacity + 50% saturation reduction for optional/zero cardinality. |
| Stack Offset | Conditional | 4px right and 4px down; 2 layers visible behind front. |
| Icon Scaling | Required | Min 16px, max 48px. Hidden if object too small for min icon. |
| Z-Order | Required | Later-added objects on top; context menu allows bring to front/send to back. |
| Name | Required | Displayed in the top border break (see 1.2). |
| Name Length | Required | Names are not unique; max length 100 characters. |
| Notes Indicator | Interactive Only | **(...)** displayed after name; opens markdown editor when clicked. |
| Icon(s) | Required | **1 or 2 icons** inset from top-right corner (see 1.2). |
| Icon Description | Optional | Descriptive text positioned below the icon(s). |

### 1.2 Name Break and Icon Positioning

To improve readability in dense diagrams, the entity name is rendered *inside the top border line* as a visual break:

- The top border line is interrupted to create a gap: `-{ NAME }-`
- The name is vertically centered on the now-invisible line segment
- The break is slightly inset from the left border for consistent alignment

Icons are aligned to the same top border line, inset from the right:

Objects display **one or two icons** in the top-right corner:

- **Single icon**: Represents the object type (e.g., Table icon, Server icon)
- **Two icons**: Primary type icon + secondary modifier icon (e.g., Field icon + Data Type icon)

Icon positioning rules:

- Inset from the top-right corner (configurable margin)
- **Centered on the top border line** (icons appear to sit on the border)
- When two icons are present, arranged horizontally, right-to-left from the corner
- Icon description text appears directly below the icon(s), inside the object boundary
- Icon scaling limits: min 16px, max 48px; hidden if object too small for min icon

```
  Name Break + Icon(s):
    ┌──────────────────────────────────────────────┐
    │ -{ NAME (...) }-                [ico1][ico2] │
    │                                              │
    │                                              │
    └──────────────────────────────────────────────┘
```

#### Two-Icon Examples

| Object Type | Primary Icon | Secondary Icon |
|-------------|--------------|----------------|
| Integer Field | Field | Integer type |
| VARCHAR Field | Field | String type |
| Date Field | Field | Date type |
| Foreign Key Field | Field | Key/Link |
| Stored Procedure | Procedure | Language (SQL/Python/etc.) |

### 1.3 Typography

| Element | Configuration |
|---------|---------------|
| Name Text | Font family and size are configurable per template |
| Icon Description | Font family and size are configurable per template |
| Scaling | All text scales proportionally with object resize |

### 1.4 Sizing and Scaling

#### Minimum Size
- Each object has a defined **minimum width** and **minimum height**
- Objects cannot be resized smaller than their minimum dimensions
- Minimum size is defined per object template

#### Scaling Behavior
- Objects can be resized larger than minimum size
- Fonts and icons scale proportionally with resize
- Contained (child) objects resize proportionally until they reach their minimum size
- **Child objects that would scale below minimum size become hidden**
- Icons clamp to 16px minimum and 48px maximum; if the object is too small for
  the minimum icon size, the icon is hidden

### 1.5 Grid Layout (Container Objects)

Objects are classified as either **Container** or **Non-Container** types:

#### Container Objects

Container objects can hold child objects and use a grid-based layout system:

| Property | Description |
|----------|-------------|
| Columns | Configurable number of grid columns |
| Rows | Configurable number of grid rows |
| Gap | Small consistent gap between all grid cells |
| Cell Spanning | Child objects can span multiple cells (1x1, 1x2, 2x1, 2x2, etc.) |

Child object placement rules:
- Objects snap to grid cells
- Objects can span multiple contiguous cells
- Gap spacing is maintained between all objects
- Grid dimensions are configurable per parent object
- Object placement is rejected if the target object would not fit
- When placement fails, valid drop zones are highlighted
- Grid is fixed by default; users may expand it manually
- Objects occupy their allocated cells fully (accounting for gaps)
- Empty cells remain hidden unless "show grid" is enabled (dotted boundaries)
- Drag-and-drop reordering swaps positions with a preview during drag

#### Non-Container Objects (Leaf Objects)

Some object types are defined **without a placement grid** and cannot contain child objects:

- Fields (all types)
- Sequences
- Database Triggers
- Individual Users, Roles, Groups

Non-container objects:
- Have no grid configuration
- Cannot accept child objects
- Are always leaf nodes in the containment hierarchy
- May still have object notes and all other visual properties

```
┌─────────────────────────────────────┐
│ Parent Object                       │
│  ┌─────────┐ ┌─────────┐ ┌───────┐  │
│  │  1x1    │ │  1x1    │ │ 1x1   │  │
│  └─────────┘ └─────────┘ └───────┘  │
│  ┌───────────────────┐ ┌──────────┐ │
│  │       2x1         │ │   1x1    │ │
│  └───────────────────┘ └──────────┘ │
│  ┌─────────┐ ┌───────────────────┐  │
│  │  1x2    │ │                   │  │
│  │         │ │       2x2         │  │
│  │         │ │                   │  │
│  └─────────┘ └───────────────────┘  │
└─────────────────────────────────────┘
```

### 1.6 Cardinality Display Modes

Objects can be displayed in four cardinality modes affecting their visual appearance:

| Mode | Visual Representation | Description |
|------|----------------------|-------------|
| **Zero or One** | Single object, ghostly appearance | Gray-shifted colors to appear translucent/ghostly |
| **Zero or Many** | Stacked objects, ghostly appearance | Multiple offset copies, gray-shifted colors |
| **Exactly One** | Single object, standard appearance | Normal rendering, default mode |
| **One or Many** | Stacked objects, standard appearance | Multiple offset copies with standard colors |

#### Stack Rendering (Many modes)
- Front object plus 2 back layers visible behind it
- Offset direction: down and to the right
- Offset distance: 4px right and 4px down per layer

#### Ghostly Rendering (Zero modes)
- 50% opacity and 50% saturation reduction
- Indicates optional/nullable relationship

### 1.7 Object Templates

Each object is configured from a template that defines its visual and behavioral properties. Standard templates include:

#### Organization Objects
- Company (container)
- Division / Business Unit (container)
- Department (container)
- Team (container)
- Product / Project (container)
- Cost Center (leaf)

#### Location Objects
- Region (container)
- Site / Campus (container)
- Building (container)
- Floor (container)
- Room (container)

#### Datacenter Objects
- Data Center (container)
- Data Hall (container)
- Row (container)
- Rack (container)
- Chassis (container)

#### Infrastructure Objects
- Cluster (container)
- Host (container)
- Server (container)
- Domain (leaf)
- Hypervisor (leaf)
- Virtual Machine (container)
- Container (leaf)

#### Network Objects
- Network / VPC (container)
- Subnet / VLAN (container)
- Security Zone (container)
- Load Balancer (leaf)
- Firewall (leaf)

#### Platform Objects
- Node (leaf)
- Shard (leaf)
- Replica (leaf)
- Listener (leaf)
- Parser (leaf)
- Connection Pool (leaf)

#### Storage Objects
- Volume (leaf)
- Disk (leaf)

#### Database Objects
- Database (container)
- Schema (container)
- FileSpace (container)
- Tablespace (leaf)

#### Security Objects
- Users (container)
- Roles (container)
- Groups (container)

#### Programmability Objects
- Procedures (leaf)
- Functions (leaf)
- Database Triggers (leaf)
- Sequences (leaf)

#### Data Objects
- Tables (container)
- Views (container)
- Fields (leaf, with type-specific subtypes)

#### Operations Objects
- Job (leaf)
- ETL Pipeline (container)
- Backup Task (leaf)
- Health Check (leaf)

#### Additional Templates
- Custom templates can be defined as needed (see Section 7: Object Template Editor)

### 1.8 Object Notes (Interactive Mode Only)

Each object instance can have associated documentation accessible via the **(...)** indicator.

#### Visual Indicator

- Displayed as **"(...)"** immediately after the object name in the upper-left corner
- Only visible in **Interactive Mode** (hidden in exports/prints)
- Visual state indicates note presence:
  - `(...)` in muted/gray: No notes attached
  - `(...)` in accent color: Notes present

#### Notes Editor

Clicking the **(...)** indicator opens a markdown editor panel:

| Feature | Description |
|---------|-------------|
| Format | Full Markdown support |
| Links | Internal links to other objects, external URLs |
| Images | Embedded images supported |
| Auto-save | Changes persist automatically |
| Search | Notes content is searchable across the diagram |

```
┌─────────────────────────────────────────────────────┐
│ Object Notes: CustomerTable                      [X]│
├─────────────────────────────────────────────────────┤
│ ## Description                                      │
│                                                     │
│ Primary customer data table. Contains PII fields   │
│ that require encryption at rest.                   │
│                                                     │
│ ## Change History                                   │
│ - 2026-01-15: Added email_verified column          │
│ - 2025-11-20: Initial creation                     │
│                                                     │
│ ## Related                                          │
│ - [CustomerOrders](link:obj/CustomerOrders)        │
│ - [GDPR Compliance Doc](https://docs.example.com)  │
└─────────────────────────────────────────────────────┘
```

---

## 2. Notes

Notes are annotation elements for adding documentation and context to diagrams.

### 2.1 Visual Properties

| Property | Value |
|----------|-------|
| Background Color | Yellow (fixed) |
| Shape | Rectangle with chamfered corner |
| Chamfer Location | Typically top-right corner |
| Chamfer Size | Fixed 12px |
| Border | Visible boundary |
| Typography | Configurable; default sans-serif 12px |
| Z-order | Above objects, below dependency lines |

### 2.2 Content

- Contains **text only** (no nested objects)
- Supports **Markdown** formatting
- Supports **hyperlinks**
- Text wraps within note boundaries

### 2.3 Sizing

- Notes can be resized by the user
- Text reflows to fit available width
- Minimum size: 80x60px
- Maximum size: 800x600px

### 2.4 Anchoring and Layering

- Notes can be anchored to an object (move with parent) or float freely
- Render above objects but below dependency lines

```
    ┌────────────────────╮
    │ Note Title         │
    │                    │
    │ Markdown content   │
    │ with **formatting**│
    │ and [links](url)   │
    │                    │
    └────────────────────┘
```

---

## 3. Dependency Lines (Uses/Depends Upon)

Dependency lines show relationships between objects that are **not** referential integrity constraints. They represent:

- Table distribution across servers/databases
- Database dependencies
- ETL data flows
- Service dependencies

**Clarification**: Dependency lines are **additive**. They do **not** replace ERD relationships. A relationship does not imply a dependency, and a dependency does not imply a relationship.

### 3.1 Line Properties

| Property | Description |
|----------|-------------|
| Color | Configurable per line |
| Thickness | 2px default, configurable 1-4px |
| Directionality | Arrow at target end (destination) |
| Arrowhead | Hollow triangle |
| Anchor Points | Auto-select nearest edge center point (top/right/bottom/left) |
| Hitbox | 8px on either side of the line |
| Routing | Horizontal and vertical segments only (**no diagonals**) |
| Corners | 90-degree turns |
| Spacing | Standard minimum distance between parallel lines |

### 3.2 Reference Label

Each dependency line includes a reference label:

| Property | Description |
|----------|-------------|
| Shape | Rounded-corner rectangle |
| Content | Name or reference for the dependency action |
| Position | Approximately midpoint of the line path |
| Placement | Automatically positioned to avoid overlap with objects, notes, and other labels |
| Overflow | Truncate at 30 characters with ellipsis; full text on hover tooltip |

```
    ┌─────────┐                    ┌─────────┐
    │ Source  │                    │  Dest   │
    │         ├────┬───────────────┤         │
    └─────────┘    │               └─────────┘
                   │
              ╭─────────╮
              │ ETL Job │
              ╰─────────╯
```

### 3.3 Line Routing Rules

1. **Orthogonal Only**: Lines use only horizontal and vertical segments
2. **Parallel Spacing**: Lines running parallel maintain standard minimum distance
3. **Crossing Indication**: When lines cross, a half-circle "hop" is rendered on the **horizontal** line to indicate no connection
4. **Auto-routing**: Default route is auto-generated
5. **Manual Override**: Users can add waypoints to control routing
6. **Self-referential**: Allowed; exits right, loops around top, enters top
7. **Multiple Lines**: Allowed; each line has its own label and preserves spacing

#### Line Crossing Example
```
         │
    ─────┴─────    (vertical line continues through)
         │

    vs.

         │
    ────╮ ╭────    (horizontal line hops over, no connection)
        ╰─╯
         │
```

### 3.4 Auto-Layout Considerations

- Reference labels are positioned to avoid overlapping:
  - Objects
  - Notes
  - Other reference labels
- Lines are routed to minimize crossings where possible
- Parallel lines maintain consistent spacing

---

## 4. Diagram-Level Features

### 4.1 Zoom and Navigation

- Zoom range: 10% to 400%
- Zoom steps: 10, 25, 50, 75, 100, 150, 200, 300, 400
- Mouse wheel supports smooth zooming
- Pan controls: drag on empty canvas, scroll bars, arrow keys when no selection
- Minimap: bottom-right, collapsible
- Fit-to-view: Fit All and Fit Selection
- Zoom persistence: saved per diagram

### 4.2 Object Visibility by Zoom Level

| Zoom Level | Typical Visible Objects |
|------------|------------------------|
| Far (overview) | Clusters, Hosts, Servers |
| Medium | Databases, Schemas |
| Close | Tables, Views, Procedures |
| Detail | Fields, Triggers, Constraints |

### 4.3 File and Document Management

- File format: JSON with `.sbdgm` extension (diagram), `.sberd` for ERD
- Auto-save: every 60 seconds to a temp location
- Manual save: explicit save to file
- Version compatibility: schema version in file; migrate on load if older
- Large diagram performance: virtual rendering for >500 objects, lazy-load collapsed containers

### 4.4 Export Details

- Formats: PNG, SVG, PDF
- Resolution (PNG/PDF): 72, 150, 300 DPI
- Partial export: full diagram, visible viewport, or selection
- Print layout: auto-paginate large diagrams, optional header (name/date), page numbers

### 4.5 Search and Filter

- Search by name (substring), type, and notes content
- Filter palette to show/hide object types
- Highlight matches in yellow; non-matches dim to 50% opacity
- Navigate results with Next/Previous buttons or F3/Shift+F3

### 4.6 Accessibility

- Keyboard navigation: Tab through objects in tree order, Enter to expand/edit
- Screen readers announce type, name, and cardinality
- Color blindness support: patterns for dependency lines, shape indicators for cardinality
- High contrast mode respects OS settings

### 4.7 Data View Panels (Query Results)

Each diagram can embed **Data View** panels that show a snapshot of a query result set.

Core rules:

- A Data View is backed by a `SELECT` statement.
- Results are stored in the project and displayed in a grid with column headers.
- Maximum rows displayed: **10** (first 10 rows of the captured snapshot).
- Results can be refreshed manually to pull a new snapshot.
- Data View panels are non-destructive and read-only.

Display rules:

- Column names appear as the header row.
- Each row shows values in a grid to support row/column identification.
- Row numbers (1–10) and column letter/index labels are visible by default.
- Panels can be resized but must preserve the grid.

Metadata stored with the panel:

- Query text (SELECT)
- Query references (parsed `schema.table` list, optional)
- Captured timestamp
- Connection reference (if any)
- Column metadata (name, type)

### 4.8 Data View Refresh Behavior

Data View refresh is explicit and policy-governed. Refresh never auto-mutates the underlying model.

Manual refresh:

- User clicks `Refresh` on a Data View panel.
- The query is re-executed and a new snapshot is stored (max 10 rows).
- `captured_at` is updated and the panel re-renders.

Scheduled refresh:

- Optional per-panel schedule (e.g., every 15 minutes).
- Requires a valid `connection_ref` and project permission to execute queries.
- Schedules are disabled by default and must be explicitly enabled.

Schema change invalidation:

- If the schema of referenced objects changes (columns added/removed/types changed),
  the Data View is marked **stale**.
- Stale Data Views display a warning badge and suspend auto-refresh until reviewed.
- Manual refresh prompts the user to confirm updated column mappings.

Permissions:

- Only users with `Docs` or `Design` permission can refresh.
- Scheduled refresh requires `Docs` + `Deploy` (or an equivalent policy-defined role).

---

## 5. Configuration Schema

### 5.1 Object Template Configuration

```yaml
object_template:
  name: string                    # Template identifier (unique)
  display_name: string            # Human-readable name
  group: string                   # Group identifier for organization
  source: builtin | user | shared # Origin of template definition

  icons:
    primary: string               # Required primary icon identifier
    secondary: string | null      # Optional secondary/modifier icon
    description: string           # Optional description text below icons

  colors:
    background: color             # Default background color
    border: color                 # Border color
    text: color                   # Text color

  typography:
    name_font: font_spec          # Font for object name
    description_font: font_spec   # Font for icon description

  sizing:
    min_width: number             # Minimum width in pixels
    min_height: number            # Minimum height in pixels
    default_width: number         # Default width
    default_height: number        # Default height

  container:
    is_container: boolean         # Whether object can contain children
    grid:                         # Only applicable if is_container: true
      columns: number             # Default column count
      rows: number                # Default row count
      gap: number                 # Gap between child objects
    allowed_children: string[]    # List of allowed child template names

  allowed_parents: string[]       # List of allowed parent template names

  metadata:
    created: datetime             # Creation timestamp
    modified: datetime            # Last modification timestamp
    author: string                # Creator identifier
```

### 5.2 Template Group Configuration

```yaml
template_group:
  id: string                      # Unique group identifier
  name: string                    # Display name
  icon: string                    # Optional group icon
  color: color                    # Optional accent color
  order: number                   # Sort order in palette
  source: builtin | user          # Whether system or user-defined
  parent_group: string | null     # Parent group ID for nesting
```

### 5.3 Dependency Line Configuration

```yaml
dependency_line:
  id: string                      # Unique identifier
  source: object_reference        # Source object
  target: object_reference        # Target object
  label: string                   # Reference label text
  color: color                    # Line color
  style: solid | dashed | dotted  # Line style (optional)
```

### 5.4 Data View Configuration

```yaml
data_view:
  id: string                      # Unique identifier
  name: string                    # Display name
  query: string                   # SELECT statement (optional for dummy views)
  query_refs: string[] | null     # Optional parsed references (schema.table)
  connection_ref: string | null   # Connection reference (optional)
  max_rows: number                # Max rows stored/rendered (default 10)
  mode: live | dummy              # dummy = manual rows/columns
  captured_at: datetime | null    # Last capture time
  columns:
    - name: string
      type: string | null
  rows: string[][]                # 2D array of row values (max_rows)
  refresh:
    enabled: boolean
    schedule_minutes: number | null
  stale: boolean                 # Optional: marks stale data view
  stale_reason: string | null    # Optional: reason for staleness
  scroll_row: number | null      # Optional: UI scroll row offset
  selected_row: number | null    # Optional: UI selection row
  selected_col: number | null    # Optional: UI selection column
  position:
    x: number
    y: number
  size:
    width: number
    height: number
```

Notes:

- `dummy` mode allows manual columns and rows with no query.
- `live` mode requires a query and connection to refresh.

Example (dummy data view):

```yaml
data_view:
  id: "dv_example"
  name: "Sample Orders"
  query: ""
  query_refs: []
  connection_ref: null
  max_rows: 10
  mode: dummy
  captured_at: null
  columns:
    - name: "order_id"
      type: "UUID"
    - name: "total"
      type: "DECIMAL(10,2)"
  rows:
    - ["b1", "120.00"]
    - ["b2", "80.00"]
    - ["b3", "42.50"]
  refresh:
    enabled: false
    schedule_minutes: null
  stale: false
  stale_reason: null
  scroll_row: 0
  selected_row: null
  selected_col: null
  position:
    x: 640
    y: 80
  size:
    width: 320
    height: 160
```

### 5.5 Note Configuration

```yaml
note:
  id: string                      # Unique identifier
  content: markdown_string        # Markdown content
  position:
    x: number
    y: number
  size:
    width: number
    height: number
```

---

## 6. Interaction Behaviors

### 6.1 Object Interactions

| Action | Behavior |
|--------|----------|
| Click | Select object |
| Click (...) | Open Object Notes editor panel (Interactive Mode only) |
| Double-click | Edit object properties |
| Drag | Move object (snaps to parent grid if applicable) |
| Resize handles | Resize object (respects minimum size) |

### 6.2 Line Interactions

| Action | Behavior |
|--------|----------|
| Click line | Select line |
| Click label | Select line and edit label |
| Drag endpoints | Reconnect to different objects |

### 6.3 Note Interactions

| Action | Behavior |
|--------|----------|
| Click | Select note |
| Double-click | Edit note content |
| Drag | Move note |
| Resize handles | Resize note |

### 6.4 Editing Operations

- Undo/redo: 50 levels
- Copy/paste: within and between diagrams; paste generates new UUID v4 IDs
- Multi-select: Shift+click to add, Ctrl+click to toggle, drag marquee to select
- Keyboard shortcuts: delete, copy/paste, undo/redo, arrow-key nudging
- Object names are not unique; each object has a UUID v4 identifier

---

## 8. Workflow Example: Party Actor Modeling

This section defines a concrete workflow for modeling a PARTY as a decision‑making actor with realizations and roles, consistent with Silverston subtype conventions and the expanded tool layout.

### 8.1 Diagram Snippet (Conceptual)

```text
┌──────────────────────────────────────────────────────────────┐
│ -{ PARTY }-                                       [icon][ico] │
│  # PARTY ID                                                   │
│  * PARTY NAME                                                 │
│  o STATUS                                                     │
│                                                              │
│  ┌───────────── Realization (Subtype Set A) ───────────────┐  │
│  │  ┌──────────────┐  ┌───────────┐  ┌──────────────┐      │  │
│  │  │ COMPANY      │  │ GROUP     │  │ PERSON       │      │  │
│  │  └──────────────┘  └───────────┘  └──────────────┘      │  │
│  └─────────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌────────────── Roles (Subtype Set B) ────────────────┐      │
│  │ ┌───────────┐ ┌───────────┐ ┌───────────┐           │      │
│  │ │ CUSTOMER  │ │ VENDOR    │ │ PROSPECT  │           │      │
│  │ └───────────┘ └───────────┘ └───────────┘           │      │
│  └─────────────────────────────────────────────────────┘      │
└──────────────────────────────────────────────────────────────┘

PARTY ──(may be)── CUSTOMER PROFILE
PARTY ──(may be)── VENDOR CONTRACT
PARTY ──(may be)── PROSPECT PIPELINE
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_1.svg)

### 8.2 Tool Interaction Rules

**Create supertype**
- Drag a `PARTY` entity to the canvas; it becomes a supertype container.

**Add subtypes (realizations)**
- Drag `COMPANY`, `GROUP`, `PERSON` into `PARTY`.
- Subtypes auto‑align to the **subtype grid** inside the supertype.
- The grid is **layout‑only** and does not change semantics.

**Add a second subtype set (roles)**
- Insert a **subtype set box** (unnamed group) inside `PARTY`.
- Drag `CUSTOMER`, `VENDOR`, `PROSPECT` into that group.
- Multiple subtype set boxes indicate **non‑mutually exclusive** classification sets.

**Add attributes**
- Attributes can be added as **names only** (no datatype required yet).
- Attribute markers: `#` primary key, `*` mandatory, `o` optional.

**Connect to external entities**
- Draw relationships from `PARTY` to role‑specific entities.
- Set cardinality and optionality on the relationship line (crow’s foot + solid/dotted).

### 8.3 Semantics vs Presentation

- The **subtype grid and grouping boxes** are presentation layers only.
- All inheritance and subtype semantics are derived from nesting and grouping, not layout.
- Layout changes never alter the semantic model.

---

## 9. Replicated Schema Groups (Divisions)

This section defines how a schema can contain replicated divisions (e.g., a corporation with many divisions). Each division shares the same structure but holds different data. The diagram uses a layered/stacked appearance to communicate replication beyond pure display layout.

### 9.1 Visual Snippet (Replicated Group)

```text
┌──────────────────────────────────────────────────────────────┐
│ -{ CORPORATE SCHEMA }-                            [schema]    │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ -{ Division A }-                             [division] │  │
│  │  (structure: Party/Company/Transaction/etc.)            │  │
│  └────────────────────────────────────────────────────────┘  │
│    ┌────────────────────────────────────────────────────┐    │
│    │ -{ Division B }-                           [division]   │
│    │  (same structure, different data)                    │
│    └────────────────────────────────────────────────────┘
│      ┌──────────────────────────────────────────────────┐
│      │ -{ Division C }-                         [division] │
│      │  (same structure, different data)                  │
│      └──────────────────────────────────────────────────┘
└──────────────────────────────────────────────────────────────┘
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_2.svg)

### 9.2 Semantics vs. Presentation

**Semantics (Model Truth):**
- The schema defines a single logical structure.
- Each division is a **replica instance** of that structure.
- Replicas share the same entity/relationship model.

**Presentation (Diagram Rendering):**
- Replicated divisions are rendered as **stacked/layered groups** to indicate repeated structure.
- Offset layers are **visual only** and do not imply additional relationships.
- Users may collapse layers into a “stack” marker to reduce clutter.

### 9.3 Editing Rules

**Shared Structure Edits**
- Editing the topmost division (or the schema template) updates the shared structure for all replicas **by default**.
- Structural changes (entities, relationships, attributes) are propagated to all divisions **unless opted out**.

**Per‑Division Metadata**
- Each division may store **local metadata** (labels, notes, tags, status).
- Local metadata does **not** alter the shared ERD structure.
- Examples: division-specific notes, color accent, ownership tags.

**Opt‑Out / Template Override**
- Divisions can opt out of the shared template.\n- An object can switch templates (or a group of objects can be batch‑retemplated).
- Templates define default layout/appearance/child rules; objects may override template defaults locally.

### 9.4 Interaction Behavior

- **Toggle Replication**: Any container can be marked as “replicated group.”  
- **Edit Structure**: Always applies to the shared template.  
- **Edit Metadata**: Applies only to the selected replica.  
- **View Mode**: Users can switch between “stacked” and “expanded” view for replicas.

---

## 10. Executable Objects and Diagram‑Within‑Diagram

Triggers, procedures, and functions are displayed on the diagram and can themselves contain sub‑objects. This enables “diagram‑within‑diagram” workflows where a sub‑object is shown in summary form but can be expanded into its full detail view.

### 10.1 Executable Object Scopes

Executable objects can exist at different scopes:

- **Cluster‑level trigger** (applies to cluster events)
- **Database‑level trigger**
- **Table/Entity‑level trigger**
- **Procedure / Function** (logical or physical scope depending on backend)

Scope is represented by **containment**:
- Cluster‑level triggers are contained by the Cluster container.
- Database‑level triggers are contained by the Database container.
- Table‑level triggers are contained by the specific entity/table container.

**Reusable Triggers**:
- Triggers/procedures may be **shared** across scopes when semantics allow (e.g., common UUID assignment).
- Reuse is represented as **cloned instances with a shared indicator** (using the secondary icon).

### 10.2 Diagram‑Within‑Diagram (Drill‑Down)

All objects can render in two modes:

| Mode | Description |
| --- | --- |
| Summary | Minimal box with name + icons only |
| Expanded | Full box with attributes, notes, and child layout |

**Behavior**:
- Double‑click a summary box to open the expanded sub‑diagram.
- The expanded view is the same object (not a copy).
- Closing the expanded view returns to the parent diagram.

**ASCII Example**:

```text
┌──────────────────────────────────────────────┐
│ -{ PARTY }-                        [entity]  │
│  # PARTY ID                                    │
│  * PARTY NAME                                  │
│                                                │
│  ┌───────────────┐                             │
│  │ -{ TRIGGER }- │  (summary mode)             │
│  │   [trg][uuid] │                             │
│  └───────────────┘                             │
└──────────────────────────────────────────────┘

Double‑click TRIGGER → opens expanded sub‑diagram:

┌──────────────────────────────────────────────┐
│ -{ TRIGGER: Assign UUID }-          [trg][id] │
│  (expanded diagram)                            │
│  ┌───────────────┐                             │
│  │ -{ PARTY }-   │  (summary object inside)    │
│  │   [entity]    │                             │
│  └───────────────┘                             │
│  ... rules, notes, conditions ...              │
└──────────────────────────────────────────────┘
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_3.svg)

### 10.3 Sub‑Object Rendering Rules

When an object is embedded inside another:

- The **parent template** controls whether children appear in Summary or Expanded mode.
- Children always keep their **semantic identity** (entity, trigger, procedure, etc.).
- Summary rendering is **presentation‑only** and does not alter semantics.

**Template‑Driven Detail**:
- Whether relationships and attribute lists render in summary mode is a **template decision**.\n- Designers can choose dense or minimal summary views without losing stored semantics.

**Group Icon Suppression**:
- If a subtype group box provides a shared icon/type label, child icons are suppressed **only when they match the group type**.

### 10.4 Super‑Object Template Extensions

Super‑object templates define how children are embedded and displayed:

| Property | Description |
| --- | --- |
| `allowed_children` | Allowed embedded object types |
| `child_display` | Summary or Expanded default |
| `grid` | Placement grid for children |
| `style` | Line color, background, name font/color |
| `print_profile` | Which child layers are visible in export |

### 10.5 Printing and Export

- Any expanded sub‑diagram can be exported as a standalone print.\n- Parent diagrams can optionally include children in summary form.\n- Print profiles control layer visibility (e.g., “structure only” vs “details”).

---

## 11. Topology Superset (Location/Ownership Layer)

The diagram can include **topology supersets** (e.g., Region → Building → NOC) to show *where* data elements reside while preserving ERD semantics. This enables a single diagram to communicate both the data model and the physical/organizational context.

### 11.1 Semantics

- Topology containers do **not** change ERD relationships.
- They express **location, ownership, or operational domain** for contained objects.
- The data model remains valid even if topology containers are collapsed or hidden.

### 11.2 Short Example

```text
┌──────────────────────────────────────────────────────────────┐
│ -{ REGION: West }-                                  [region] │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ -{ BUILDING: HQ }-                          [building] │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │ -{ NOC }-                               [noc]    │  │  │
│  │  │   ┌──────────────────────────────────────────┐   │  │  │
│  │  │   │ -{ PARTY }-                    [entity] │   │  │  │
│  │  │   │  # PARTY ID                              │   │  │  │
│  │  │   │  * PARTY NAME                            │   │  │  │
│  │  │   └──────────────────────────────────────────┘   │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_4.svg)

### 11.3 Presentation Rules

- Topology containers are rendered as standard super‑objects.
- They may use distinct iconography (region/building/NOC).
- Collapsing a topology container does not alter the semantic ERD model.

**Reverse‑Engineering Default**:
- If no topology metadata is provided, assume a single location and place all objects in one topology container.\n- Users can later define and distribute objects across topology containers.

---

## 12. Progressive Detail and Resolution Profiles

As designs mature, supersets can render increasing levels of ERD detail (columns, domains, types, constraints) or remain at a high‑level summary. This is controlled by configurable **resolution profiles** and **display rules**.

### 12.1 Resolution Tiers

| Tier | Intended Use | Typical Rendering |
| --- | --- | --- |
| Overview | Early design, executive review | Name + icons only |
| Mid | Structural review | Name + key attributes + major relationships |
| Detail | Implementation-ready | Full ERD detail (columns, types, domains, constraints) |

### 12.2 Superset Display Rules

Each superset defines how its children render at each tier:

- **Summary Mode**: compact boxes, minimal attributes
- **Expanded Mode**: full ERD listing (rows/columns, domains, types)
- **Replacement Mode**: superset collapses and the ERD detail replaces it directly

**Replacement Frame Behavior**:
- Whether the superset frame remains visible in replacement mode is **template‑configurable**.\n- Designers may hide the frame for clarity or keep it as a labeled boundary.

### 12.3 Display Configuration Schema (Conceptual)

```yaml
display_profile:
  name: string
  tiers:
    overview:
      show_icons: true
      show_attributes: false
      show_relationships: false
    mid:
      show_icons: true
      show_key_attributes: true
      show_relationships: true
    detail:
      show_icons: true
      show_all_attributes: true
      show_types_domains: true
      show_constraints: true

superset_display:
  template: string
  allowed_children: [entity, trigger, procedure, function]
  tier_overrides:
    overview: summary
    mid: expanded
    detail: replacement
```

### 12.4 Reverse Engineering

- The diagram can be **reverse‑engineered** from a live database using the ERD notation rules.\n- Superset containers are inferred from schema, catalog, or ownership metadata.\n- Missing metadata yields summary‑only objects until enriched.

### 12.5 Forward Build

- If sufficient metadata is present (types, domains, constraints, keys), the diagram can generate:\n  - Database schema\n  - Database instance\n  - Cluster layout (if topology supersets are defined)

**Untyped Attributes**:
- Untyped attributes are allowed in early design.\n- For implementation‑ready outputs, untyped attributes receive a default type from a **configurable project setting**.

### 12.6 Printing and Export

- Each tier can be exported independently.\n- Sub‑diagrams can be printed as standalone documents.

### 12.7 Worked Example (Party Superset Across Tiers)

**Overview Tier** (summary only):
```text
┌──────────────────────────────┐
│ -{ PARTY }-         [entity] │
└──────────────────────────────┘
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_5.svg)

**Mid Tier** (key attributes + relationships):
```text
┌──────────────────────────────┐
│ -{ PARTY }-         [entity] │
│  # PARTY ID                  │
│  * PARTY NAME                │
└──────────────────────────────┘
PARTY ──(may be)── CUSTOMER PROFILE
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_6.svg)

**Detail Tier** (full ERD detail):
```text
┌──────────────────────────────┐
│ -{ PARTY }-         [entity] │
│  # PARTY ID : UUID           │
│  * PARTY NAME : VARCHAR(80)  │
│  o STATUS : PARTY_STATUS     │
│  o CREATED_AT : TIMESTAMPTZ  │
└──────────────────────────────┘
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_7.svg)

---

## 13. ERD Relationship Model and Editing Rules

This section defines the semantic model for ERD relationships and how they are created/edited in the tool.

### 13.1 Relationship Object Model

Each ERD relationship is an explicit object with the following fields:

- `id` (UUID)
- `name_forward` (string) — name when read source→target
- `name_reverse` (string) — name when read target→source
- `source_entity_id` (UUID)
- `target_entity_id` (UUID)
- `cardinality` (enum): `one_to_one`, `one_to_many`, `many_to_many`
- `optional_source` (bool)
- `optional_target` (bool)
- `identifying` (bool) — true when foreign key is part of child PK (tilde)
- `exclusive_arc_id` (UUID | null)
- `notes` (markdown, optional)

### 13.2 Relationship Creation

- User drags from source entity to target entity.
- Tool prompts for relationship names (forward/reverse).
- Cardinality and optionality are set via inline handles or a dialog.

### 13.3 Relationship Editing

- Cardinality is edited via crow’s‑foot handles at the line ends.
- Optionality is edited by toggling dotted/solid line per end.
- Identifying relationships use the tilde (~) marker across the line.

### 13.4 Exclusive Arcs

- Exclusive arcs are created by selecting multiple relationships and applying “Exclusive Arc.”
- All relationships in the arc must share the same source entity.

### 13.5 Validation Rules

- Many‑to‑many relationships must be resolved by an associative entity (unless explicitly allowed for conceptual‑only mode).
- Exclusive arcs must have 2+ relationships.
- Identifying relationships require a child entity with composite PK.

---

## 14. Diagram File Format (JSON Schema)

This section defines the on‑disk JSON structure for Silverston diagrams.

### 14.1 Top‑Level Schema (Conceptual)

```json
{
  "version": "1.0",
  "diagram_type": "silverston",
  "metadata": {
    "created_at": "2026-02-05T12:00:00Z",
    "updated_at": "2026-02-05T12:10:00Z",
    "author": "alice"
  },
  "layout": {
    "zoom": 1.0,
    "viewport": {"x": 0, "y": 0, "width": 1200, "height": 800},
    "display_profile": "detail"
  },
  "entities": [],
  "relationships": [],
  "notes": [],
  "templates": [],
  "groups": []
}
```

### 14.2 Entity Record

```json
{
  "id": "uuid",
  "name": "PARTY",
  "template": "entity.standard",
  "attributes": [
    {"name": "PARTY_ID", "key": true, "required": true, "type": "UUID"}
  ],
  "position": {"x": 100, "y": 120},
  "size": {"w": 240, "h": 160},
  "children": [],
  "notes": ""
}
```

### 14.3 Relationship Record

```json
{
  "id": "uuid",
  "source_entity_id": "uuid",
  "target_entity_id": "uuid",
  "name_forward": "composed of",
  "name_reverse": "part of",
  "cardinality": "one_to_many",
  "optional_source": false,
  "optional_target": true,
  "identifying": true
}
```

### 14.4 Template Record

```json
{
  "name": "entity.standard",
  "display_name": "Entity",
  "icons": {"primary": "entity", "secondary": null},
  "container": {"is_container": true, "grid": {"columns": 2, "rows": 3}},
  "child_display": "summary",
  "style": {"border": "#333", "background": "#fff", "text": "#000"}
}
```

---

## 15. Template Inheritance, Overrides, and Validation

### 15.1 Inheritance and Override Rules

- Templates can inherit from a base template.
- Object‑level overrides apply only to the specific object instance.
- Precedence order: **object override → template → template base**.

### 15.2 Batch Template Application

- Users can select multiple objects and apply a new template.
- When applied, object overrides are cleared unless explicitly preserved.

### 15.3 Validation

- Enforce allowed child types based on template.
- Validate grid placement and spanning.
- Ensure required fields (icons, min sizes) are present.

---

## 7. Object Template Editor (Interactive Mode Only)

The Object Template Editor allows users to define custom object templates that persist between program sessions.

### 7.1 Template Definition

Users can create new object templates with the following properties:

| Property | Required | Description |
|----------|----------|-------------|
| Template Name | Yes | Unique identifier for the template |
| Display Name | Yes | Human-readable name shown in UI |
| Group | Yes | User-defined group for organization (see 7.3) |
| Primary Icon | Yes | Main icon representing the object type |
| Secondary Icon | No | Optional modifier/type icon |
| Icon Description | No | Text displayed below icons |
| Is Container | Yes | Whether objects can contain children |
| Grid Defaults | If Container | Default columns, rows, gap settings |
| Allowed Children | If Container | Which template types can be placed inside |
| Allowed Parents | No | Which template types can contain this object |
| Colors | No | Default background, border, text colors |
| Typography | No | Font settings for name and description |
| Min/Default Size | Yes | Minimum and default dimensions |

### 7.2 Template Persistence

User-defined templates are stored persistently:

| Aspect | Behavior |
|--------|----------|
| Storage Location | Application data directory (user-specific) |
| Format | Serialized configuration (JSON/YAML) |
| Scope | Per-user or shared (configurable) |
| Backup | Exportable/importable template packages |
| Versioning | Templates track creation/modification dates |

#### Template Storage Structure

```
~/.silverston/
├── templates/
│   ├── builtin/              # Read-only standard templates
│   │   ├── infrastructure.json
│   │   ├── database.json
│   │   └── ...
│   ├── user/                 # User-defined templates
│   │   ├── custom-templates.json
│   │   └── imported/
│   └── shared/               # Team-shared templates (optional)
├── groups.json               # User-defined group definitions
└── config.json               # Editor preferences
```

### 7.3 Template Groups

Templates are organized into user-defined groups for easier navigation:

#### Built-in Groups (Read-only)
- Infrastructure
- Database Objects
- Security
- Programmability
- Data Objects

#### User-Defined Groups

Users can create custom groups to organize their templates:

| Feature | Description |
|---------|-------------|
| Create Group | Define new group with name and optional icon |
| Assign Templates | Move templates between groups |
| Group Ordering | Drag-and-drop to reorder groups in palette |
| Nested Groups | Groups can contain sub-groups (optional) |
| Group Colors | Optional color coding for visual distinction |

```
┌─────────────────────────────────────┐
│ Object Palette                      │
├─────────────────────────────────────┤
│ ▼ Infrastructure                    │
│     Cluster  Host  Server  Domain   │
│ ▼ Database Objects                  │
│     Database  Schema  FileSpace     │
│ ▼ Data Objects                      │
│     Table  View  Field              │
│ ▼ My Custom Objects          [+]    │  ← User-defined group
│     AuditLog  TenantConfig          │
│ ▼ ETL Components             [+]    │  ← User-defined group
│     Pipeline  Transform  Source     │
└─────────────────────────────────────┘
```

### 7.4 Template Editor Interface

```
┌─────────────────────────────────────────────────────────────────┐
│ Template Editor: New Template                            [Save] │
├─────────────────────────────────────────────────────────────────┤
│ Template Name: [________________]                               │
│ Display Name:  [________________]                               │
│ Group:         [▼ Select Group    ]  [+ New Group]              │
│                                                                 │
│ ┌─ Icons ─────────────────────────────────────────────────────┐ │
│ │ Primary:   [🔲 Select...]    Secondary: [🔲 Select...]      │ │
│ │ Description: [________________]                              │ │
│ └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│ ┌─ Container Settings ────────────────────────────────────────┐ │
│ │ [✓] Is Container (can hold child objects)                   │ │
│ │     Default Columns: [3 ]  Rows: [4 ]  Gap: [8px]           │ │
│ │     Allowed Children: [Multi-select template list...]       │ │
│ └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│ ┌─ Sizing ────────────────────────────────────────────────────┐ │
│ │ Minimum:  Width [100]px  Height [80 ]px                     │ │
│ │ Default:  Width [200]px  Height [150]px                     │ │
│ └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│ ┌─ Appearance ────────────────────────────────────────────────┐ │
│ │ Background: [■ #FFFFFF]  Border: [■ #333333]                │ │
│ │ Text Color: [■ #000000]                                     │ │
│ │ Name Font:  [▼ Sans-serif] Size: [14]                       │ │
│ └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│ ┌─ Preview ───────────────────────────────────────────────────┐ │
│ │                      ┌──[icon]──┐                           │ │
│ │      ┌───────────────┤          │                           │ │
│ │      │ Sample (...)  │          │                           │ │
│ │      │               └──────────┘                           │ │
│ │      │                          │                           │ │
│ │      └──────────────────────────┘                           │ │
│ └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### 7.5 Template Import/Export

Templates can be shared between users or installations:

| Action | Description |
|--------|-------------|
| Export Single | Export one template as `.silverston-template` file |
| Export Group | Export entire group with all templates |
| Export All | Export all user-defined templates |
| Import | Load templates from file; handle name conflicts |
| Merge | Combine imported templates with existing |

---

## Appendix A: Standard Color Palette

(To be defined based on implementation requirements)

## Appendix B: Standard Icon Set

(To be defined - icons for each standard object template)

## Appendix C: Containment Hierarchy

Defines which object types can contain which other types:

```
Company
└── Division / Business Unit
    └── Department
        └── Team
            └── Product / Project
                ├── Cluster
                ├── Database
                └── ETL Pipeline

Region
└── Site / Campus
    └── Building
        └── Floor
            └── Room
                └── Data Center
                    └── Data Hall
                        └── Row
                            └── Rack
                                └── Chassis
                                    ├── Host
                                    │   ├── Hypervisor
                                    │   ├── Virtual Machine
                                    │   │   └── Container
                                    │   └── Server
                                    └── Storage
                                        ├── Volume
                                        └── Disk

Network / VPC
└── Subnet / VLAN
    └── Security Zone
        ├── Load Balancer
        └── Firewall

Cluster
├── Node
├── Shard
├── Replica
├── Listener
├── Parser
└── Connection Pool

Database
├── Schema
│   ├── Tables
│   │   └── Fields
│   ├── Views
│   │   └── Fields
│   ├── Procedures
│   ├── Functions
│   ├── Sequences
│   └── Database Triggers
├── Users
├── Roles
├── Groups
└── FileSpace
    └── Tablespace

ETL Pipeline
├── Job
└── Health Check
```

Explicit allowed-child rules (summary):
- Company -> Division/Business Unit
- Division/Business Unit -> Department
- Department -> Team
- Team -> Product/Project
- Product/Project -> Cluster, Database, ETL Pipeline
- Region -> Site/Campus
- Site/Campus -> Building
- Building -> Floor
- Floor -> Room
- Room -> Data Center
- Data Center -> Data Hall
- Data Hall -> Row
- Row -> Rack
- Rack -> Chassis
- Chassis -> Host, Server, Storage
- Host -> Hypervisor, Virtual Machine, Container
- Virtual Machine -> Container
- Network/VPC -> Subnet/VLAN
- Subnet/VLAN -> Security Zone
- Security Zone -> Load Balancer, Firewall
- Cluster -> Node, Shard, Replica, Listener, Parser, Connection Pool
- Database -> Schema, Users, Roles, Groups, FileSpace
- Schema -> Tables, Views, Procedures, Functions, Sequences, Database Triggers
- Tables -> Fields
- Views -> Fields
- FileSpace -> Tablespace
- ETL Pipeline -> Job, Health Check

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.3 | 2026-01-09 | - | Added GUI implementation decisions (selection, hover, grid, zoom, export, accessibility) |
| 1.1 | 2026-01-23 | - | Added Rendering Modes (Interactive vs Export), Object Notes feature, Object Template Editor (Section 7), template persistence, user-defined groups, clarified 1-2 icon support, defined container vs non-container objects |
| 1.0 | 2026-01-23 | - | Initial draft specification |

---

# Annex A: Silverston Conventions Reference

# Silverston Diagramming Conventions Reference

**Status**: Draft (Derived from Silverston)  
**Last Updated**: 2026-02-05

---

## Source

- *The Data Model Resource Book: A Library of Universal Data Models for All Enterprises* (Len Silverston)
- Section: "Conventions and Standards Used in This Book"

This reference summarizes the diagramming conventions and illustrates them with the book’s example figures.

---

## 1) Entities

**Definition**: An entity represents something of significance about which the enterprise stores information.  
**Naming**: Singular, meaningful noun; use suffix `TYPE` when representing a classification rather than a real-world instance (e.g., `ORDER TYPE` vs. `ORDER`).  
**Representation**: Entities are **rounded rectangles**. In the expanded tooling, the name is embedded in the top border as a visible break: `-{ NAME }-` (presentation‑only).

**Example (Fig 1.1)**: A single entity named `ORDER` is shown as a rounded box.

**Visual (Expanded Tool Layout)**:

```text
┌──────────────────────────────────────────────┐
│ -{ ORDER (...) }-                 [ico1][ico2]│
│                                              │
│                                              │
└──────────────────────────────────────────────┘
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_8.svg)

---

## Workflow Example: Party Actor Modeling

This mirrors the example in the Silverston diagramming tool specification and shows how a PARTY supertype can host multiple subtype sets (realizations and roles).

```text
┌──────────────────────────────────────────────────────────────┐
│ -{ PARTY }-                                       [icon][ico] │
│  # PARTY ID                                                   │
│  * PARTY NAME                                                 │
│  o STATUS                                                     │
│                                                              │
│  ┌───────────── Realization (Subtype Set A) ───────────────┐  │
│  │  ┌──────────────┐  ┌───────────┐  ┌──────────────┐      │  │
│  │  │ COMPANY      │  │ GROUP     │  │ PERSON       │      │  │
│  │  └──────────────┘  └───────────┘  └──────────────┘      │  │
│  └─────────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌────────────── Roles (Subtype Set B) ────────────────┐      │
│  │ ┌───────────┐ ┌───────────┐ ┌───────────┐           │      │
│  │ │ CUSTOMER  │ │ VENDOR    │ │ PROSPECT  │           │      │
│  │ └───────────┘ └───────────┘ └───────────┘           │      │
│  └─────────────────────────────────────────────────────┘      │
└──────────────────────────────────────────────────────────────┘

PARTY ──(may be)── CUSTOMER PROFILE
PARTY ──(may be)── VENDOR CONTRACT
PARTY ──(may be)── PROSPECT PIPELINE
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_9.svg)

---

## Replicated Schema Groups (Divisions)

This mirrors the replicated-division pattern from the Silverston diagramming tool specification. It uses layered containers to show the same structure repeated across divisions.

```text
┌──────────────────────────────────────────────────────────────┐
│ -{ CORPORATE SCHEMA }-                            [schema]    │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ -{ Division A }-                             [division] │  │
│  │  (structure: Party/Company/Transaction/etc.)            │  │
│  └────────────────────────────────────────────────────────┘  │
│    ┌────────────────────────────────────────────────────┐    │
│    │ -{ Division B }-                           [division]   │
│    │  (same structure, different data)                    │
│    └────────────────────────────────────────────────────┘
│      ┌──────────────────────────────────────────────────┐
│      │ -{ Division C }-                         [division] │
│      │  (same structure, different data)                  │
│      └──────────────────────────────────────────────────┘
└──────────────────────────────────────────────────────────────┘
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_10.svg)

**Key rules**:
- The schema defines one shared structure.\n- Divisions are replicas; layering is presentation only.\n- Structural edits propagate to all divisions; per-division metadata does not.\n
---

## Executable Objects and Diagram‑Within‑Diagram

Triggers, procedures, and functions can appear inside larger objects (e.g., a PARTY entity having a trigger for UUID assignment). These embedded objects can be shown as summaries and expanded on demand.

```text
┌──────────────────────────────────────────────┐
│ -{ PARTY }-                        [entity]  │
│  # PARTY ID                                    │
│  * PARTY NAME                                  │
│                                                │
│  ┌───────────────┐                             │
│  │ -{ TRIGGER }- │  (summary mode)             │
│  │   [trg][uuid] │                             │
│  └───────────────┘                             │
└──────────────────────────────────────────────┘

Double‑click TRIGGER → opens expanded sub‑diagram:

┌──────────────────────────────────────────────┐
│ -{ TRIGGER: Assign UUID }-          [trg][id] │
│  (expanded diagram)                            │
│  ┌───────────────┐                             │
│  │ -{ PARTY }-   │  (summary object inside)    │
│  │   [entity]    │                             │
│  └───────────────┘                             │
│  ... rules, notes, conditions ...              │
└──────────────────────────────────────────────┘
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_11.svg)

Key rules:
- Embedded objects may be **summary‑only** inside parents.\n- Double‑click opens the **full object diagram**.\n- Structural edits apply to the shared object model.\n- Scope (cluster/db/table) is defined by container placement.\n
---

## Topology Superset (Location/Ownership Layer)

Topology supersets (Region → Building → NOC) can contain ERD elements to show *where* data lives without changing its semantics.

```text
┌──────────────────────────────────────────────────────────────┐
│ -{ REGION: West }-                                  [region] │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ -{ BUILDING: HQ }-                          [building] │  │
│  │  ┌──────────────────────────────────────────────────┐  │  │
│  │  │ -{ NOC }-                               [noc]    │  │  │
│  │  │   ┌──────────────────────────────────────────┐   │  │  │
│  │  │   │ -{ PARTY }-                    [entity] │   │  │  │
│  │  │   │  # PARTY ID                              │   │  │  │
│  │  │   │  * PARTY NAME                            │   │  │  │
│  │  │   └──────────────────────────────────────────┘   │  │  │
│  │  └──────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/silverston_diagram_specification_ascii_12.svg)

Key rules:
- Topology containers are presentation/ownership layers.\n- Collapsing them does not change ERD semantics.\n
---

## 2) Subtypes and Supertypes

**Definition**: Subtypes are classifications of a supertype that inherit its attributes/relationships.  
**Representation**: Subtypes are drawn as **nested boxes** inside the supertype box.  
**Inheritance**: Attributes on the supertype are inherited by the subtype.

**Example (Fig 1.2)**:
- `ORGANIZATION` is the supertype.
- `LEGAL ORGANIZATION` and `INFORMAL ORGANIZATION` are subtypes within.
- Further nested: `CORPORATION`, `GOVERNMENT AGENCY`, `TEAM`, `FAMILY`, `OTHER INFORMAL ORGANIZATION`.
- Attributes specific to the subtype appear in the subtype box (e.g., `FEDERAL TAX ID NUM` only on `LEGAL ORGANIZATION`).

**Rule**: Subtypes should be collectively complete and mutually exclusive, unless explicitly modeled as non‑mutually exclusive (see below).

---

## 3) Non‑Mutually Exclusive Subtypes

**Definition**: A supertype may be subtyped in more than one way, and multiple subtype sets can apply simultaneously.  
**Representation**: Use **separate grouping boxes** (unnamed) to show different subtype sets.

**Example (Fig 1.3)**:
- `REQUIREMENT` has two independent subtype sets:
  - `CUSTOMER REQUIREMENT` / `INTERNAL REQUIREMENT`
  - `PRODUCT REQUIREMENT` / `WORK REQUIREMENT`
- A requirement can belong to one subtype in each set.

---

## 4) Attributes

**Definition**: Attributes represent properties of an entity.  
**Representation**: Listed inside the entity box.

**Markers**:
- `#` = primary key attribute
- `*` = mandatory attribute
- `o` = optional attribute

**Example (Fig 1.4)**:
- `# ORDER ID` (primary key)
- `* ORDER DATE` (mandatory)
- `o ENTRY DATE` (optional)

**Naming Conventions** (examples):
- `ID` = system‑generated unique identifier
- `Seq ID` = sequence within a parent
- `Code` = mnemonic identifier
- `Name` = proper noun
- `Description` = human-readable description
- `Ind` = boolean indicator
- `from date` / `thru date` = inclusive date range

---

## 5) Relationships

**Definition**: Associations between entities.  
**Representation**: Lines with names in both directions.

**Naming rule**: Relationships should read as a full sentence in both directions:
"Each ENTITY {must be/may be} relationship name {one and only one/one or more} ENTITY, over time."

---

## 6) Relationship Optionality

**Definition**: Whether a relationship is mandatory or optional.  
**Representation**:
- **Solid line** = mandatory (must be)
- **Dotted line** = optional (may be)

**Example (Fig 1.5)**:
- `SHIPMENT` must be shipped from one and only one `POSTAL ADDRESS` (solid line).
- `POSTAL ADDRESS` may be the source of one or more `SHIPMENT`s (optional in the reverse direction).

---

## 7) Relationship Cardinality

**Definition**: One‑to‑one, one‑to‑many, many‑to‑many.  
**Representation**:
- **Crow’s foot** = many
- No crow’s foot = one

**Example (Fig 1.6)**:
- `ORDER` composed of one or more `ORDER ITEM`s (crow’s foot at ORDER ITEM).
- Each `ORDER ITEM` must belong to one and only one `ORDER`.

**Note**: “Over time” may change cardinality (e.g., ORDER ↔ ORDER STATUS).

---

## 8) Foreign Key Relationships

**Definition**: One‑to‑many implies PK of the “one” side is a FK in the “many” side.  
**Convention**: Foreign keys are *not* shown as attributes; the relationship itself indicates the FK.

---

## 9) Foreign Key Inheritance (Identifying Relationships)

**Definition**: When the inherited FK is part of the child entity’s primary key.  
**Representation**: A **tilde (~)** symbol across the relationship line.

**Example (Fig 1.6)**:
- `ORDER ITEM` PK includes `ORDER ID` from `ORDER`.

---

## 10) Intersection / Associative Entities

**Definition**: Resolve many‑to‑many relationships.  
**Representation**: An entity between the two, with identifying (~) relationships to each parent.

**Example (Fig 1.7)**:
- `PARTY CONTACT MECHANISM` resolves many‑to‑many between `PARTY` and `CONTACT MECHANISM`.
- Includes `from date` as part of PK.

---

## 11) Exclusive Arcs

**Definition**: An entity may relate to *one of several* other entities, but not more than one at the same time.  
**Representation**: A **curved arc** across multiple relationship lines.

**Example (Fig 1.8)**:
- `INVENTORY ITEM` must be located at either `FACILITY` or `CONTAINER`, but not both.

---

## 12) Recursive Relationships

**Definition**: An entity relates to itself.  
**Representation**: A loopback relationship or a many‑to‑many resolved by an intersection entity.

**Example (Fig 1.9)**:
- `WORK EFFORT` may be “redone via” another `WORK EFFORT` (one‑to‑many recursive).
- `WORK EFFORT ASSOCIATION` resolves many‑to‑many recursive relationships (dependency/breakdown).

---

## 13) Physical Models

Physical models use the same notation, but boxes represent tables and attributes represent columns. This is used for data warehouse and implementation diagrams.

---

## 14) Illustration Tables

**Definition**: Example tables showing sample data.  
**Convention**: Values referenced in text are surrounded by double quotes.

---

## 15) Figure References

**Convention**: Figures are referenced as `Vx:Figure x.x` (Volume 1 or 2). If `Vx` is omitted, the figure is in the current volume.

---

## Appendix: Figures Used in This Reference

- **Fig 1.1** Entity (rounded box)
- **Fig 1.2** Subtypes and supertypes (nested boxes)
- **Fig 1.3** Non‑mutually exclusive subtype sets (grouping boxes)
- **Fig 1.4** Attribute markers (#, *, o)
- **Fig 1.5** Optionality (solid vs. dotted line)
- **Fig 1.6** Cardinality + identifying (~) relationship
- **Fig 1.7** Associative entity
- **Fig 1.8** Exclusive arc
- **Fig 1.9** Recursive relationships

Figure images from the book are stored for embedding at:
- `docs/specifications/diagramming/assets/silverston_book_figs/`

### Embedded Figures

**Fig 1.1 — Entity (rounded box)**  

### Embedded Figures (SVG)

![Figure 1.1 Entity](docs/specifications/diagramming/assets/silverston_book_figs_svg_vec/fig-000.svg)

**Fig 1.2 — Subtypes and supertypes (nested boxes)**  
![Figure 1.2 Subtypes and Supertypes](docs/specifications/diagramming/assets/silverston_book_figs_svg_vec/fig-001.svg)

**Fig 1.3 — Non‑mutually exclusive subtype sets**  
![Figure 1.3 Non-mutually exclusive subtypes](docs/specifications/diagramming/assets/silverston_book_figs_svg_vec/fig-002.svg)

**Fig 1.4 — Attribute markers (#, *, o)**  
![Figure 1.4 Attribute markers](docs/specifications/diagramming/assets/silverston_book_figs_svg_vec/fig-003.svg)

**Fig 1.5 — Optionality (solid vs. dotted line)**  
![Figure 1.5 Optionality](docs/specifications/diagramming/assets/silverston_book_figs_svg_vec/fig-004.svg)

**Fig 1.6 — Cardinality + identifying (~) relationship**  
![Figure 1.6 Cardinality and identifying relationship](docs/specifications/diagramming/assets/silverston_book_figs_svg_vec/fig-005.svg)

**Fig 1.7 — Associative entity (many-to-many)**  
![Figure 1.7 Associative entity](docs/specifications/diagramming/assets/silverston_book_figs_svg_vec/fig-006.svg)

**Fig 1.8 — Exclusive arc**  
![Figure 1.8 Exclusive arc](docs/specifications/diagramming/assets/silverston_book_figs_svg_vec/fig-007.svg)

**Fig 1.9 — Recursive relationships**  
![Figure 1.9 Recursive relationships](docs/specifications/diagramming/assets/silverston_book_figs_svg_vec/fig-008.svg)
