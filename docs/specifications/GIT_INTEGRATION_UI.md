# Git Integration UI Specification

**Status**: Beta Placeholder  
**Target Release**: Beta  
**Priority**: P2

---

## Overview

Git Integration provides version control for database schemas, migration scripts, and database projects. It enables database developers to use familiar Git workflows for tracking schema changes, collaborating with team members, and deploying with confidence.

---

## Planned Features

### 1. Repository Management
- Initialize new database project repositories
- Clone existing repositories
- Connect to remote Git servers (GitHub, GitLab, Bitbucket, etc.)
- Branch and tag management
- Remote synchronization

### 2. Schema Versioning
- Automatic DDL generation from database objects
- Schema snapshot comparison
- Diff generation for schema changes
- One-click schema export to Git
- Import schema from Git to database

### 3. Migration Script Management
- Forward migration generation (upgrade scripts)
- Backward migration generation (downgrade scripts)
- Migration version sequencing
- Checksum validation
- Migration execution tracking

### 4. Visual Diff & Merge
- Side-by-side schema comparison
- Syntax-highlighted SQL diffs
- Three-way merge for conflict resolution
- Interactive diff navigation
- Batch accept/reject changes

### 5. Git Workflows
- Feature branch workflow support
- Trunk-based development
- GitFlow for database projects
- Pull request integration
- Code review for DDL changes

### 6. Deployment Integration
- CI/CD pipeline hooks
- Automated testing for migrations
- Deployment approval workflows
- Environment promotion
- Deployment history tracking

---

## UI Components

### Main Git Window
```
+----------------------------------------------------------+
|  Git Integration - [Repository Name]                     |
+----------------------------------------------------------+
|  [Commit] [Pull] [Push] [Branch] [Merge] [Stash]        |
+----------------------------------------------------------+
|  +----------+  +--------------------------------------+  |
|  | Branches |  | [Status] [History] [Diff] [Migrations]|  |
|  |          |  |                                       |  |
|  | * main   |  | Working Directory                     |  |
|  |   develop|  | M schema/tables/users.sql             |  |
|  |   feature|  | A migrations/V003__add_index.sql      |  |
|  |   release|  |                                       |  |
|  |          |  | Staged Changes                        |  |
|  | Remotes  |  | M schema/tables/orders.sql            |  |
|  |          |  |                                       |  |
|  | Tags     |  | +----------------------------------+  |  |
|  |          |  | | Commit Message                    |  |  |
|  |          |  | |                                   |  |  |
|  |          |  | +----------------------------------+  |  |
|  +----------+  +--------------------------------------+  |
```

### Tabs

#### Status Tab
- Working directory changes
- Staged changes
- Unstaged changes
- File status indicators (modified, added, deleted, renamed)
- Commit message editor
- Commit action

#### History Tab
- Commit log with graph
- Commit details (author, date, message)
- File changes per commit
- Branch/tag labels
- Search/filter commits

#### Diff Tab
- Side-by-side or unified diff view
- Schema-aware SQL highlighting
- Line-by-line change navigation
- Stage/unstage individual lines (future)

#### Migrations Tab
- Migration script list
- Applied vs pending status
- Execution history
- One-click apply/rollback

#### Branches Tab
- Local branches list
- Remote branches
- Branch operations (create, checkout, merge, delete)
- Branch comparison

---

## Data Models

See `src/core/git_model.h`:

- `GitRepository` - Repository information
- `GitBranch` - Branch details
- `GitCommit` - Commit information
- `GitChangedFile` - File change status
- `GitFileDiff` - Diff content
- `GitStash` - Stash entries
- `GitTag` - Tag information
- `GitRemote` - Remote configuration
- `DbObjectGitMapping` - DB object to file mapping
- `MigrationScript` - Migration tracking
- `GitDbConfig` - Git configuration for DB projects
- `GitWorkflowConfig` - Workflow settings

---

## Schema Export Format

```
project-repo/
├── schema/
│   ├── tables/
│   │   ├── public/
│   │   │   ├── users.sql
│   │   │   ├── orders.sql
│   │   │   └── products.sql
│   │   └── accounting/
│   │       └── invoices.sql
│   ├── views/
│   ├── indexes/
│   ├── triggers/
│   └── procedures/
├── migrations/
│   ├── V001__initial_schema.sql
│   ├── V002__add_user_indexes.sql
│   └── V003__create_orders_table.sql
├── seeds/
│   └── reference_data.sql
├── .gitignore
└── scratchrobin-project.toml
```

---

## Migration Script Format

```sql
-- Migration: V003__create_orders_table.sql
-- Author: Jane Developer
-- Date: 2026-02-03

-- Up migration
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    total DECIMAL(10,2),
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_orders_user ON orders(user_id);
CREATE INDEX idx_orders_created ON orders(created_at);

-- Down migration (for rollback)
-- DROP INDEX idx_orders_created;
-- DROP INDEX idx_orders_user;
-- DROP TABLE orders;
```

---

## Supported Git Providers

| Provider | Status | Notes |
|----------|--------|-------|
| GitHub | Planned | Full integration |
| GitLab | Planned | Full integration |
| Bitbucket | Planned | Cloud and Server |
| Azure DevOps | Future | Git repos only |
| Self-hosted | Planned | Generic Git support |

---

## Implementation Status

- [x] Data models defined (`git_model.h`)
- [x] Stub UI implemented (`git_integration_frame.cpp`)
- [ ] Git command integration
- [ ] Schema export engine
- [ ] Diff viewer
- [ ] Migration management
- [ ] CI/CD webhooks

---

## Future Enhancements

- Web-based conflict resolution
- Team collaboration features
- Automated migration testing
- Database review apps
- Schema documentation generation
- Change impact analysis
- Integration with popular ORMs
