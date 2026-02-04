# ScratchRobin Workflow Guide

## Quick Start: Design-to-Deployment Workflow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        SCRATCHROBIN WORKFLOW                                │
└─────────────────────────────────────────────────────────────────────────────┘

STEP 1: CREATE PROJECT
══════════════════════════════════════════════════════════════════════════════

  ┌──────────────┐
  │  New Project │
  │    Wizard    │
  └──────┬───────┘
         │
         ▼
  ┌─────────────────────────────────────┐
  │ • Name: "Customer Portal DB Redesign"│
  │ • Source: prod-db.company.com       │
  │ • Target: PostgreSQL 15             │
  │ • Team: Alice, Bob, Carol           │
  └─────────────────────────────────────┘
         │
         ▼
  ┌─────────────────────────────────────┐
  │ Extract schema from source          │
  │ (tables, views, procedures)         │
  │ All objects marked "extracted"      │
  └─────────────────────────────────────┘


STEP 2: DESIGN PHASE
══════════════════════════════════════════════════════════════════════════════

  ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
  │  Whiteboard  │     │   Mind Map   │     │    Draft     │
  │  (ideas)     │────▶│  (structure) │────▶│   Objects    │
  └──────────────┘     └──────────────┘     └──────┬───────┘
                                                   │
                                                   ▼
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  EXISTING          MODIFIED           NEW             DELETED            │
  │  ────────          ────────           ───             ───────            │
  │  [users]  ──────▶  [users]            [portal_]       [legacy_]          │
  │  (gray)            (orange)           sessions]       table]             │
  │                    +phone_col         (purple)        (red strike)       │
  └──────────────────────────────────────────────────────────────────────────┘
         │
         ▼
  ┌─────────────────────────────────────┐
  │  Create ERD Diagram                 │
  │  • Drag tables from catalog         │
  │  • Draw relationships               │
  │  • Add annotations                  │
  │  • Share with team for review       │
  └─────────────────────────────────────┘


STEP 3: COLLABORATION
══════════════════════════════════════════════════════════════════════════════

  ┌──────────────────────────────────────────────────────────────────────────┐
  │                           TEAM COLLABORATION                             │
  │                                                                          │
  │  Alice (Designer)    Bob (Reviewer)        Carol (DBA)                   │
  │  ──────────────────  ───────────────       ───────────                   │
  │  "Modified users     "Phone column        "Approved, but                 │
  │   table" ✎          should be            add index on                   │
  │                      VARCHAR(20)           phone for                     │
  │                      not VARCHAR(50)       lookups" ✓                    │
  │                       ─────────────                                      │
  │                      Status: PENDING                                     │
  └──────────────────────────────────────────────────────────────────────────┘
         │
         │ @mention notifications
         │ Change tracking
         │ Version history
         ▼
  ┌─────────────────────────────────────┐
  │ Resolve comments                    │
  │ Update design                       │
  │ Mark objects as "approved"          │
  └─────────────────────────────────────┘


STEP 4: TESTING
══════════════════════════════════════════════════════════════════════════════

  ┌──────────────────────────────────────────────────────────────────────────┐
  │                        TEST SUITE EXECUTION                              │
│                                                                          │
  │  ✓ Unit Test: users table constraints ...................... 12ms        │
  │  ✓ Unit Test: portal_sessions foreign keys .................. 8ms        │
  │  ✓ Integration: User login workflow ........................ 45ms        │
  │  ✓ Performance: Query with 1M users ......................... 89ms       │
  │  ✗ Data Quality: Phone format validation ................... 23ms        │
  │    └─ Found 3 records with invalid format                                │
  │                                                                          │
  │  Fix issues ──▶ Re-run tests ──▶ All pass                                │
  └──────────────────────────────────────────────────────────────────────────┘


STEP 5: DEPLOYMENT
══════════════════════════════════════════════════════════════════════════════

  ┌─────────────────────────────────────┐
  │ Create Deployment Plan              │
  │ Version: 2.1.0                      │
  │ Target: staging-db.company.com      │
  └─────────────┬───────────────────────┘
                │
                ▼
  ┌──────────────────────────────────────────────────────────────────────────┐
  │                         DEPLOYMENT STEPS                                 │
  │                                                                          │
  │  Step 1: Pre-deployment backup ........................... [BACKUP]      │
  │  Step 2: Create portal_sessions table .................... [CREATE]      │
  │  Step 3: Add phone column to users ....................... [ALTER]       │
  │  Step 4: Create indexes .................................. [INDEX]       │
  │  Step 5: Deploy procedures ............................... [DEPLOY]      │
  │  Step 6: Run post-deployment tests ....................... [TEST]        │
  │                                                                          │
  │  ┌─────────┐    ┌─────────┐    ┌─────────┐                              │
  │  │  STAGE  │───▶│  TEST   │───▶│   PROD  │                              │
  │  └─────────┘    └─────────┘    └─────────┘                              │
  │       │              │              │                                    │
  │       └──────────────┴──────────────┘                                    │
  │              Approval Gate                                               │
  └──────────────────────────────────────────────────────────────────────────┘


STEP 6: MONITORING
══════════════════════════════════════════════════════════════════════════════

  ┌─────────────────────────────────────┐
  │ Post-Deployment Monitoring          │
  │                                     │
  │ • Query performance                 │
  │ • Error rates                       │
  │ • User feedback                     │
  │ • Resource usage                    │
  └─────────────┬───────────────────────┘
                │
                ▼
  ┌─────────────────────────────────────┐
  │ Update documentation                │
  │ Mark objects "implemented"          │
  │ Archive design artifacts            │
  │ Lessons learned                     │
  └─────────────────────────────────────┘


═══════════════════════════════════════════════════════════════════════════════
                    OBJECT STATE LIFECYCLE
═══════════════════════════════════════════════════════════════════════════════

    EXTRACTED          DRAFT/MODIFIED        PENDING          APPROVED
    ─────────          ───────────────       ───────          ────────
       │                    │                   │                │
       │  Extract from DB   │  Design changes   │  Review        │  Deploy
       ▼                    ▼                   ▼                ▼
    ┌──────┐            ┌──────┐           ┌──────┐        ┌──────────┐
    │ Gray │───────────▶│Orange│──────────▶│Yellow│───────▶│  Green   │
    │ Icon │            │ Icon │           │ Icon │        │   Icon   │
    └──────┘            └──────┘           └──────┘        └────┬─────┘
       │                    │                   │                │
       │                    │                   │                ▼
       │                    │                   │           ┌──────────┐
       │                    │                   │           │IMPLEMENTED│
       │                    │                   │           │  (Blue)   │
       │                    │                   │           └──────────┘
       │                    │                   │
       │                    │                   └──── Reject ──▶ ┌─────────┐
       │                    │                                    │ REJECTED│
       │                    │                                    │  (Red)  │
       │                    │                                    └─────────┘
       │                    │
       │                    └──── Delete request ──▶ ┌─────────┐
       │                                            │ DELETED │
       │                                            │(Strike) │
       │                                            └─────────┘
       │
       └─────── Unchanged in design ──────────────────────────────▶


═══════════════════════════════════════════════════════════════════════════════
                         ICON REFERENCE
═══════════════════════════════════════════════════════════════════════════════

Design Phase Icons:
  [70] Whiteboard    - Brainstorming canvas
  [71] Mind Map      - Conceptual planning
  [72] Design        - Design document
  [73] Draft         - Work in progress
  [74] Template      - Reusable pattern
  [75] Blueprint     - Architecture plan
  [76] Concept       - Idea/proposal
  [77] Plan          - Implementation plan

State Icons:
  [80] Implemented   - Deployed (green check)
  [81] Pending       - Awaiting review (clock)
  [82] Modified      - Changed (pencil)
  [83] Deleted       - To be removed (X)
  [84] New Item      - New addition (+)

Action Icons:
  [90] Sync          - Synchronize
  [91] Diff          - Compare differences
  [93] Migrate       - Data migration
  [94] Deploy        - Deploy to environment

Collaboration Icons:
  [100] Shared       - Shared resource
  [101] Collaboration- Team working
  [102] Team         - Team members

Infrastructure Icons:
  [60] Server        - Database server
  [61] Client        - Client application
  [62] Filespace     - Storage
  [63] Network       - Network topology
  [64] Cluster       - Database cluster
  [65] Instance      - DB instance
  [66] Replica       - Read replica
  [67] Shard         - Partition


═══════════════════════════════════════════════════════════════════════════════
                      COLLABORATION SCENARIOS
═══════════════════════════════════════════════════════════════════════════════

SCENARIO 1: Design Review
─────────────────────────────────────────────────────────────────────────────
1. Designer creates/modifies objects
2. System auto-generates diff report
3. Reviewers get notified
4. Reviewers add comments on objects
5. Designer addresses feedback
6. Approver marks as "approved"
7. Ready for deployment

SCENARIO 2: Concurrent Editing
─────────────────────────────────────────────────────────────────────────────
1. Alice edits "users" table structure
2. Bob modifies "users" table constraints
3. System detects concurrent edit
4. Both see each other's cursor/selection
5. Changes merged in real-time
6. Conflict resolution if needed

SCENARIO 3: Branching Design
─────────────────────────────────────────────────────────────────────────────
1. Main branch: Approved production design
2. Create feature branch: "add-customer-portal"
3. Make changes in branch
4. Open pull request
5. Team reviews diff
6. Merge to main
7. Deploy from main

SCENARIO 4: Emergency Hotfix
─────────────────────────────────────────────────────────────────────────────
1. Issue detected in production
2. Create hotfix branch from production
3. Make minimal fix
4. Fast-track review
5. Deploy with rollback plan
6. Monitor
7. Merge fix back to main


═══════════════════════════════════════════════════════════════════════════════
                         BEST PRACTICES
═══════════════════════════════════════════════════════════════════════════════

✓ Always extract baseline before making changes
✓ Use meaningful object names and comments
✓ Write tests for complex changes
✓ Review with team before deploying
✓ Test deployment in staging first
✓ Keep backups before major changes
✓ Document design decisions
✓ Use version control for projects
✓ Tag stable versions
✓ Monitor after deployment

✗ Don't deploy directly to production
✗ Don't skip testing
✗ Don't modify production without rollback plan
✗ Don't delete objects without confirming dependencies
✗ Don't ignore validation warnings


═══════════════════════════════════════════════════════════════════════════════
                       KEYBOARD SHORTCUTS
═══════════════════════════════════════════════════════════════════════════════

General:
  Ctrl+N      New project
  Ctrl+O      Open project
  Ctrl+S      Save
  Ctrl+Shift+S Save all

Design:
  Ctrl+E      Extract from database
  Ctrl+D      Show diff
  Ctrl+T      Run tests
  F5          Refresh from source

Deployment:
  Ctrl+Shift+D Deploy
  Ctrl+Shift+R Rollback
  Ctrl+Shift+B Backup

Navigation:
  Ctrl+1      Catalog tree
  Ctrl+2      Diagram view
  Ctrl+3      SQL editor
  Ctrl+4      Documentation
  Ctrl+P      Quick open

Collaboration:
  Ctrl+Shift+C Add comment
  Ctrl+Shift+M Toggle mind map
  Ctrl+Shift+W New whiteboard

