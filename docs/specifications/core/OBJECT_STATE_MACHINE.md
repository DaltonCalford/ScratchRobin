# ScratchRobin Object State Machine

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the lifecycle states for project objects and the rules for transitioning between them. This specification ensures consistent behavior across UI, Git sync, deployment planning, and auditing.

---

## States

| State | Meaning | Typical Owner | Mutability |
| --- | --- | --- | --- |
| EXTRACTED | Baseline object read from source DB | System | Read-only |
| NEW | Object created in design workspace | Designer | Editable |
| MODIFIED | Existing object changed from baseline | Designer | Editable |
| DELETED | Marked for removal from target | Designer | Editable until approved |
| PENDING | Ready for review | Reviewer | Read-only by default |
| APPROVED | Approved for deployment | Approver | Locked (changes require re-review) |
| REJECTED | Rejected by reviewer | Reviewer | Editable |
| IMPLEMENTED | Deployed to target environment | System | Read-only |
| CONFLICTED | Sync conflict detected | System | Read-only until resolved |

---

## Transition Rules

### Allowed Transitions

| From | Allowed To | Notes |
| --- | --- | --- |
| EXTRACTED | MODIFIED, DELETED | Editing baseline creates a diff |
| NEW | PENDING, DELETED | Delete removes pending creation |
| MODIFIED | PENDING, DELETED | Delete indicates drop on deploy |
| DELETED | PENDING | Requires review to execute drop |
| PENDING | APPROVED, REJECTED | Review decision |
| APPROVED | IMPLEMENTED, REJECTED | Rejected if post-approval issues found |
| REJECTED | MODIFIED, DELETED, PENDING | Rework and resubmit |
| IMPLEMENTED | MODIFIED, DELETED | New change after deployment |
| CONFLICTED | MODIFIED, DELETED | After resolution, object is editable |

### Disallowed Transitions

- EXTRACTED → APPROVED (must be MODIFIED or DELETED before review)
- NEW → APPROVED (must be PENDING first)
- PENDING → MODIFIED (must be REJECTED or explicitly “reopen” by reviewer)
- IMPLEMENTED → PENDING (must be MODIFIED/DELETED then PENDING)

---

## State Triggers

### User Actions

- **Create object** → NEW
- **Edit object**:
  - if EXTRACTED → MODIFIED
  - if IMPLEMENTED → MODIFIED
- **Delete object**:
  - if NEW → DELETED
  - if MODIFIED/EXTRACTED/IMPLEMENTED → DELETED
- **Submit for review** → PENDING
- **Approve** → APPROVED
- **Reject** → REJECTED

### System Actions

- **Deploy success** → IMPLEMENTED
- **Sync conflict** → CONFLICTED
- **Resolve conflict** → MODIFIED (preferred) or DELETED

---

## Guard Conditions

- Only reviewers/approvers can move PENDING → APPROVED or REJECTED.
- Approved objects are locked; editing requires explicit “reopen” which moves to MODIFIED and clears approval.
- Deletion requires confirmation when object has dependencies.

---

## Review and Audit

Every transition must create an audit entry with:
- `timestamp`
- `actor`
- `previous_state`
- `new_state`
- `reason`
- optional `comment`

---

## Examples

### Example 1: New Table Creation
1. Create table → NEW
2. Submit → PENDING
3. Approve → APPROVED
4. Deploy → IMPLEMENTED

### Example 2: Modify Existing Table
1. Extracted table edited → MODIFIED
2. Submit → PENDING
3. Reject → REJECTED
4. Edit again → MODIFIED
5. Resubmit → PENDING

### Example 3: Conflict Resolution
1. Sync detects conflict → CONFLICTED
2. User resolves → MODIFIED
3. Submit → PENDING

---

## Edge Cases

- **Delete then recreate**: Delete moves to DELETED; recreating should create a new object with NEW, not reusing the deleted object unless explicitly “restore.”
- **Approved then edited**: Editing an approved object resets to MODIFIED and clears approval.
- **Rollback**: If deployment is rolled back, object remains IMPLEMENTED but deployment record references rollback status; do not auto-revert state.

---

## Implementation Notes

- State transitions must be centralized in the project model to avoid inconsistent UI logic.
- UI should reflect the state via icon/color and disable invalid transitions.

