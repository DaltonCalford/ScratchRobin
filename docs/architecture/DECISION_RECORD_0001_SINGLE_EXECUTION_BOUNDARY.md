# Decision Record 0001: Single Execution Boundary

## Date

2026-02-18

## Decision

The architecture enforces one engine execution boundary at `ServerSession`.

## Rationale

1. Matches current ScratchBird engine contract.
2. Prevents hidden execution paths in adapters or UI services.

## Consequences

1. Adapter code is strictly compile/translate/forward.
2. Engine-facing tests focus on SBLR-only enforcement.
