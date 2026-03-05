# Native Adapter Contract

## Role

`native_adapter` in this migration project is a parser/wire and session state
boundary. It is not an execution engine.

## Responsibilities

1. Accept SQL text from UI/client session path.
2. Compile SQL to SBLR using native parser.
3. Apply parser-per-port mapping validation.
4. Forward bytecode request to `ServerSession` path.
5. Track protocol/session state transitions.

Current implementation note:
- `ScratchbirdSbwpClient` submits SQL over SBWP to the native listener.
- Native listener/parser layer performs SQL -> SBLR and forwards to engine boundary.

## Forbidden Behavior

1. Direct execution of SQL text.
2. Bypassing `ServerSession` for query execution.
3. Fallback protocol parser auto-detection.
