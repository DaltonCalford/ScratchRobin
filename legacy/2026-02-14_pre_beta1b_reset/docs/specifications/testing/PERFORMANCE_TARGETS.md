# Performance Targets

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines minimum performance thresholds for key operations.

---

## Targets

- App startup: < 2 seconds on mid-tier laptop.
- Project load (10k objects): < 3 seconds.
- Query execution UI response: < 200 ms for result start.
- Diagram auto-layout (500 nodes): < 2 seconds.

---

## Measurement

- Benchmarks run on standardized hardware profile.
- Metrics stored in `docs/specifications/testing/PERFORMANCE_BENCHMARKS.md`.

---

## Edge Cases

- If targets not met, feature must degrade gracefully (progress indicator, cancel option).

