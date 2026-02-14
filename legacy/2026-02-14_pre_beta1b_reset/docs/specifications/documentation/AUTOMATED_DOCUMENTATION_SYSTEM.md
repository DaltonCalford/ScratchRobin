# Automated Documentation System

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the template‑driven documentation system that generates data dictionaries, code libraries, and system documentation from diagrams and models.

---

## 1. Inputs

- Diagrams (Silverston, ERD, DFD, Whiteboard)
- Model metadata (entities, attributes, domains, relationships)
- Template definitions

---

## 2. Outputs

- Data Dictionary (entities, attributes, types, constraints)
- Code Libraries (SQL, API scaffolding)
- Process Documentation (from DFD)
- Methods/Operations/Procedures References (see generated docs spec)

---

## 3. Template System

Templates define:
- Sections and layout
- Which diagram elements to include
- Formatting rules
- Output mappings for executable objects (procedures, triggers, jobs)

### Template Discovery

Default templates are discovered by scanning:

- `docs/templates/`

Supported file extensions:

- `.yaml`
- `.yml`
- `.json`

The file name is used as the template id (e.g., `mop_template.yaml` → `mop_template`).

---

## 4. Example Template (YAML)

```yaml
document:
  title: "Data Dictionary"
  sections:
    - title: "Entities"
      source: "erd.entities"
      fields: [name, description, attributes]
```

---

## 5. Rendering Rules

- Each template can generate multiple output formats (Markdown, PDF, HTML).
- Embedded diagrams appear as SVG in documentation.

---

## 6. Versioning

- Documentation is versioned alongside project revisions.
- Generated docs include timestamps and build metadata.

---

## 7. Worked Example (Embedded DFD → Data Dictionary)

**Input**: A Silverston entity `CUSTOMER` embeds a DFD summary `Onboard Customer`.

**Generated Output (Excerpt)**:

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

## 8. Generated Executable Documentation

For detailed rules and output structure for procedures, triggers, and operations, see:

- `docs/specifications/documentation/GENERATED_METHODS_AND_OPERATIONS_SPEC.md`
