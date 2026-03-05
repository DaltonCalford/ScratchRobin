# System Architecture

## Layers

1. UI layer (`src/ui`)
   - presents FlameRobin-style interaction surface
   - no engine execution semantics
2. App/session orchestration (`src/app`, `src/backend/session_client`)
   - issues operations via adapter contracts
3. Native adapter boundary (`src/backend/native_adapter_gateway`)
   - submits SQL text to native parser listener (SBWP)
   - relies on native parser to compile SQL into SBLR before engine submission
   - enforces parser-per-port mapping
   - forwards request over protocol boundary
   - uses `ScratchbirdSbwpClient` for native listener/session wiring
4. Engine boundary (`src/backend/server_session_gateway`)
   - receives only bytecode-flagged SBLR requests
   - rejects SQL text input

## Core Principle

UI and adapter layers may work with SQL text as user-facing material.
Engine-facing execution must operate on SBLR only.

## Expected Future Modules

1. Metadata browser adapters for UUID-backed object identity
2. DDL/DML action models backed by SBLR templates
3. Connection/session state manager aligned with ScratchBird wire protocol
4. Capability negotiation from server metadata, not from legacy assumptions
