# Auto-Layout Specification (Silverston)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the auto-layout behavior for Silverston diagrams. This spec focuses on deterministic, template-aware layout that respects container grids, project topology, and diagram readability for both overview and detailed tiers.

---

## 1. Scope and Assumptions

- Applies to Silverston diagrams and embedded sub-diagrams.
- Topology objects (`region`, `site`, `building`, `noc`) are project-level containers (not database objects).
- Auto-layout is always **container-aware** and **template-aware**.
- Layout preserves explicit user placement where the object is locked or pinned.

---

## 2. Inputs

Required inputs for layout:

- Diagram graph (nodes, edges, relationships, dependency lines)
- Container hierarchy and grid defaults
- Object templates (size, min size, label notch, icon slots)
- Resolution tier (`overview`, `mid`, `detail`)
- Manual constraints (pins, locked nodes, excluded edges)
- Style constraints (edge routing preference, label position)

Optional inputs:

- Placement hints (ordered lists, preferred zones)
- Relationship weights
- Alignment guides
- Layout seed for deterministic placement

---

## 3. Layout Stages (Pipeline)

1. **Normalize & Filter**
   - Remove hidden objects at the current tier.
   - Convert surrogate objects (summary-only boxes) to layout proxies.

2. **Container Pass (Top-Down)**
   - Layout project-level containers (`region`, `site`, `building`, `noc`) first.
   - Compute bounding boxes for each container based on children.

3. **Grid Allocation**
   - If container has a grid, auto-place children into grid slots.
   - Grid slots are filled by:
     1. Pinned items (preserve slot)
     2. Priority items (supertype, primary objects)
     3. Remaining items (stable sort by name)

4. **Subtype Set Placement**
   - Subtype sets are layout-only grouping boxes.
   - Subtype children are auto-aligned within the group grid.

5. **Relationship Routing**
   - ERD relationships use orthogonal routing.
   - Dependency lines use dashed routing and can share channels.
   - Route edges within a container before crossing container bounds.

6. **Collision Resolution**
   - Resolve overlaps by expanding container grid or inserting spacing.
   - Respect minimum node sizes, label notch width, and icon slots.

7. **Tidy & Align**
   - Align rows/columns, distribute nodes evenly.
   - Snap edges to ports with consistent spacing.

8. **Finalize**
   - Emit layout positions and edge paths.
   - Store layout checksum for deterministic re-runs.

---

## 4. Container and Grid Rules

- Containers may define `grid_defaults` (columns, rows, gap).
- Children auto-place by row-major order unless overridden.
- Containers can expand to fit children; shrinking below child bounds is invalid.
- Group boxes (subtype sets, trigger groups) are **presentation-only** and do not affect the underlying model.

---

## 5. Project Topology Layout Rules

Topology objects are ordered as follows:

1. `region` containers (top-most)
2. `site` containers inside `region`
3. `building` containers inside `site`
4. `noc` containers inside `building` or `site`

Rules:

- Each topology container expands to fit its contained `cluster` and `database` objects.
- Topology containers may share a single diagram; they are not required to be expanded at the same time.
- A topology container may be collapsed to an icon-only view at `overview` tier.

---

## 6. Silverston-Specific Behavior

### 6.1 Supertype / Subtype Sets

- Subtype sets are grouped into a layout-only box.
- Auto-layout aligns subtypes to a grid inside the set.
- The group label and group icon are rendered once on the container.
- Subtype objects in the group can omit icons when `inherit_group_icons = true`.

### 6.2 Label Notch + Icon Slots

- Label notch width is reserved on the top-left edge of the object box.
- Icon slots are reserved on the top-right edge.
- Auto-layout must avoid edge routing through label notch and icon slots.

### 6.3 Replication Groups

- Replicated containers appear as stacked layers.
- The base layer defines grid bounds; replicas mirror placement unless explicitly overridden.
- Auto-layout keeps replicas offset by a fixed delta (default 8px).

---

## 7. Relationship Routing

- **ERD relationships**: solid lines with cardinality markers, orthogonal routing preferred.
- **Dependency lines**: dashed lines, no cardinality, may overlap with ERD lines only if visually distinct.
- Relationship routing respects container boundaries and attempts to avoid crossing label notches.

---

## 8. User Controls

Supported user actions:

- `Auto-layout: diagram` (full graph)
- `Auto-layout: container` (scope to selected container)
- `Auto-layout: selection` (only selected items)

Locked items:

- Locked nodes are never repositioned.
- Pinned edges preserve existing waypoints.

---

## 9. Determinism and Performance

- Layout is deterministic when the same seed and inputs are used.
- Degrade gracefully for large graphs:
  - > 1,000 nodes: use partial layout per container.
  - > 5,000 nodes: perform layered layout only in focused scope.

---

## 10. Output Format

Auto-layout output must record:

- Node position and dimensions
- Edge routes (waypoints)
- Applied layout algorithm
- Layout seed and checksum

Output is stored in diagram serialization under `layout`.

---

## 11. Worked Layout Example

ASCII sketch (topology containers with auto-placed database objects):

```text
+----------------------------------------------+
| Project                                      |
|  +-------------------------+                 |
|  | Region: US-East          |                 |
|  |  +--------------------+ |                 |
|  |  | Site: Ashburn      | |                 |
|  |  |  +---------------+ | |                 |
|  |  |  | Building A    | | |                 |
|  |  |  |  +---------+  | | |                 |
|  |  |  |  |  NOC    |  | | |                 |
|  |  |  |  | +-----+ |  | | |                 |
|  |  |  |  | |C1  | |  | | |                 |
|  |  |  |  | +-----+ |  | | |                 |
|  |  |  |  | +-----+ |  | | |                 |
|  |  |  |  | |DB1 | |  | | |                 |
|  |  |  |  | +-----+ |  | | |                 |
|  |  |  |  +---------+  | | |                 |
|  |  |  +---------------+ | |                 |
|  |  +--------------------+ |                 |
|  +-------------------------+                 |
+----------------------------------------------+
```

SVG version (vector):

- `docs/specifications/diagramming/assets/auto_layout_example.svg`

---

## 12. Worked Layout Example (Subtype Sets + Replication Groups)

Legend:

- Solid line: ERD relationship (cardinality + optionality)
- Dashed line: Dependency line (non-ERD annotation)

ASCII sketch showing subtype set grouping and replicated containers:

```text
+---------------------------------------------+
| Project                                     |
|  +--------------------+   +---------------+ |
|  | Region: US-East    |   | Region: US-West|
|  |  +--------------+  |   | (replica)     |
|  |  | DB: Sales    |  |   +---------------+ |
|  |  |  +--------+  |  |                     |
|  |  |  | PARTY  |  |  |                     |
|  |  |  | -{name}-|  |  |                     |
|  |  |  | [Role Types] |                    |
|  |  |  |  Customer    |                    |
|  |  |  |  Vendor      |                    |
|  |  |  +-------------+                    |
|  |  +--------------+                      |
|  +--------------------+                   |
+---------------------------------------------+
```

Behavior notes:

- The subtype set box (`[Role Types]`) is layout-only and auto-aligned to a grid.
- Replication groups render stacked containers; `US-West` mirrors the `US-East` structure.
- The replica container can override layout locally without altering the shared template.

SVG version (vector):

- `docs/specifications/diagramming/assets/auto_layout_example_subtypes_replication.svg`

---

## 13. Worked Layout Example (Replication Overrides + Dependency Lines)

Legend:

- Solid line: ERD relationship (cardinality + optionality)
- Dashed line: Dependency line (non-ERD annotation)

ASCII sketch showing dependency lines and a replica override:

```text
+------------------------------------------------------+
| Project                                              |
|  +--------------------+   +-----------------------+  |
|  | Region: US-East    |   | Region: US-West      |  |
|  |  +--------------+  |   | (replica override)   |  |
|  |  | DB: Billing  |  |   +-----------------------+  |
|  |  | +---------+  |  |                           |  |
|  |  | | INVOICE |  |  |   +--------------+        |  |
|  |  | +---------+  |  |   | DB: Billing  |        |  |
|  |  |  |..dep..|-----|---|->| INVOICE    |        |  |
|  |  | +---------+  |  |   +--------------+        |  |
|  |  | +---------+  |  |                           |  |
|  |  | |PAYMENT |  |  |                           |  |
|  |  | +---------+  |  |                           |  |
|  |  +--------------+  |                           |  |
|  +--------------------+                           |  |
+------------------------------------------------------+
```

Behavior notes:

- Dependency lines (dashed) route around label notches and avoid icon slots.
- In dense layouts, dependency lines may share a channel if they remain visually distinct from ERD links.
- Replication override allows the `US-West` replica to hide `PAYMENT` while retaining the shared base structure.

SVG version (vector):

- `docs/specifications/diagramming/assets/auto_layout_example_dense_dependencies.svg`
