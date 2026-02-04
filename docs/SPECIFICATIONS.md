# ScratchRobin Comprehensive Specifications

## Table of Contents
1. [Project System](#1-project-system)
2. [Expanded Diagramming](#2-expanded-diagramming)
3. [Design Workflow](#3-design-workflow)
4. [Collaboration Model](#4-collaboration-model)
5. [Testing & Deployment](#5-testing--deployment)
6. [Resource Tracking](#6-resource-tracking)
7. [Documentation System](#7-documentation-system)

---

## 1. Project System

### 1.1 Project Structure

```
Project (Workspace)
├── Metadata
│   ├── Project Name, Description, Version
│   ├── Created/Modified timestamps
│   ├── Owner & Team Members
│   └── Tags & Categories
│
├── Connections
│   ├── Source Database Connections (read-only extract)
│   ├── Target Database Connections (deployment targets)
│   └── File/Resource Connections
│
├── Design Elements
│   ├── Whiteboards (free-form brainstorming)
│   ├── Mind Maps (conceptual relationships)
│   ├── Diagrams (ERD, Schema, Flow)
│   └── Data Models (logical/physical)
│
├── Database Objects
│   ├── Extracted Schema (from source)
│   ├── Draft Objects (design phase)
│   ├── Modified Objects (pending changes)
│   ├── New Objects (to be created)
│   └── Deleted Objects (to be dropped)
│
├── Scripts & Code
│   ├── SQL Scripts (DDL, DML, Queries)
│   ├── Migration Scripts
│   ├── Test Scripts
│   └── Utility Scripts
│
├── Documentation
│   ├── Design Documents
│   ├── API Documentation
│   ├── Data Dictionaries
│   └── Runbooks
│
├── Tests
│   ├── Unit Tests (per object)
│   ├── Integration Tests
│   ├── Performance Tests
│   └── Validation Rules
│
├── Deployments
│   ├── Deployment Plans
│   ├── Release History
│   ├── Rollback Scripts
│   └── Environment Configs
│
└── Resources
    ├── Time Tracking
    ├── Cost Analysis
    └── Asset Inventory
```

### 1.2 Project Lifecycle

```
┌─────────┐    ┌──────────┐    ┌───────────┐    ┌──────────┐    ┌─────────┐
│  INIT   │───▶│  DESIGN  │───▶│  VALIDATE │───▶│ DEPLOY   │───▶│ MONITOR │
│         │    │          │    │           │    │          │    │         │
│ Create  │    │ Extract  │    │ Test     │    │ Execute  │    │ Track   │
│ Config  │    │ Model    │    │ Review   │    │ Migrate  │    │ Verify  │
└─────────┘    └──────────┘    └───────────┘    └──────────┘    └─────────┘
     │               │                │               │               │
     ▼               ▼                ▼               ▼               ▼
  Setup           Iterate          Approve        Rollback        Archive
  Team            Collaborate      Sign-off       if needed       Lessons
```

### 1.3 Project Creation

**New Project Wizard:**
1. **Template Selection**
   - Blank Project
   - Extract from Existing Database
   - Import from File (SQL, JSON, XML)
   - Clone from Template
   - Fork from Existing Project

2. **Configuration**
   - Project Name & Description
   - Target Database Type (PostgreSQL, MySQL, Firebird, etc.)
   - Version Control Integration (Git repo link)
   - Team Members & Roles
   - Default Schema/Namespace

3. **Initial Extraction** (optional)
   - Connect to source database
   - Select objects to extract
   - Create baseline snapshot
   - Generate initial documentation

### 1.4 Project Sharing

**Share Levels:**
```
PRIVATE ──▶ TEAM ──▶ ORGANIZATION ──▶ PUBLIC
  │           │            │              │
  ▼           ▼            ▼              ▼
Owner only  Members     All org       Anyone
Can edit    Can:        users can     can view
            - View      view          Request
            - Comment   Comment       access
            - Suggest   Suggest
```

**Permission Matrix:**
| Role | View | Comment | Edit Design | Deploy | Admin |
|------|------|---------|-------------|--------|-------|
| Viewer | ✓ | ✓ | ✗ | ✗ | ✗ |
| Designer | ✓ | ✓ | ✓ | ✗ | ✗ |
| Deployer | ✓ | ✓ | ✓ | ✓ | ✗ |
| Owner | ✓ | ✓ | ✓ | ✓ | ✓ |

**Collaboration Features:**
- Real-time cursor presence
- Change notifications
- Comment threads on objects
- @mentions system
- Activity feed
- Version history with diff
- Branch/merge for designs

---

## 2. Expanded Diagramming

### 2.1 Diagram Types

#### 2.1.1 Entity-Relationship Diagrams (ERD)
```
┌─────────────────┐         ┌─────────────────┐
│    users        │         │    orders       │
├─────────────────┤         ├─────────────────┤
│ PK user_id      │────┐    │ PK order_id     │
│    username     │    │    │ FK user_id      │◄───┐
│    email        │    │    │    order_date   │    │
│    created_at   │    │    │    total        │    │
└─────────────────┘    │    └─────────────────┘    │
                       │    ┌─────────────────┐    │
                       └───▶│  order_items    │    │
                            ├─────────────────┤    │
                            │ PK item_id      │    │
                            │ FK order_id     │────┘
                            │ FK product_id   │────┐
                            │    quantity     │    │
                            └─────────────────┘    │
                                                   │
                            ┌─────────────────┐    │
                            │   products      │◄───┘
                            ├─────────────────┤
                            │ PK product_id   │
                            │    name         │
                            │    price        │
                            └─────────────────┘
```

**Features:**
- Auto-layout algorithms (hierarchical, organic, circular)
- Custom styling (colors, fonts, shapes)
- Relationship labels and cardinality
- Crow's foot notation
- Table grouping (schemas, subject areas)
- Column display options (all, keys only, custom)
- Zoom and pan
- Export (PNG, SVG, PDF, SQL)

#### 2.1.2 Schema Diagrams
Database-specific visualization showing:
- Schema boundaries
- Object dependencies
- Permission boundaries
- Tablespaces/filegroups
- Partitions

#### 2.1.3 Data Flow Diagrams (DFD)
```
┌─────────┐     ┌─────────┐     ┌─────────┐     ┌─────────┐
│  Source │────▶│Transform│────▶│  Load   │────▶│ Target  │
│  System │     │  Logic  │     │ Process │     │  DWH    │
└─────────┘     └─────────┘     └─────────┘     └─────────┘
                     │
                     ▼
                ┌─────────┐
                │  Audit  │
                │   Log   │
                └─────────┘
```

**Use Cases:**
- ETL pipeline design
- Data migration planning
- Integration architecture
- Event streaming

#### 2.1.4 Mind Maps
```
                    ┌─────────┐
                    │  CORE   │
                    │ CONCEPT │
                    └────┬────┘
           ┌───────────┼───────────┐
           ▼           ▼           ▼
      ┌────────┐  ┌────────┐  ┌────────┐
      │ Branch │  │ Branch │  │ Branch │
      │   A    │  │   B    │  │   C    │
      └───┬────┘  └───┬────┘  └───┬────┘
          │           │           │
    ┌─────┼─────┐     │     ┌─────┼─────┐
    ▼     ▼     ▼     ▼     ▼     ▼     ▼
   Leaf  Leaf  Leaf  Leaf  Leaf  Leaf  Leaf
```

**Features:**
- Free-form node placement
- Color-coded branches
- Collapsible nodes
- Icons per node
- Relationships between branches
- Export to outline

#### 2.1.5 Whiteboards
Free-form canvas for:
- Brainstorming
- Sketching
- Sticky notes
- Drawing
- Screenshots
- Annotations

**Tools:**
- Pen/pencil (freehand)
- Shapes (rectangle, circle, arrow)
- Text boxes
- Sticky notes
- Images/screenshots
- Connectors
- Layers

#### 2.1.6 Architecture Diagrams
- System topology
- Network diagrams
- Cloud infrastructure
- Microservice relationships
- API dependency maps

### 2.2 Diagram Elements

#### 2.2.1 Stencils Library
```
Database:
  [Table] [View] [Index] [Sequence] [Trigger] [Procedure]
  [Schema] [Database] [Server] [Cluster]

General:
  [Rectangle] [Rounded] [Circle] [Diamond] [Cylinder] [Document]
  [Person] [Cloud] [Mobile] [Browser] [Server Rack]

Flow:
  [Start/End] [Process] [Decision] [Data] [Manual Input]
  [Database] [Document] [Multi-doc] [Display] [Delay]

Custom:
  - Import SVG stencils
  - Create reusable groups
  - Template library
```

#### 2.2.2 Smart Connectors
- Auto-routing (orthogonal, curved, direct)
- Anchor points (cardinality markers)
- Labels (above, below, inline)
- Arrow styles
- Line styles (solid, dashed, dotted)

### 2.3 Diagram Synchronization

**Live Connection to Metadata:**
```
┌─────────────────────────────────────────────────────────────┐
│  DIAGRAM ◄──────► DESIGN MODEL ◄──────► DATABASE            │
│                                                             │
│  • Drag table from catalog to diagram                       │
│  • Edit in diagram updates design model                     │
│  • Validate against database constraints                    │
│  • Generate DDL from diagram                                │
└─────────────────────────────────────────────────────────────┘
```

**Change Propagation:**
- Rename table → Update all diagrams
- Add column → Optional: add to diagrams
- Delete object → Mark as deleted in diagrams
- Relationship change → Update connector

---

## 3. Design Workflow

### 3.1 Design States

Every object in a project has a design state:

```
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
│ EXTRACTED│───▶│  DRAFT   │───▶│ REVIEWED │───▶│APPROVED  │
│ (gray)   │    │ (yellow) │    │ (blue)   │    │(green)   │
└──────────┘    └──────────┘    └──────────┘    └────┬─────┘
      │                                               │
      │         ┌──────────┐    ┌──────────┐         │
      │         │ MODIFIED │    │ REJECTED │         │
      └────────▶│ (orange) │    │ (red)    │◄────────┘
                └──────────┘    └──────────┘
                      │
                      ▼
                ┌──────────┐
                │ DELETED  │
                │ (striked)│
                └──────────┘
```

| State | Icon | Description |
|-------|------|-------------|
| Extracted | Gray | Read-only copy from source DB |
| New/Draft | Purple | New object being designed |
| Modified | Orange | Existing object being changed |
| Pending | Yellow | Ready for review |
| Approved | Green | Approved for deployment |
| Implemented | Blue | Already deployed |
| Deleted | Red strikethrough | Marked for removal |
| Conflicted | Red exclamation | Merge conflict |

### 3.2 Design Actions

**Extract:**
```
Source DB ──▶ Read metadata ──▶ Create design copy ──▶ Mark as "extracted"
```

**Modify:**
```
Extracted object ──▶ Edit properties ──▶ Mark as "modified"
                     (columns, constraints, indexes)
```

**Create:**
```
New object ──▶ Define properties ──▶ Mark as "new"
               (table, view, procedure, etc.)
```

**Delete:**
```
Existing object ──▶ Mark as "deleted" ──▶ Keep in design, flag for drop
```

### 3.3 Design Validation

**Static Analysis:**
- Naming conventions
- Data type compatibility
- Reserved word checks
- Dependency validation
- Circular reference detection

**Cross-Reference:**
- Foreign key target exists
- View references valid tables
- Procedure calls valid objects
- Index columns exist

**Impact Analysis:**
```
Change: Modify column "price" from DECIMAL(10,2) to DECIMAL(12,4)

Impacts:
  ✓ Table: products
  ⚠ View: product_summary (depends on price)
  ⚠ Procedure: calculate_tax (uses price)
  ✗ Trigger: audit_products (logs old.price)
  
Recommendations:
  • Update view definition
  • Test procedure with new precision
  • Modify trigger to handle precision change
```

### 3.4 Compare & Diff

**Visual Diff Viewer:**
```
┌─────────────────┬─────────────────┬─────────────────┐
│   SOURCE (DB)   │     DESIGN      │   DIFFERENCE    │
├─────────────────┼─────────────────┼─────────────────┤
│                 │                 │                 │
│  users          │  users          │                 │
│  ─────────      │  ─────────      │                 │
│  id INT PK      │  id INT PK      │                 │
│  name VARCHAR   │  name VARCHAR   │                 │
│  email VARCHAR  │  email VARCHAR  │                 │
│                 │+ phone VARCHAR  │ ◀── ADDED       │
│                 │                 │                 │
│  products       │  products       │                 │
│  ─────────      │  ─────────      │                 │
│  price DECIMAL  │  price DECIMAL  │ ◀── MODIFIED    │
│  (10,2)         │  (12,4)         │     precision   │
│                 │                 │                 │
│  orders         │  orders         │                 │
│  ─────────      │  ─────────      │     DELETED ──▶ │
└─────────────────┴─────────────────┴─────────────────┘
```

**Diff Actions:**
- Apply design change to source
- Revert design to source
- Merge (selective apply)
- Ignore difference

---

## 4. Collaboration Model

### 4.1 Real-Time Collaboration

**Features:**
- WebSocket-based synchronization
- Operational Transform for concurrent edits
- Cursor presence (colored cursors with names)
- Selection highlighting
- Lock-on-edit (optional)
- Conflict resolution UI

**Session Management:**
```
User A ──┐
         ├──▶ Collaboration Server ◀──┐
User B ──┘                              ├──▶ Database
                              User C ───┘
```

### 4.2 Comments & Annotations

**Comment Types:**
- **Object Comments:** Attached to tables, columns, etc.
- **Diagram Comments:** Placed on canvas
- **Line Comments:** Attached to specific lines in SQL
- **General Comments:** Project-wide discussions

**Comment Features:**
- Threaded replies
- @mentions with notifications
- Resolve/unresolve
- Attach screenshots
- Link to commits/deployments
- Export to documentation

### 4.3 Version Control Integration

**Git Integration:**
```
Project Changes ──▶ Commit ──▶ Push to Git Repo
                        │
                        ▼
                   Change Log:
                   • Added users table
                   • Modified orders.price precision
                   • Removed legacy_orders table
```

**Branching:**
- Design branches (feature branches)
- Main branch (approved designs)
- Release branches
- Pull requests for design review

### 4.4 Activity Feed

```
┌─────────────────────────────────────────────────┐
│ Activity Feed                                   │
├─────────────────────────────────────────────────┤
│ 👤 Alice created table "customers"         2m   │
│ 👤 Bob modified column "price" precision   15m  │
│ 👤 Carol deployed changes to staging       1h   │
│ 👤 Dave commented on "orders diagram"      2h   │
│ 🔄 System backed up project automatically  4h   │
│ 👤 Alice shared project with Team Alpha    1d   │
└─────────────────────────────────────────────────┘
```

---

## 5. Testing & Deployment

### 5.1 Test Framework

**Test Types:**

1. **Unit Tests** (per object)
```sql
-- Test: users table constraints
TEST users_table:
  ASSERT INSERT users(name, email) VALUES ('Test', 'invalid') FAILS;
  ASSERT INSERT users(name, email) VALUES ('Test', 'test@test.com') SUCCEEDS;
  ASSERT SELECT COUNT(*) FROM users WHERE email IS NULL = 0;
```

2. **Integration Tests**
```sql
-- Test: Order creation workflow
TEST order_workflow:
  SETUP:
    INSERT INTO users(user_id, name) VALUES (1, 'Test User');
  
  TEST:
    INSERT INTO orders(user_id, total) VALUES (1, 100.00);
    ASSERT SELECT COUNT(*) FROM order_items WHERE order_id = LAST_INSERT_ID() = 0;
    
    INSERT INTO order_items(order_id, product_id, qty) VALUES (LAST_INSERT_ID(), 1, 2);
    ASSERT SELECT total FROM orders WHERE order_id = LAST_INSERT_ID() = 100.00;
  
  TEARDOWN:
    DELETE FROM order_items WHERE order_id = LAST_INSERT_ID();
    DELETE FROM orders WHERE user_id = 1;
    DELETE FROM users WHERE user_id = 1;
```

3. **Performance Tests**
```sql
TEST performance_orders_query:
  LOAD dataset 'orders_1M_rows';
  
  BENCHMARK:
    QUERY: SELECT * FROM orders WHERE order_date > '2024-01-01';
    EXPECT: execution_time < 100ms;
    EXPECT: rows_examined < 10000;
  
  VERIFY INDEX: idx_orders_date EXISTS;
```

4. **Data Quality Tests**
```sql
TEST data_quality_users:
  ASSERT SELECT COUNT(*) FROM users WHERE email NOT LIKE '%@%.%' = 0;
  ASSERT SELECT COUNT(*) FROM users WHERE LENGTH(name) < 2 = 0;
  ASSERT SELECT COUNT(DISTINCT email) = COUNT(*) FROM users;
```

### 5.2 Test Runner

**Execution Modes:**
- Dry Run (validate scripts only)
- Rollback Test (verify transactions)
- Full Execution
- Parallel/Sequential

**Test Results:**
```
Test Suite: Project Alpha v1.2.3
═══════════════════════════════════════════════════════
✓ users_table .................................... 12ms
✓ order_workflow ................................. 45ms
✓ performance_orders_query ....................... 89ms
✗ data_quality_users ............................. 23ms
  └─ FAIL: Found 3 invalid email addresses
✓ foreign_key_integrity .......................... 34ms

Results: 4 passed, 1 failed, 0 skipped
Time: 203ms
Coverage: 87% of objects tested
```

### 5.3 Deployment System

**Deployment Plans:**

A deployment plan is a sequence of changes:
```yaml
deployment_plan:
  name: "Release v2.1.0 - Add Customer Portal"
  version: "2.1.0"
  
  pre_deployment:
    - backup_database
    - notify_team
    - run_health_checks
  
  steps:
    - step: 1
      name: "Create new tables"
      objects:
        - customers_portal
        - portal_sessions
        - portal_preferences
      rollback: "drop_tables"
      
    - step: 2
      name: "Modify existing tables"
      objects:
        - users (add portal_access column)
      rollback: "revert_column"
      
    - step: 3
      name: "Create indexes"
      objects:
        - idx_portal_sessions_user
      rollback: "drop_index"
      
    - step: 4
      name: "Deploy procedures"
      objects:
        - sp_portal_login
        - sp_portal_logout
      rollback: "drop_procedures"
  
  post_deployment:
    - run_tests
    - verify_deployment
    - update_documentation
  
  rollback_plan: "full_rollback_v2.1.0.sql"
```

**Deployment Targets:**
- Development
- Testing/QA
- Staging
- Production
- Multiple regions

**Deployment Modes:**
1. **Atomic:** All or nothing (transaction wrapped)
2. **Incremental:** Step by step with checkpoints
3. **Blue/Green:** Deploy to new instance, switch traffic
4. **Canary:** Deploy to subset of data/users
5. **Offline:** Stop services, deploy, restart

**Safety Features:**
- Automatic backup before deployment
- Pre-deployment validation
- Rollback on failure
- Deployment locking (prevent concurrent deploys)
- Change approval workflow
- Deployment audit log

### 5.4 Migration Scripts

**Auto-Generated Migrations:**
```sql
-- Migration: 20240204_001_add_customer_portal
-- Generated by ScratchRobin from design changes

-- Step 1: Create table
CREATE TABLE portal_sessions (
    session_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id INTEGER NOT NULL REFERENCES users(user_id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL,
    last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Step 2: Add index
CREATE INDEX idx_portal_sessions_user 
ON portal_sessions(user_id) 
WHERE expires_at > CURRENT_TIMESTAMP;

-- Step 3: Add column to users
ALTER TABLE users 
ADD COLUMN portal_enabled BOOLEAN DEFAULT FALSE;

-- Rollback:
-- DROP TABLE portal_sessions;
-- ALTER TABLE users DROP COLUMN portal_enabled;
```

**Migration Validation:**
- Check for destructive changes
- Estimate execution time
- Identify locks/long operations
- Verify backward compatibility

---

## 6. Resource Tracking

### 6.1 Time Tracking

**Time Entry:**
```
┌───────────────────────────────────────────────────────┐
│ Time Entry                                            │
├───────────────────────────────────────────────────────┤
│ Date: [2024-02-04]                                    │
│ Activity: [Design ▼]                                  │
│ Object:  [users table ▼]                              │
│ Description: [Adding audit columns]                   │
│ Hours: [3.5]                                          │
│ Billable: [✓]                                         │
└───────────────────────────────────────────────────────┘
```

**Activity Categories:**
- Design/Modeling
- Development
- Testing
- Documentation
- Meeting/Discussion
- Research
- Deployment
- Bug Fixing

**Reports:**
- Time by person
- Time by object
- Time by activity type
- Burn-down charts
- Budget vs actual

### 6.2 Cost Analysis

**Cost Factors:**
- Personnel time (hourly rates)
- Infrastructure costs
- Storage costs
- Data transfer costs
- Tool/licensing costs

**Cost Projections:**
```
Design "Data Warehouse v2"
─────────────────────────────────────────
Personnel:     $12,500 (250 hours @ $50/hr)
Infrastructure: $2,400 ($200/mo x 12)
Storage:        $1,200 (2TB @ $0.05/GB/mo)
─────────────────────────────────────────
Total:          $16,100

Break-even: 6 months (vs current solution)
```

### 6.3 Asset Inventory

**Tracked Resources:**
- Database connections
- File storage
- External services
- API keys/credentials
- Documentation links
- Reference materials

---

## 7. Documentation System

### 7.1 Auto-Generated Documentation

**Data Dictionary:**
```markdown
# Data Dictionary: E-Commerce Database

## Table: users

| Column | Type | Nullable | Default | Description |
|--------|------|----------|---------|-------------|
| user_id | INTEGER | NO | auto | Primary key |
| username | VARCHAR(50) | NO | - | Unique login name |
| email | VARCHAR(255) | NO | - | Contact email |
| created_at | TIMESTAMP | NO | now() | Registration date |

### Relationships
- One-to-Many with `orders` via `user_id`
- Many-to-Many with `roles` via `user_roles`

### Indexes
- PRIMARY KEY (user_id)
- UNIQUE INDEX idx_username (username)
- INDEX idx_email (email)

### Triggers
- trg_users_audit: Audits all changes

### Business Rules
- Email must be unique across system
- Username cannot be changed after creation
```

**ERD Documentation:**
- Entity descriptions
- Relationship cardinalities
- Business rules
- Data flow

### 7.2 Manual Documentation

**Document Types:**
- Design Decisions (ADR format)
- Runbooks (procedures)
- Architecture Overview
- API Documentation
- User Guides
- Training Materials

**Rich Text Editor:**
- Markdown support
- Embedded diagrams
- Code blocks with syntax highlighting
- Tables
- Images/screenshots
- Version history

### 7.3 Documentation Publishing

**Output Formats:**
- HTML (web)
- PDF (print)
- Markdown (GitHub)
- Word (docx)

**Templates:**
- Technical Spec
- Data Dictionary
- API Reference
- User Guide
- Executive Summary

---

## 8. Integration Points

### 8.1 External Systems

**Version Control:**
- Git (GitHub, GitLab, Bitbucket)
- SVN
- Perforce

**CI/CD:**
- Jenkins
- GitHub Actions
- GitLab CI
- Azure DevOps

**Monitoring:**
- Prometheus
- Grafana
- Datadog
- New Relic

**Ticketing:**
- Jira
- GitHub Issues
- Azure DevOps Work Items

### 8.2 API

**REST API:**
```
GET    /api/v1/projects              # List projects
POST   /api/v1/projects              # Create project
GET    /api/v1/projects/{id}          # Get project
PUT    /api/v1/projects/{id}          # Update project
DELETE /api/v1/projects/{id}          # Delete project

GET    /api/v1/projects/{id}/objects  # List objects
POST   /api/v1/projects/{id}/deploy   # Deploy project
GET    /api/v1/projects/{id}/diff     # Get diff
```

**Webhooks:**
- Deployment events
- Design changes
- Test results
- Collaboration events

---

## 9. Security & Compliance

### 9.1 Data Protection

**Encryption:**
- At-rest (database files)
- In-transit (TLS)
- Sensitive fields (column-level)

**Access Control:**
- Role-based (RBAC)
- Attribute-based (ABAC)
- Row-level security
- Audit logging

### 9.2 Compliance Features

- Data lineage tracking
- Change audit trails
- PII detection/masking
- GDPR data mapping
- SOX compliance reports

---

## Appendix A: Icon Quick Reference

| Icon | Index | Used For |
|------|-------|----------|
| whiteboard | 70 | Brainstorming canvas |
| mindmap | 71 | Concept mapping |
| design | 72 | Design documents |
| draft | 73 | Work in progress |
| template | 74 | Reusable patterns |
| blueprint | 75 | Architecture plans |
| concept | 76 | Ideas/proposals |
| plan | 77 | Implementation plans |
| implemented | 80 | Deployed objects |
| pending | 81 | Staged changes |
| modified | 82 | Changed objects |
| deleted | 83 | Removed objects |
| newitem | 84 | New additions |
| sync | 90 | Synchronization |
| diff | 91 | Comparisons |
| migrate | 93 | Data migration |
| deploy | 94 | Deployment |
| shared | 100 | Shared resources |
| collaboration | 101 | Team work |
| team | 102 | Team members |
| server | 60 | Infrastructure |
| cluster | 64 | Database clusters |
| replica | 66 | Replicated data |
| shard | 67 | Partitioned data |
