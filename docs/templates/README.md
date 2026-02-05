# Templates

This directory contains default documentation templates used by the automated documentation system.

## Discovery

- ScratchRobin discovers templates by scanning `docs/templates/` at project load.
- Only files with supported extensions are loaded:
  - `.yaml`
  - `.yml`
  - `.json`
- The file name is used as the template id (e.g., `mop_template.yaml` => `mop_template`).

## Versioning

- Each template must include a `document.version` field (string, semantic version).
- Template revisions are tracked in project history when used to generate docs.
- Backwards-incompatible template changes should increment the major version.

## Conventions

- Templates should be deterministic and not require external state.
- If a template references deployment plans or diagrams, it should use paths relative to project root.

## Example

See:
- `docs/templates/mop_template.yaml`
