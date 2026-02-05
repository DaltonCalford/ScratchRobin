# Diagram Undo/Redo Specification

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines undo/redo behavior for diagram editing.

---

## Rules

- Each user action is recorded as an atomic operation.
- Undo stack size is configurable (default 100).
- Redo stack is cleared when a new action occurs.

---

## Edge Cases

- Auto-layout is treated as a single undoable action.
- Bulk imports generate one grouped undo step.

