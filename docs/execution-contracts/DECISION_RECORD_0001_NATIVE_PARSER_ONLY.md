# Decision Record 0001: Native Parser Only

## Date

2026-02-18

## Decision

Only native ScratchBird parser support is in scope for this project.

## Rationale

1. Keeps execution path deterministic.
2. Avoids dialect fallback complexity during migration.

## Consequences

1. Non-native dialect requests are rejected.
2. Port-to-parser mapping is explicit and validated.
