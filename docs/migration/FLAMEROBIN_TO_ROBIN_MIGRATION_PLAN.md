# FlameRobin to robin-migrate Plan

## Scope

Migrate FlameRobin experience patterns into a ScratchBird-native client while
removing Firebird-specific backend assumptions.

## Workstream Sequence

1. Inventory FlameRobin UI surfaces and interaction flows.
2. Classify each flow as:
   - reusable UI shell behavior
   - backend-bound behavior requiring ScratchBird contract rewrite
3. Implement contract-safe backend adapters first.
4. Reconnect UI flows to adapter APIs.
5. Run conformance and drift checks before each stage gate.

## Required Rewrites

1. Replace Firebird execution assumptions with native parser + SBLR pipeline.
2. Replace metadata identity assumptions with UUID-aware lookups.
3. Replace legacy connection/session options with parser-per-port policy.

## Exit Criteria

1. No Firebird execution assumptions remain in backend execution path.
2. All query flows pass through compile-to-SBLR before `ServerSession`.
3. UI parity baseline is documented and test-backed.
