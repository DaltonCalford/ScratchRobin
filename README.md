# ScratchRobin

ScratchRobin is a lightweight, ScratchBird-native database administration tool inspired by FlameRobin.
This repository has been reset to a fresh rewrite baseline; the previous implementation remains in Git history only.

## Scope (Initial)
- ScratchBird only (no other dialects yet)
- SDI windowing model (each editor/inspector is its own top-level window)
- Native widget stack via wxWidgets for portability and low overhead

## Status
Early rewrite skeleton. The app boots, opens a main window, and can spawn an SQL editor window.

## Build
Prerequisites:
- CMake 3.20+
- C++17 compiler
- wxWidgets 3.2+ development packages

Build:
```bash
mkdir build
cmake -S . -B build
cmake --build build
```

Run:
```bash
./build/scratchrobin
```

## Docs
- `docs/ARCHITECTURE.md`
- `docs/CONFIGURATION.md`
- `docs/ROADMAP.md`
- `docs/plans/ROADMAP.md`

## Configuration
Modernized config lives in TOML. Example files:
- `config/scratchrobin.toml.example`
- `config/connections.toml.example`
