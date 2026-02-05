# Silverston Object Catalog (ScratchBird)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the canonical list of Silverston objects for ScratchBird, including infrastructure and database objects. This catalog is the authoritative reference for extraction into Silverston diagrams and reconstruction back into ScratchBird.

---

## 1. Catalog Principles

- All objects are treated as first‑class “database objects” if ScratchBird metadata can represent them.
- Objects are organized by functional group but share a common template schema.
- Every object type has:
  - Template name
  - Default icons
  - Allowed parents/children
  - Resolution tier visibility rules

---

## 2. Template Schema (Shared)

Each catalog entry uses the following structure:

```yaml
object_type:
  id: string
  display_name: string
  group: string
  icons:
    primary: string
    secondary: string | null
  spec_ref: [string]
  container:
    is_container: boolean
    grid_defaults: { columns: int, rows: int, gap: int }
    allowed_children: [string]
    allowed_parents: [string]
  display:
    overview: [name, icons]
    mid: [name, key_attributes]
    detail: [all_attributes, types, constraints]
```

### 2.1 Icon Key Definitions

The following icon keys are used across the catalog. Each maps to a single visual asset in the unified style system:

- `cluster` — Cluster / deployment group
- `node` — Node
- `shard` — Shard
- `replica` — Replica
- `listener` — Listener
- `parser` — Parser
- `pool` — Connection pool
- `storage` — Storage engine
- `filespace` — FileSpace
- `tablespace` — Tablespace
- `volume` — Volume
- `disk` — Disk
- `database` — Database
- `schema` — Schema
- `table` — Table
- `field` — Column/Field
- `datatype` — Data type marker
- `view` — View
- `materialized` — Materialized modifier
- `index` — Index
- `sequence` — Sequence
- `trigger` — Trigger
- `function` — Function
- `procedure` — Procedure
- `package` — Package
- `domain` — Domain
- `type` — Type
- `constraint` — Constraint
- `fk` — Foreign key
- `users` — Users container
- `user` — User
- `roles` — Roles container
- `role` — Role
- `groups` — Groups container
- `group` — Group
- `grant` — Grant/Privilege
- `job` — Job
- `etl` — ETL pipeline
- `backup` — Backup task
- `health` — Health check
- `project` — Project root
- `region` — Region/geo
- `site` — Site/campus
- `building` — Building/site
- `noc` — NOC/operations

### 2.2 ScratchBird Spec Reference Map

Each object is tied to its authoritative ScratchBird specification. References below point to the mirrored ScratchBird specs in this repo.\n

| Object ID | ScratchBird Spec Reference |
| --- | --- |
| cluster | `docs/specifications/scratchbird/specifications/Cluster Specification Work/SBCLUSTER-SUMMARY.md` |
| node | `docs/specifications/scratchbird/specifications/Cluster Specification Work/SBCLUSTER-02-MEMBERSHIP-AND-IDENTITY.md` |
| shard | `docs/specifications/scratchbird/specifications/Cluster Specification Work/SBCLUSTER-05-SHARDING.md` |
| replica | `docs/specifications/scratchbird/specifications/Cluster Specification Work/SBCLUSTER-07-REPLICATION.md` |
| listener | `docs/specifications/scratchbird/specifications/network/NETWORK_LISTENER_AND_PARSER_POOL_SPEC.md` |
| parser | `docs/specifications/scratchbird/specifications/parser/README.md` |
| connection_pool | `docs/specifications/scratchbird/specifications/api/CONNECTION_POOLING_SPECIFICATION.md` |
| storage_engine | `docs/specifications/scratchbird/specifications/storage/STORAGE_ENGINE_MAIN.md` |
| filespace | `docs/specifications/scratchbird/specifications/storage/ON_DISK_FORMAT.md` |
| tablespace | `docs/specifications/scratchbird/specifications/storage/TABLESPACE_SPECIFICATION.md` |
| volume | `docs/specifications/scratchbird/specifications/storage/STORAGE_ENGINE_MAIN.md` |
| disk | `docs/specifications/scratchbird/specifications/storage/STORAGE_ENGINE_PAGE_MANAGEMENT.md` |
| database | `docs/specifications/scratchbird/specifications/ddl/DDL_DATABASES.md` |
| schema | `docs/specifications/scratchbird/specifications/ddl/DDL_SCHEMAS.md` |
| table | `docs/specifications/scratchbird/specifications/ddl/DDL_TABLES.md` |
| column | `docs/specifications/scratchbird/specifications/ddl/DDL_TABLES.md`<br>`docs/specifications/scratchbird/specifications/types/03_TYPES_AND_DOMAINS.md` |
| view | `docs/specifications/scratchbird/specifications/ddl/DDL_VIEWS.md` |
| materialized_view | `docs/specifications/scratchbird/specifications/ddl/DDL_VIEWS.md` |
| index | `docs/specifications/scratchbird/specifications/ddl/DDL_INDEXES.md` |
| sequence | `docs/specifications/scratchbird/specifications/ddl/DDL_SEQUENCES.md` |
| trigger | `docs/specifications/scratchbird/specifications/ddl/DDL_TRIGGERS.md` |
| function | `docs/specifications/scratchbird/specifications/ddl/DDL_FUNCTIONS.md` |
| procedure | `docs/specifications/scratchbird/specifications/ddl/DDL_PROCEDURES.md` |
| package | `docs/specifications/scratchbird/specifications/ddl/DDL_PACKAGES.md` |
| domain | `docs/specifications/scratchbird/specifications/types/DDL_DOMAINS_COMPREHENSIVE.md` |
| type | `docs/specifications/scratchbird/specifications/ddl/DDL_TYPES.md`<br>`docs/specifications/scratchbird/specifications/types/03_TYPES_AND_DOMAINS.md` |
| constraint | `docs/specifications/scratchbird/specifications/ddl/DDL_TABLES.md` |
| foreign_key | `docs/specifications/scratchbird/specifications/ddl/DDL_TABLES.md` |
| users | `docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md` |
| user | `docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md` |
| roles | `docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md` |
| role | `docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md` |
| groups | `docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md` |
| group | `docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md` |
| grant | `docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md` |
| job | `docs/specifications/scratchbird/specifications/scheduler/SCHEDULER_JOB_RUNNER_CANONICAL_SPEC.md` |
| etl_pipeline | `docs/specifications/scratchbird/specifications/beta_requirements/big-data-streaming/README.md` |
| backup_task | `docs/specifications/scratchbird/specifications/BACKUP_AND_RESTORE.md` |
| health_check | `docs/specifications/scratchbird/specifications/operations/MONITORING_SQL_VIEWS.md` |
| project | `docs/specifications/core/PROJECT_PERSISTENCE_FORMAT.md` |
| region | `docs/specifications/scratchbird/specifications/Alpha Phase 2/08-Deployment-Guide.md` |
| site | `docs/specifications/scratchbird/specifications/Alpha Phase 2/08-Deployment-Guide.md` |
| building | `docs/specifications/scratchbird/specifications/Alpha Phase 2/08-Deployment-Guide.md` |
| noc | `docs/specifications/scratchbird/specifications/Alpha Phase 2/08-Deployment-Guide.md` |

---

## 3. Object Catalog (Explicit)

### Topology / Location

Topology objects are project-level containers. They represent deployment or organizational structure and are not database objects.

```yaml
- id: project
  display_name: Project
  group: Project Topology
  icons: { primary: project, secondary: null }
  spec_ref:
    - docs/specifications/core/PROJECT_PERSISTENCE_FORMAT.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 8 }
    allowed_children: [region, site, building, noc, cluster, database]
    allowed_parents: []
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: region
  display_name: Region
  group: Project Topology
  icons: { primary: region, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/Alpha Phase 2/08-Deployment-Guide.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 6 }
    allowed_children: [site, building, noc, cluster, database]
    allowed_parents: []
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: site
  display_name: Site
  group: Project Topology
  icons: { primary: site, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/Alpha Phase 2/08-Deployment-Guide.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 6 }
    allowed_children: [building, noc, cluster, database]
    allowed_parents: [region]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: building
  display_name: Building
  group: Project Topology
  icons: { primary: building, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/Alpha Phase 2/08-Deployment-Guide.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 6 }
    allowed_children: [noc, cluster, database]
    allowed_parents: [region, site]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: noc
  display_name: NOC
  group: Project Topology
  icons: { primary: noc, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/Alpha Phase 2/08-Deployment-Guide.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 6 }
    allowed_children: [cluster, database]
    allowed_parents: [region, site, building]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }
```

### Infrastructure

```yaml
- id: cluster
  display_name: Cluster
  group: Infrastructure
  icons: { primary: cluster, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/Cluster Specification Work/SBCLUSTER-SUMMARY.md
  container:
    is_container: true
    grid_defaults: { columns: 3, rows: 3, gap: 6 }
    allowed_children: [node, shard, replica, listener, parser, connection_pool]
    allowed_parents: [region, site, building, noc]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: node
  display_name: Node
  group: Infrastructure
  icons: { primary: node, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/Cluster Specification Work/SBCLUSTER-02-MEMBERSHIP-AND-IDENTITY.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [cluster]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: shard
  display_name: Shard
  group: Infrastructure
  icons: { primary: shard, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/Cluster Specification Work/SBCLUSTER-05-SHARDING.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [cluster]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: replica
  display_name: Replica
  group: Infrastructure
  icons: { primary: replica, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/Cluster Specification Work/SBCLUSTER-07-REPLICATION.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [cluster]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: listener
  display_name: Listener
  group: Infrastructure
  icons: { primary: listener, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/network/NETWORK_LISTENER_AND_PARSER_POOL_SPEC.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [cluster]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: parser
  display_name: Parser
  group: Infrastructure
  icons: { primary: parser, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/parser/README.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [cluster]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: connection_pool
  display_name: Connection Pool
  group: Infrastructure
  icons: { primary: pool, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/api/CONNECTION_POOLING_SPECIFICATION.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [cluster]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: storage_engine
  display_name: Storage Engine
  group: Infrastructure
  icons: { primary: storage, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/storage/STORAGE_ENGINE_MAIN.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 6 }
    allowed_children: [filespace, tablespace, volume, disk]
    allowed_parents: [database, cluster]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: filespace
  display_name: FileSpace
  group: Infrastructure
  icons: { primary: filespace, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/storage/ON_DISK_FORMAT.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 6 }
    allowed_children: [tablespace]
    allowed_parents: [storage_engine, database]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: tablespace
  display_name: Tablespace
  group: Infrastructure
  icons: { primary: tablespace, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/storage/TABLESPACE_SPECIFICATION.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [filespace, database]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: volume
  display_name: Volume
  group: Infrastructure
  icons: { primary: volume, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/storage/STORAGE_ENGINE_MAIN.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [storage_engine, filespace]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: disk
  display_name: Disk
  group: Infrastructure
  icons: { primary: disk, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/storage/STORAGE_ENGINE_PAGE_MANAGEMENT.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [storage_engine, filespace]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }
```

### Database Containers

```yaml
- id: database
  display_name: Database
  group: Database
  icons: { primary: database, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_DATABASES.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 3, gap: 6 }
    allowed_children: [schema, users, roles, groups, filespace, tablespace, trigger, procedure, function]
    allowed_parents: [cluster, region, site, building, noc]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }

- id: schema
  display_name: Schema
  group: Database
  icons: { primary: schema, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_SCHEMAS.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 6, gap: 4 }
    allowed_children: [table, view, materialized_view, index, sequence, trigger, procedure, function, package, domain, type, constraint]
    allowed_parents: [database]
  display: { overview: [name, icons], mid: [name], detail: [name, notes] }
```

### Data Objects

```yaml
- id: table
  display_name: Table
  group: Data Objects
  icons: { primary: table, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_TABLES.md
  container:
    is_container: true
    grid_defaults: { columns: 1, rows: 8, gap: 4 }
    allowed_children: [column, constraint, index, trigger]
    allowed_parents: [schema]
  display:
    overview: [name, icons]
    mid: [name, key_attributes]
    detail: [all_attributes, types, constraints]

- id: column
  display_name: Column
  group: Data Objects
  icons: { primary: field, secondary: datatype }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_TABLES.md
    - docs/specifications/scratchbird/specifications/types/03_TYPES_AND_DOMAINS.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [table, view]
  display: { overview: [name, icons], mid: [name], detail: [name, type, constraints] }

- id: view
  display_name: View
  group: Data Objects
  icons: { primary: view, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_VIEWS.md
  container:
    is_container: true
    grid_defaults: { columns: 1, rows: 6, gap: 4 }
    allowed_children: [column]
    allowed_parents: [schema]
  display: { overview: [name, icons], mid: [name, key_attributes], detail: [all_attributes, types] }

- id: materialized_view
  display_name: Materialized View
  group: Data Objects
  icons: { primary: view, secondary: materialized }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_VIEWS.md
  container:
    is_container: true
    grid_defaults: { columns: 1, rows: 6, gap: 4 }
    allowed_children: [column, index]
    allowed_parents: [schema]
  display: { overview: [name, icons], mid: [name, key_attributes], detail: [all_attributes, types] }

- id: index
  display_name: Index
  group: Data Objects
  icons: { primary: index, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_INDEXES.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [schema, table, view]
  display: { overview: [name, icons], mid: [name], detail: [name, definition] }

- id: sequence
  display_name: Sequence
  group: Data Objects
  icons: { primary: sequence, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_SEQUENCES.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [schema]
  display: { overview: [name, icons], mid: [name], detail: [name, options] }
```

### Programmability

```yaml
- id: trigger
  display_name: Trigger
  group: Programmability
  icons: { primary: trigger, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_TRIGGERS.md
  container:
    is_container: true
    grid_defaults: { columns: 1, rows: 4, gap: 4 }
    allowed_children: [entity, condition, action]
    allowed_parents: [cluster, database, table]
  display: { overview: [name, icons], mid: [name, summary], detail: [full_definition] }

- id: function
  display_name: Function
  group: Programmability
  icons: { primary: function, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_FUNCTIONS.md
  container:
    is_container: true
    grid_defaults: { columns: 1, rows: 4, gap: 4 }
    allowed_children: [entity, parameter, return]
    allowed_parents: [schema, database]
  display: { overview: [name, icons], mid: [name, summary], detail: [full_definition] }

- id: procedure
  display_name: Procedure
  group: Programmability
  icons: { primary: procedure, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_PROCEDURES.md
  container:
    is_container: true
    grid_defaults: { columns: 1, rows: 4, gap: 4 }
    allowed_children: [entity, parameter, action]
    allowed_parents: [schema, database]
  display: { overview: [name, icons], mid: [name, summary], detail: [full_definition] }

- id: package
  display_name: Package
  group: Programmability
  icons: { primary: package, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_PACKAGES.md
  container:
    is_container: true
    grid_defaults: { columns: 1, rows: 4, gap: 4 }
    allowed_children: [procedure, function, type]
    allowed_parents: [schema, database]
  display: { overview: [name, icons], mid: [name], detail: [name, members] }
```

### Types and Constraints

```yaml
- id: domain
  display_name: Domain
  group: Types
  icons: { primary: domain, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/types/DDL_DOMAINS_COMPREHENSIVE.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [schema]
  display: { overview: [name, icons], mid: [name], detail: [name, definition] }

- id: type
  display_name: Type
  group: Types
  icons: { primary: type, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_TYPES.md
    - docs/specifications/scratchbird/specifications/types/03_TYPES_AND_DOMAINS.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [schema]
  display: { overview: [name, icons], mid: [name], detail: [name, definition] }

- id: constraint
  display_name: Constraint
  group: Types
  icons: { primary: constraint, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_TABLES.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [table, schema]
  display: { overview: [name, icons], mid: [name], detail: [name, definition] }

- id: foreign_key
  display_name: Foreign Key
  group: Types
  icons: { primary: fk, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_TABLES.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [table]
  display: { overview: [name, icons], mid: [name], detail: [name, definition] }
```

### Security

```yaml
- id: users
  display_name: Users
  group: Security
  icons: { primary: users, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 4 }
    allowed_children: [user]
    allowed_parents: [database]
  display: { overview: [name, icons], mid: [name], detail: [name, members] }

- id: user
  display_name: User
  group: Security
  icons: { primary: user, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [users]
  display: { overview: [name, icons], mid: [name], detail: [name, definition] }

- id: roles
  display_name: Roles
  group: Security
  icons: { primary: roles, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 4 }
    allowed_children: [role]
    allowed_parents: [database]
  display: { overview: [name, icons], mid: [name], detail: [name, members] }

- id: role
  display_name: Role
  group: Security
  icons: { primary: role, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [roles]
  display: { overview: [name, icons], mid: [name], detail: [name, definition] }

- id: groups
  display_name: Groups
  group: Security
  icons: { primary: groups, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 4 }
    allowed_children: [group]
    allowed_parents: [database]
  display: { overview: [name, icons], mid: [name], detail: [name, members] }

- id: group
  display_name: Group
  group: Security
  icons: { primary: group, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [groups]
  display: { overview: [name, icons], mid: [name], detail: [name, definition] }

- id: grant
  display_name: Grant
  group: Security
  icons: { primary: grant, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [user, role]
  display: { overview: [name, icons], mid: [name], detail: [name, privileges] }
```

### Operations

```yaml
- id: job
  display_name: Job
  group: Operations
  icons: { primary: job, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/scheduler/SCHEDULER_JOB_RUNNER_CANONICAL_SPEC.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [etl_pipeline, database]
  display: { overview: [name, icons], mid: [name], detail: [name, schedule] }

- id: etl_pipeline
  display_name: ETL Pipeline
  group: Operations
  icons: { primary: etl, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/beta_requirements/big-data-streaming/README.md
  container:
    is_container: true
    grid_defaults: { columns: 2, rows: 2, gap: 4 }
    allowed_children: [job]
    allowed_parents: [database]
  display: { overview: [name, icons], mid: [name], detail: [name, jobs] }

- id: backup_task
  display_name: Backup Task
  group: Operations
  icons: { primary: backup, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/BACKUP_AND_RESTORE.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [database]
  display: { overview: [name, icons], mid: [name], detail: [name, schedule] }

- id: health_check
  display_name: Health Check
  group: Operations
  icons: { primary: health, secondary: null }
  spec_ref:
    - docs/specifications/scratchbird/specifications/operations/MONITORING_SQL_VIEWS.md
  container:
    is_container: false
    grid_defaults: { columns: 1, rows: 1, gap: 0 }
    allowed_children: []
    allowed_parents: [cluster, database]
  display: { overview: [name, icons], mid: [name], detail: [name, definition] }
```

---

## 4. Extraction Mapping (ScratchBird → Silverston)

| ScratchBird Metadata | Silverston Object |
| --- | --- |
| Cluster | Cluster |
| Database | Database |
| Schema | Schema |
| Table | Table |
| View | View |
| Materialized View | Materialized View |
| Index | Index |
| Trigger | Trigger |
| Procedure | Procedure |
| Function | Function |
| Sequence | Sequence |
| Domain | Domain |
| Type | Type |
| User/Role/Group | User / Role / Group |

---

## 5. Reconstruction Mapping (Silverston → ScratchBird)

- All objects with sufficient metadata (name, type, attributes, constraints) can be emitted as ScratchBirdSQL.
- Infrastructure objects map to cluster configuration metadata.
- Unknown or incomplete objects are flagged during build.

---

## 6. Reference (ScratchBird Parser)

The ScratchBird parser specifications are mirrored in:
- `docs/specifications/scratchbird/specifications/parser/`

These references define the authoritative list of objects and SQL constructs.
