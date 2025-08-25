# MariaDB Support for ScratchRobin

This document outlines the comprehensive MariaDB support that has been implemented in ScratchRobin, enabling it to serve as a complete management and design tool for MariaDB databases.

## Overview

ScratchRobin now includes full MariaDB support with the following capabilities:

- **Complete Database Connectivity**: Connect to MariaDB databases with full authentication and SSL support
- **Comprehensive Feature Detection**: Automatic detection of MariaDB-specific features and capabilities
- **Full SQL Syntax Support**: MariaDB-specific syntax highlighting, validation, and parsing
- **System Catalog Access**: Complete access to MariaDB system tables, views, and metadata
- **Advanced Connection Management**: Robust connection handling with MariaDB-specific options
- **Query Analysis and Optimization**: MariaDB-specific query analysis and performance suggestions

## Architecture

The MariaDB support is implemented through several key components:

### Core Components

1. **mariadb_features.h/cpp** - MariaDB-specific data types, features, and capabilities
2. **mariadb_catalog.h/cpp** - System catalog queries and metadata discovery
3. **mariadb_connection.h/cpp** - Connection management and validation
4. **mariadb_syntax.h/cpp** - SQL syntax parsing, validation, and analysis
5. **database_driver_manager.cpp** - Updated with MariaDB driver support

## Features Implemented

### 1. Database Connectivity

- **MySQL Driver Integration**: Uses Qt's QMYSQL driver for MariaDB connectivity
- **Multiple Authentication Methods**:
  - Standard MySQL/MariaDB authentication
  - SSL/TLS encryption with certificate validation
  - Unix socket connections (Linux/Unix)
  - Named pipe connections (Windows)
- **Connection Options**:
  - Character set and collation configuration
  - Compression support
  - Connection pooling
  - Auto-reconnect functionality
  - Custom initialization commands

### 2. Data Types Support

**Standard SQL Types:**
- Numeric: tinyint, smallint, mediumint, int, integer, bigint, decimal, dec, numeric, float, double, real, bit, serial, bigserial
- String: char, varchar, tinytext, text, mediumtext, longtext, binary, varbinary, tinyblob, blob, mediumblob, longblob, enum, set
- Date/Time: date, datetime, timestamp, time, year
- Other: cursor, sql_variant, table, uniqueidentifier

**MariaDB-Specific Types:**
- Spatial: geometry, point, linestring, polygon, multipoint, multilinestring, multipolygon, geometrycollection
- JSON: json (MariaDB 10.2+)
- Dynamic Columns: dynamic

### 3. SQL Feature Support

**Core SQL Features:**
- CTEs (Common Table Expressions) with recursion (MariaDB 10.2+)
- Window functions (OVER, PARTITION BY, ORDER BY) (MariaDB 10.2+)
- Sequences (MariaDB 10.3+)
- Virtual/Persistent columns (MariaDB 5.2+)
- Dynamic columns (MariaDB 5.5+)
- Table partitioning (MariaDB 5.1+)

**Advanced Features:**
- JSON functions (MariaDB 10.2+)
- Spatial data types and functions
- Full-text search integration
- GTID (Global Transaction IDs) support
- Multi-source replication
- Storage engine selection (InnoDB, MyISAM, Aria, etc.)

### 4. System Catalog Access

**System Databases:**
- information_schema, mysql, performance_schema, sys

**System Views:**
- INFORMATION_SCHEMA.TABLES, COLUMNS, VIEWS, ROUTINES, TRIGGERS, KEY_COLUMN_USAGE
- PERFORMANCE_SCHEMA views for monitoring and diagnostics
- MySQL system tables (user, db, tables_priv, etc.)

**Metadata Discovery:**
- Complete table, column, and constraint information
- Index and foreign key details
- Stored procedure and function definitions
- Trigger information
- Event scheduler details
- Storage engine information

### 5. SQL Syntax Support

**Syntax Parsing:**
- Complete MariaDB keyword recognition (200+ keywords)
- Data type validation
- Function and operator recognition
- Identifier parsing and validation

**Query Analysis:**
- Performance issue detection
- Best practice recommendations
- Index usage analysis
- Security vulnerability detection
- MariaDB-specific optimization suggestions

**Code Formatting:**
- MariaDB-specific indentation and formatting
- Keyword case normalization
- Comment preservation

### 6. Connection Management

**Connection Parameters:**
- Host, port, database, username, password
- SSL/TLS encryption configuration
- Character set and collation
- Connection timeouts and options
- Local socket and named pipe support

**Connection Testing:**
- Basic connectivity verification
- Database access validation
- Permission testing
- Feature capability detection
- Performance benchmarking

## Supported MariaDB Versions

- MariaDB 5.1, 5.2, 5.3, 5.5
- MariaDB 10.0, 10.1, 10.2, 10.3, 10.4, 10.5, 10.6, 10.7, 10.8, 10.9, 10.10, 10.11, 11.0, 11.1, 11.2, 11.3, 11.4
- Percona Server (compatible)
- MySQL 5.6, 5.7, 8.0 (compatible mode)

## Key MariaDB Features Supported

### Storage Engines
- **InnoDB**: Default transactional engine with full ACID compliance
- **MyISAM**: High-performance non-transactional engine
- **Aria**: Crash-safe MyISAM replacement
- **XtraDB**: Enhanced InnoDB (Percona)
- **RocksDB**: LSM-based engine for analytical workloads
- **TokuDB**: Fractal tree-based engine
- **Spider**: Sharding and partitioning engine
- **CONNECT**: Data federation engine

### Advanced Features
- **Virtual Columns**: Computed columns with persistent storage options
- **Dynamic Columns**: Flexible column storage and retrieval
- **Sequences**: Standard-compliant sequence objects
- **Window Functions**: Advanced analytical functions
- **JSON Support**: Native JSON data type and functions
- **Spatial Data**: Geographic and geometric data types
- **Full-Text Search**: Natural language and boolean search

### Performance & Monitoring
- **Performance Schema**: Detailed performance monitoring
- **Information Schema**: Comprehensive metadata access
- **Query Optimization**: Index suggestions and query analysis
- **Connection Monitoring**: Active connection tracking
- **Resource Usage**: Memory, CPU, and I/O monitoring

## Usage

### Basic Connection

```cpp
// Create connection parameters
MariaDBConnectionParameters params;
params.host = "localhost";
params.port = 3306;
params.database = "mydb";
params.username = "myuser";
params.password = "mypassword";
params.charset = "utf8mb4";
params.collation = "utf8mb4_general_ci";

// Test connection
QString errorMessage;
bool connected = MariaDBConnectionTester::testBasicConnection(params, errorMessage);
```

### Feature Detection

```cpp
// Detect server capabilities
QStringList supportedFeatures;
QString errorMessage;
bool success = MariaDBConnectionTester::testServerFeatures(params, supportedFeatures, errorMessage);

if (success) {
    for (const QString& feature : supportedFeatures) {
        qDebug() << "Supported feature:" << feature;
    }
}
```

### SQL Analysis

```cpp
// Analyze query for issues and suggestions
QString sql = "SELECT * FROM users WHERE created_at > '2023-01-01'";
QStringList issues, suggestions;
MariaDBQueryAnalyzer::analyzeQuery(sql, issues, suggestions);

for (const QString& issue : issues) {
    qDebug() << "Issue:" << issue;
}
for (const QString& suggestion : suggestions) {
    qDebug() << "Suggestion:" << suggestion;
}
```

### System Catalog Access

```cpp
// Get database information
QStringList databases = MariaDBCatalog::getUserDatabasesQuery();

// Get table information
QString query = MariaDBCatalog::getTablesQuery("", "mydb");

// Get performance metrics
QString perfQuery = MariaDBCatalog::getPerformanceCountersQuery();
```

## Configuration

### Connection String Examples

**Basic Connection:**
```
host=localhost;port=3306;database=mydb;user=myuser;password=mypass;charset=utf8mb4
```

**SSL Connection:**
```
host=localhost;port=3306;database=mydb;user=myuser;password=mypass;ssl=1;ssl_ca=/path/to/ca.pem
```

**Unix Socket Connection:**
```
unix_socket=/var/run/mysqld/mysqld.sock;database=mydb;user=myuser;password=mypass
```

### Performance Tuning

```cpp
// Enable performance monitoring
MariaDBConnectionParameters params;
params.host = "localhost";
params.database = "performance_schema";

// Get performance metrics
QStringList metrics = MariaDBCatalog::getPerformanceCountersQuery();
```

## Security Features

### Authentication
- Standard MariaDB authentication
- SSL/TLS encryption support
- Certificate-based authentication
- PAM integration (MariaDB-specific)

### Access Control
- User privilege management
- Role-based access control
- SSL connection requirements
- Password policies and expiration

### Audit Features
- General query log monitoring
- Binary log analysis
- Performance schema security monitoring
- Connection attempt logging

## Performance Features

### Query Optimization
- Automatic detection of performance issues
- Index usage analysis and recommendations
- Query complexity estimation
- Best practice enforcement

### Connection Pooling
- Configurable pool sizes
- Connection health monitoring
- Automatic cleanup and recovery
- Load balancing support

### Monitoring & Diagnostics
- Real-time performance monitoring
- Query execution statistics
- Resource usage tracking
- Lock and deadlock detection
- Slow query analysis

## Storage Engine Optimization

### InnoDB Optimization
- Buffer pool configuration
- Redo log optimization
- Index optimization
- Transaction isolation level tuning

### MyISAM Optimization
- Key buffer configuration
- Table optimization
- Repair and check operations
- Index statistics maintenance

### Aria Optimization
- Page cache configuration
- Transaction log configuration
- Crash recovery settings

## Replication Support

### Master-Slave Replication
- Binary log monitoring
- Slave status tracking
- Replication lag monitoring
- Failover detection

### Multi-Source Replication
- Multiple master configuration
- Conflict resolution
- Parallel replication

### Galera Cluster Support
- Cluster status monitoring
- Node synchronization tracking
- Automatic node provisioning

## Backup and Recovery

### Physical Backups
- Mariabackup integration
- XtraBackup compatibility
- Point-in-time recovery

### Logical Backups
- mysqldump integration
- Schema-only backups
- Data-only backups

### Binary Log Backup
- Binary log file management
- Point-in-time recovery
- Incremental backup support

## Troubleshooting

### Common Connection Issues
1. **Authentication Failed**: Verify credentials and SSL settings
2. **Connection Refused**: Check server status and firewall settings
3. **SSL Connection Failed**: Verify certificate files and paths
4. **Access Denied**: Check user privileges and host restrictions

### Performance Issues
1. **Slow Queries**: Use query analyzer for optimization suggestions
2. **High Memory Usage**: Monitor buffer pool and connection settings
3. **Lock Contention**: Analyze lock waits and deadlocks
4. **Disk I/O Bottlenecks**: Check storage engine configuration

### Replication Issues
1. **Replication Lag**: Monitor slave status and network latency
2. **Binary Log Errors**: Check binary log configuration
3. **Master-Slave Sync**: Verify GTID and server IDs

## Best Practices

### Schema Design
- Use appropriate storage engines for each table
- Implement proper indexing strategies
- Use virtual columns for computed values
- Leverage JSON data type for flexible schemas

### Query Optimization
- Use EXPLAIN to analyze query execution plans
- Implement proper indexing
- Avoid SELECT * in production
- Use prepared statements for security

### Security
- Enable SSL for production environments
- Use strong passwords and password policies
- Implement proper access controls
- Regularly audit user privileges

### Performance Monitoring
- Enable Performance Schema for detailed monitoring
- Monitor slow query logs
- Track connection usage and resource consumption
- Implement regular maintenance tasks

## Conclusion

The MariaDB support implementation provides ScratchRobin with comprehensive capabilities for managing and designing MariaDB databases. The modular architecture allows for easy maintenance and future enhancements while providing robust functionality for database professionals.

This implementation enables ScratchRobin to serve as a complete MariaDB management tool, rivaling commercial database tools in functionality while maintaining the flexibility and customization advantages of an open-source solution.

Key benefits include:
- Full compatibility with MariaDB-specific features
- Comprehensive performance monitoring and optimization
- Advanced security and access control management
- Complete backup and recovery support
- Replication monitoring and management
- Storage engine optimization and selection
