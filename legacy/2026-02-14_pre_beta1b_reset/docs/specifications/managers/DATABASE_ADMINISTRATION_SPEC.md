# ScratchRobin Database Administration Specification

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Scope**: Database lifecycle management - Install, Configure, Optimize, Alter, Remove

---

## Table of Contents

1. [Overview](#overview)
2. [Database Installation](#database-installation)
3. [Database Configuration](#database-configuration)
4. [Database Optimization](#database-optimization)
5. [Database Alteration](#database-alteration)
6. [Database Removal](#database-removal)
7. [UI Requirements](#ui-requirements)
8. [Implementation Details](#implementation-details)

---

## Overview

This specification defines the database administration capabilities for ScratchRobin, covering the complete lifecycle of database management from initial creation through removal.

### Supported Database Types

| Type | Connection Modes | Administration Level |
|------|------------------|---------------------|
| **ScratchBird** | Embedded, IPC, Network | Full (Create, Alter, Drop, Configure) |
| **PostgreSQL** | Network | External (Connect, Configure, Drop) |
| **MySQL/MariaDB** | Network | External (Connect, Configure, Drop) |
| **Firebird** | Network | External (Connect, Configure, Drop) |

### Connection Mode Matrix

| Mode | Use Case | Performance | Requirements |
|------|----------|-------------|--------------|
| **Embedded** | Single-user, development, edge deployment | Fastest (no IPC) | ScratchBird library linked |
| **IPC** | Local multi-user, production on same host | Fast (shared memory) | ScratchBird server running |
| **Network** | Remote access, distributed systems | Standard TCP | Network connectivity |

---

## Database Installation

### 1.1 Create Database (ScratchBird)

#### SQL Interface
```sql
-- Create database with default settings
CREATE DATABASE 'mydb.sbd';

-- Create with specific page size
CREATE DATABASE 'mydb.sbd' PAGE_SIZE = 16384;

-- Create with character set
CREATE DATABASE 'mydb.sbd' DEFAULT CHARACTER SET UTF8;

-- Create with owner
CREATE DATABASE 'mydb.sbd' OWNER 'admin';
```

#### Programmatic Interface
```cpp
struct CreateDatabaseOptions {
    std::string path;                    // Database file path
    uint32_t pageSize = 8192;           // Page size (4096, 8192, 16384, 32768)
    std::string characterSet = "UTF8";   // Default character set
    std::string owner;                   // Database owner username
    std::string password;                // Owner password
    bool overwrite = false;              // Allow overwrite existing
    
    // Advanced options
    uint32_t initialPages = 100;        // Initial allocation
    uint32_t maxConnections = 100;      // Max concurrent connections
    bool enableEncryption = false;      // Enable at-rest encryption
    std::string encryptionKey;          // Encryption key (if enabled)
};

Status CreateDatabase(const CreateDatabaseOptions& options, ErrorContext* ctx);
```

#### UI: Create Database Dialog

```
+----------------------------------------------------------+
| Create New Database                                        |
+----------------------------------------------------------+
|                                                            |
| Connection Mode:                                           |
| (*) Embedded    ( ) IPC    ( ) Network                   |
|                                                            |
| Database File:                                             |
| [/home/user/databases/] [mydb.sbd]        [Browse...]    |
|                                                            |
| Page Size: [8192 v]   Charset: [UTF8 v]                  |
|                                                            |
| Initial Size: [100] pages    Max Connections: [100]      |
|                                                            |
| Owner: [admin]          Password: [********]             |
|                                                            |
| [x] Enable Encryption    [Configure...]                  |
|                                                            |
| [Advanced Options...]                                    |
|                                                            |
|              [Cancel]           [Create]                 |
+----------------------------------------------------------+
```

### 1.2 Clone/Copy Database

```sql
-- Create copy of existing database
CREATE DATABASE 'newdb.sbd' FROM 'existingdb.sbd';

-- Copy with data
CREATE DATABASE 'newdb.sbd' FROM 'existingdb.sbd' WITH DATA;

-- Copy schema only
CREATE DATABASE 'newdb.sbd' FROM 'existingdb.sbd' SCHEMA ONLY;
```

### 1.3 Database Templates

Predefined templates for common use cases:

| Template | Description | Default Settings |
|----------|-------------|------------------|
| `development` | Local development | Small cache, verbose logging |
| `production` | Production server | Large cache, WAL enabled |
| `embedded` | Edge/IoT deployment | Minimal footprint, single-user |
| `analytics` | Data warehouse | Columnar storage, large pages |
| `oltp` | Transaction processing | Row storage, high concurrency |

---

## Database Configuration

### 2.1 Server Configuration

#### Configuration File (`sb_server.conf`)

```ini
# ScratchBird Server Configuration

[database]
page_size = 8192
default_charset = UTF8
max_connections = 100
shared_buffers = 256MB
temp_buffers = 8MB
work_mem = 4MB

[wal]
wal_level = replica
wal_buffers = 16MB
max_wal_size = 1GB
min_wal_size = 80MB
checkpoint_timeout = 5min

[logging]
log_destination = stderr
log_statement = mod
log_duration = on
log_min_duration_statement = 1000

[network]
listen_addresses = localhost
port = 3092
unix_socket_directories = /var/run/scratchbird
ssl = on
ssl_cert_file = server.crt
ssl_key_file = server.key

[performance]
effective_cache_size = 1GB
random_page_cost = 1.1
effective_io_concurrency = 200
max_worker_processes = 8
```

#### Runtime Configuration

```sql
-- View current settings
SHOW ALL;
SHOW shared_buffers;
SHOW max_connections;

-- Change runtime setting (session)
SET work_mem = '64MB';
SET LOCAL work_mem = '64MB';  -- transaction-local

-- Change setting for database
ALTER DATABASE mydb SET work_mem = '32MB';

-- Change system default
ALTER SYSTEM SET shared_buffers = '512MB';
SELECT sb_reload_conf();  -- Apply changes
```

### 2.2 Connection Pool Configuration

```cpp
struct PoolConfig {
    uint32_t minConnections = 5;
    uint32_t maxConnections = 50;
    uint32_t connectionTimeoutMs = 30000;
    uint32_t idleTimeoutMs = 600000;    // 10 minutes
    uint32_t maxLifetimeMs = 1800000;   // 30 minutes
    bool testOnBorrow = true;
    bool testOnReturn = false;
    std::string validationQuery = "SELECT 1";
};
```

### 2.3 User and Security Configuration

```sql
-- Create user
CREATE USER analyst WITH PASSWORD 'secure123';

-- Grant privileges
GRANT CONNECT ON DATABASE mydb TO analyst;
GRANT USAGE ON SCHEMA public TO analyst;
GRANT SELECT ON ALL TABLES IN SCHEMA public TO analyst;

-- Create role
CREATE ROLE readonly;
GRANT SELECT ON ALL TABLES IN SCHEMA public TO readonly;
GRANT readonly TO analyst;

-- Set password policy
ALTER USER admin WITH PASSWORD_POLICY 'strong' VALID UNTIL '2026-12-31';
```

### 2.4 Backup Configuration

```sql
-- Configure automated backups
SELECT sb_configure_backup(
    database => 'mydb',
    schedule => '0 2 * * *',  -- Daily at 2 AM
    retention => '7 days',
    destination => '/backup/mydb/',
    compression => 'zstd',
    encryption => true
);

-- Manual backup
BACKUP DATABASE mydb TO '/backup/mydb-20260203.sbbak';

-- Incremental backup
BACKUP DATABASE mydb INCREMENTAL TO '/backup/mydb-incr.sbbak';
```

---

## Database Optimization

### 3.1 Statistics and Analysis

```sql
-- Update statistics for all tables
ANALYZE;

-- Update specific table
ANALYZE users;

-- Update with sampling
ANALYZE users WITH SAMPLE 10000 ROWS;

-- View table statistics
SELECT * FROM sb_table_stats WHERE table_name = 'users';
```

### 3.2 Index Maintenance

```sql
-- Reindex table
REINDEX TABLE users;

-- Reindex specific index
REINDEX INDEX idx_users_email;

-- Vacuum and analyze
VACUUM ANALYZE users;

-- Full vacuum (reclaims space)
VACUUM FULL users;
```

### 3.3 Query Performance

```cpp
// Query plan visualization
struct QueryPlan {
    std::string operation;
    double cost;
    double rows;
    std::string index;
    std::vector<QueryPlan> children;
};

// Get execution plan
EXPLAIN (ANALYZE, BUFFERS, FORMAT JSON)
SELECT * FROM users WHERE email = 'test@example.com';
```

#### UI: Query Plan Visualizer

```
+----------------------------------------------------------+
| Query Plan                                                |
+----------------------------------------------------------+
|                                                           |
| Hash Join (cost=100.50..250.75 rows=1000 width=128)      |
|   Hash Cond: (orders.user_id = users.id)                 |
|   Buffers: shared hit=50 read=100                        |
|   -> Seq Scan on orders (cost=0..100.00 rows=10000)      |
|        Filter: (status = 'completed')                    |
|        Buffers: shared read=80                           |
|   -> Hash (cost=80.00..80.00 rows=1000 width=64)         |
|        Buckets: 1024 Batches: 1                          |
|        -> Index Scan using idx_users_email on users      |
|             Index Cond: (email = 'test@example.com')     |
|             Buffers: shared hit=50                       |
|                                                           |
| [Visual] [Text] [JSON]  Execution Time: 2.4ms            |
+----------------------------------------------------------+
```

### 3.4 Storage Optimization

```sql
-- Table partitioning
CREATE TABLE events (
    id BIGINT,
    created_at TIMESTAMP,
    data JSONB
) PARTITION BY RANGE (created_at);

CREATE TABLE events_2026_01 PARTITION OF events
    FOR VALUES FROM ('2026-01-01') TO ('2026-02-01');

-- Compression
ALTER TABLE logs SET (compression = 'zstd');

-- Column storage for analytics
CREATE TABLE analytics_data (
    id BIGINT,
    metrics JSONB
) USING COLUMNAR;
```

### 3.5 Memory Tuning Guide

| Setting | Small (<4GB) | Medium (4-16GB) | Large (>16GB) |
|---------|--------------|-----------------|---------------|
| shared_buffers | 256MB | 1GB | 4GB |
| work_mem | 4MB | 16MB | 64MB |
| maintenance_work_mem | 64MB | 256MB | 1GB |
| effective_cache_size | 2GB | 8GB | 32GB |

---

## Database Alteration

### 4.1 Schema Changes

```sql
-- Rename database
ALTER DATABASE mydb RENAME TO production_db;

-- Change owner
ALTER DATABASE mydb OWNER TO new_admin;

-- Change default tablespace
ALTER DATABASE mydb SET TABLESPACE fast_ssd;

-- Change encoding (requires rebuild)
ALTER DATABASE mydb WITH ENCODING UTF8;

-- Enable/disable features
ALTER DATABASE mydb ENABLE FEATURE 'vector_search';
ALTER DATABASE mydb DISABLE FEATURE 'json_path';
```

### 4.2 Tablespace Management

```sql
-- Create tablespace
CREATE TABLESPACE fast_ssd
    LOCATION '/ssd/scratchbird/data'
    WITH (compression = 'lz4');

-- Move table to tablespace
ALTER TABLE large_table SET TABLESPACE fast_ssd;

-- Move index to tablespace
ALTER INDEX idx_large_table SET TABLESPACE fast_ssd;
```

### 4.3 Online Schema Changes

```sql
-- Add column (online)
ALTER TABLE users ADD COLUMN phone VARCHAR(20);

-- Add column with default (online with backfill)
ALTER TABLE users ADD COLUMN created_at TIMESTAMP DEFAULT NOW();

-- Rename column
ALTER TABLE users RENAME COLUMN phone TO phone_number;

-- Drop column (mark as dropped, reclaim later)
ALTER TABLE users DROP COLUMN temp_data;
```

### 4.4 Database Migration

```cpp
struct MigrationOptions {
    std::string sourceDatabase;
    std::string targetDatabase;
    bool online = true;           // Online migration with minimal downtime
    bool verifyData = true;       // Verify data integrity
    double maxDowntimeSeconds = 30;
    std::function<void(double)> progressCallback;
};

Status MigrateDatabase(const MigrationOptions& options);
```

#### UI: Migration Wizard

```
+----------------------------------------------------------+
| Database Migration Wizard                                |
+----------------------------------------------------------+
| Step 3 of 4: Migration Settings                          |
|                                                          |
| Source: mydb (ScratchBird Embedded)                      |
| Target: production (ScratchBird Network)                 |
|                                                          |
| Migration Mode:                                          |
| (*) Online (minimal downtime, ~30s)                      |
| ( ) Offline (faster, requires maintenance window)        |
|                                                          |
| [x] Verify data integrity after migration                |
| [x] Update connection strings automatically              |
| [ ] Create rollback point                                |
|                                                          |
| Estimated Downtime: 30 seconds                           |
| Estimated Duration: 15 minutes                           |
|                                                          |
| [<< Back]    [Cancel]    [Start Migration >>]            |
+----------------------------------------------------------+
```

---

## Database Removal

### 5.1 Drop Database

```sql
-- Drop database (with confirmation)
DROP DATABASE mydb;

-- Drop if exists (no error if missing)
DROP DATABASE IF EXISTS mydb;

-- Drop with force (disconnect active connections)
DROP DATABASE mydb WITH FORCE;
```

### 5.2 Archive Before Drop

```sql
-- Archive and drop
ARCHIVE DATABASE mydb TO '/archive/mydb-2026.sbbak' THEN DROP;

-- Archive with metadata
ARCHIVE DATABASE mydb TO '/archive/' 
    WITH METADATA (reason, archived_by, timestamp)
    THEN DROP;
```

### 5.3 Soft Delete

```sql
-- Mark database as deleted (recoverable)
ALTER DATABASE mydb SET STATE = 'DELETED';

-- View deleted databases
SELECT * FROM sb_databases WHERE state = 'DELETED';

-- Restore deleted database
ALTER DATABASE mydb SET STATE = 'ACTIVE';

-- Purge deleted databases after retention period
SELECT sb_purge_deleted_databases(older_than => '30 days');
```

---

## UI Requirements

### 6.1 Database Manager Frame

```
+----------------------------------------------------------+
| Database Manager                              [+ New...] |
+----------------------------------------------------------+
| +------------------+  +--------------------------------+ |
| | Databases        |  | Details: production             | |
| | + mydb           |  +--------------------------------+ |
| | + production *   |  | Connection: Network             | |
| | + analytics      |  | Host: db.company.com:3092       | |
| | + [deleted] old  |  | Version: Alpha 1.8.2            | |
| |                  |  | Size: 2.4 GB                    | |
| |                  |  | Tables: 156                     | |
| |                  |  | Indexes: 312                    | |
| |                  |  | Connections: 23 active          | |
| |                  |  |                                 | |
| |                  |  | [Connect] [Configure...]        | |
| |                  |  | [Backup] [Optimize...]          | |
| |                  |  | [Migrate...] [Drop...]          | |
| +------------------+  +--------------------------------+ |
+----------------------------------------------------------+
```

### 6.2 Connection Dialog with Mode Selection

```
+----------------------------------------------------------+
| Connection Properties                                    |
+----------------------------------------------------------+
|                                                          |
| Name: [Production Database                    ]          |
|                                                          |
| Connection Mode:                                         |
| +--+----------------+----------------+--------+          |
| |E | Network (TCP)   | IPC (Local)    |Embedded|         |
| |m |                 |                |        |         |
| |b | Host: [db.com  | Socket: [/var/ |Path:[~/|         |
| |e | Port: [3092  ]  |  run/sb.sock]  |db.sbd ]|         |
| |d |                 |                |        |         |
| |d | [Test Conn.]    | [Test Conn.]   |[Create]|         |
| |e |                 |                |        |         |
| +--+----------------+----------------+--------+          |
|                                                          |
| Authentication:                                          |
| Username: [admin        ]   Password: [********]         |
|                                                          |
| [Advanced...]                  [Cancel]    [Save]        |
+----------------------------------------------------------+
```

---

## Implementation Details

### 7.1 Files

| Component | Files |
|-----------|-------|
| Embedded Backend | `src/core/embedded_backend.h/cpp` |
| IPC Backend | `src/core/ipc_backend.h/cpp` |
| Network Backend | `src/core/network_backend.h/cpp` |
| Connection Manager | `src/core/connection_manager.h/cpp` |
| Database Manager UI | `src/ui/database_manager_frame.h/cpp` |

### 7.2 Key Classes

```cpp
// Connection mode enumeration
enum class ConnectionMode {
    Network,    // TCP/IP
    Ipc,        // Unix sockets / named pipes
    Embedded    // In-process
};

// Extended connection profile
struct ConnectionProfile {
    // ... existing fields ...
    ConnectionMode mode = ConnectionMode::Network;
    std::string ipcPath;
};

// Backend factory functions
std::unique_ptr<ConnectionBackend> CreateEmbeddedBackend();
std::unique_ptr<ConnectionBackend> CreateIpcBackend();
std::unique_ptr<ConnectionBackend> CreateNetworkBackend();
```

### 7.3 Build Configuration

```cmake
# CMakeLists.txt options
option(SCRATCHROBIN_USE_SCRATCHBIRD "Enable ScratchBird native support" ON)
option(SCRATCHROBIN_USE_POSTGRES "Enable PostgreSQL backend" ON)
option(SCRATCHROBIN_USE_MYSQL "Enable MySQL backend" ON)
option(SCRATCHROBIN_USE_FIREBIRD "Enable Firebird backend" OFF)

# Backend sources
target_sources(scratchrobin PRIVATE
    src/core/embedded_backend.cpp
    src/core/ipc_backend.cpp
    src/core/network_backend.cpp
    src/core/postgres_backend.cpp
    src/core/mysql_backend.cpp
    src/core/firebird_backend.cpp
)
```

### 7.4 Feature Availability Matrix

| Feature | Embedded | IPC | Network |
|---------|----------|-----|---------|
| Create Database | ✅ | ❌* | ❌* |
| Drop Database | ✅ | ❌* | ❌* |
| Backup/Restore | ✅ | ✅ | ✅ |
| Configure | ✅ | ✅ | ✅ |
| Optimize | ✅ | ✅ | ✅ |
| User Management | ✅ | ✅ | ✅ |
| Real-time Monitoring | ✅ | ✅ | ✅ |

*Server-side operations require appropriate privileges

---

## Future Enhancements

- **Replication Setup**: Master-slave configuration UI
- **Sharding Management**: Distributed database partitions
- **Cloud Integration**: AWS RDS, Azure Database, GCP Cloud SQL
- **Container Orchestration**: Kubernetes operator integration
- **AI-Powered Optimization**: Automatic index and tuning recommendations
