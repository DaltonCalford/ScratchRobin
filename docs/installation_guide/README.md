# ScratchRobin Installation Guide

## Scope

This guide covers installation of ScratchRobin beta1b application artifacts.

## Linux

1. Install the package for your platform profile.
2. Confirm these files are present after install:
   - `config/scratchrobin.toml.example`
   - `config/connections.toml.example`
   - `docs/LICENSE.txt`
   - `docs/ATTRIBUTION.txt`
3. Start runtime validation:
   - `scratchrobin --runtime-startup`

## Manifest Validation

Before release, validate package manifest contracts:

- `scratchrobin --validate-package-manifest=manifest/profile_manifest.json`

Optional explicit resources:

- `--surface-registry=resources/schemas/package_surface_id_registry.json`
- `--manifest-schema=resources/schemas/package_profile_manifest.schema.json`

## Release Gate

Validate blocker-based promotability:

- `scratchrobin --release-gate-check`

Override blocker register path:

- `--blocker-register=<path/to/BLOCKER_REGISTER.csv>`
