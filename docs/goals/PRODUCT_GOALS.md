# Product Goals

## Objective

Deliver a FlameRobin-style desktop administration client that is native to
ScratchBird contracts and does not preserve Firebird-specific execution
assumptions.

## Primary Goals

1. Preserve the user-level interaction model:
   - object tree navigation
   - SQL/editor workflow
   - result grid and metadata inspection
2. Route all executable work through ScratchBird-native boundaries.
3. Support ScratchBird feature surface area as the backend capability source.
4. Keep parser support fixed to native ScratchBird parser only.
5. Make backend boundaries explicit and testable at compile and runtime.

## Architectural Invariants

1. `ServerSession` is the single engine execution path.
2. Engine accepts only validated SBLR for query execution.
3. SQL text must be compiled before submission to `ServerSession`.
4. `native_adapter` is parser/wire translation and session state, not engine.
5. One parser dialect per configured port, with no parser auto-detect fallback.

## Stage Gates

1. Stage A: skeleton and boundary contracts complete.
2. Stage B: UI parity scaffolding complete.
3. Stage C: native adapter/session integration contract complete.
4. Stage D: feature parity matrix and conformance checks complete.
5. Stage E: release hardening and packaging baseline complete.
