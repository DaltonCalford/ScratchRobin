# Decision Record 0001: Contract-First Validation

## Date

2026-02-18

## Decision

Testing strategy starts with execution-contract conformance before feature-depth
expansion.

## Rationale

1. Boundary correctness is the highest-risk area in migration.
2. Contract regressions can invalidate all higher-level behavior.

## Consequences

1. Unit and integration gates focus first on SBLR and boundary checks.
2. Feature tests are layered after contract gates are stable.
