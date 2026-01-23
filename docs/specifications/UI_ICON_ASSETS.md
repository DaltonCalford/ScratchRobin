# UI Icon Assets

Status: Draft
Last Updated: 2026-01-09

This document defines the icon asset location, format, and sizing for
ScratchRobin UI commands.

## Location

- Base directory: `assets/icons/`

## Formats

- SVG: canonical source
- PNG: raster exports for runtime usage

## Sizes (PNG)

- 24x24
- 32x32
- 48x48

Notes:
- 16x16 is intentionally unsupported.
- Icons should be designed to read at 24x24 and scale cleanly to 48x48.
- All icons should be provided in both light and dark variants when needed.

## Naming Convention

- `command-name.svg`
- `command-name@24.png`
- `command-name@32.png`
- `command-name@48.png`

Example:
- `connect.svg`
- `connect@24.png`
- `connect@32.png`
- `connect@48.png`

## Iconbar Rules

- The iconbar mirrors menu actions.
- Each window type can add or remove icons in its local configuration.
- Users may hide the iconbar per window type to maximize workspace.

