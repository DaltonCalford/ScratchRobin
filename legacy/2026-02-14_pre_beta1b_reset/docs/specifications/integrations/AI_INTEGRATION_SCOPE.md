# AI Integration Scope and Policy

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the boundaries, privacy constraints, and usage scope of AI features in ScratchRobin.

---

## Scope

AI features may include:
- SQL assistance
- Query optimization hints
- Documentation drafting

AI features must not:
- Execute destructive SQL without user confirmation
- Send credentials or connection strings to external services

---

## Data Handling

- Redact secrets before any AI request.
- Provide opt-in user consent for AI features.
- Allow offline mode with AI disabled.

---

## Edge Cases

- If AI service is unavailable, UI should degrade gracefully and log the failure.

