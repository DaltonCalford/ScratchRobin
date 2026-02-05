# Generated Methods, Operations, and Procedures Documentation

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the documentation outputs and generation rules for methods, operations, procedures, triggers, and automation objects derived from Silverston diagrams and ScratchBird metadata.

This spec focuses on **project-level generated documentation**, not database catalogs. It is used by the automated documentation system to emit readable, versioned references.

---

## 1. Inputs

- Silverston diagrams (procedures, functions, triggers, jobs, packages)
- ERD metadata (tables, columns, constraints)
- DFD metadata (processes, inputs/outputs, data stores)
- Whiteboard notes and domain hints
- ScratchBird metadata (DDL, procedural language, scheduler, operations)
- Templates and style profiles

---

## 2. Output Document Types

Required generated documents:

- **Procedure Reference**: Procedures, functions, packages
- **Trigger Catalog**: Triggers, scope, invocation, side effects
- **Operations Runbook**: Jobs, pipelines, backups, health checks
- **Method Index**: Cross-linked listing of all executable objects
- **Parameter Glossary**: Normalized parameters and domains
- **Traceability Matrix**: Diagram object → generated doc → source spec

Optional outputs:

- **Security Map** (roles/privileges to executable objects)
- **API Stubs** (language-specific wrappers)

---

## 3. Canonical Sections

Each generated object entry includes:

- Name and identifier
- Scope (cluster, database, schema, table)
- Signature (parameters, return type)
- Triggers (for procedures/events)
- Preconditions and constraints
- Side effects (tables updated, external calls)
- Error conditions and retry policy
- Dependencies (other objects, external services)
- Example invocation

---

## 4. Template Model

Templates are declarative and target a document type.

```yaml
document:
  type: "procedure_reference"
  title: "Procedure Reference"
  sections:
    - title: "Procedures"
      source: "catalog.procedures"
      fields:
        - name
        - scope
        - signature
        - dependencies
        - errors
```

---

## 5. Generation Rules

1. **Object Resolution**
   - Objects are collected from diagrams and ScratchBird metadata.
   - Diagram overrides metadata when both exist.

2. **Signature Resolution**
   - Parameter types are resolved via domains when available.
   - Untyped params fall back to `DEFAULT_TYPE` until resolved.

3. **Cross-Linking**
   - Every object entry links to:
     - The diagram that declared it
     - The ScratchBird spec that defines it
     - Any dependent objects

4. **Versioning**
   - Each generated document includes:
     - `build_id`
     - `diagram_revision`
     - `timestamp`

---

## 6. Output Layout and Paths

Generated documents are stored under:

- `docs/generated/`
  - `methods/`
  - `procedures/`
  - `operations/`
  - `triggers/`
  - `indexes/`

Each run creates:

- `docs/generated/index.md`
- A per-object file: `docs/generated/procedures/<name>.md`

---

## 7. Worked Example (Procedure)

**Diagram Object**: `PROC_CREATE_CUSTOMER` (Scope: `schema.public`)

**Generated Entry**:

```markdown
## PROC_CREATE_CUSTOMER

Scope: schema.public  
Signature: (p_name TEXT, p_email TEXT) -> CUSTOMER_ID UUID

Dependencies:
- Table: CUSTOMER
- Domain: EMAIL_ADDRESS

Errors:
- EMAIL_INVALID
- CUSTOMER_DUPLICATE
```

---

## 8. Traceability Rules

- If a procedure is defined in multiple diagrams, one is selected as primary and the others appear as references.
- Trigger objects in diagrams always generate a Trigger Catalog entry.
- Operations (job/pipeline/backup) always generate a runbook entry.

