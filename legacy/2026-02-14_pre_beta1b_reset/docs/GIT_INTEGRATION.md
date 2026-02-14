# Git Integration Specifications

## Overview

ScratchRobin implements a **dual-repo Git model** that coordinates version control between:
1. **Project Repository** - Design files, documentation, tests (Git)
2. **Database Repository** - Schema changes, data migrations (Git-like via ScratchBird)

This enables seamless tracking of design-to-implementation workflows with full auditability.

## Dual-Repo Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SCRATCHROBIN PROJECT                                │
│                                                                             │
│  ┌─────────────────────────┐    ┌─────────────────────────────────────────┐│
│  │   PROJECT REPO (.git)   │    │   DATABASE REPO (ScratchBird Git)      ││
│  │   ===================   │    │   ==============================       ││
│  │                         │    │                                         ││
│  │  📁 designs/            │    │  📁 schema/                             ││
│  │    ├── users.table.json │    │    ├── public.users.table.sql          ││
│  │    └── orders.view.json │◄──►│    ├── public.orders.view.sql          ││
│  │                         │    │    └── indexes/                        ││
│  │  📁 diagrams/           │    │                                         ││
│  │    └── erd.svg          │    │  📁 migrations/                         ││
│  │                         │    │    ├── 001_create_users.sql            ││
│  │  📁 docs/               │    │    ├── 002_add_indexes.sql             ││
│  │    └── data-dictionary/ │    │    └── 003_create_orders_view.sql      ││
│  │                         │    │                                         ││
│  │  📁 tests/              │    │  📁 snapshots/                          ││
│  │    └── validation.yml   │    │    ├── v1.0.0.baseline.sql             ││
│  │                         │    │    └── v1.1.0.pre-portal.sql           ││
│  │  📁 config/             │    │                                         ││
│  │    └── connections.yml  │    │  📁 procedures/                         ││
│  │                         │    │    └── public.calculate_total.func.sql ││
│  │  📄 scratchrobin.yml    │    │                                         ││
│  │  📄 README.md           │    │  📄 scratchbird.git.yml                ││
│  │                         │    │                                         ││
│  └─────────────────────────┘    └─────────────────────────────────────────┘│
│            │                              │                                 │
│            │        SYNC ENGINE           │                                 │
│            └────────────┬─────────────────┘                                 │
│                         │                                                   │
│                         ▼                                                   │
│            ┌─────────────────────────┐                                      │
│            │  Cross-Repo References  │                                      │
│            │  • Commit SHA mapping   │                                      │
│            │  • Design→DDL linkage   │                                      │
│            │  • State synchronization│                                      │
│            └─────────────────────────┘                                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Repository Types

### 1. Project Repository (Standard Git)

**Location**: `.git/` in project directory
**Contents**:
```
my-project/
├── .git/                          # Git repository
├── scratchrobin.yml               # Project manifest
├── designs/                       # Design objects
│   ├── public.users.table.json
│   ├── public.orders.table.json
│   └── public.calculate_total.function.json
├── diagrams/                      # Visual diagrams
│   ├── erd-main.svg
│   └── data-flow-etl.svg
├── whiteboards/                   # Brainstorming
│   └── initial-design.canvas
├── mindmaps/                      # Concept maps
│   └── domain-model.mind
├── docs/                          # Documentation
│   ├── data-dictionary/
│   ├── decisions/
│   └── runbooks/
├── tests/                         # Test definitions
│   ├── unit/
│   ├── integration/
│   └── performance/
├── deployments/                   # Deployment plans
│   ├── v1.0.0-to-prod.yml
│   └── v1.1.0-to-staging.yml
└── .scratchrobin/
    ├── cache/                     # Local cache
    ├── temp/                      # Temp files
    └── sync-state.json            # Cross-repo sync state
```

**scratchrobin.yml** (Project Manifest):
```yaml
project:
  name: "Customer Portal Database"
  version: "1.2.0"
  description: "Redesign for customer self-service portal"
  
  repository:
    type: git
    url: https://github.com/company/cust-portal-db
    branch: main
    
  database:
    type: scratchbird
    version: "3.2.1"
    git:
      enabled: true
      repo_url: scratchbird://prod-db.company.com:5432/ecommerce
      branch: main
      sync_mode: bidirectional
      
  connections:
    source:
      name: "Production (Read-Only)"
      url: scratchbird://prod-db.company.com:5432/ecommerce
      git_branch: main
    
    target_staging:
      name: "Staging"
      url: scratchbird://staging-db.company.com:5432/ecommerce
      git_branch: develop
      
    target_prod:
      name: "Production"
      url: scratchbird://prod-db.company.com:5432/ecommerce
      git_branch: main
      deployment_approval: required
      
  git:
    workflow: gitflow
    require_signed_commits: true
    protected_branches: [main, develop]
    
    commit_hooks:
      pre_commit:
        - validate_designs
        - run_tests
      post_commit:
        - sync_to_scratchbird
        
    scratchbird_sync:
      # When to sync project commits to database repo
      auto_sync_branches: [develop, feature/*]
      manual_sync_branches: [main]
      
      # Mappings between project files and database objects
      object_mappings:
        "designs/*.table.json": "schema/{schema}.{table}.sql"
        "designs/*.view.json": "schema/{schema}.{view}.sql"
        "designs/*.function.json": "procedures/{schema}.{function}.sql"
```

### 2. Database Repository (ScratchBird Git)

**Location**: Inside the ScratchBird database cluster
**Access**: Via ScratchBird Git protocol
**Contents**:
```
scratchbird://cluster-db/
├── schema/                        # Current schema state
│   ├── public/
│   │   ├── users.table.sql
│   │   ├── users.table.indexes.sql
│   │   ├── orders.table.sql
│   │   └── orders.view.sql
│   └── analytics/
│       └── summary.view.sql
│
├── migrations/                    # Sequential migrations
│   ├── 0001_initial_schema.sql
│   ├── 0002_add_user_indexes.sql
│   ├── 0003_create_orders.sql
│   └── 0004_add_portal_tables.sql
│
├── snapshots/                     # Point-in-time snapshots
│   ├── v1.0.0.baseline.sql
│   ├── v1.1.0.pre-migration.sql
│   └── v1.2.0.current.sql
│
├── procedures/                    # Stored code
│   ├── public.calculate_tax.func.sql
│   └── public.validate_order.proc.sql
│
├── config/                        # Database config
│   ├── parameters.yml
│   └── extensions.yml
│
└── meta/                          # Repository metadata
    ├── commits/                   # Commit history
    ├── branches/                  # Branch pointers
    └── tags/                      # Tag references
```

## Cross-Repo Synchronization

### Synchronization Modes

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     SYNC MODE COMPARISON                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  BIDIRECTIONAL                    UNIDIRECTIONAL (Project → DB)            │
│  =============                    ============================             │
│                                                                             │
│  Project ◄──────► Database        Project ───────► Database                │
│                                                                             │
│  • Design changes sync TO db      • Design changes sync TO db              │
│  • DB changes sync BACK to project • DB changes don't affect project       │
│  • Conflict detection needed      • No conflicts                           │
│  • Best for: Active development   • Best for: Production deployments       │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  UNIDIRECTIONAL (DB → Project)    DECOUPLED                                │
│  =============================    =========                                  │
│                                                                             │
│  Project ◄─────── Database        Project          Database                │
│                                                                             │
│  • Extract DB state to project    • Separate commit histories              │
│  • Manual linking                 • Manual sync only                       │
│  • Best for: Legacy DB import     • Best for: Multiple projects per DB     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Sync State Tracking

**File**: `.scratchrobin/sync-state.json`
```json
{
  "version": "1.0.0",
  "last_sync": "2024-02-04T15:30:00Z",
  "project_repo": {
    "head_commit": "a1b2c3d4e5f6",
    "branch": "feature/add-portal",
    "dirty_files": ["designs/public.users.table.json"]
  },
  "database_repo": {
    "head_commit": "f6e5d4c3b2a1",
    "branch": "feature/add-portal",
    "dirty_objects": []
  },
  "mappings": [
    {
      "project_file": "designs/public.users.table.json",
      "db_object": "schema/public.users.table.sql",
      "last_sync_commit": "a1b2c3d4e5f6",
      "sync_status": "in_sync"
    },
    {
      "project_file": "designs/public.portal_sessions.table.json",
      "db_object": "schema/public.portal_sessions.table.sql",
      "last_sync_commit": null,
      "sync_status": "new_in_project"
    }
  ],
  "pending_sync": {
    "project_to_db": ["portal_sessions.table"],
    "db_to_project": [],
    "conflicts": []
  }
}
```

### Commit Message Format

**Project commits** include database metadata:
```
feat: Add customer portal session management

- New table: portal_sessions with UUID PK
- New index: idx_sessions_user for lookups
- New function: validate_session_token()

Database-Repo: scratchbird://prod-db/ecommerce
Database-Commit: f6e5d4c3b2a1
Database-Branch: feature/add-portal
Database-Objects: [portal_sessions.table, validate_session_token.function]

Refs: SCR-123
Co-authored-by: Alice <alice@company.com>
```

**Database commits** include project references:
```
feat: Add customer portal session management

SQL:
CREATE TABLE portal_sessions (...);
CREATE INDEX idx_sessions_user ON portal_sessions(user_id);
CREATE FUNCTION validate_session_token(...) ...

Project-Repo: https://github.com/company/cust-portal-db
Project-Commit: a1b2c3d4e5f6
Project-Branch: feature/add-portal
Project-Files: [designs/portal_sessions.table.json]

Migration-File: migrations/0042_add_portal_sessions.sql
Rollback-File: migrations/0042_add_portal_sessions.rollback.sql
```

## Workflow Integration

### Feature Development Workflow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FEATURE DEVELOPMENT WORKFLOW                             │
└─────────────────────────────────────────────────────────────────────────────┘

1. CREATE FEATURE BRANCH (Project & DB)
═══════════════════════════════════════════════════════════════════════════════

   Local Project                          ScratchBird Database
   ─────────────                          ───────────────────
   
   $ git checkout -b feature/portal       $ sb git checkout -b feature/portal
   
   Project now on:                        Database now on:
   feature/portal branch                  feature/portal branch
   
   
2. DESIGN & IMPLEMENT
═══════════════════════════════════════════════════════════════════════════════

   ┌─────────────────┐                    ┌─────────────────┐
   │ Modify designs/ │                    │ Modify schema   │
   │ portal_users.   │◄───── SYNC ──────►│ via SQL or      │
   │ table.json      │   (bidirectional) │ design tool     │
   └────────┬────────┘                    └────────┬────────┘
            │                                      │
            ▼                                      ▼
   ┌─────────────────┐                    ┌─────────────────┐
   │ Add tests/      │                    │ Run tests       │
   │ portal_users.   │                    │ against DB      │
   │ test.yml        │                    │                 │
   └────────┬────────┘                    └────────┬────────┘
            │                                      │
            └──────────────┬───────────────────────┘
                         │
                         ▼
            ┌─────────────────┐
            │ Validate design │
            │ Run test suite  │
            │ Generate DDL    │
            └─────────────────┘
   
   
3. COMMIT CHANGES
═══════════════════════════════════════════════════════════════════════════════

   Project Commit:                        Database Commit:
   ───────────────                        ────────────────
   
   git add designs/portal_users.table.json
   git add tests/portal_users.test.yml
   git commit -m "feat: Add portal user tables
   
   - Support for customer portal login
   - Session management
   
   Database-Commit: d4c3b2a1"
   
   sb git commit -m "feat: Add portal user tables
   
   SQL: CREATE TABLE portal_users (...)
   
   Project-Commit: a1b2c3d4"
   
   
4. SYNC VERIFICATION
═══════════════════════════════════════════════════════════════════════════════

   $ scratchrobin sync status
   
   Project Branch: feature/portal (a1b2c3d4)
   Database Branch: feature/portal (d4c3b2a1)
   Sync Status: IN_SYNC
   
   Objects:
   ✓ portal_users.table.json ↔ schema/public.portal_users.table.sql
   ✓ tests synced
   
   
5. PUSH & PULL REQUEST
═══════════════════════════════════════════════════════════════════════════════

   Project:                                Database:
   ───────                                 ────────
   
   git push origin feature/portal         sb git push origin feature/portal
   
   → Create PR on GitHub                  → Create PR in ScratchBird
   → Link to DB PR: #42                   → Link to Project PR: company/cust-portal-db#156
   
   
6. CODE REVIEW (Cross-Repo)
═══════════════════════════════════════════════════════════════════════════════

   Reviewers see unified diff:
   
   ┌─────────────────────────────────────────────────────────────────────────┐
   │ PR #156: Add portal user tables                                         │
   ├─────────────────────────────────────────────────────────────────────────┤
   │                                                                         │
   │ Project Changes:                                        Database Changes:│
   │ ────────────────                                        ─────────────── │
   │ designs/public.portal_users.table.json                  schema/public/  │
   │                                                         portal_users.sql│
   │ + Added: session_token (VARCHAR(255))                   + CREATE TABLE  │
   │ + Added: expires_at (TIMESTAMP)                         + session_token │
   │                                                         + expires_at    │
   │ Tests:                                                  Indexes:        │
   │ ──────                                                  ────────        │
   │ ✓ Validate session token format                         + idx_sessions_ │
   │ ✓ Check expiration logic                                  user          │
   │                                                                         │
   │ Cross-Repo Status: SYNCED                                               │
   │                                                                         │
   │ Reviewers:                                                              │
   │ [ ] Alice (Schema review)                                               │
   │ [ ] Bob (Security review)                                               │
   │ [ ] Carol (DBA approval)                                                │
   │                                                                         │
   └─────────────────────────────────────────────────────────────────────────┘
   
   
7. MERGE & DEPLOY
═══════════════════════════════════════════════════════════════════════════════

   After approval:
   
   Project:                                Database:
   ───────                                 ────────
   
   git checkout develop                   sb git checkout develop
   git merge feature/portal               sb git merge feature/portal
   git push origin develop                sb git push origin develop
   
   → Auto-deploy to Staging               → Auto-apply to Staging DB
   → Run integration tests                → Run DB validation
   
   
8. PROMOTE TO PRODUCTION
═══════════════════════════════════════════════════════════════════════════════

   git checkout main                      sb git checkout main
   git merge develop                      sb git merge develop
   git tag v1.2.0                         sb git tag v1.2.0
   
   → Create deployment plan                → Create migration script
   → Execute with rollback ready           → Execute with backup ready
```

### Hotfix Workflow

```
EMERGENCY HOTFIX
═══════════════════════════════════════════════════════════════════════════════

Production Issue Detected
         │
         ▼
┌─────────────────┐
│ Create hotfix   │
│ branches from   │
│ production tags │
└────────┬────────┘
         │
    ┌────┴────┐
    │         │
Project     Database
hotfix/     hotfix/
issue-999   issue-999
    │         │
    ▼         ▼
┌─────────────────┐
│ Make minimal    │
│ fix, test       │
└────────┬────────┘
         │
    ┌────┴────┐
    │         │
Commit     Commit
(fast)     (fast)
    │         │
    ▼         ▼
┌─────────────────┐
│ Fast-track      │
│ review          │
└────────┬────────┘
         │
    ┌────┴────┐
    │         │
Merge to   Merge to
main       main
    │         │
    ▼         ▼
┌─────────────────┐
│ Deploy with     │
│ rollback plan   │
└────────┬────────┘
         │
    Monitor
         │
    ┌────┴────┐
    │         │
Success   Failure
    │         │
    ▼         ▼
Close     Rollback
PR        both repos

Merge     Back to
hotfix    develop
→ develop branches
```

## Conflict Resolution

### Conflict Types

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CONFLICT SCENARIOS                                  │
└─────────────────────────────────────────────────────────────────────────────┘

TYPE 1: Project File vs Database Object (Same branch)
═══════════════════════════════════════════════════════════════════════════════

   Alice (Project)                Bob (Database via SQL)
   ───────────────                ──────────────────────
   
   Edit: users.table.json         ALTER TABLE users ADD COLUMN phone;
   Add: email_verified column     
   
   Result: CONFLICT on users table
   
   Resolution:
   ┌─────────────────────────────────────────────────────────────────────────┐
   │ Conflict Resolution UI                                                  │
   ├─────────────────────────────────────────────────────────────────────────┤
   │                                                                         │
   │ Project Changes:              Database Changes:                         │
   │ + email_verified BOOLEAN      + phone VARCHAR(20)                       │
   │                                                                         │
   │ Options:                                                                │
   │ (•) Keep both changes                                                   │
   │ ( ) Keep project only                                                   │
   │ ( ) Keep database only                                                  │
   │ ( ) Manual merge                                                        │
   │                                                                         │
   │ Merged Result:                                                          │
   │ CREATE TABLE users (...                                                 │
   │   email_verified BOOLEAN,                                               │
   │   phone VARCHAR(20)                                                     │
   │ );                                                                      │
   │                                                                         │
   │ [Resolve & Sync]                                                        │
   └─────────────────────────────────────────────────────────────────────────┘


TYPE 2: Branch Divergence
═══════════════════════════════════════════════════════════════════════════════

   Project                        Database
   ───────                        ────────
   
   feature/A ──► develop          feature/A ──► develop
        │                              │
        │ (merged)                     │ (not merged)
        ▼                              ▼
   develop has changes             develop is behind
   that aren't in DB               
   
   Resolution:
   - Force sync project → DB
   - Or create feature branch in DB and merge


TYPE 3: Schema Drift Detection
═══════════════════════════════════════════════════════════════════════════════

   Database was modified outside of Git workflow
   
   $ scratchrobin sync status
   
   WARNING: Schema drift detected
   
   Objects modified outside Git:
   • public.users.table (added column: temp_field)
   • public.orders.table (dropped index: idx_old)
   
   Actions:
   [ ] Import drift to project (creates commit)
   [ ] Revert drift in database
   [ ] Mark as intentional (update baseline)
```

## Git Operations

### Command Line Interface

```bash
# Initialize Git integration for project
scratchrobin git init --repo-type=dual

# Clone project with database sync
scratchrobin clone https://github.com/company/project \
  --database=scratchbird://prod-db/ecommerce \
  --sync-mode=bidirectional

# Check sync status
scratchrobin git status

# Sync project changes to database
scratchrobin git sync to-db

# Sync database changes to project
scratchrobin git sync from-db

# Resolve conflicts
scratchrobin git resolve --interactive

# Create deployment commit (syncs both repos)
scratchrobin git commit -m "feat: Add feature" \
  --sync \
  --create-migration \
  --target=staging

# View cross-repo history
scratchrobin git log --cross-repo --graph

# Create deployment plan from commits
scratchrobin git deploy-plan v1.1.0..v1.2.0 \
  --target=production \
  --output=deploy-v1.2.0.yml
```

### Git Hooks Integration

**`.git/hooks/pre-commit`**:
```bash
#!/bin/bash
# Validate designs before commit
scratchrobin validate --strict
if [ $? -ne 0 ]; then
    echo "Validation failed. Commit aborted."
    exit 1
fi

# Check sync status
scratchrobin sync check
if [ $? -ne 0 ]; then
    echo "Sync conflict detected. Run 'scratchrobin sync resolve'"
    exit 1
fi
```

**`.git/hooks/post-commit`**:
```bash
#!/bin/bash
# Auto-sync to database if on feature branch
BRANCH=$(git branch --show-current)
if [[ $BRANCH == feature/* ]]; then
    scratchrobin sync to-db --branch=$BRANCH
fi
```

## Configuration Reference

### scratchrobin.yml - Git Section

```yaml
git:
  # Repository configuration
  repository:
    type: git
    url: https://github.com/company/project.git
    default_branch: main
    
  # Database repository (ScratchBird)
  database_repository:
    type: scratchbird-git
    cluster: prod-db.company.com
    database: ecommerce
    port: 5432
    
    # Authentication
    auth:
      method: tls-client-cert
      cert_path: ~/.scratchrobin/certs/db-git.crt
      key_path: ~/.scratchrobin/certs/db-git.key
      
    # Sync configuration
    sync:
      mode: bidirectional
      auto_sync: true
      conflict_resolution: manual
      
      # Which branches auto-sync
      auto_sync_branches:
        - feature/*
        - develop
        
      # Protected branches (manual sync only)
      protected_branches:
        - main
        - release/*
        
  # Commit conventions
  commits:
    # Require conventional commits format
    require_conventional: true
    
    # Require issue references
    require_issue_ref: true
    issue_prefix: "SCR-"
    
    # Auto-generate database commit messages
    auto_db_messages: true
    
    # Include design diff in commit message
    include_design_diff: true
    
  # Pull request settings
  pull_requests:
    # Require cross-repo sync before merge
    require_sync: true
    
    # Require DB review for schema changes
    require_db_review: true
    db_reviewers:
      - alice@company.com
      - bob@company.com
      
    # Automated checks
    checks:
      - validate_designs
      - check_sync_status
      - run_tests
      - generate_migration_preview
      
  # Deployment integration
  deployment:
    # Auto-create deployment plans from commits
    auto_create_plans: true
    
    # Require approval for production
    require_approval:
      production: true
      staging: false
      
    # Tag format for releases
    tag_format: "v{major}.{minor}.{patch}"
    
    # Changelog generation
    changelog:
      enabled: true
      format: conventional
      include_db_changes: true
```

## Security Considerations

### Access Control

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         ACCESS CONTROL MATRIX                               │
└─────────────────────────────────────────────────────────────────────────────┘

                           Project Repo     Database Repo
                           ────────────     ─────────────
Developer (Junior)         Read/Feature     Read Only
Developer (Senior)         Read/Write       Read/Feature
DBA                        Read/Write       Read/Write
DevOps                     Admin            Deploy Only
Security                   Audit            Audit

Branch Protection:
- main: Require 2 reviewers + passing checks + DBA approval
- develop: Require 1 reviewer + passing checks
- feature/*: No restrictions (work in progress)
```

### Audit Trail

All cross-repo operations are logged:
```json
{
  "timestamp": "2024-02-04T15:30:00Z",
  "user": "alice@company.com",
  "operation": "sync_to_database",
  "project_commit": "a1b2c3d4",
  "database_commit": "d4c3b2a1",
  "objects_synced": [
    {
      "project_file": "designs/users.table.json",
      "db_object": "schema/public.users.table.sql",
      "operation": "modified"
    }
  ],
  "approval": {
    "required": true,
    "obtained_from": "bob@company.com",
    "timestamp": "2024-02-04T15:25:00Z"
  }
}
```

## Troubleshooting

### Common Issues

**Issue**: Sync fails with "Database repo ahead of project"
```bash
# Solution: Pull database changes first
scratchrobin sync from-db
# Resolve any conflicts
scratchrobin sync resolve
# Then push to database
scratchrobin sync to-db
```

**Issue**: Schema drift detected
```bash
# View drift details
scratchrobin drift show

# Option 1: Import drift to project
scratchrobin drift import --commit-msg="Import production hotfix"

# Option 2: Revert drift
scratchrobin drift revert --objects=users.table
```

**Issue**: Merge conflict in both repos
```bash
# Start interactive resolution
scratchrobin git resolve --interactive

# Or resolve project first, then sync
scratchrobin git resolve --project-only
git commit -m "Resolve conflicts"
scratchrobin sync to-db --force
```

## API Reference

See [API.md](API.md) for REST API endpoints for Git operations.
