# Dataflow Diagram Specification

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the dataflow diagram (DFD) model, notation, behaviors, serialization, and round‑trip rules in ScratchRobin. Dataflow diagrams describe how data moves between processes, stores, and external entities.

---

## 1. Core Concepts

### 1.1 Element Types

- **Process**: transforms or routes data
- **Data Store**: persistent storage
- **External Entity**: data source/sink outside the system
- **Data Flow**: directional movement of data between elements

### 1.2 Levels of Detail

- Context (Level 0): single process with external entities
- Level 1+: decomposed processes with internal flows

---

## 2. Visual Notation

### 2.1 Process

- Rounded rectangle or circle
- Label inside (verb + object recommended)

### 2.2 Data Store

- Open‑ended rectangle (or double‑line rectangle)
- Label on top

### 2.3 External Entity

- Rectangle
- Label inside

### 2.4 Data Flow

- Arrowed line
- Label on the line (data name)

---

## 3. Data Model

### 3.1 Diagram Document

- `version`
- `diagram_id`
- `title`
- `level`
- `metadata` (created_at, updated_at, author)
- `elements` (processes, stores, externals)
- `flows` (connections)
- `layout` (viewport, grid, zoom)

### 3.2 Process

- `id`
- `name`
- `description`
- `position`, `size`
- `decomposition_ref` (link to child diagram)

### 3.3 Data Store

- `id`
- `name`
- `description`
- `position`, `size`

### 3.4 External Entity

- `id`
- `name`
- `description`
- `position`, `size`

### 3.5 Data Flow

- `id`
- `source_id`
- `target_id`
- `label`
- `notes`

---

## 4. Interaction Rules

- Drag to create elements from palette
- Drag from element edge to create flow
- Double‑click process to open decomposition (Level n+1)
- Label flows inline or via properties panel

---

## 5. Validation Rules

- Flows must connect valid element types
- No flow directly between two data stores (must pass through a process)
- No flow directly between two external entities
- Each diagram must have unique element IDs

---

## 6. Serialization Format

**File Extension**: `.sbdfd`

```json
{
  "version": "1.0",
  "diagram_id": "uuid",
  "title": "Order Fulfillment",
  "level": 1,
  "metadata": {"created_at": "2026-02-05T12:00:00Z"},
  "layout": {"zoom": 1.0, "grid": true},
  "elements": [
    {"id": "p1", "type": "process", "name": "Validate Order", "position": {"x": 120, "y": 140}, "size": {"w": 180, "h": 80}},
    {"id": "ds1", "type": "data_store", "name": "Orders", "position": {"x": 380, "y": 140}, "size": {"w": 160, "h": 80}},
    {"id": "e1", "type": "external", "name": "Customer", "position": {"x": 40, "y": 140}, "size": {"w": 140, "h": 80}}
  ],
  "flows": [
    {"id": "f1", "source_id": "e1", "target_id": "p1", "label": "Order Request"},
    {"id": "f2", "source_id": "p1", "target_id": "ds1", "label": "Validated Order"}
  ]
}
```

---

## 7. Round‑Trip Rules

- Dataflow diagrams are conceptual and do not directly generate schema.
- Processes may be linked to ERD entities for traceability.
- Reverse‑engineering from database is not automatic.

### 7.1 DFD → ERD Traceability (Short Rules)

- Each **Process** may reference one or more ERD entities (e.g., `Process: Validate Order` → `ORDER`, `ORDER ITEM`).\n- Each **Data Store** should map to a target ERD entity or table when known.\n- Each **External Entity** may map to an ERD “Party” or system boundary actor.\n- Traceability links are metadata only and do not alter the DFD layout.

---

## 8. Printing and Export

- Export formats: PNG, SVG, PDF.
- Export options: full diagram, viewport, or selection.

---

## 9. Edge Cases

- Large diagrams must use virtualized rendering.
- Orphaned flows are highlighted and require resolution.

---

## 10. Worked Example (Order Fulfillment DFD)

```text
 [Customer] --> (Validate Order) --> ||Orders||
      |                |               |
      |                v               v
      |            (Check Credit)   ||Payments||
      v
 [Shipping Dept] <-- (Create Shipment) --> ||Shipments||
```

![ASCII Diagram](/home/dcalford/CliWork/ScratchRobin/docs/specifications/diagramming/assets/ascii_svgs/dataflow_diagram_specification_ascii_1.svg)

**Legend**:
- `(Process)` = Process\n- `||Store||` = Data Store\n- `[External]` = External Entity
