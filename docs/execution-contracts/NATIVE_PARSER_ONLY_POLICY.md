# Native Parser Only Policy

## Policy Statement

`robin-migrate` supports only the native ScratchBird parser and native wire
translation path.

## Implications

1. No cross-dialect parser plugin support in this phase.
2. No compatibility parser fallback.
3. Port and parser mapping is explicit and immutable at runtime unless
   reconfigured.

## Acceptance Criteria

1. Any non-native parser configuration attempt fails validation.
2. All successful query requests flow through native parser -> SBLR.
3. Engine execution path sees SBLR only.
