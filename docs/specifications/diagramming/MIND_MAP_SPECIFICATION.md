# Mind Map Diagram Specification

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the data model, UI behaviors, serialization format, keyboard shortcuts, and round‑trip rules for ScratchRobin mind map diagrams.

Mind maps are used for conceptual planning, exploratory design, and early‑stage schema ideation before formal ERD work.

---

## 1. Data Model

### 1.1 Node

A node represents a concept or idea. Nodes form a tree with a single root.

**Fields:**
- `id` (string, unique)
- `text` (string)
- `parent_id` (string, empty for root)
- `children` (array of node ids)
- `position` (x, y) for layout override
- `style`:
  - `color`
  - `icon`
  - `shape` (rounded, rectangle, pill)
  - `font_size`

### 1.2 Edge

Implicit, based on parent/child. Edge styling is derived from node style.

### 1.3 Mind Map Document

- `version`
- `root_id`
- `nodes` (array)
- `layout` (algorithm + parameters)
- `metadata` (created_at, updated_at, author)

---

## 2. UI Behaviors

### 2.1 Creation

- Create a mind map with a single root node.
- Root node is always present and cannot be deleted.

### 2.2 Editing

- Double‑click or Enter to edit node text.
- Tab creates a child node.
- Shift+Tab creates a sibling node.

### 2.3 Navigation

- Arrow keys move focus between nodes (up/down sibling, left/right parent/child).
- Ctrl+Arrow reorders nodes among siblings.

### 2.4 Styling

- Each branch inherits root color unless overridden.
- Icons optional; color palette should be consistent with ScratchRobin design system.

### 2.5 Collapse/Expand

- Nodes can be collapsed; children hidden.
- Collapse state is stored in diagram state, not metadata model.

---

## 3. Serialization Format

Mind maps are stored in JSON.

**File Extension**: `.sbmind`

**Example:**

```json
{
  "version": "1.0",
  "root_id": "node_root",
  "metadata": {
    "created_at": "2026-02-05T12:00:00Z",
    "updated_at": "2026-02-05T12:10:00Z",
    "author": "alice"
  },
  "layout": {
    "algorithm": "radial",
    "spacing": 120
  },
  "nodes": [
    {
      "id": "node_root",
      "text": "Project Goals",
      "parent_id": "",
      "children": ["node_1", "node_2"],
      "style": {"color": "#3A78C3", "shape": "pill", "font_size": 16}
    },
    {
      "id": "node_1",
      "text": "Schema Design",
      "parent_id": "node_root",
      "children": [],
      "style": {"color": "#5CA15C", "shape": "rounded", "font_size": 14}
    }
  ]
}
```

---

## 4. Keyboard Shortcuts

| Shortcut | Action |
| --- | --- |
| `Enter` | Edit node text |
| `Tab` | Create child node |
| `Shift+Tab` | Create sibling node |
| `Delete` | Delete node (not root) |
| `Ctrl+Arrow` | Reorder among siblings |
| `Space` | Collapse/expand node |

---

## 5. Round‑Trip Rules

### 5.1 Mind Map → Metadata

Mind maps are conceptual and do not directly generate metadata objects by default. Optionally:
- Nodes tagged with schema/table icons may be converted to Draft objects.
- Conversion is explicit via UI action “Convert to Drafts.”

### 5.2 Metadata → Mind Map

- No automatic population from metadata.
- User may import existing objects as a new branch.

---

## 6. Edge Cases

- Deleting a node deletes its subtree.
- Reparenting nodes updates all descendants’ positions.
- If file is missing style fields, defaults are applied.

---

## 7. Implementation Notes

- Rendering should be fast for up to 1,000 nodes.
- Layout algorithms: radial for small maps, horizontal for large.
- Serialization should ignore unknown fields for forward compatibility.

