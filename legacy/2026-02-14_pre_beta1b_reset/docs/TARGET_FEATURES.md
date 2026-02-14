# ScratchRobin Target Features

## Overview

ScratchRobin is a comprehensive database design, modeling, and deployment tool that bridges the gap between database administration and modern software development workflows. This document describes each feature and its purpose.

---

## Core Features

### 1. Project System
**What it is:** A workspace-centric approach to database design that encapsulates all design artifacts, documentation, tests, and deployment configurations.

**Why it exists:** 
- Traditional database tools work directly on live databases, making changes risky and irreversible
- Modern software development uses version control, code reviews, and staged deployments
- Database changes need the same rigor as application code

**Key capabilities:**
- Multiple projects with isolated design spaces
- Object state tracking (extracted, new, modified, pending, approved, implemented)
- Design-to-deployment workflow
- Change history and audit trail
- Binary project persistence with deterministic chunking
- Governance metadata (owners, environments, compliance, AI policies)

---

### 2. Dual-Repo Git Integration
**What it is:** Synchronization between a standard Git repository (design files) and a database-native Git repository (schema and migrations).

**Why it exists:**
- Design artifacts (ERDs, documentation) need version control
- Database schema changes need to be tracked and deployed
- The two repositories need to stay in sync for consistency
- Enables branching, merging, and rollback of database changes

**Key capabilities:**
- Bidirectional sync between project and database repos
- Automatic DDL generation from design files
- Cross-repo commit references
- Conflict detection and resolution
- Branch isolation for schema changes

---

### 3. Reporting & Data Views
**What it is:** A built-in reporting system with stored questions, dashboards, subscriptions, and embedded data views in diagrams.

**Why it exists:**
- Design artifacts must include live context and examples
- Teams need consistent reporting definitions tied to schema evolution

**Key capabilities:**
- Reporting object storage format (questions, dashboards, alerts, segments)
- Embedded Data View panels (cached query samples and dummy examples)
- Refresh scheduling and invalidation on schema change
- Reporting panel UI for query building, dashboards, and alert stubs

---

### 4. Security Policy Management
**What it is:** Dedicated security policy UIs for reviewing and editing governance controls tied to ScratchBird catalogs.

**Why it exists:**
- Security policies are critical and need clear visibility
- Governance actions must be auditable and repeatable
- Teams need consistent tooling across policy types

**Key capabilities:**
- RLS policy manager + editor
- Audit policy manager + retention policy dialog
- Password policy wizard + SQL generation
- Login lockout policy viewer
- Role switch policy viewer
- Policy epoch viewer
- Audit log viewer

---

### 5. Comprehensive Icon System (121 slots, 74 icons)
**What it is:** Visual representation of all object types in the catalog tree with state-based coloring.

**Why it exists:**
- Visual recognition is faster than reading text labels
- State colors (green=approved, orange=modified, red=deleted) provide instant status awareness
- Different object types (tables, views, cubes, etc.) need distinct visual identities

**Icon categories:**
- Database objects (tables, views, indexes, procedures)
- Infrastructure (servers, clusters, networks, replicas)
- Design elements (whiteboards, mindmaps, blueprints)
- Workflow states (implemented, pending, modified, new)
- Synchronization (sync, diff, deploy, migrate)
- Collaboration (shared, team, comments)

---

### 6. Design State Management
**What it is:** A state machine tracking the lifecycle of each database object from extraction to deployment.

**Why it exists:**
- Need to distinguish between what's in production vs. what's being designed
- Team members need to know which objects are ready for review vs. still being worked on
- Deployment planning requires knowing which changes are approved

**States:**
| State | Icon | Purpose |
|-------|------|---------|
| Extracted | Gray | Read-only baseline from database |
| New | Purple | Object being created |
| Modified | Orange | Existing object being changed |
| Pending | Yellow | Ready for review |
| Approved | Green | Approved for deployment |
| Implemented | Blue | Successfully deployed |
| Deleted | Red X | Marked for removal |

---

### 7. Expanded Diagramming & Modeling
**What it is:** A multi-notation diagramming system covering ERD, Silverston, whiteboard, mind map, and data flow.

**Why it exists:**
- Teams need multiple representations for different stages of design
- Complex systems require topology, replication, and workflow views

**Key capabilities:**
- Silverston supertype/subtype grids with replication groups
- Whiteboard models with domain wizard + freeform attributes
- Mind map serialization and round-trip rules
- DFD diagrams with ERD traceability
- SVG-first rendering for consistent export
- Embedded Data View panels (stale indicators, refresh UI, grid rendering)

---

## Diagramming & Design

### 5. Entity-Relationship Diagrams (ERD)
**What it is:** Visual representation of database tables, columns, and relationships with automatic sync to the catalog.

**Why it exists:**
- Visual schemas are easier to understand than SQL DDL
- Relationships (foreign keys) are clearer in diagrams
- New team members can understand the database structure quickly
- Changes in the diagram update the design model automatically

**Features:**
- Auto-layout algorithms
- Crow's foot notation
- Relationship cardinality
- Table grouping by schema
- Export to PNG, SVG, PDF, SQL

---

### 6. Whiteboards
**What it is:** Free-form canvas for brainstorming, sketching, and collaborative ideation with typed objects for structured diagramming.

**Why it exists:**
- Not all design work is structured (ERDs)
- Early ideation benefits from free-form drawing
- Teams can sketch concepts before formalizing them
- Captures the "why" behind design decisions
- Visual database object mapping helps understand architectures

**Features:**
- **Typed Objects**: 12 object types (Database, Schema, Table, View, Procedure, Function, Trigger, Index, Datastore, Server, Cluster, Generic)
- **Visual Header**: Colored header showing `Type: Name` with type-specific colors
- **Details Area**: Free-form text area inside each object for specifications
- **Editing**: Double-click header to edit name, body to edit details
- Freehand drawing
- Sticky notes
- Shapes and connectors
- Image import
- Multiple pages/layers
- Auto-sizing based on content

**Object Types & Colors:**
| Type | Color | Use Case |
|------|-------|----------|
| Database | Blue | Database servers |
| Schema | Green | Schema containers |
| Table | Peach | Data tables |
| View | Plum | Database views |
| Procedure | Yellow | Stored procedures |
| Function | Light Blue | Functions |
| Trigger | Pink | Triggers |
| Index | Silver | Indexes |
| Datastore | Tan | File storage |
| Server | Steel Blue | Application servers |
| Cluster | Sea Green | Server clusters |
| Generic | Gray | Custom objects |

---

### 7. Mind Maps
**What it is:** Hierarchical visual organization of concepts, starting from a central idea.

**Why it exists:**
- Brainstorming and concept exploration
- Understanding domain relationships
- Organizing thoughts before database design
- Non-linear thinking about data structures

**Features:**
- Collapsible branches
- Color-coded nodes
- Export to outline/text
- Icons per node

---

### 8. OLAP Cube Design
**What it is:** Design interface for data warehouse cubes with dimensions, measures, and hierarchies.

**Why it exists:**
- Analytical databases have different design patterns than transactional
- Cubes require dimensions (time, geography) and measures (sum, count)
- Business intelligence teams need visual cube design tools
- Multiple OLAP engines (SSAS, Kylin, ClickHouse) need a unified interface

**Features:**
- Dimension and measure definition
- Hierarchy management
- Calculated members (formulas)
- Support for multiple OLAP engines
- Cube deployment

---

## Testing & Quality

### 9. Testing Framework
**What it is:** Comprehensive test suite for database objects with 6 test types.

**Why it exists:**
- Database changes need validation before deployment
- Testing catches errors before they reach production
- Different types of tests catch different types of problems
- Automated testing enables CI/CD pipelines

**Test types:**
| Type | Purpose | Example |
|------|---------|---------|
| Unit | Object structure | Column exists, type is correct |
| Integration | Workflows | Order processing end-to-end |
| Performance | Speed | Query executes < 100ms |
| Data Quality | Data integrity | No orphaned records |
| Security | Access control | Unauthorized users blocked |
| Migration | Schema changes | Migration applies cleanly |

**Output formats:** Text, JSON, HTML, JUnit XML, Markdown

---

### 10. Data Lineage Tracking
**What it is:** Visual and queryable tracking of data flow from source to destination.

**Why it exists:**
- Impact analysis: "If I change this column, what breaks?"
- Regulatory compliance requires understanding data flow
- Debugging data issues requires knowing where data came from
- Data governance requires visibility into transformations

**Features:**
- Column-level lineage
- Transformation tracking
- Impact analysis
- Visual lineage graphs

---

### 11. Data Masking & Anonymization
**What it is:** Rules-based system for protecting sensitive data in non-production environments.

**Why it exists:**
- Production data often contains PII, PCI, PHI
- Development and testing shouldn't use real customer data
- GDPR, CCPA, HIPAA require data protection
- Different environments need different masking levels

**Masking methods:**
- Partial (***@domain.com)
- Regex pattern replacement
- Cryptographic hashing
- Format-preserving encryption
- Fake data generation (realistic but synthetic)

---

## Deployment & Operations

### 12. Deployment Management
**What it is:** Structured deployment of database changes with rollback capability.

**Why it exists:**
- Database changes are risky and hard to undo
- Need safe deployment to multiple environments (dev, staging, prod)
- Deployment plans enable review before execution
- Rollback plans protect against failures

**Features:**
- Deployment plans with step-by-step execution
- Multiple environments (dev, test, staging, prod)
- Blue/green, canary, and atomic deployment modes
- Automatic rollback on failure
- Pre and post-deployment validation

---

### 13. Migration Generation
**What it is:** Automatic generation of SQL migration scripts from design changes.

**Why it exists:**
- Manually writing migrations is error-prone
- Design changes need to be translated to DDL
- Migrations need rollback scripts
- Order of operations matters (dependencies)

**Features:**
- DDL generation from design diffs
- Rollback script generation
- Dependency ordering
- Migration validation
- Version tracking

---

### 14. Event Streaming / CDC
**What it is:** Change Data Capture from database to message brokers.

**Why it exists:**
- Modern architectures need real-time data flows
- Microservices need to react to database changes
- Data warehouses need incremental updates
- Audit trails require change tracking

**Features:**
- Capture INSERT, UPDATE, DELETE operations
- Support for Kafka, Kinesis, Pub/Sub, RabbitMQ
- Debezium-compatible message format
- Selective table monitoring

---

## Collaboration & Documentation

### 15. Real-Time Collaboration
**What it is:** Multi-user editing with live cursors, comments, and change notifications.

**Why it exists:**
- Database design is often a team effort
- Distributed teams need to work together
- Code reviews catch errors early
- Comments capture design rationale

**Features:**
- Live cursor presence
- Operational Transform for concurrent edits
- Comment threads with @mentions
- Activity feed
- Permission-based access control

---

### 16. API Generation
**What it is:** Automatic generation of REST APIs from database schemas.

**Why it exists:**
- Applications need APIs to access databases
- Manual API development is repetitive
- Database changes should propagate to APIs
- Documentation should stay in sync

**Features:**
- Auto-generate CRUD endpoints
- Custom endpoint definitions
- OpenAPI/Swagger documentation
- JWT authentication
- Rate limiting

---

### 17. Auto-Generated Documentation
**What it is:** Automatic creation of data dictionaries, ERDs, and API docs.

**Why it exists:**
- Documentation becomes outdated quickly
- Manual documentation is tedious
- New team members need current documentation
- Compliance often requires documentation

**Features:**
- Data dictionaries with column descriptions
- Relationship diagrams
- Change logs
- Markdown/HTML/PDF export
- Template-based customization

---

## Multi-Database Support

### 18. Database Engine Support
**What it is:** Native support for multiple database engines with engine-specific features.

**Supported engines:**
- **PostgreSQL** - Full feature support, JSON, arrays, advanced types
- **MySQL/MariaDB** - Popular web database, good replication
- **Firebird** - Embedded and server modes, excellent for desktop apps
- **ScratchBird** - Native embedded database with Git support

**Why multiple engines:**
- Organizations use different databases for different needs
- Migration between engines requires understanding both
- Features vary between engines (need capability detection)
- Cross-database queries and comparisons

---

## Why ScratchRobin Exists

### The Problem
Traditional database tools fall into two categories:

1. **Administration tools** (pgAdmin, MySQL Workbench)
   - Work directly on live databases
   - Limited version control integration
   - No design workflow
   - Risky changes with no rollback

2. **Modeling tools** (ERWin, PowerDesigner)
   - Expensive enterprise software
   - Disconnected from development workflow
   - No deployment automation
   - Poor collaboration features

### The Solution
ScratchRobin bridges the gap by:

1. **Bringing software engineering practices to databases**
   - Version control (Git)
   - Code review (pull requests)
   - Testing (automated test suites)
   - CI/CD (deployment pipelines)

2. **Separating design from implementation**
   - Design in isolation from production
   - Review and approve before deployment
   - Track design state and history
   - Rollback when needed

3. **Supporting the full data lifecycle**
   - Design (ERD, whiteboards, mind maps)
   - Development (Git workflow, testing)
   - Deployment (migration scripts, plans)
   - Operations (monitoring, CDC, lineage)

4. **Enabling collaboration**
   - Real-time multi-user editing
   - Comments and annotations
   - Activity tracking
   - Permission management

---

## Feature Roadmap

### Phase 1: Core (Completed)
- [x] Project system with state management
- [x] Dual-repo Git integration (designs)
- [x] Icon system (74 icons)
- [x] Basic UI framework

### Phase 2: Design (Completed)
- [x] ERD diagramming
- [x] Whiteboards with typed objects
- [x] Mind maps
- [x] Data Flow diagrams (DFD) with ERD traceability
- [ ] OLAP cube design

### Phase 3: Testing (Completed)
- [x] Testing framework core (6 test types)
- [x] Test execution UI with progress tracking
- [x] Auto-test generation (stubs)
- [x] Performance benchmarking
- [x] Data quality testing
- [x] Security testing

### Phase 4: Deployment (Completed)
- [x] Migration generation with DDL export
- [x] Deployment plans with approval workflow
- [x] Multi-environment support (dev, test, staging, prod)
- [x] Rollback automation (stubs)
- [x] Docker deployment management

### Phase 5: Collaboration (Completed)
- [x] Real-time collaboration core (OT algorithm)
- [x] Operational Transform implementation
- [x] Conflict resolution strategies
- [x] Lock management

### Phase 5: Advanced (Future)
- [ ] Data lineage tracking
- [ ] Data masking
- [ ] API generation
- [ ] CDC/Streaming
- [ ] AI-assisted design

---

## Getting Started

To use ScratchRobin effectively:

1. **Create a project** from an existing database or from scratch
2. **Extract the baseline schema** (everything marked as "extracted")
3. **Make design changes** (new objects or modifications)
4. **Create tests** to validate your changes
5. **Request review** (mark objects as "pending")
6. **Deploy** approved changes to staging, then production

---

## Contributing

ScratchRobin is open source and welcomes contributions:

- **Documentation:** Help improve docs and examples
- **Testing:** Report bugs and test edge cases
- **Features:** Implement features from the roadmap
- **Database Support:** Add support for new database engines

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## License

ScratchRobin is licensed under the Initial Developer's Public License Version 1.0.

---

*Last updated: 2026-02-03*
*Version: 0.1.0-beta*
