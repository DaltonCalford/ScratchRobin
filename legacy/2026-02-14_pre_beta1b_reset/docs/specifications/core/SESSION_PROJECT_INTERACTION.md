# Session and Project Interaction Rules

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how session persistence and project persistence interact.

---

## Rules

- Session state stores UI layout and open windows only.
- Project state stores design data and configurations.
- Session restore must validate that project exists; if missing, skip windows.

---

## Edge Cases

- If session references a project that was moved, prompt user to locate it.
- If project fails to load, session restore should continue for other projects.

