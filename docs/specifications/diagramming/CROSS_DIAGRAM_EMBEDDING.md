# Cross‑Diagram Embedding and Editing

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how one diagram type can embed or link another diagram type while maintaining unified style and shared semantics.

---

## 1. Embedding Model

- Any diagram may embed another as a **linked sub‑diagram object**.
- Embedded diagrams render as summary boxes with title, type icon, and status.
- Double‑click opens the linked diagram in its native editor.

---

## 2. Link Types

- **Embed (live)**: changes propagate both ways.
- **Reference (read‑only)**: view only, no edits.

---

## 3. Supported Mappings

Examples:
- Silverston entity → DFD process link
- Whiteboard card → ERD entity
- Mind map node → Silverston container

---

## 6. Worked Example (Silverston → DFD → Documentation)

### Step 1: Embed a DFD in a Silverston Entity

```text
┌──────────────────────────────────────────────┐
│ -{ CUSTOMER }-                     [entity] │
│  # CUSTOMER_ID                                  │
│  * NAME                                         │
│  o STATUS                                       │
│                                                │
│  ┌──────────────────────────────────────────┐  │
│  │ -{ DFD: Onboard Customer }-     [dfd]    │  │
│  │  (embedded summary)                       │  │
│  └──────────────────────────────────────────┘  │
└──────────────────────────────────────────────┘
```

The embedded DFD is linked to process steps such as **Validate Customer**, **Create Account**, and **Notify Sales**.

**Embedded DFD (SVG)**:

![DFD Onboard Customer](docs/specifications/diagramming/assets/dfd_onboard_customer.svg)

### Step 2: Generate Documentation (Data Dictionary)

```markdown
## Entity: CUSTOMER

| Attribute | Type | Required | Description |
| --- | --- | --- | --- |
| CUSTOMER_ID | UUID | Yes | System-generated identifier |
| NAME | VARCHAR(80) | Yes | Customer display name |
| STATUS | CUSTOMER_STATUS | No | Active/Inactive indicator |

Linked Process:
- DFD: Onboard Customer (Validate Customer → Create Account → Notify Sales)
```

---

## 4. Editing Rules

- Embedded objects can be renamed, but core structure edits must be done in the source diagram.
- Structural changes propagate to all linked parents.

---

## 5. Serialization

Embedded objects store:
- `diagram_id`
- `diagram_type`
- `link_type`
- `summary_view` config
