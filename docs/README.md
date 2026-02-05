# ScratchRobin Documentation

## Overview

ScratchRobin is a comprehensive database design, modeling, and deployment tool that bridges the gap between database administration and software development workflows.

## Key Capabilities

### 🤖 **AI-Powered Database Assistance** ✅ NEW
- **Multi-Provider Support** - OpenAI, Anthropic, Ollama, Google Gemini
- **SQL Assistant** - Natural language to SQL, query explanation, optimization
- **Schema-Aware AI** - AI understands your database structure
- **Secure Credential Storage** - API keys stored in system keyring

### 🔗 **Issue Tracker Integration** ✅ NEW
- **Multi-Platform** - Jira, GitHub, GitLab integration
- **Object-Issue Linking** - Link database objects to external issues
- **Bi-Directional Sync** - Real-time updates via webhooks
- **Issue Templates** - Auto-create issues with context

### 🗂️ **Project-Centric Workflow**
- Extract database schemas into design projects
- Make changes in isolation from production
- Track design states (extracted, draft, modified, approved, implemented)
- Deploy changes with full audit trail

### 📊 **Expanded Diagramming**
- **ERD**: Traditional entity-relationship diagrams with auto-sync
- **Whiteboards**: Free-form brainstorming and sketching
- **Mind Maps**: Conceptual organization and planning
- **Data Flow**: ETL and integration architecture
- **Schema Diagrams**: Database-specific visualizations

### 👥 **Real-Time Collaboration**
- Multi-user editing with operational transform
- Comments and annotations
- Design review workflows
- Activity feeds and notifications
- @mentions and threading

### 🧪 **Integrated Testing**
- Unit tests per database object
- Integration tests for workflows
- Performance benchmarking
- Data quality validation
- Pre-deployment validation

### 🚀 **Deployment Management**
- Deployment plans with step-by-step execution
- Multiple target environments
- Automatic rollback on failure
- Blue/green and canary deployment options
- Migration script generation

### 📈 **Resource Tracking**
- Time tracking by activity and object
- Cost analysis and projections
- Asset inventory
- Budget monitoring

### 📚 **Documentation System**
- Auto-generated data dictionaries
- Rich text documentation
- Template-based publishing
- Version-controlled docs

## Documentation Index

### Core Specifications
- **[SPECIFICATIONS.md](SPECIFICATIONS.md)** - Complete technical specifications for all features
- **[WORKFLOW_GUIDE.md](WORKFLOW_GUIDE.md)** - Step-by-step workflow guide with visual diagrams
- **[planning/IMPLEMENTATION_ROADMAP.md](planning/IMPLEMENTATION_ROADMAP.md)** - Current execution roadmap

### AI Integration
- **[AI_INTEGRATION_SPEC.md](AI_INTEGRATION_SPEC.md)** - AI provider setup and configuration

### Issue Tracking
- **[ISSUE_TRACKER_SPECIFICATION.md](ISSUE_TRACKER_SPECIFICATION.md)** - Issue tracker integration guide

### Git Integration
- **[GIT_INTEGRATION.md](GIT_INTEGRATION.md)** - Dual-repo Git system specifications
- **[DATABASE_GIT_PROTOCOL.md](DATABASE_GIT_PROTOCOL.md)** - ScratchBird Git protocol details
- **[GIT_WORKFLOW_EXAMPLES.md](GIT_WORKFLOW_EXAMPLES.md)** - Practical Git workflow examples

### Docker Deployment
- **[DOCKER_DEPLOYMENT.md](DOCKER_DEPLOYMENT.md)** - Docker container deployment and management
  - Dockerfile configuration
  - Docker Compose setup
  - ScratchRobin Docker Manager
  - Environment variables reference
  - Backup and restore procedures

### Additional Features
- **[ADDITIONAL_FEATURES.md](ADDITIONAL_FEATURES.md)** - Extended features including:
  - OLAP Cube Design for data warehousing
  - Comprehensive Testing Framework
  - Data Lineage Tracking
  - Data Masking & Anonymization
  - API Generation from database
  - Event Streaming / CDC

### Expanded Specifications (ScratchBird Mirror)
- **[specifications/README.md](specifications/README.md)** - Full spec index, including mirrored ScratchBird specs
- **[specifications/SPEC_MAP.md](specifications/SPEC_MAP.md)** - Spec map and navigation

**Dual-Repo Model:**
```
Project Repo (Git)          Database Repo (ScratchBird Git)
────────────────            ───────────────────────────────
designs/*.json     ◄──────► schema/*.sql
diagrams/*.svg     ◄──────► migrations/*.sql
tests/*.yml        ◄──────► snapshots/*.sql
docs/*.md          ◄──────► procedures/*.sql
```

### Icon System
- **[icons/README.md](icons/README.md)** - Icon inventory and usage guide
- 68 unique icons covering:
  - Database objects (tables, views, procedures, etc.)
  - Infrastructure (servers, clusters, networks)
  - Design elements (whiteboards, mind maps, drafts)
  - Workflow states (implemented, pending, modified)
  - Collaboration (shared, team, comments)
  - Deployment (sync, diff, migrate, deploy)

### User Guides
- **Getting Started** - Create your first project
- **Design Workflow** - From extraction to deployment
- **Collaboration** - Working with your team
- **Testing** - Writing and running tests
- **Deployment** - Safe production changes

### Developer Documentation
- **Architecture Overview** - System design
- **API Reference** - REST API documentation
- **Plugin Development** - Extending ScratchRobin
- **Contributing** - Contribution guidelines

## Quick Start

### Creating a New Project

```
1. File → New Project
2. Select template (Blank, Extract from DB, Import)
3. Configure connections
4. Extract baseline schema
5. Start designing!
```

### Typical Workflow

```
Extract ──▶ Design ──▶ Collaborate ──▶ Test ──▶ Deploy ──▶ Monitor
   │          │            │           │         │          │
   ▼          ▼            ▼           ▼         ▼          ▼
Baseline   Modify      Review      Validate  Execute    Verify
(gray)   (orange)    (comments)   (tests)   (deploy)   (monitor)
```

### Design States

| State | Color | Icon | Description |
|-------|-------|------|-------------|
| Extracted | Gray | Database | Read-only from source |
| New | Purple | Star | Newly created |
| Modified | Orange | Pencil | Changed from source |
| Pending | Yellow | Clock | Awaiting review |
| Approved | Green | Check | Ready to deploy |
| Implemented | Blue | Database | Deployed |
| Deleted | Red | X | Marked for removal |

## Feature Matrix

| Feature | Status | Phase |
|---------|--------|-------|
| Project Infrastructure | ✅ Complete | 1 |
| Design State Management | ✅ Complete | 2 |
| ERD Diagramming | ✅ Complete | 3 |
| AI Assistance | ✅ Complete | 9 |
| Issue Tracker Integration | ✅ Complete | 10 |
| Whiteboard | 🔄 In Progress | 3 |
| Mind Maps | 🔄 In Progress | 3 |
| Real-time Collaboration | 📋 Planned | 4 |
| Comments & Annotations | 📋 Planned | 4 |
| Test Framework | 📋 Planned | 5 |
| Deployment System | 📋 Planned | 6 |
| Time Tracking | 📋 Planned | 7 |
| Documentation System | 📋 Planned | 8 |
| Git Integration | 📋 Planned | 11 |

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     SCRATCHROBIN                            │
├─────────────────────────────────────────────────────────────┤
│  UI Layer (wxWidgets)                                       │
│  ├── Main Window (Tree, Inspector, SQL Editor)             │
│  ├── Diagram Canvas (ERD, Whiteboard, Mind Map)            │
│  ├── Property Panels (Object editing)                      │
│  └── Dialogs (Wizards, Preferences)                        │
├─────────────────────────────────────────────────────────────┤
│  Application Layer                                          │
│  ├── Project Manager (Create, Open, Save)                  │
│  ├── Design Workflow (State machine)                       │
│  ├── Collaboration Client (Sync)                           │
│  ├── Test Runner (Execute tests)                           │
│  └── Deployment Engine (Execute plans)                     │
├─────────────────────────────────────────────────────────────┤
│  Core Layer                                                 │
│  ├── Metadata Model (Schema representation)                │
│  ├── Connection Manager (DB connections)                   │
│  ├── SQL Parser/Generator                                  │
│  └── Diff Engine (Compare schemas)                         │
├─────────────────────────────────────────────────────────────┤
│  Storage Layer                                              │
│  ├── Project Files (JSON/YAML)                             │
│  ├── Local Cache (SQLite)                                  │
│  └── External Storage (Git, Cloud)                         │
└─────────────────────────────────────────────────────────────┘
```

## Icon System

The icon system supports 121 icon slots with 68 unique icons:

```
000-009: Application (root, connection, settings, error, diagram)
010-024: Database Objects (tables, views, indexes, triggers, etc.)
025-029: Schema Organization (database, catalog, schema, folder)
030-034: Security (users, hosts, permissions)
035-044: Projects (project, sql, note, timeline, job)
045-049: Version Control (git, branch)
050-059: Maintenance (backup, restore)
060-069: Infrastructure (server, client, network, cluster, replica, shard)
070-079: Design (whiteboard, mindmap, design, draft, template, blueprint)
080-089: Design States (implemented, pending, modified, deleted, new)
090-099: Synchronization (sync, diff, compare, migrate, deploy)
100-109: Collaboration (shared, collaboration, team)
110-119: Security States (lock, unlock, history, audit, tag, bookmark)
120: Default
```

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

ScratchRobin is licensed under the Initial Developer's Public License Version 1.0.

## Support

- **Documentation**: [docs/](.)
- **Issues**: GitHub Issues
- **Discussions**: GitHub Discussions
- **Wiki**: GitHub Wiki

---

*Last updated: 2026-02-03*
