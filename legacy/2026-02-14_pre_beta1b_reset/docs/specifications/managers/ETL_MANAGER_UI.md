# ETL Manager UI Specification

**Status**: Beta Placeholder  
**Target Release**: Beta  
**Priority**: P2

---

## Overview

The ETL (Extract, Transform, Load) Manager provides a comprehensive visual environment for designing, scheduling, and monitoring data integration workflows between databases and external data sources.

---

## Planned Features

### 1. Visual Job Designer
- Drag-and-drop job design canvas
- Component palette with data sources, transforms, and targets
- Connection routing between components
- Job validation and error highlighting
- Zoom and pan controls

### 2. Data Source Connectors

#### Database Sources
- PostgreSQL
- MySQL
- Firebird
- ScratchBird
- SQL Server (future)
- Oracle (future)

#### File Sources
- CSV (with delimiter/encoding options)
- JSON (flat and nested)
- XML
- Excel (.xlsx, .xls)
- Parquet
- Fixed-width text

#### API Sources
- REST APIs (JSON/XML)
- GraphQL endpoints
- Message queues (RabbitMQ, Kafka, SQS)
- Webhooks

#### Stream Sources
- Change Data Capture (CDC)
- Database triggers
- Event streams

### 3. Transformations

#### Column Operations
- Map - Direct column mapping
- Rename - Column renaming
- Cast - Type conversion
- Default - Set default values
- Calculated - Formula/expression columns

#### Row Operations
- Filter - Row filtering (WHERE conditions)
- Sort - Multi-column sorting
- Deduplicate - Remove duplicates
- Sample - Random sampling
- Limit - Row limiting

#### Data Cleansing
- Trim - Whitespace removal
- Normalize - Value standardization
- Validate - Data validation rules
- Replace - Find and replace
- Regex Replace - Pattern-based replacement
- Null Handling - NULL strategies

#### Advanced
- Aggregate - Group by with aggregates
- Pivot/Unpivot - Pivot tables
- Lookup - Reference data joins
- Custom SQL - SQL transformations
- Script - Python/JavaScript transforms
- Split - Multi-output routing

### 4. Data Quality
- Rule-based validation
- Schema validation
- Statistical profiling
- Data cleansing workflows
- Quality scorecards
- Anomaly detection

### 5. Workflow Orchestration
- Multi-job pipelines
- Dependency management
- Conditional branching
- Error handling and retry
- Parallel execution
- Schedule-based triggers

### 6. Monitoring & Scheduling
- Job execution history
- Real-time progress tracking
- Performance metrics
- Error logs and diagnostics
- Cron-based scheduling
- Event-driven triggers

---

## UI Components

### Job Designer Layout
```
+----------------------------------------------------------+
|  ETL Job Designer - [Job Name]                           |
+----------------------------------------------------------+
|  [Save] [Validate] [Run] [Schedule] [Export]            |
+----------------------------------------------------------+
|  +----------+  +--------------------------------------+  |
|  | Palette  |  | Canvas                               |  |
|  |          |  |                                      |  |
|  | Sources  |  |   +---------+      +-----------+    |  |
|  | - DB     |  |   | Source  |----->| Transform |    |  |
|  | - File   |  |   +---------+      +-----+-----+    |  |
|  | - API    |  |                          |          |  |
|  |          |  |                    +-----v-----+    |  |
|  | Transforms|  |                    |  Target   |    |  |
|  | - Column |  |                    +-----------+    |  |
|  | - Row    |  |                                      |  |
|  | - Clean  |  |                                      |  |
|  |          |  |                                      |  |
|  | Targets  |  |                                      |  |
|  +----------+  +--------------------------------------+  |
|  +--------------------------------------------------+  |
|  | Properties Panel (context-sensitive)             |  |
|  +--------------------------------------------------+  |
```

### Job List View
- Job name, description, schedule
- Last run status and duration
- Next scheduled run
- Owner and tags
- Quick actions (run, edit, disable)

### Run History
- Execution timestamp
- Duration and row counts
- Success/failure status
- Error details
- Performance metrics

---

## Data Models

See `src/core/etl_model.h`:

- `EtlJob` - Job definition
- `EtlJobRun` - Execution instance
- `DataSource` - Source configuration
- `DataTarget` - Target configuration
- `TransformRule` - Transformation definition
- `DataQualityRule` - Quality check rules
- `EtlWorkflow` - Multi-job pipeline

---

## Supported Transformations

| Category | Transform | Status |
|----------|-----------|--------|
| Column | Map | Planned |
| Column | Rename | Planned |
| Column | Cast | Planned |
| Column | Calculated | Planned |
| Row | Filter | Planned |
| Row | Sort | Planned |
| Row | Deduplicate | Planned |
| Cleanse | Trim | Planned |
| Cleanse | Normalize | Planned |
| Cleanse | Validate | Planned |
| Advanced | Aggregate | Planned |
| Advanced | Pivot | Future |
| Advanced | Script | Future |

---

## Implementation Status

- [x] Data models defined (`etl_model.h`)
- [x] Stub UI implemented (`etl_manager_frame.cpp`)
- [ ] Visual designer canvas
- [ ] Component palette
- [ ] Data source connectors
- [ ] Transform engine
- [ ] Job scheduler
- [ ] Monitoring dashboard

---

## Future Enhancements

- Machine learning-based data quality
- Auto-mapping suggestions
- Data lineage visualization
- Impact analysis
- Version control for jobs
- Team collaboration features
- Cloud data warehouse connectors
- Real-time streaming pipelines
