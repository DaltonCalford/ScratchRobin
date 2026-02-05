# Connection Profile Editor Specification

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the UI and behavior for creating and managing database connection profiles.

---

## Functional Requirements

- Create, edit, duplicate, delete profiles
- Backend selection (ScratchBird/PostgreSQL/MySQL/Firebird)
- Credential store integration
- SSL/TLS configuration
- Connection test workflow

---

## Validation Rules

- Required fields per backend
- Port range validation
- Hostname/IP format validation
- SSL certificate path validation

---

## Edge Cases

- Credential store unavailable → fallback to plaintext warning

