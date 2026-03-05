# Validation Strategy

## Layers

1. Unit tests
   - parser-port validation rules
   - compile-to-SBLR output typing
   - engine-boundary rejection behavior
2. Integration tests
   - adapter compile + forward + execute path
   - session state transitions across query lifecycle
3. Conformance tests
   - requirement-to-test mapping from execution contracts
4. UI smoke tests
   - object navigation, editor execution, result rendering

## Gates

1. Gate A: boundary unit tests pass.
2. Gate B: adapter integration tests pass.
3. Gate C: conformance matrix rows pass.
4. Gate D: UI smoke scenarios pass.
