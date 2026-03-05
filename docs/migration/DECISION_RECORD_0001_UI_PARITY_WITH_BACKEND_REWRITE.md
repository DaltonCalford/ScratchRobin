# Decision Record 0001: UI Parity with Backend Rewrite

## Date

2026-02-18

## Decision

Migration sequencing prioritizes UI interaction parity while replacing backend
execution semantics with ScratchBird-native flows.

## Rationale

1. Reduces user retraining costs.
2. Isolates backend risk behind explicit adapter contracts.

## Consequences

1. Backend integration checkpoints are mandatory before parity sign-off.
2. Visual parity does not imply backend parity unless conformance gates pass.
