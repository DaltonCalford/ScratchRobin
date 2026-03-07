# ScratchBird V3 SQL Dialect - Quick Reference for UI Development

> **Source**: `SCRATCHBIRD_V3_DIALECT.md` (Full EBNF Specification)
> 
> **Purpose**: Quick reference for DDL generation and UI forms

---

## Data Types

### Numeric Types
```sql
INTEGER, INT                    -- 32-bit signed
BIGINT                          -- 64-bit signed
SMALLINT                        -- 16-bit signed
DECIMAL(p, s), NUMERIC(p, s)    -- Fixed-point
REAL, FLOAT4                    -- 32-bit float
DOUBLE PRECISION, FLOAT8        -- 64-bit float
SERIAL, BIGSERIAL               -- Auto-incrementing integers
```

### Character Types
```sql
CHAR(n), CHARACTER(n)           -- Fixed-length
VARCHAR(n), CHARACTER VARYING   -- Variable-length
TEXT                            -- Unbounded
```

### Binary Types
```sql
BYTEA                           -- Variable-length binary
BLOB(n)                         -- Binary large object
```

### Temporal Types
```sql
DATE                            -- Date only
TIME [WITHOUT TIME ZONE]        -- Time only
TIMESTAMP [WITHOUT TIME ZONE]   -- Date + time
TIMESTAMPTZ, TIMESTAMP WITH TIME ZONE
INTERVAL                        -- Time duration
```

### Boolean
```sql
BOOLEAN, BOOL                   -- TRUE/FALSE/NULL
```

### Other Types
```sql
UUID                            -- 128-bit UUID
JSON, JSONB                     -- JSON data (binary storage)
XML                             -- XML data
ARRAY                           -- Array of any type
DOMAIN <name>                   -- User-defined domain
```

---

## DDL Statements

### CREATE TABLE
```sql
CREATE TABLE [GLOBAL | TEMP | TEMPORARY] [UNLOGGED] [IF NOT EXISTS] <name> (
    <column-def> [, ...]
    [, <table-constraint> ...]
) [WITH (<storage-param> [, ...])]
  [TABLESPACE <tablespace>];

-- Column definition
<column-def> ::= <name> <type> 
    [DEFAULT <expr>]
    [NOT NULL | NULL]
    [GENERATED ALWAYS | BY DEFAULT AS IDENTITY]
    [UNIQUE | PRIMARY KEY]
    [REFERENCES <table>[(<col>)] [ON DELETE ...] [ON UPDATE ...]]
    [CHECK (<condition>)]
    [COMMENT <string>]

-- Table constraints
<table-constraint> ::=
    CONSTRAINT <name> 
    ( PRIMARY KEY (<cols>)
    | UNIQUE (<cols>)
    | CHECK (<expr>)
    | FOREIGN KEY (<cols>) REFERENCES <table>(<cols>) [ON DELETE ...]
    | EXCLUDE [USING <index-method>] (<exclude-element> WITH <operator>)
    )
```

### CREATE VIEW
```sql
CREATE [OR REPLACE | OR ALTER] VIEW [IF NOT EXISTS] <name> [(<cols>)] AS
    <query>
    [WITH [CASCADED | LOCAL] CHECK OPTION];

-- Materialized view
CREATE MATERIALIZED VIEW [IF NOT EXISTS] <name> [(<cols>)] AS
    <query>
    [WITH [NO] DATA]
    [TABLESPACE <tablespace>];
```

### CREATE TRIGGER
```sql
CREATE [OR REPLACE] TRIGGER <name>
    { BEFORE | AFTER | INSTEAD OF }
    { INSERT | UPDATE [OF <cols>] | DELETE | TRUNCATE }
    [OR { INSERT | UPDATE | DELETE | TRUNCATE } ...]
    ON <table>
    [FROM <referenced-table>]
    [NOT DEFERRABLE | [DEFERRABLE] [INITIALLY IMMEDIATE | DEFERRED]]
    [FOR [EACH] { ROW | STATEMENT }]
    [WHEN (<condition>)]
    EXECUTE { FUNCTION | PROCEDURE } <function>(<args>);
```

### CREATE INDEX
```sql
CREATE [UNIQUE] INDEX [IF NOT EXISTS] <name> ON <table>
    [USING <method>] (<index-col> [, ...])
    [INCLUDE (<cols>)]
    [WITH (<storage-param> [, ...])]
    [TABLESPACE <tablespace>]
    [WHERE <predicate>];

<index-col> ::= <col> [COLLATE <collation>] [<opclass>] [ASC | DESC] [NULLS FIRST | LAST]
```

### CREATE FUNCTION/PROCEDURE
```sql
CREATE [OR REPLACE] FUNCTION <name> ([<args>])
    RETURNS <type>
    [LANGUAGE <lang>]
    [TRANSFORM ...]
    [WINDOW]
    [IMMUTABLE | STABLE | VOLATILE]
    [NOT LEAKPROOF | LEAKPROOF]
    [CALLED ON NULL INPUT | RETURNS NULL ON NULL INPUT | STRICT]
    [EXTERNAL] SECURITY INVOKER | [EXTERNAL] SECURITY DEFINER
    [PARALLEL { UNSAFE | RESTRICTED | SAFE }]
    [COST <cost>]
    [ROWS <rows>]
    [SUPPORT <support-func>]
    [SET <config-param> { TO | = } <value> [, ...]]
    AS '<definition>' [, '<obj-file>', '<link-symbol>'];

CREATE [OR REPLACE] PROCEDURE <name> ([<args>])
    [LANGUAGE <lang>]
    [EXTERNAL] SECURITY INVOKER | [EXTERNAL] SECURITY DEFINER
    [SET <config-param> { TO | = } <value> [, ...]]
    AS '<definition>';
```

### CREATE SEQUENCE
```sql
CREATE [TEMP | TEMPORARY] SEQUENCE [IF NOT EXISTS] <name>
    [AS <data-type>]
    [INCREMENT [BY] <increment>]
    [MINVALUE <minvalue> | NO MINVALUE]
    [MAXVALUE <maxvalue> | NO MAXVALUE]
    [START [WITH] <start>]
    [CACHE <cache>]
    [[NO] CYCLE]
    [OWNED BY <table.column> | NONE];
```

### CREATE USER/ROLE
```sql
CREATE USER <name> [WITH]
    [SUPERUSER | NOSUPERUSER]
    [CREATEDB | NOCREATEDB]
    [CREATEROLE | NOCREATEROLE]
    [INHERIT | NOINHERIT]
    [LOGIN | NOLOGIN]
    [REPLICATION | NOREPLICATION]
    [BYPASSRLS | NOBYPASSRLS]
    [CONNECTION LIMIT <connlimit>]
    [ENCRYPTED] PASSWORD '<password>' | PASSWORD NULL
    [VALID UNTIL '<timestamp>']
    [IN ROLE <role> [, ...]]
    [IN GROUP <role> [, ...]]
    [ROLE <role> [, ...]]
    [ADMIN <role> [, ...]]
    [USER <role> [, ...]]
    [SYSID <uid>];

CREATE ROLE <name> [WITH <option> ...];  -- Same options
```

---

## ALTER Statements

### ALTER TABLE
```sql
ALTER TABLE [IF EXISTS] <name>
    ( ADD COLUMN [IF NOT EXISTS] <col-def>
    | DROP COLUMN [IF EXISTS] <col> [CASCADE | RESTRICT]
    | ALTER COLUMN <col> 
        ( SET DATA TYPE <type> [COLLATE <collation>] [USING <expr>]
        | SET DEFAULT <expr>
        | DROP DEFAULT
        | SET NOT NULL
        | DROP NOT NULL
        | ADD GENERATED ALWAYS | BY DEFAULT AS IDENTITY [(<seq-options>)]
        | DROP IDENTITY [IF EXISTS]
        | SET STATISTICS <integer>
        | SET (<attribute> = <value> [, ...])
        | RESET (<attribute> [, ...])
        | SET STORAGE { PLAIN | EXTERNAL | EXTENDED | MAIN | DEFAULT }
        )
    | ADD <table-constraint> [NOT VALID]
    | ADD <table-constraint> USING INDEX <index>
    | ALTER CONSTRAINT <constraint> DEFERRABLE | NOT DEFERRABLE | INITIALLY ...
    | VALIDATE CONSTRAINT <constraint>
    | DROP CONSTRAINT [IF EXISTS] <constraint> [CASCADE | RESTRICT]
    | RENAME COLUMN <col> TO <new-col>
    | RENAME TO <new-name>
    | SET SCHEMA <new-schema>
    | ATTACH PARTITION <partition> { FOR VALUES ... | DEFAULT }
    | DETACH PARTITION <partition> [CONCURRENTLY | FINALIZE]
    | ENABLE | DISABLE TRIGGER [ <trigger> | ALL | USER ]
    | ENABLE | DISABLE RULE <rule>
    | ENABLE | DISABLE REPLICA TRIGGER <trigger>
    | ENABLE | DISABLE ALWAYS TRIGGER <trigger>
    | FORCE | NO FORCE ROW LEVEL SECURITY
    | CLUSTER ON <index>
    | SET WITHOUT CLUSTER
    | SET WITHOUT OIDS
    | SET TABLESPACE <tablespace>
    | SET { LOGGED | UNLOGGED }
    | SET (<storage-param> = <value> [, ...])
    | RESET (<storage-param> [, ...])
    | INHERIT <parent>
    | NO INHERIT <parent>
    | OF <type>
    | NOT OF
    | OWNER TO { <new-owner> | CURRENT_USER | SESSION_USER }
    );
```

---

## SHOW Commands (Introspection)

```sql
-- Schema/Database info
SHOW DATABASES;
SHOW SCHEMAS [IN | FROM <path>];
SHOW CURRENT SCHEMA;
SHOW SEARCH PATH;

-- Tables and columns
SHOW TABLES [IN | FROM <path>];
SHOW TABLE <name>;
SHOW COLUMNS [IN | FROM <path>];
SHOW CREATE TABLE <name>;

-- Views
SHOW VIEWS;
SHOW VIEW <name>;
SHOW CREATE VIEW <name>;

-- Indexes
SHOW INDEXES [IN | FROM <path>];
SHOW INDEX <name>;

-- Triggers
SHOW TRIGGERS;
SHOW TRIGGER <name>;

-- Routines
SHOW FUNCTIONS;
SHOW FUNCTION <name>;
SHOW PROCEDURES;
SHOW PROCEDURE <name>;

-- Users and roles
SHOW ROLES;
SHOW ROLE <name>;
SHOW GRANTS;

-- Replication
SHOW PUBLICATIONS;
SHOW PUBLICATION <name>;
SHOW SUBSCRIPTIONS;
SHOW SUBSCRIPTION <name>;

-- System
SHOW VARIABLES [<name>];
SHOW VERSION;
SHOW METRICS;
SHOW TRANSACTION ISOLATION LEVEL;
```

---

## COMMENT ON (Documentation)

```sql
COMMENT ON 
    ( TABLE <name>
    | VIEW <name>
    | INDEX <name>
    | COLUMN <name>
    | SEQUENCE <name>
    | DOMAIN <name>
    | TYPE <name>
    | FUNCTION <name>
    | PROCEDURE <name>
    | TRIGGER <name> ON <table>
    | PACKAGE <name>
    | EXCEPTION <name>
    | ROLE <name>
    | USER <name>
    | GROUP <name>
    | POLICY <name> ON <table>
    | SCHEMA <name>
    | DATABASE <name>
    | TABLESPACE <name>
    | ... )
IS '<comment-text>';
```

---

## Privilege Management (DCL)

### GRANT
```sql
GRANT <privileges> ON <object> TO <grantees> [WITH GRANT OPTION];

<privileges> ::= 
    ( SELECT | INSERT | UPDATE | DELETE | TRUNCATE | REFERENCES | TRIGGER | 
      USAGE | EXECUTE | ALL [PRIVILEGES] )
    [, ...]

<object> ::=
    TABLE <name> | VIEW <name> | SEQUENCE <name> | 
    FUNCTION <name> | PROCEDURE <name> | DATABASE <name> |
    SCHEMA <name> | DOMAIN <name> | ...

<grantees> ::= 
    <username> | ROLE <rolename> | GROUP <groupname> | PUBLIC
    [, ...]
```

### REVOKE
```sql
REVOKE [GRANT OPTION FOR] <privileges> ON <object> FROM <grantees> [CASCADE | RESTRICT];
```

---

## Common Options

### Storage Parameters (WITH clause)
```sql
fillfactor = <integer>          -- Index page fill percentage (10-100)
autovacuum_enabled = <boolean>
autovacuum_vacuum_threshold = <integer>
autovacuum_vacuum_scale_factor = <float>
toast_tuple_target = <integer>
parallel_workers = <integer>
```

### Constraint Options
```sql
ON DELETE { CASCADE | SET NULL | SET DEFAULT | RESTRICT | NO ACTION }
ON UPDATE { CASCADE | SET NULL | SET DEFAULT | RESTRICT | NO ACTION }
DEFERRABLE | NOT DEFERRABLE
INITIALLY IMMEDIATE | INITIALLY DEFERRED
```

---

## UI Form Mapping

| UI Manager | Primary DDL | Secondary DDL | SHOW Commands |
|------------|-------------|---------------|---------------|
| View Manager | CREATE VIEW, DROP VIEW | CREATE MATERIALIZED VIEW, REFRESH | SHOW VIEWS, SHOW CREATE VIEW |
| Trigger Manager | CREATE TRIGGER, DROP TRIGGER | ALTER TRIGGER ENABLE/DISABLE | SHOW TRIGGERS |
| Table Designer | CREATE TABLE, ALTER TABLE | DROP TABLE, COMMENT ON | SHOW TABLES, SHOW COLUMNS |
| Index Manager | CREATE INDEX, DROP INDEX | ALTER INDEX, REINDEX | SHOW INDEXES |
| Function Manager | CREATE FUNCTION, DROP FUNCTION | CREATE OR REPLACE, ALTER | SHOW FUNCTIONS |
| Procedure Manager | CREATE PROCEDURE, DROP PROCEDURE | CREATE OR REPLACE, ALTER | SHOW PROCEDURES |
| User Manager | CREATE USER, DROP USER | ALTER USER, GRANT, REVOKE | SHOW ROLES |
| Sequence Manager | CREATE SEQUENCE, DROP SEQUENCE | ALTER SEQUENCE | SHOW SEQUENCES |
| Job Manager | CREATE JOB, DROP JOB | ALTER JOB, EXECUTE JOB | SHOW JOBS |
| Publication Manager | CREATE PUBLICATION | ALTER PUBLICATION, DROP | SHOW PUBLICATIONS |
| Subscription Manager | CREATE SUBSCRIPTION | ALTER SUBSCRIPTION, DROP | SHOW SUBSCRIPTIONS |

---

## Important Notes for UI

1. **IF EXISTS / IF NOT EXISTS**: Use for safe DDL operations
2. **CASCADE / RESTRICT**: DROP behavior - CASCADE drops dependent objects
3. **OR REPLACE**: Recreate objects without dropping first
4. **Search Path**: Schema resolution uses session search path
5. **Quoted Identifiers**: Use double quotes for case-sensitive names
6. **Comments**: All objects support COMMENT ON for documentation
7. **Domains**: Global namespace - must be unique across database
8. **Privileges**: Separate from object ownership - use GRANT/REVOKE

---

## Canonical Style Guidelines

- Prefer explicit grammar forms (e.g., `EXTRACT(<field> FROM <expr>)`)
- Use plural list forms for SHOW commands (`SHOW TABLES` not `SHOW TABLE`)
- Prefer `OR REPLACE` over `DROP + CREATE` for views/functions
- Use `IF EXISTS`/`IF NOT EXISTS` in UI-generated DDL for safety
- Include `COMMENT ON` statements for documentation

---

## Full Reference

See `SCRATCHBIRD_V3_DIALECT.md` for complete EBNF grammar including:
- Complete expression grammar
- Full SELECT syntax
- Partitioning specifications
- Replication (PUBLICATION/SUBSCRIPTION)
- Events and scheduling
- Cluster/Cube management
- Foreign data wrappers
- Security policies (RLS)
- Custom types and domains
