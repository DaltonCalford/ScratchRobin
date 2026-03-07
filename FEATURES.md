# ScratchRobin Features

Complete feature documentation for ScratchRobin Database Administration Tool.

## Table of Contents

1. [Core Features](#core-features)
2. [Database Object Management](#database-object-management)
3. [Version Control Integration](#version-control-integration)
4. [Reporting and Analytics](#reporting-and-analytics)
5. [Dashboard Builder](#dashboard-builder)
6. [Docking System](#docking-system)
7. [Backend Integration](#backend-integration)

---

## Core Features

### SQL Editor
- Multi-tab SQL editor with syntax highlighting
- Code completion for SQL keywords and schema objects
- Query execution with result grid display
- Find/Replace functionality
- SQL formatting and case conversion
- Query history and favorites

### Connection Management
- Multiple simultaneous database connections
- Connection profiles with saved credentials
- SSL/TLS encryption support
- Connection pooling and monitoring

### Transaction Control
- Manual transaction management (BEGIN, COMMIT, ROLLBACK)
- Savepoint support
- Auto-commit toggle
- Isolation level configuration
- Read-only transaction mode

---

## Database Object Management

### View Manager
The View Manager provides comprehensive view administration:

**Features:**
- Browse all views in the database
- Create new views with visual query builder
- Edit existing view definitions
- View data preview with pagination
- Show view dependencies
- Materialized view support
- Export/Import view definitions

**Menu Access:** `View → Database Managers → View Manager`  
**Shortcut:** `Ctrl+Shift+V`

**DDL Support:**
```sql
-- Create View
CREATE VIEW schema.view_name AS SELECT ...;

-- Create Materialized View
CREATE MATERIALIZED VIEW schema.view_name AS SELECT ...;

-- Alter View
CREATE OR REPLACE VIEW schema.view_name AS SELECT ...;

-- Drop View
DROP VIEW schema.view_name [CASCADE];
```

### Trigger Manager
Complete trigger lifecycle management:

**Features:**
- List all triggers with status (enabled/disabled)
- Create triggers with visual designer
- Edit trigger definitions
- Enable/Disable triggers
- View trigger execution history
- Dependency tracking

**Menu Access:** `View → Database Managers → Trigger Manager`  
**Shortcut:** `Ctrl+Shift+T`

**DDL Support:**
```sql
-- Create Trigger
CREATE TRIGGER name
{BEFORE | AFTER | INSTEAD OF} {INSERT | UPDATE | DELETE}
ON table_name
FOR EACH ROW
BEGIN
  -- trigger logic
END;

-- Enable/Disable Trigger
{ENABLE | DISABLE} TRIGGER schema.trigger_name;

-- Drop Trigger
DROP TRIGGER schema.trigger_name;
```

### Table Designer
Visual table design and modification:

**Features:**
- Create tables with column definitions
- Edit existing table structures
- Add/Remove/Modify columns
- Index management
- Constraint definition
- Preview generated DDL

**Menu Access:** `View → Database Managers → Table Designer`

### User Manager
Complete user, role, and group administration:

**Features:**
- Browse users, roles, and groups
- Create/Edit/Delete security principals
- Role hierarchy and membership management
- Authentication method configuration
- Password policy management
- Default privileges per schema

**Menu Access:** `View → Database Managers → User/Role Manager`  
**Shortcut:** `Ctrl+Shift+U`

**DDL Support:**
```sql
-- Create User
CREATE USER username WITH PASSWORD 'password' [OPTIONS];

-- Create Role
CREATE ROLE rolename [WITH LOGIN] [INHERIT];
GRANT rolename TO other_role;

-- Create Group
CREATE GROUP groupname WITH MEMBERS user1, user2;
```

### Privilege Manager
Security administration for GRANT/REVOKE operations:

**Features:**
- Visual privilege browser
- Grant privileges to users, roles, or PUBLIC
- WITH GRANT OPTION support
- Column-level privileges
- Effective permissions calculation (role inheritance)
- Revoke with CASCADE/RESTRICT

**Menu Access:** `View → Database Managers → Privilege Manager`

**DDL Support:**
```sql
-- Grant privileges
GRANT {SELECT|INSERT|UPDATE|DELETE|ALL} 
  ON [TABLE|VIEW|SEQUENCE|FUNCTION] object 
  TO {user|role|PUBLIC} [WITH GRANT OPTION];

-- Revoke privileges
REVOKE [GRANT OPTION FOR] privilege 
  ON object 
  FROM {user|role|PUBLIC} [CASCADE|RESTRICT];
```

### Procedure Manager
Function and stored procedure administration:

**Features:**
- Browse functions, procedures, and UDRs
- Create/Edit/Delete procedures
- Parameter management with data types
- Return type specification
- Language selection (SQL, PLpgSQL, C, Java)
- DETERMINISTIC option
- External library support for UDRs
- Source code editing with syntax highlighting

**Menu Access:** `View → Database Managers → Procedure Manager`

**DDL Support:**
```sql
-- Create Function
CREATE [OR REPLACE] FUNCTION schema.func_name(param1 type1, ...)
RETURNS return_type
[LANGUAGE {SQL|PLpgSQL|C|JAVA}]
[DETERMINISTIC]
AS $$
  -- function body
$$;

-- Create Procedure
CREATE PROCEDURE schema.proc_name(param1 type1, ...)
AS $$
  -- procedure body
$$;

-- Drop Function
DROP FUNCTION [IF EXISTS] schema.func_name;
```

### Sequence Manager
Database sequence management and monitoring:

**Features:**
- Browse all sequences with current value
- Create/Edit/Delete sequences
- Set sequence parameters (start, increment, min, max, cache)
- CYCLE/NO CYCLE control
- View next values (with safeguards)
- Set/Advance sequence values
- Ownership tracking (OWNED BY)
- Reset sequence to initial value

**Menu Access:** `View → Database Managers → Sequence Manager`

**DDL Support:**
```sql
-- Create Sequence
CREATE SEQUENCE schema.sequence_name
  [START WITH value]
  [INCREMENT BY value]
  [MINVALUE value | NO MINVALUE]
  [MAXVALUE value | NO MAXVALUE]
  [CACHE cache_size]
  [CYCLE | NO CYCLE]
  [OWNED BY {table.column|NONE}];

-- Alter Sequence
ALTER SEQUENCE schema.sequence_name
  [RESTART [WITH value]]
  [INCREMENT BY value]
  ...;

-- Drop Sequence
DROP SEQUENCE [IF EXISTS] schema.sequence_name [CASCADE];
```

### Package Manager
PL/SQL-style package administration for modular code organization:

**Features:**
- Browse packages with specification and body status
- Create/Edit/Drop packages
- Separate spec and body editors
- Member browsing (constants, types, functions, procedures)
- Compile and validate packages
- Dependency tracking for packages
- AUTHID CURRENT_USER/DEFINER support

**Menu Access:** `View → Database Managers → Package Manager`

**DDL Support:**
```sql
-- Create Package Specification
CREATE [OR REPLACE] PACKAGE [schema.]package_name
[AUTHID {CURRENT_USER | DEFINER}] IS
  -- Public declarations
  CONSTANT constant_name datatype := value;
  TYPE type_name IS ...;
  FUNCTION function_name(param type) RETURN type;
  PROCEDURE procedure_name(param type);
END package_name;

-- Create Package Body
CREATE [OR REPLACE] PACKAGE BODY [schema.]package_name IS
  -- Private declarations
  -- Function and procedure implementations
END package_name;

-- Compile Package
ALTER PACKAGE schema.package_name COMPILE;

-- Drop Package
DROP PACKAGE [BODY] schema.package_name;
```

### Import Data Wizard
4-step wizard for importing data from various formats:

**Features:**
- Step 1: Select source file and format (CSV, TSV, JSON, Excel, SQL)
- Step 2: Preview data and map columns
- Step 3: Configure import options (delimiter, encoding, target table)
- Step 4: Execute import with progress tracking
- Auto-detect file format
- Column type auto-detection
- Support for large files (streaming)
- Error logging and handling

**Menu Access:** `Tools → Import Data`

**Supported Formats:**
- CSV (Comma Separated Values)
- TSV (Tab Separated Values)
- JSON
- Excel (.xlsx)
- SQL INSERT statements

### Bulk Data Loader
High-performance data loading for large datasets:

**Features:**
- Multi-threaded loading for maximum throughput
- Configurable batch sizes
- Transaction control (commit intervals)
- Progress tracking with ETA
- Resume capability for failed loads
- Error handling with skip threshold
- Validation options
- Export load reports

**Menu Access:** `Tools → Bulk Load`

**Performance Features:**
- Parallel loading threads (1-16)
- Batch insert optimization
- Prepared statement reuse
- Connection pooling

### Data Compare/Sync
Table comparison and synchronization tool:

**Features:**
- Compare tables within same or different databases
- Row-level difference detection
- Key column selection for matching
- Ignore options (case, whitespace, NULLs)
- Visual comparison results
- Generate synchronization scripts
- Bidirectional sync support
- Export comparison reports

**Menu Access:** `Tools → Data Compare`

**Comparison Options:**
- Compare all columns or selected columns
- Custom key column mapping
- Type-aware comparison
- Timestamp tolerance

### Synonym Manager
Oracle-style synonym administration for object aliasing:

**Features:**
- Browse public and private synonyms
- Create/Edit/Drop synonyms
- Target object browser
- Validation of synonym targets
- DB Link support for remote objects
- Status tracking (valid/invalid)

**Menu Access:** `View → Database Managers → Synonym Manager`

**DDL Support:**
```sql
-- Create Private Synonym
CREATE [OR REPLACE] SYNONYM [schema.]synonym_name
  FOR [schema.]object_name[@dblink];

-- Create Public Synonym
CREATE [OR REPLACE] PUBLIC SYNONYM synonym_name
  FOR [schema.]object_name[@dblink];

-- Drop Synonym
DROP SYNONYM [schema.]synonym_name;

-- Drop Public Synonym
DROP PUBLIC SYNONYM synonym_name;
```

### Partition Manager
Table partitioning administration for large datasets:

**Features:**
- Browse partitioned tables and partitions
- Create partitioned tables (Range, List, Hash)
- Add/Drop partitions
- Split/Merge partitions
- Attach/Detach partitions
- Partition statistics and row counts
- Analyze partitions

**Menu Access:** `View → Database Managers → Partition Manager`

**DDL Support:**
```sql
-- Create Partitioned Table
CREATE TABLE schema.table_name (...)
PARTITION BY RANGE (column_name);

-- Add Partition
CREATE TABLE partition_name PARTITION OF parent_table
FOR VALUES FROM (start) TO (end);

-- Split Partition
ALTER TABLE parent_table SPLIT PARTITION partition_name
INTO (PARTITION p1 ..., PARTITION p2 ...);

-- Detach Partition
ALTER TABLE parent_table DETACH PARTITION partition_name;

-- Drop Partition
DROP TABLE partition_name;
```

### Foreign Data Wrapper Manager
External data source administration:

**Features:**
- Manage Foreign Data Wrappers (FDW)
- Create/configure foreign servers
- Manage user mappings
- Create foreign tables
- Import foreign schemas
- Test connections

**Menu Access:** `View → Database Managers → FDW Manager`

**DDL Support:**
```sql
-- Create Foreign Data Wrapper
CREATE FOREIGN DATA WRAPPER fdw_name HANDLER handler_func;

-- Create Foreign Server
CREATE SERVER server_name FOREIGN DATA WRAPPER fdw_name
OPTIONS (host '...', port '...', dbname '...');

-- Create User Mapping
CREATE USER MAPPING FOR local_user SERVER server_name
OPTIONS (user 'remote_user', password '...');

-- Create Foreign Table
CREATE FOREIGN TABLE table_name (...) SERVER server_name
OPTIONS (table_name 'remote_table');

-- Import Foreign Schema
IMPORT FOREIGN SCHEMA remote_schema FROM SERVER server INTO local_schema;
```

### Extension Manager
Database extension/module administration:

**Features:**
- Browse available/installed extensions
- Install/Enable extensions
- Uninstall extensions
- Update extensions
- Configure extension settings
- View extension documentation

**Menu Access:** `View → Database Managers → Extension Manager`

**DDL Support:**
```sql
-- Install Extension
CREATE EXTENSION extension_name [WITH SCHEMA schema] [VERSION version];

-- Update Extension
ALTER EXTENSION extension_name UPDATE TO new_version;

-- Drop Extension
DROP EXTENSION extension_name [CASCADE];
```

### Full-Text Search (FTS) Configuration Manager
Text search configuration administration:

**Features:**
- Manage FTS configurations
- Create FTS dictionaries
- Configure text search parsers
- Alter configuration mappings
- Test FTS configurations
- Dictionary templates

**Menu Access:** `View → Database Managers → FTS Configuration`

**DDL Support:**
```sql
-- Create FTS Configuration
CREATE TEXT SEARCH CONFIGURATION config_name (PARSER = parser_name);

-- Create FTS Dictionary
CREATE TEXT SEARCH DICTIONARY dict_name (
  TEMPLATE = template,
  OPTION = value
);

-- Alter FTS Configuration
ALTER TEXT SEARCH CONFIGURATION config_name
ADD MAPPING FOR token_type WITH dictionary;
```

### Replication Manager
Database replication administration:

**Features:**
- View replication status
- Create/manage replication slots
- Create/manage publications (logical replication)
- Create/manage subscriptions
- Monitor replication lag
- Perform failover operations
- View replication statistics

**Menu Access:** `View → Administration → Replication Manager`

**DDL Support:**
```sql
-- Create Replication Slot
SELECT * FROM pg_create_physical_replication_slot('slot_name');

-- Create Publication
CREATE PUBLICATION pub_name FOR TABLE table1, table2
WITH (publish = 'insert, update, delete');

-- Create Subscription
CREATE SUBSCRIPTION sub_name CONNECTION 'conninfo' PUBLICATION pub_name;

-- Enable/Disable Subscription
ALTER SUBSCRIPTION sub_name ENABLE/DISABLE;
```

### Connection Pool Monitor
Monitor database connection pool statistics:

**Features:**
- View pool status (active/idle connections)
- Monitor wait queue
- Connection usage statistics
- Auto-refresh with configurable interval
- Disconnect idle connections
- Configure pool settings
- Query statistics per connection

**Menu Access:** `View → Monitoring → Connection Pool`

### Lock Manager
Database lock monitoring and management:

**Features:**
- View current locks in real-time
- Detect blocking and waiting locks
- Kill blocking sessions
- Analyze lock wait chains
- Filter by database/lock type
- Export lock reports
- Auto-refresh monitoring

**Menu Access:** `View → Monitoring → Lock Manager`

### Session Manager
Active database session management:

**Features:**
- View all active sessions
- Filter by user/database/state
- Kill sessions
- Cancel queries
- View session details and history
- Session statistics
- Auto-refresh monitoring
- Kill all sessions for a user

**Menu Access:** `View → Monitoring → Session Manager`

### Dependency Tracking
Object dependency analysis and impact assessment:

**Features:**
- View object dependencies (what it depends on)
- View object dependents (what depends on it)
- Hard vs Soft dependency classification
- Impact analysis before DROP operations
- Cascade dependency visualization
- Circular dependency detection
- Dependency export to DOT/PNG

**Requirements:** See `docs/specifications/DEPENDENCY_TRACKING_REQUIREMENTS.md`

**Status:** Pending engine capability verification

---

## Version Control Integration

### Git Integration
Native Git support for SQL script versioning:

**Features:**
- Repository browser
- Commit with staging area
- Branch management (create, merge, delete)
- Push/Pull/Fetch operations
- Diff viewer with syntax highlighting
- File blame annotation
- Commit history graph
- Stash management

**Menu Access:** `View → Git Integration`  
**Shortcut:** `Ctrl+Shift+G`

**Supported Operations:**
- `git init` - Initialize repository
- `git clone` - Clone remote repository
- `git commit` - Commit changes
- `git branch` - Branch management
- `git merge` - Merge branches
- `git push/pull` - Remote synchronization
- `git diff` - View changes
- `git stash` - Stash changes

---

## Reporting and Analytics

### Report Manager
Create and manage database reports:

**Features:**
- Visual report designer
- SQL-based report queries
- Parameter binding
- Multiple export formats (PDF, Excel, HTML, CSV)
- Report scheduling
- Report templates
- Preview before export

**Menu Access:** `View → Reporting → Report Manager`

**Export Formats:**
- PDF - Portable Document Format
- Excel (.xlsx) - Microsoft Excel
- HTML - Web format with styling
- CSV - Comma-separated values

### Chart Builder
Visual data visualization:

**Chart Types:**
- Bar Chart
- Line Chart
- Pie Chart
- Area Chart
- Scatter Plot
- Donut Chart
- Radar Chart
- Heat Map

**Features:**
- Drag-and-drop chart configuration
- Data source selection
- Axis configuration
- Series management
- Real-time preview
- Chart styling options

---

## Dashboard Builder

### Interactive Dashboards
Create custom database dashboards:

**Widget Types:**
- **Chart Widget** - Bar, Line, Pie, Area charts
- **Table Widget** - Data grid with sorting/filtering
- **KPI Widget** - Key performance indicators with trends
- **Gauge Widget** - Circular/radial gauges
- **Text Widget** - Rich text and markdown
- **Filter Widget** - Interactive filter controls

**Features:**
- Drag-and-drop designer canvas
- Widget resizing and positioning
- Grid-based alignment
- Real-time data refresh
- Global filter application
- Auto-refresh intervals
- Full-screen mode
- Export to PDF/PNG

**Menu Access:** `View → Reporting → Dashboard Manager`

### Dashboard Viewer
Runtime dashboard viewer:

**Features:**
- Interactive widgets
- Drill-down capabilities
- Filter controls
- Auto-refresh
- Export options
- Print support

---

## Docking System

### DockWorkspace Architecture
Advanced docking system for panel management:

**Features:**
- Tabbed docking areas (Left, Right, Top, Bottom, Center)
- Floating panels
- Panel grouping
- Auto-hide panels
- Layout persistence
- Multiple layout profiles

**Dock Areas:**
- **Left** - Project Navigator, View Manager, Trigger Manager
- **Right** - Git Integration, Report Manager, Dashboard Manager, Properties
- **Bottom** - Query Results, Messages, Execution Log
- **Center** - SQL Editor tabs

**Layout Management:**
- Save current layout
- Load saved layouts
- Reset to default
- Layout import/export

### Registered Panels

| Panel ID | Title | Category | Default Area |
|----------|-------|----------|--------------|
| project_navigator | Project Navigator | core | Left |
| query_results | Query Results | core | Bottom |
| view_manager | View Manager | database | Left |
| trigger_manager | Trigger Manager | database | Left |
| user_role_manager | User Manager | database | Left |
| privilege_manager | Privilege Manager | database | Right |
| procedure_manager | Procedure Manager | database | Left |
| sequence_manager | Sequence Manager | database | Left |
| package_manager | Package Manager | database | Left |
| synonym_manager | Synonym Manager | database | Left |
| import_wizard | Import Data Wizard | tools | Dialog |
| bulk_data_loader | Bulk Data Loader | tools | Right |
| data_compare_sync | Data Compare/Sync | tools | Right |
| partition_manager | Partition Manager | database | Left |
| fdw_manager | Foreign Data Wrappers | database | Left |
| extension_manager | Extension Manager | database | Left |
| fts_manager | FTS Configuration | database | Left |
| git_integration | Git Integration | version | Right |
| report_manager | Report Manager | reporting | Right |
| dashboard_manager | Dashboard Manager | reporting | Right |
| replication_manager | Replication Manager | administration | Right |
| connection_pool_monitor | Connection Pool Monitor | monitoring | Right |
| lock_manager | Lock Manager | monitoring | Bottom |
| session_manager | Session Manager | monitoring | Bottom |

---

## Backend Integration

### ScratchBird Metadata Provider
Direct integration with ScratchBird database engine:

**Features:**
- Catalog queries via `information_schema`
- DDL generation via `SHOW CREATE` commands
- Real-time metadata refresh
- Dependency tracking
- Statistics gathering

**Supported Operations:**

#### Schema Operations
```cpp
QStringList schemas();                    // List schemas
QString currentSchema();                  // Get current
bool setSchema(const QString& schema);    // Change schema
```

#### Table Operations
```cpp
QList<TableMetadata> tables();            // List tables
TableMetadata table(name);                // Get metadata
QString tableDdl(name);                   // Generate DDL
bool createTable(metadata, columns);      // Create table
bool alterTable(name, alterStmt);         // Alter table
bool dropTable(name, cascade);            // Drop table
```

#### View Operations
```cpp
QList<ViewMetadata> views();              // List views
ViewMetadata view(name);                  // Get metadata
QString viewDdl(name);                    // Generate DDL
bool createView(view);                    // Create view
bool alterView(name, newDefinition);      // Alter view
bool refreshMaterializedView(name);       // Refresh MV
```

#### Trigger Operations
```cpp
QList<TriggerMetadata> triggers();        // List triggers
TriggerMetadata trigger(name);            // Get metadata
QString triggerDdl(name);                 // Generate DDL
bool createTrigger(trigger);              // Create trigger
bool alterTrigger(name, enable);          // Enable/Disable
```

#### Index Operations
```cpp
QList<IndexMetadata> indexes();           // List indexes
bool createIndex(index);                  // Create index
bool dropIndex(name);                     // Drop index
bool rebuildIndex(name);                  // Rebuild index
```

### DDL Generation

The backend provides comprehensive DDL generation:

**Table DDL:**
```sql
CREATE TABLE schema.table_name (
  column_name data_type [NOT NULL] [DEFAULT value] [COMMENT 'text'],
  ...
);
```

**View DDL:**
```sql
CREATE [MATERIALIZED] VIEW schema.view_name AS
SELECT ...
[WITH {LOCAL | CASCADED} CHECK OPTION];
```

**Trigger DDL:**
```sql
CREATE TRIGGER name
{BEFORE | AFTER | INSTEAD OF} {INSERT | UPDATE | DELETE}
ON table_name
[FOR EACH ROW]
BEGIN
  -- trigger logic
END;
```

**Index DDL:**
```sql
CREATE [UNIQUE] INDEX name ON schema.table (column1, column2, ...);
```

### Validation

The backend validates SQL before execution:

```cpp
bool validateViewDefinition(sql, &errorMessage);
bool validateTriggerDefinition(sql, &errorMessage);
bool objectExists(type, name, schema);
```

---

## Keyboard Shortcuts

### General
| Shortcut | Action |
|----------|--------|
| Ctrl+N | New Connection |
| Ctrl+O | Open SQL Script |
| Ctrl+S | Save |
| F5 | Execute Query |
| Ctrl+Shift+V | View Manager |
| Ctrl+Shift+T | Trigger Manager |
| Ctrl+Shift+G | Git Integration |

### Query Editor
| Shortcut | Action |
|----------|--------|
| F5 | Execute |
| Ctrl+Enter | Execute Selection |
| Ctrl+F | Find |
| Ctrl+H | Find and Replace |
| Ctrl+/ | Toggle Comment |

---

## Configuration

Settings are stored using QSettings in:
- Windows: `HKEY_CURRENT_USER\Software\ScratchRobin`
- macOS: `~/Library/Preferences/com.scratchbird.ScratchRobin.plist`
- Linux: `~/.config/ScratchRobin.conf`

### Layout Persistence
Layouts are saved under the `Layouts` group with:
- Panel positions and sizes
- Floating window states
- Active layout name

---

## Development

### Building Tests
```bash
cd build
make test_ddl_generation
make test_ui_components
```

### Running Tests
```bash
./tests/test_ddl_generation
./tests/test_ui_components
```

### Adding New Panels

To add a new dockable panel:

```cpp
// 1. Create panel class inheriting from DockPanel
class MyPanel : public DockPanel {
    // Implementation
};

// 2. Register with DockWorkspace
DockPanelInfo info;
info.id = "my_panel";
info.title = "My Panel";
info.category = "tools";
info.defaultArea = DockAreaType::Right;
dockWorkspace->registerPanel(info, myPanel);

// 3. Add menu action
viewMenu->addAction("My Panel", this, &MainWindow::onViewMyPanel);
```

---

## License

ScratchRobin is licensed under the MIT License.

## Support

For support and documentation, visit:
- Website: https://scratchbird.dev
- Documentation: https://scratchbird.dev/docs
- Issues: https://github.com/scratchbird/scratchrobin/issues
