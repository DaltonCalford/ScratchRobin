---
id: ui.diagram.workflow
title: Diagram Workflow
category: ui
window: Diagram
commands: ["command.diagram.create.erd"]
backends: ["scratchbird", "postgresql", "mysql", "firebird", "mock"]
---

# Diagram Workflow

Diagram windows support ERD, data-flow, and UML workflows.

Typical flow:
1. Create a diagram from the Connections or Window menu.
2. Choose a notation and diagram type (conceptual/logical/physical).
3. Add entities and relationships on the canvas.
4. Reverse-engineer from an existing catalog when a connection is available.
5. Forward-engineer to preview DDL output.

Full ERD modeling rules are described in the ERD specification.
