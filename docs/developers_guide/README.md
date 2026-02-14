# ScratchRobin Developers Guide

## Core Service Layers

- `runtime`: startup config/session lifecycle contracts.
- `project`: project binary and specset support contracts.
- `connection`: backend adapter and capability gates.
- `ui`: deterministic UI workflow contracts.
- `diagram`: model, canvas, export, and migration workflows.
- `reporting`: question/dashboard/runtime/storage/schedule contracts.
- `advanced`: CDC, masking, collaboration, extension, and preview gate contracts.
- `packaging`: manifest/resource/spec-support validation contracts.
- `release`: blocker gates, alpha-preservation checks, promotability verdict.

## Conformance Workflow

1. Build and run tests:
   - `cmake --build build -j$(nproc)`
   - `ctest --test-dir build --output-on-failure`
2. Conformance output:
   - test binaries emit `SummaryJson` records.
   - conformance vector test emits `ConformanceSummaryJson` with failed case ids.
3. Release gate:
   - `scratchrobin --release-gate-check`

## Packaging Resources

- `resources/schemas/package_profile_manifest.schema.json`
- `resources/schemas/package_surface_id_registry.json`
- `resources/templates/package_profile_manifest.example.json`

Use `scratchrobin --validate-package-manifest=<path>` for file-based contract validation.
Use `scratchrobin --check-package-artifacts=<path>` for filesystem artifact contract validation.
