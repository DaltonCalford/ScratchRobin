# Audit Logging Specification

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines audit logging for project object changes, approvals, and deployments.

---

## Events Logged

- Object state transitions
- Deployment plan executions
- Git sync operations
- Conflict resolutions
- Reporting actions (run, refresh, alert evaluation, subscriptions)

---

## Log Format

```json
{
  "timestamp": "2026-02-05T12:00:00Z",
  "actor": "alice",
  "event": "state_transition",
  "object_id": "uuid",
  "from": "MODIFIED",
  "to": "PENDING",
  "reason": "ready for review"
}
```

---

## Storage

- Stored in project-local `audit.log` (append-only)
- May also be exported to external logging systems

---

## Edge Cases

- If log write fails, UI should warn but not block.
