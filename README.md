# robin-migrate

`robin-migrate` is a FlameRobin-style database administration client skeleton
retargeted to ScratchBird.

This repository is the migration workspace for a ScratchBird-native client with
similar look and feel to FlameRobin, while enforcing ScratchBird execution
contracts:

1. Engine execution boundary is `ServerSession` only.
2. Engine executes validated SBLR bytecode, not SQL text.
3. `native_adapter` is parser/wire translation, not a second engine path.
4. One dialect parser per configured port, with no parser auto-detect fallback.
5. Native ScratchBird parser is the only parser supported by this project.

## Current State

This is an implementation skeleton with:
- CMake build wiring
- backend contracts encoded as compile-time structure
- SBWP transport integration points for ScratchBird native listener
- wxWidgets desktop shell with FlameRobin-style split layout
- task-oriented specs in `docs/`

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/robin-migrate            # preview mode (default, no server required)
./build/src/robin-migrate --mode=managed   # auto-start managed listener/server
./build/src/robin-migrate --mode=listener-only  # connect to local listener on configured port
./build/src/robin-migrate --mode=ipc-only      # connect via IPC/socket path
./build/src/robin-migrate --mode=embedded      # embedded mode (single local engine instance)
./build/robin-migrate                 # CLI binary
./build/src/robin-migrate              # also available in build/src
./build/robin-migrate-gui              # GUI binary
./build/src/robin-migrate-gui          # also available in build/src
```

## Review Mode

`robin-migrate` now defaults to a non-connected preview mode so UI/flow review
can proceed while parser/native listener work is in progress.

- CLI: preview by default, use `--mode=` to pick the transport mode:
  - `managed` (auto-start/auto-discover)
  - `listener-only` (TCP localhost)
  - `ipc-only` (socket/pipe)
  - `embedded` (single-connection, no-server maintenance mode)
  - `--live` remains supported as an alias for `listener-only`.
- GUI: FlameRobin-style main window (tree + search bar + menus), open SQL editor via
  `Database -> Open new Volatile SQL Editor...`
- GUI mode switch: `Database -> Use Preview Mode (No Connection)` (default),
  `Managed`, `Listener-Only`, `IPC-Only`, or `Embedded`.

## Repository Layout

- `src/`: executable and core/backend/UI scaffolding
- `tests/`: contract-focused test skeletons
- `docs/`: goals, architecture, migration specs, execution contracts, tests, plans
