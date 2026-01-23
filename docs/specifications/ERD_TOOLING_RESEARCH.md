# ERD Tooling Research

Status: Draft
Last Updated: 2026-01-09

This document surveys diagramming/tooling options for implementing ERD
features in ScratchRobin (wxWidgets/C++). It includes references and a
recommendation path.

## Scope

- Diagram canvas and interaction model
- Auto-layout / routing engines
- Serialization and export
- Licensing and maintenance risk

## Candidate Libraries and Approaches

### 1) Custom Canvas (wxWidgets + wxGraphicsContext)

Summary:
- Build an in-house diagram canvas using wxWidgets drawing primitives
  (`wxGraphicsContext` or custom paint with `wxDC`).

Pros:
- Full control over interaction model and performance.
- No external dependencies.
- Tailored to ERD requirements and multi-notation rendering.

Cons:
- Requires building shape/connector frameworks from scratch.
- Auto-layout and routing must be integrated separately.

Notes:
- The current ScratchRobin stack already uses wxWidgets; this is the lowest
  friction option for long-term control.

### 2) wxShapeFramework (wxSF)

Summary:
- A wxWidgets-based diagram framework for graphical shapes and connectors.
- Supports shape canvas, serialization, clipboard, drag/drop, undo/redo, and
  export to bitmap.

Evidence:
- wxSF readme describes a shape canvas with serialization to XML, clipboard,
  drag/drop, undo/redo, and export support.
  Source: https://raw.githubusercontent.com/marius-luca-87/wxShapeFramework/master/readme_src.txt

Pros:
- Feature-rich base (canvas, undo, serialization).
- License compatible with wxWidgets (wxWindows Library Licence 3.1).
  Source: https://raw.githubusercontent.com/marius-luca-87/wxShapeFramework/master/license.txt

Cons:
- Original author references 2007; maintenance risk.
- Integration effort to modernize, update build system, and align with
  ScratchRobin’s data model.

### 3) wxArt2D

Summary:
- A 2D drawing/diagramming library built on wxWidgets.

Evidence:
- Repository indicates LGPL 2.1 licensing.
  Source: https://raw.githubusercontent.com/hextzd/wxart2d/master/LICENSE
- Project home: https://www.wxart2d.org/cgi-bin/moin.cgi/wxArt2D

Pros:
- Mature drawing primitives.

Cons:
- LGPL licensing may complicate distribution.
- Unknown maintenance cadence and integration cost.

### 4) Graphviz for Auto-Layout

Summary:
- External layout engine to compute node positions for ERDs.

Evidence:
- Graphviz is open source graph visualization software.
  Source: https://graphviz.org/
- Layout engines include dot, neato, fdp, sfdp, circo, twopi.
  Source: https://graphviz.org/docs/layouts/
- Licensed under the Common Public License 1.0.
  Source: https://graphviz.org/license/

Pros:
- Proven layouts for large graphs.
- Multiple layout engines suited for different diagram types.

Cons:
- External dependency and process/library integration required.
- CPL license considerations for redistribution.

## Local Findings

- wxWidgets 3.2.9 source in `/home/dcalford/CliWork/wxWidgets-3.2.9` does not
  include the legacy OGL module; a built-in diagram framework is not available
  out-of-the-box. (Local source inspection)

## Recommendation

Primary path (lowest risk, most control):
- Implement a custom diagram canvas using wxWidgets drawing primitives.
- Add an internal object model for nodes/edges and render per notation.
- Add optional Graphviz integration for auto-layout, with a fallback
  in-app layout for small graphs.

Secondary path (optional accelerator):
- Evaluate wxShapeFramework as a prototyping base if a faster proof-of-
  concept is needed, but treat it as a dependency-risk due to age.

## Integration Guidance

Canvas:
- Use `wxScrolledWindow` + `wxGraphicsContext` for vector drawing.
- Maintain a scene graph of diagram elements with UUID mappings.

Auto-layout:
- Graphviz DOT serialization for layout runs.
- Import positions back into the canvas scene.

Routing:
- Start with orthogonal routing heuristics for ERD relationships.
- Extend with a dedicated router if required (future work).

Serialization:
- JSON schema aligned with `ERD_MODELING_AND_ENGINEERING.md`.

## Open Questions

- Do we need embedded Graphviz (library) or external process invocation?
- Should layout be deterministic (stable between runs)?
- How should notation switching impact element geometry?

