# Execution Boundaries

## Normative Rules

1. The only engine execution boundary is `ServerSession`.
2. `ServerSession` execution requires bytecode flag and SBLR payload.
3. SQL text is rejected at the engine boundary.
4. `native_adapter` compiles and forwards; it does not execute engine queries.

## ScratchBird Baseline References

The migration design is aligned to the current ScratchBird behavior called out
by request context:

- `src/server/server_session.cpp:793` handles `QUERY`
- `src/server/server_session.cpp:815` executes on `QueryFlags::BYTECODE`
- `src/server/server_session.cpp:818` rejects SQL text
- `src/server/server_session.cpp:979` executes validated SBLR
- `src/protocol/adapters/native_adapter.cpp:829` marks adapter role
- `src/protocol/adapters/native_adapter.cpp:2011`
- `src/protocol/adapters/native_adapter.cpp:2030`
- `src/protocol/adapters/native_adapter.cpp:2040`
- `src/protocol/adapters/protocol_adapter.cpp:532`
- `src/protocol/adapters/protocol_adapter.cpp:589`

## Enforcement Strategy

1. Encode contracts in backend interfaces.
2. Unit-test rejection paths (wrong payload type, missing bytecode flag).
3. Reject any feature PR that introduces SQL execution in engine-side code.
