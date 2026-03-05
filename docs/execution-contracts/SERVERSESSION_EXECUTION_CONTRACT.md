# ServerSession Execution Contract

## Contract

1. `ServerSession` is the only engine execution boundary.
2. Execution is permitted only for bytecode-flagged SBLR payloads.
3. SQL text requests are invalid at this layer.

## Input Preconditions

1. `QueryFlags::BYTECODE` equivalent must be true.
2. Payload type must be SBLR bytecode.
3. Request must already pass parser/adapter validation.

## Rejection Paths

1. Missing bytecode flag -> reject request.
2. SQL payload at engine boundary -> reject request.
3. Unknown/invalid payload format -> reject request.

## Test Contract

1. Positive: bytecode + flag executes.
2. Negative: SQL text fails.
3. Negative: bytecode payload without flag fails.
