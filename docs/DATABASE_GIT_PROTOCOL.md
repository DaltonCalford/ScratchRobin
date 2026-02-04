# ScratchBird Git Protocol Specification

## Overview

The ScratchBird Git Protocol extends Git semantics to database schema management, enabling version control of database objects with full history, branching, and merging capabilities.

## Protocol Design

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    SCRATCHBIRD GIT PROTOCOL                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                     SCRATCHBIRD CLUSTER                              │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │   │
│  │  │   Node 1     │  │   Node 2     │  │   Node 3     │              │   │
│  │  │  (Primary)   │  │  (Replica)   │  │  (Replica)   │              │   │
│  │  │              │  │              │  │              │              │   │
│  │  │ ┌──────────┐ │  │ ┌──────────┐ │  │ ┌──────────┐ │              │   │
│  │  │ │ Git Repo │ │  │ │ Git Repo │ │  │ │ Git Repo │ │              │   │
│  │  │ │  Store   │ │◄─┼►│ │  Store   │ │◄─┼►│ │  Store   │ │              │   │
│  │  │ └──────────┘ │  │ └──────────┘ │  │ └──────────┘ │              │   │
│  │  │              │  │              │  │              │              │   │
│  │  │ ┌──────────┐ │  │ ┌──────────┐ │  │ ┌──────────┐ │              │   │
│  │  │ │  Schema  │ │  │ │  Schema  │ │  │ │  Schema  │ │              │   │
│  │  │ │  Store   │ │  │ │  Store   │ │  │ │  Store   │ │              │   │
│  │  │ └──────────┘ │  │ └──────────┘ │  │ └──────────┘ │              │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘              │   │
│  │                                                                     │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │              GIT COORDINATION LAYER                          │   │   │
│  │  │  • Commit consensus across nodes                             │   │   │
│  │  │  • Branch replication                                        │   │   │
│  │  │  • Conflict resolution                                       │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    ▲                                        │
│                     Wire Protocol  │                                        │
│                          (gRPC/WebSocket)                                   │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                         CLIENTS                                      │   │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐            │   │
│  │  │ScratchRobin│  │  CLI Tool │  │  CI/CD   │  │  IDE     │            │   │
│  │  │  (GUI)   │  │          │  │  Pipeline│  │  Plugin  │            │   │
│  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘            │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Core Concepts

### Git Objects in Database Context

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    DATABASE GIT OBJECT MAPPING                              │
└─────────────────────────────────────────────────────────────────────────────┘

Standard Git          Database Git              Example
────────────          ────────────              ───────

Blob (file content)   Schema Object DDL         CREATE TABLE users (...)
                                              CREATE INDEX idx_users_email

Tree (directory)      Schema Namespace          schema/public/
                                              schema/analytics/

Commit (snapshot)     Schema Snapshot           All objects at point in time
                                              + metadata

Branch (pointer)      Database Branch           main, develop, feature/xyz
                                              Isolated schema state

Tag (named commit)    Release Tag               v1.2.0, baseline-2024
                                              Immutable reference

Remote                Cluster Node              Primary, Replica-1, Replica-2

Merge                 Schema Merge              Combine branch schemas
                                              with conflict detection
```

### Storage Architecture

```sql
-- Internal Git storage tables

-- Object store (content-addressable)
CREATE TABLE _git.objects (
    hash BYTEA PRIMARY KEY,           -- SHA-256 of content
    type VARCHAR(10),                 -- blob, tree, commit, tag
    content BYTEA,                    -- Compressed content
    size INTEGER,
    created_at TIMESTAMP
);

-- References (branches, tags)
CREATE TABLE _git.refs (
    ref_name VARCHAR(255) PRIMARY KEY,  -- refs/heads/main, refs/tags/v1.0
    commit_hash BYTEA REFERENCES _git.objects(hash),
    updated_at TIMESTAMP,
    updated_by VARCHAR(255)
);

-- Commit relationships
CREATE TABLE _git.commits (
    hash BYTEA PRIMARY KEY REFERENCES _git.objects(hash),
    tree_hash BYTEA REFERENCES _git.objects(hash),
    parent_hashes BYTEA[],            -- Array of parent commits
    author VARCHAR(255),
    author_time TIMESTAMP,
    committer VARCHAR(255),
    commit_time TIMESTAMP,
    message TEXT
);

-- Schema-to-Git mapping
CREATE TABLE _git.schema_objects (
    schema_name VARCHAR(64),
    object_name VARCHAR(128),
    object_type VARCHAR(32),          -- table, view, index, function, etc.
    current_blob_hash BYTEA REFERENCES _git.objects(hash),
    created_commit BYTEA REFERENCES _git.commits(hash),
    modified_commit BYTEA REFERENCES _git.commits(hash),
    PRIMARY KEY (schema_name, object_name, object_type)
);

-- Branches with isolation
CREATE TABLE _git.branches (
    branch_name VARCHAR(128) PRIMARY KEY,
    head_commit BYTEA REFERENCES _git.commits(hash),
    is_active BOOLEAN DEFAULT false,
    created_at TIMESTAMP,
    created_by VARCHAR(255),
    isolation_level VARCHAR(32)       -- schema, database, transaction
);
```

## Wire Protocol

### Connection Establishment

```protobuf
// Protocol Buffers definition
syntax = "proto3";

package scratchbird.git.v1;

service GitService {
    // Repository operations
    rpc Clone(CloneRequest) returns (CloneResponse);
    rpc Fetch(FetchRequest) returns (stream FetchResponse);
    rpc Push(stream PushRequest) returns (PushResponse);
    
    // Reference operations
    rpc ListRefs(ListRefsRequest) returns (ListRefsResponse);
    rpc UpdateRef(UpdateRefRequest) returns (UpdateRefResponse);
    
    // Object operations
    rpc GetObject(GetObjectRequest) returns (GetObjectResponse);
    rpc PutObject(stream PutObjectRequest) returns (PutObjectResponse);
    rpc HasObject(HasObjectRequest) returns (HasObjectResponse);
    
    // Schema operations
    rpc CheckoutBranch(CheckoutRequest) returns (CheckoutResponse);
    rpc GetSchemaDiff(DiffRequest) returns (DiffResponse);
    rpc ApplyMigration(ApplyMigrationRequest) returns (ApplyMigrationResponse);
    
    // Real-time sync
    rpc SubscribeToChanges(SubscribeRequest) returns (stream ChangeEvent);
}

message CloneRequest {
    string repository_url = 1;
    string branch = 2;
    AuthCredentials credentials = 3;
}

message AuthCredentials {
    oneof method {
        TLSClientCert tls_cert = 1;
        TokenAuth token = 2;
        PasswordAuth password = 3;
    }
}

message TLSClientCert {
    bytes certificate = 1;
    bytes private_key = 2;
}
```

### Schema Diff Format

```json
{
  "diff_version": "1.0",
  "from_commit": "a1b2c3d4...",
  "to_commit": "e5f6a7b8...",
  "from_branch": "main",
  "to_branch": "feature/portal",
  
  "objects_added": [
    {
      "schema": "public",
      "name": "portal_sessions",
      "type": "table",
      "ddl": "CREATE TABLE portal_sessions (...)"
    }
  ],
  
  "objects_modified": [
    {
      "schema": "public",
      "name": "users",
      "type": "table",
      "diff_type": "column_added",
      "column": {
        "name": "portal_access",
        "type": "BOOLEAN",
        "default": "false"
      },
      "ddl_before": "CREATE TABLE users (id INT, ...)",
      "ddl_after": "CREATE TABLE users (id INT, ..., portal_access BOOLEAN)",
      "migration_sql": "ALTER TABLE users ADD COLUMN portal_access BOOLEAN DEFAULT false",
      "rollback_sql": "ALTER TABLE users DROP COLUMN portal_access"
    }
  ],
  
  "objects_deleted": [
    {
      "schema": "public",
      "name": "legacy_orders",
      "type": "table",
      "ddl": "CREATE TABLE legacy_orders (...)",
      "rollback_sql": "CREATE TABLE legacy_orders (...)"  -- Full recreate DDL
    }
  ],
  
  "objects_unchanged": [
    {
      "schema": "public",
      "name": "products",
      "type": "table"
    }
  ],
  
  "dependency_changes": [
    {
      "object": "public.orders_view",
      "change": "dependency_added",
      "depends_on": "public.portal_sessions"
    }
  ],
  
  "metadata": {
    "total_changes": 3,
    "estimated_execution_time_ms": 1500,
    "requires_lock": true,
    "can_rollback": true
  }
}
```

## Branching Model

### Schema Isolation Levels

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      BRANCH ISOLATION LEVELS                                │
└─────────────────────────────────────────────────────────────────────────────┘

1. TRANSACTION ISOLATION (Lightweight)
═══════════════════════════════════════════════════════════════════════════════

   Shared Schema State
   ┌─────────────────────────────────────────────────────────────────────┐
   │  ┌─────────┐  ┌─────────┐  ┌─────────┐                             │
   │  │ Table A │  │ Table B │  │ Table C │                             │
   │  └─────────┘  └─────────┘  └─────────┘                             │
   └─────────────────────────────────────────────────────────────────────┘
                           ▲
                           │
          ┌────────────────┼────────────────┐
          │                │                │
      Branch 1         Branch 2         Branch 3
   ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
   │ View of     │  │ View of     │  │ View of     │
   │ uncommitted │  │ uncommitted │  │ uncommitted │
   │ changes     │  │ changes     │  │ changes     │
   └─────────────┘  └─────────────┘  └─────────────┘
   
   • All branches see same physical schema
   • Changes isolated by transaction
   • No DDL isolation
   • Best for: Short-lived feature branches
   • Limitation: Cannot modify same object concurrently


2. SCHEMA ISOLATION (Default)
═══════════════════════════════════════════════════════════════════════════════

   Branch 1 Schema          Branch 2 Schema          Main Schema
   ┌─────────────┐          ┌─────────────┐          ┌─────────────┐
   │ _git_b1.    │          │ _git_b2.    │          │ public.     │
   │   Table A   │          │   Table A   │          │   Table A   │
   │ _git_b1.    │          │ _git_b2.    │          │ public.     │
   │   Table B   │          │   Table B   │          │   Table B   │
   └─────────────┘          └─────────────┘          └─────────────┘
   
   • Each branch gets schema namespace
   • Full DDL isolation
   • Can modify same objects independently
   • Sync via merge
   • Best for: Long-running features


3. DATABASE ISOLATION (Full)
═══════════════════════════════════════════════════════════════════════════════

   ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
   │ Database:       │  │ Database:       │  │ Database:       │
   │   branch_1      │  │   branch_2      │  │   main          │
   │                 │  │                 │  │                 │
   │ ┌─────────────┐ │  │ ┌─────────────┐ │  │ ┌─────────────┐ │
   │ │ Table A     │ │  │ │ Table A     │ │  │ │ Table A     │ │
   │ │ Table B     │ │  │ │ Table B     │ │  │ │ Table B     │ │
   │ └─────────────┘ │  │ └─────────────┘ │  │ └─────────────┘ │
   └─────────────────┘  └─────────────────┘  └─────────────────┘
   
   • Each branch = separate database
   • Complete isolation
   • Cross-database queries possible
   • Resource intensive
   • Best for: Complex refactoring, performance testing
```

### Branch Operations

```sql
-- Create new branch from current
SELECT git.checkout_branch('feature/add-portal', 'main');

-- List branches
SELECT * FROM git.list_branches();

 branch_name        | created_at          | head_commit | is_current
--------------------+---------------------+-------------+------------
 main               | 2024-01-01 00:00:00 | a1b2c3d4    | false
 develop            | 2024-01-15 10:30:00 | b2c3d4e5    | false
 feature/add-portal | 2024-02-04 09:15:00 | c3d4e5f6    | true
(3 rows)

-- Switch branch
SELECT git.checkout_branch('main');

-- Create commit
SELECT git.commit(
    message := 'Add portal user table',
    author := 'Alice <alice@company.com>',
    objects := ARRAY['schema/public/portal_users.table.sql']
);

-- Merge branch
SELECT git.merge(
    source_branch := 'feature/add-portal',
    target_branch := 'develop',
    strategy := 'recursive',
    commit_message := 'Merge portal feature'
);
```

## Migration Generation

### Automatic Migration Scripts

```python
# Migration generator logic

class MigrationGenerator:
    def generate_migration(self, from_commit, to_commit):
        diff = self.get_schema_diff(from_commit, to_commit)
        
        migration = Migration()
        migration.version = self.get_next_version()
        migration.dependencies = [from_commit]
        
        steps = []
        
        # Phase 1: Pre-deployment (can run without downtime)
        for obj in diff.objects_added:
            if obj.type in ['table', 'index', 'view']:
                steps.append(MigrationStep(
                    phase='pre_deploy',
                    sql=obj.ddl,
                    rollback_sql=obj.get_drop_sql(),
                    description=f"Create {obj.name}"
                ))
        
        # Phase 2: Deployment (may require locks)
        for obj in diff.objects_modified:
            if obj.is_column_addition:
                steps.append(MigrationStep(
                    phase='deploy',
                    sql=f"ALTER TABLE {obj.schema}.{obj.table} ADD COLUMN {obj.column_def}",
                    rollback_sql=f"ALTER TABLE {obj.schema}.{obj.table} DROP COLUMN {obj.column_name}",
                    description=f"Add column {obj.column_name} to {obj.table}"
                ))
            elif obj.is_column_type_change:
                # Complex: create new column, migrate data, drop old
                steps.extend(self.generate_column_type_change_steps(obj))
        
        # Phase 3: Post-deployment (cleanup)
        for obj in diff.objects_deleted:
            steps.append(MigrationStep(
                phase='post_deploy',
                sql=obj.get_drop_sql(),
                rollback_sql=obj.ddl,  -- Full recreate
                description=f"Drop {obj.name}"
            ))
        
        # Validate migration
        self.validate_migration(steps)
        
        return MigrationScript(
            steps=steps,
            version=migration.version,
            estimated_duration=self.estimate_duration(steps),
            requires_downtime=self.check_requires_downtime(steps)
        )
```

### Migration Script Example

```sql
-- Migration: 0042_add_customer_portal.sql
-- Generated: 2024-02-04T15:30:00Z
-- From: a1b2c3d4... (v1.1.0)
-- To: e5f6a7b8... (v1.2.0)

BEGIN;

-- Step 1: Pre-deployment (no locks)
-- Create new tables
CREATE TABLE public.portal_sessions (
    session_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id INTEGER NOT NULL REFERENCES public.users(user_id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL,
    last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ip_address INET,
    user_agent TEXT
);

CREATE INDEX idx_portal_sessions_user 
ON public.portal_sessions(user_id) 
WHERE expires_at > CURRENT_TIMESTAMP;

COMMENT ON TABLE public.portal_sessions IS 'Active user portal sessions';

-- Step 2: Deployment (brief locks)
-- Add column to existing table
ALTER TABLE public.users 
ADD COLUMN portal_enabled BOOLEAN NOT NULL DEFAULT false;

COMMENT ON COLUMN public.users.portal_enabled IS 'Whether user can access portal';

-- Step 3: Post-deployment
-- Create function
CREATE OR REPLACE FUNCTION public.validate_session_token(
    p_token UUID
) RETURNS TABLE (
    user_id INTEGER,
    is_valid BOOLEAN,
    expires_at TIMESTAMP
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        ps.user_id,
        (ps.expires_at > CURRENT_TIMESTAMP) AS is_valid,
        ps.expires_at
    FROM public.portal_sessions ps
    WHERE ps.session_id = p_token;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

-- Update migration tracking
INSERT INTO _git.applied_migrations (
    version, 
    applied_at, 
    from_commit, 
    to_commit,
    applied_by
) VALUES (
    '0042',
    CURRENT_TIMESTAMP,
    'a1b2c3d4',
    'e5f6a7b8',
    CURRENT_USER
);

COMMIT;

-- Rollback script: 0042_add_customer_portal.rollback.sql
BEGIN;
DROP FUNCTION IF EXISTS public.validate_session_token(UUID);
ALTER TABLE public.users DROP COLUMN IF EXISTS portal_enabled;
DROP TABLE IF EXISTS public.portal_sessions CASCADE;
DELETE FROM _git.applied_migrations WHERE version = '0042';
COMMIT;
```

## Integration with ScratchRobin

### Sync Protocol

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    SCRATCHROBIN ↔ SCRATCHBIRD SYNC                          │
└─────────────────────────────────────────────────────────────────────────────┘

1. DETECT CHANGES
═══════════════════════════════════════════════════════════════════════════════

   Project Side:                          Database Side:
   ────────────                           ─────────────
   Watch: designs/*.json                  Watch: Schema catalog
   
   Hash: file contents                    Hash: object DDL
   
   Change detected?                       Change detected?
   └──► Compute diff                       └──► Get commit diff


2. RESOLVE DIRECTION
═══════════════════════════════════════════════════════════════════════════════

   Last sync state:
   - Project commit: P1
   - Database commit: D1
   
   Current state:
   - Project commit: P2 (changed)
   - Database commit: D1 (unchanged)
   
   Direction: Project → Database
   
   OR
   
   Current state:
   - Project commit: P1 (unchanged)
   - Database commit: D2 (changed)
   
   Direction: Database → Project
   
   OR
   
   Current state:
   - Project commit: P2 (changed)
   - Database commit: D2 (changed)
   
   Direction: CONFLICT - requires manual resolution


3. TRANSFORM
═══════════════════════════════════════════════════════════════════════════════

   Project → Database:
   ─────────────────
   designs/users.table.json
        │
        ├──► Parse JSON structure
        ├──► Generate DDL
        ├──► Generate migration SQL
        └──► Stage in database repo
   
   Database → Project:
   ─────────────────
   schema/public.users.table.sql
        │
        ├──► Parse DDL
        ├──► Extract metadata
        ├──► Generate JSON design
        └──► Write to designs/


4. VALIDATE
═══════════════════════════════════════════════════════════════════════════════

   Pre-sync checks:
   □ Syntax validation
   □ Dependency resolution
   □ Naming conventions
   □ Reserved word check
   □ Type compatibility
   
   Post-sync checks:
   □ Schema applies cleanly
   □ Tests pass
   □ No constraint violations


5. COMMIT
═══════════════════════════════════════════════════════════════════════════════

   Atomic transaction across both repos:
   
   BEGIN TRANSACTION;
   
   COMMIT project_repo WITH
       message: 'Sync to database',
       db_ref: 'd4c3b2a1';
   
   COMMIT database_repo WITH
       message: 'Sync from project',
       project_ref: 'a1b2c3d4';
   
   UPDATE sync_state SET
       project_commit = 'a1b2c3d4',
       database_commit = 'd4c3b2a1',
       synced_at = NOW();
   
   COMMIT;
   
   (Either both succeed, or both rollback)
```

## Security

### Authentication

```yaml
# Connection configuration
auth:
  # TLS Client Certificate (recommended)
  tls:
    enabled: true
    cert_file: /path/to/client.crt
    key_file: /path/to/client.key
    ca_file: /path/to/ca.crt
    
  # JWT Token (for service accounts)
  jwt:
    enabled: false
    token: ${GIT_TOKEN}
    
  # Kerberos (enterprise)
  kerberos:
    enabled: false
    service_name: git/scratchbird.cluster.local
```

### Authorization

```sql
-- Permission matrix
GRANT git_read ON DATABASE ecommerce TO developer;
GRANT git_write ON DATABASE ecommerce TO senior_developer;
GRANT git_admin ON DATABASE ecommerce TO dba;

-- Branch-level permissions
GRANT branch_write ON BRANCH 'feature/*' TO developer;
GRANT branch_merge ON BRANCH 'develop' TO senior_developer;
GRANT branch_merge ON BRANCH 'main' TO dba;

-- Object-level permissions
GRANT object_modify ON SCHEMA public TO developer;
REVOKE object_modify ON TABLE public.sensitive_data FROM developer;
```

## API Endpoints

### REST API

```
GET    /api/v1/git/status                    # Repository status
GET    /api/v1/git/log                       # Commit history
GET    /api/v1/git/branches                  # List branches
POST   /api/v1/git/branches                  # Create branch
PUT    /api/v1/git/branches/{name}/checkout  # Checkout branch
DELETE /api/v1/git/branches/{name}           # Delete branch

GET    /api/v1/git/diff                      # Schema diff
GET    /api/v1/git/objects/{hash}            # Get object
POST   /api/v1/git/commits                   # Create commit
POST   /api/v1/git/merge                     # Merge branches

GET    /api/v1/git/migrations                # List migrations
POST   /api/v1/git/migrations/apply          # Apply migration
POST   /api/v1/git/migrations/rollback       # Rollback migration

GET    /api/v1/git/snapshot                  # Export snapshot
POST   /api/v1/git/snapshot                  # Import snapshot
```

## CLI Reference

```bash
# Repository operations
sb-git clone scratchbird://cluster/dbname --branch=main
sb-git status
sb-git log --oneline --graph

# Branch operations
sb-git branch feature/new-table
sb-git checkout feature/new-table
sb-git branch -d feature/old-table

# Schema operations
sb-git diff main..feature/new-table
sb-git show HEAD:schema/public/users.table.sql
sb-git checkout main -- schema/public/users.table.sql

# Commit operations
sb-git add schema/public/new_table.table.sql
sb-git commit -m "Add new table"
sb-git push origin feature/new-table

# Migration operations
sb-git migrate generate v1.0.0..v1.1.0
sb-git migrate apply 0042
sb-git migrate rollback 0042

# Sync with ScratchRobin project
sb-git sync project /path/to/project --bidirectional
```

## Performance Considerations

### Optimization Strategies

1. **Lazy Loading**: Load schema objects on demand
2. **Delta Compression**: Store only differences between versions
3. **Caching**: Cache frequently accessed objects
4. **Partitioning**: Partition git tables by time
5. **Parallel Apply**: Apply independent migrations in parallel

### Benchmarks

```
Operation                    Latency     Throughput
─────────────────────────────────────────────────────────
Clone (1000 objects)         2.5s        400 obj/s
Commit (1 object)            50ms        20 commits/s
Diff (100 objects changed)   150ms       - 
Checkout branch              100ms       -
Apply migration              500ms       2 migrations/s
```
