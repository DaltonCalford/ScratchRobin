# Cross-Platform UI Constraints

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines constraints to ensure consistent UI across Windows, macOS, and Linux.

---

## Constraints

- Avoid platform-specific font metrics; use layout that tolerates variations.
- All icons must be SVG or vector-friendly.
- Minimum touch target: 24x24 px.
- DPI scaling must be supported up to 200%.

---

## Edge Cases

- If a platform lacks a native control feature, use a consistent custom widget.

