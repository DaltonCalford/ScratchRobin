# Project Definition & Governance

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines what a ScratchRobin project is, how it is governed, and how approvals, permissions, and documentation workflows are enforced. This specification ties together project roles, approval gates, and document generation rules (including MOPs and rollback procedures).

---

## 1. Project Definition

A **project** is the authoritative work product for designing, deploying, and documenting a database or cluster system. It includes:

- Design artifacts (diagrams, models, metadata snapshots)
- Versioned change history and object lifecycle state
- Deployment plans and rollback procedures
- Generated documentation (data dictionaries, runbooks, MOPs)
- Governance metadata and approval records

A project may be local (file-based) or shared (hosted in a database/cluster). The canonical storage format is defined in:

- `docs/specifications/core/PROJECT_PERSISTENCE_FORMAT.md`
- `docs/specifications/core/PROJECT_ON_DISK_LAYOUT.md`

---

## 2. Roles and Permissions

### 2.1 Standard Roles

| Role | Description | Typical Capabilities |
| --- | --- | --- |
| Owner | Project owner and ultimate approver | All actions; manage roles, policies |
| Architect | Designs system structure | Create/modify designs, approve design changes |
| Designer | Creates diagrams and models | Create/modify objects; submit for review |
| Reviewer | Reviews changes | Approve/reject changes |
| Operator | Executes deployment plans | Run deployments and rollback |
| Auditor | Read-only compliance review | View history, signatures, logs |
| Viewer | Read-only access | View diagrams, docs |

Roles can be customized per project.

### 2.2 Permission Categories

Permissions are grouped into:

- **Design**: create/edit diagrams, objects, metadata
- **Review**: approve/reject, comment, request changes
- **Deploy**: generate/run deployment and rollback plans
- **Docs**: generate MOPs, runbooks, and manuals
- **Admin**: manage project settings, users, templates

### 2.3 Role-to-Permission Matrix (Default)

| Permission | Owner | Architect | Designer | Reviewer | Operator | Auditor | Viewer |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Design | ✓ | ✓ | ✓ | ✗ | ✗ | ✗ | ✗ |
| Review | ✓ | ✓ | ✗ | ✓ | ✗ | ✗ | ✗ |
| Deploy | ✓ | ✓ | ✗ | ✗ | ✓ | ✗ | ✗ |
| Docs | ✓ | ✓ | ✓ | ✓ | ✗ | ✓ | ✓ |
| Admin | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ |

---

## 3. Approval Gates

Approval gates enforce controlled transitions between object states and deployment phases. They are project-configurable and can be enabled per environment.

### 3.1 Object Lifecycle Gates

| Transition | Gate Required | Notes |
| --- | --- | --- |
| MODIFIED → PENDING | Yes | Designer submits for review |
| PENDING → APPROVED | Yes | Reviewer/Architect/Owner |
| APPROVED → IMPLEMENTED | Yes | Deployment approval |

### 3.2 Deployment Gates

Required approvals before execution:

- **Design Approval**: all objects in plan must be APPROVED
- **Plan Approval**: deployment plan reviewed and signed
- **Operator Approval**: operator confirms target environment


### 3.3 Emergency Override

- Owner can authorize emergency overrides.
- Overrides must be logged with reason and scope.

---

## 4. MOP and Rollback Document Generation

### 4.1 MOP Definitions

MOPs (Method of Procedure) are generated, human-readable deployment instructions containing:

- Pre-checks and environment validation
- Step-by-step execution procedure
- Required approvals and checkpoints
- Rollback instructions per step
- Validation checks and success criteria

### 4.2 Source Inputs

MOPs and rollback documents are generated from:

- Deployment plan (`.srdeploy`)
- Approved diagram and model objects
- Environment policies and operational templates

### 4.3 Template Model

Templates are defined in YAML (or JSON) and map deployment plan steps into MOP sections.

```yaml
document:
  type: "mop"
  title: "Deploy v1.4 - Billing"
  sections:
    - title: "Pre-Checks"
      source: "deployment.pre_checks"
    - title: "Execution Steps"
      source: "deployment.steps"
    - title: "Rollback Plan"
      source: "deployment.steps.rollback"
    - title: "Post-Validation"
      source: "deployment.post_checks"
```

### 4.4 Rollback Rules

- Every deployment step must specify a rollback instruction or explicitly mark `rollback: none`.
- Rollback MOPs include both **step-local** and **global** rollback guidance.
- Rollback documents must reference the rollback policy:
  - `docs/specifications/deployment/ROLLBACK_POLICY.md`

### 4.5 Output Locations

- `docs/generated/mops/`
- `docs/generated/rollback/`
- Index: `docs/generated/index.md`

---

## 5. Project Governance Metadata

Each project contains governance metadata used for audit, compliance, and automated policy enforcement.

### 5.1 Required Governance Fields

- Project owners and stewards
- Approved environments (dev, staging, prod)
- Compliance tags (e.g., SOC2, HIPAA)
- Review policy (min reviewers, required roles)
- AI policy (allowed, review-required, prohibited)

### 5.2 Example (JSON)

```json
{
  "project_id": "uuid",
  "owners": ["alice", "bob"],
  "environments": [
    {"id": "dev", "approval_required": false},
    {"id": "prod", "approval_required": true, "min_reviewers": 2}
  ],
  "compliance": ["SOC2", "PCI"],
  "ai_policy": {
    "enabled": true,
    "requires_review": true,
    "allowed_scopes": ["docs", "mop"],
    "prohibited_scopes": ["deployment_execution"]
  }
}
```

---

## 6. AI Assistance Policies

AI assistance is configurable per project and per environment.

Policy dimensions:

- **Enabled/disabled**
- **Scope**: documentation, code generation, design hints
- **Review requirement**: AI output must be approved before use
- **Audit**: AI output stored with prompt, model, and revision hash

AI-generated artifacts must be tagged with:

- `ai_generated: true`
- `model_id`
- `generated_at`
- `reviewed_by` (if approved)

---

## 6.1 Runtime Enforcement Points

Governance rules must be enforced at the following runtime boundaries:

- **Object State Transitions**: create/edit/submit/approve/reject/implement
- **Deployment Actions**: generate plan, execute plan, rollback
- **Reporting Actions**: run question, refresh, schedule, alert evaluation
- **Sharing Actions**: publish links, create embeds, export data

Enforcement must check:

- Role permissions (Owner/Architect/Designer/Reviewer/Operator/Auditor/Viewer)
- Collection access (View/Curate/Admin)
- Environment gating rules (approval_required, min_reviewers)
- AI policy (allowed_scopes, requires_review)

---

## 7. Audit and Traceability

Every governed action must generate audit records:

- Role and actor
- Action type (design change, approval, deploy)
- Target object(s)
- Timestamp
- Reason/comment

Audit logs integrate with:

- `docs/specifications/core/AUDIT_LOGGING.md`

---

## 8. Integration References

Related specifications:

- Project persistence: `docs/specifications/core/PROJECT_PERSISTENCE_FORMAT.md`
- Object lifecycle: `docs/specifications/core/OBJECT_STATE_MACHINE.md`
- Deployment plans: `docs/specifications/deployment/DEPLOYMENT_PLAN_FORMAT.md`
- Rollback policy: `docs/specifications/deployment/ROLLBACK_POLICY.md`
- Documentation system: `docs/specifications/documentation/AUTOMATED_DOCUMENTATION_SYSTEM.md`
- Generated docs: `docs/specifications/documentation/GENERATED_METHODS_AND_OPERATIONS_SPEC.md`
