# Decision Record 0001: Scope and Constraints

## Date

2026-02-18

## Decision

`robin-migrate` will preserve FlameRobin-style UX while treating ScratchBird
contracts as mandatory execution constraints.

## Rationale

1. User workflow continuity is a migration priority.
2. ScratchBird execution boundaries prevent backend drift.

## Consequences

1. UI parity can proceed without inheriting Firebird engine assumptions.
2. SQL execution logic remains outside engine boundary until compiled to SBLR.
