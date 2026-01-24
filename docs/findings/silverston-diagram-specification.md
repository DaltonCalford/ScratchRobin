# Silverston Diagram Specification

Version: 1.3
Status: Draft
Last Updated: 2026-01-09

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
| Name | Required | Displayed in upper-left corner using title text styling. |
| Name Length | Required | Names are not unique; max length 100 characters. |
| Notes Indicator | Interactive Only | **(...)** displayed after name; opens markdown editor when clicked. |
| Icon(s) | Required | **1 or 2 icons** inset from top-right corner. |
| Icon Description | Optional | Descriptive text positioned below the icon(s). |

### 1.2 Icon Positioning

Objects display **one or two icons** in the top-right corner:

- **Single icon**: Represents the object type (e.g., Table icon, Server icon)
- **Two icons**: Primary type icon + secondary modifier icon (e.g., Field icon + Data Type icon)

Icon positioning rules:

- Inset from the top-right corner (configurable margin)
- **Bisected halfway by the top edge** of the object (icons appear to "hang over" the top border)
- When two icons are present, arranged horizontally, right-to-left from the corner
- Icon description text appears directly below the icon(s), inside the object boundary
- Icon scaling limits: min 16px, max 48px; hidden if object too small for min icon

```
  Single Icon:                      Two Icons:
                    ┌──[icon]──┐                      ┌[ico1][ico2]┐
                    │  desc    │                      │    desc    │
    ┌───────────────┤          │      ┌───────────────┤            │
    │ Name (...)    │          │      │ Name (...)    │            │
    │               │          │      │               │            │
    │               └──────────┘      │               └────────────┘
    │                          │      │                            │
    └──────────────────────────┘      └────────────────────────────┘
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

#### Infrastructure Objects
- Cluster
- Host
- Server
- Domain

#### Database Objects
- Database
- Schema
- FileSpace

#### Security Objects
- Users
- Roles
- Groups

#### Programmability Objects
- Procedures
- Functions
- Database Triggers
- Sequences

#### Data Objects
- Tables
- Views
- Fields (with type-specific subtypes)

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

Dependency lines show relationships between objects that are not referential integrity constraints. They represent:

- Table distribution across servers/databases
- Database dependencies
- ETL data flows
- Service dependencies

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

- File format: JSON with `.silverston` extension
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

### 5.4 Note Configuration

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
Cluster
└── Host
    └── Server
        └── Database
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
            └── Groups
```

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.3 | 2026-01-09 | - | Added GUI implementation decisions (selection, hover, grid, zoom, export, accessibility) |
| 1.1 | 2026-01-23 | - | Added Rendering Modes (Interactive vs Export), Object Notes feature, Object Template Editor (Section 7), template persistence, user-defined groups, clarified 1-2 icon support, defined container vs non-container objects |
| 1.0 | 2026-01-23 | - | Initial draft specification |
