# Dependency Tracking Requirements Document

## Executive Summary

This document specifies the requirements for object dependency tracking in ScratchRobin. Dependency tracking is essential for:
- Safe DROP operations (CASCADE vs RESTRICT decisions)
- Impact analysis before schema changes
- Documentation and data lineage
- Refactoring assistance

## Current Status

### Implemented in Backend
The `ScratchBirdMetadataProvider` has placeholder methods:
```cpp
QList<DependencyInfo> dependencies(const QString& objectName, const QString& objectType, 
                                   const QString& schema = QString()) const;
QList<DependencyInfo> dependents(const QString& objectName, const QString& objectType,
                                 const QString& schema = QString()) const;
```

### Required Engine Support
**CRITICAL QUESTION**: Does the ScratchBird engine currently provide:

1. **System catalog tables** for dependency tracking:
   - `pg_depend` (PostgreSQL-style)
   - `information_schema.dependencies` (standard SQL)
   - Custom ScratchBird dependency catalog

2. **SHOW DEPENDENCIES command**:
   ```sql
   SHOW DEPENDENCIES FOR <object-type> <object-name>;
   SHOW DEPENDENTS OF <object-type> <object-name>;
   ```

3. **Dependency types tracked**:
   - Hard dependencies (prevent DROP without CASCADE)
   - Soft dependencies (warnings only)
   - Ownership dependencies
   - Privilege dependencies

## Requirements

### 1. Dependency Types to Track

#### Hard Dependencies (CASCADE required)
| Object Type | Depends On | Example |
|-------------|------------|---------|
| VIEW | Base tables, views | View depends on underlying table structure |
| TRIGGER | Table, Functions | Trigger depends on table and function |
| INDEX | Table | Index depends on table existence |
| FOREIGN KEY | Primary key table | Referential integrity dependency |
| SEQUENCE | Table.column (OWNED BY) | Auto-increment dependency |
| FUNCTION | Other functions, Types | Function calling another function |
| MATERIALIZED VIEW | Base tables | Data source dependency |

#### Soft Dependencies (Warning only)
| Object Type | Depends On | Example |
|-------------|------------|---------|
| SYNONYM | Target object | Synonym points to table/view |
| COMMENT | Object | Documentation attached to object |
| POLICY | Table | Row-level security policy on table |

#### Privilege Dependencies
- User/Role has privileges on object
- Role membership hierarchies
- Default privileges

### 2. Required Information

For each dependency, the system must provide:

```cpp
struct DependencyInfo {
    QString objectName;        // Name of the dependent object
    QString objectType;        // TABLE, VIEW, FUNCTION, etc.
    QString objectSchema;      // Schema of dependent object
    QString dependencyName;    // Name of the object it depends on
    QString dependencyType;    // Type of the dependency target
    QString dependencySchema;  // Schema of dependency target
    QString relationship;      // "DEPENDS_ON", "REFERENCED_BY"
    QString dependencyKind;    // "HARD", "SOFT", "PRIVILEGE", "OWNERSHIP"
    bool isCircular = false;   // Circular dependency detection
};
```

### 3. SQL Interface Requirements

#### Option A: SHOW Commands (Preferred)
```sql
-- Show objects this object depends on
SHOW DEPENDENCIES FOR TABLE public.customers;

-- Show objects that depend on this object
SHOW DEPENDENTS OF TABLE public.customers;

-- Filter by dependency type
SHOW DEPENDENTS OF FUNCTION public.calculate_total WHERE type = 'VIEW';
```

**Expected Result Set:**
| object_name | object_type | object_schema | dependency_name | dependency_type | dependency_kind |
|-------------|-------------|---------------|-----------------|-----------------|-----------------|
| active_customers | VIEW | public | customers | TABLE | HARD |
| audit_trigger | TRIGGER | public | customers | TABLE | HARD |
| customer_stats | MATERIALIZED VIEW | public | customers | TABLE | HARD |

#### Option B: System Catalog Queries
```sql
-- Query dependency catalog directly
SELECT * FROM pg_depend 
WHERE refobjid = 'public.customers'::regclass;

-- Or information_schema view
SELECT * FROM information_schema.dependencies 
WHERE referenced_object_name = 'customers';
```

### 4. UI Requirements

#### Dependency Viewer Dialog
- Tree view showing dependency graph
- Direction toggle: "This object uses" vs "Used by this object"
- Color coding: Hard (red), Soft (yellow), Privilege (blue)
- Circular dependency highlighting
- Export dependency graph to DOT/PNG

#### Impact Analysis Before DROP
```
DROP TABLE public.customers CASCADE;

Warning: This will also drop:
  - VIEW public.active_customers
  - TRIGGER audit_trigger ON public.orders
  - MATERIALIZED VIEW public.customer_stats
  
  3 objects will be dropped. Continue? [Yes/No/Show Details]
```

### 5. Implementation Approaches

#### If Engine Has Native Support
Use native catalog tables:
```cpp
QString sql = QString(
    "SELECT * FROM pg_depend "
    "WHERE refobjid = '%1.%2'::regclass"
).arg(schema).arg(name);
```

#### If Engine Lacks Support (Fallback)
Implement client-side dependency tracking:

1. **Parse DDL Statements**: Extract dependencies from CREATE statements
   ```sql
   -- Parse this:
   CREATE VIEW active_customers AS SELECT * FROM customers WHERE active = 1;
   -- Extract: depends on table 'customers'
   ```

2. **Query information_schema**:
   ```sql
   -- Views depending on table
   SELECT view_name, view_definition 
   FROM information_schema.views 
   WHERE view_definition LIKE '%customers%';
   ```

3. **Static Analysis**: Parse stored procedure bodies for table references

### 6. Dependency Caching

For performance, implement a dependency cache:

```cpp
class DependencyCache {
public:
    void invalidate(const QString& schema, const QString& name);
    QList<DependencyInfo> getDependencies(const QString& schema, const QString& name);
    QList<DependencyInfo> getDependents(const QString& schema, const QString& name);
    
private:
    QHash<QString, QList<DependencyInfo>> cache_;
    QDateTime lastRefresh_;
};
```

## Questions for Engine Team

1. **Does ScratchBird track dependencies in system catalogs?**
   - If yes: What tables/views expose this information?
   - If no: Is this on the roadmap?

2. **Does ScratchBird support SHOW DEPENDENCIES commands?**
   - If yes: Document the exact syntax
   - If no: Can it be added?

3. **What dependency types are tracked?**
   - Hard (structural) dependencies?
   - Soft (reference) dependencies?
   - Privilege dependencies?
   - Ownership dependencies?

4. **Circular dependency handling:**
   - Does the engine detect circular dependencies?
   - What happens on CREATE with circular reference?

5. **Cross-database dependencies:**
   - Does ScratchBird support dependencies across databases?
   - How are foreign database links tracked?

## Proposed Implementation Timeline

### Phase 1: Basic Dependency Tracking (If Engine Supports)
- Week 1: Implement `dependencies()` and `dependents()` using engine catalogs
- Week 2: Add Dependency Viewer dialog
- Week 3: Integrate with DROP confirmation dialogs

### Phase 2: Enhanced Features (Client-Side Fallback)
- Week 4: Implement DDL parsing for dependency extraction
- Week 5: Add dependency cache layer
- Week 6: Graph visualization export

### Phase 3: Advanced Features
- Week 7: Impact analysis reports
- Week 8: Dependency change notifications
- Week 9: Refactoring assistance (safe rename)

## Related UI Components

The following managers need dependency support:

1. **View Manager**: Show base tables, show dependent views
2. **Table Manager**: Show indexes, triggers, foreign keys, views
3. **Function Manager**: Show calling functions, functions called
4. **Trigger Manager**: Show trigger dependencies (table + function)
5. **User/Role Manager**: Show object ownership, privilege dependencies

## Example Usage Scenarios

### Scenario 1: Safe Table Drop
```sql
-- User attempts:
DROP TABLE customers;

-- System checks dependencies:
-- Found: VIEW active_customers, TRIGGER audit_trigger
-- Action: Warn user, suggest CASCADE or manual cleanup
```

### Scenario 2: Refactoring Impact Analysis
```sql
-- User wants to rename column:
ALTER TABLE customers RENAME COLUMN name TO full_name;

-- System checks:
-- Found: 3 views reference 'name' column
-- Action: Show affected views, offer to update them
```

### Scenario 3: Privilege Propagation
```sql
-- User grants on table:
GRANT SELECT ON customers TO readonly;

-- System option:
-- Also grant on dependent views? [Yes/No/List]
```

## Appendix: Reference Implementation (PostgreSQL)

```sql
-- Get objects that depend on a table
SELECT 
    dependent_ns.nspname as dependent_schema,
    dependent_view.relname as dependent_view,
    source_ns.nspname as source_schema,
    source_table.relname as source_table
FROM pg_depend 
JOIN pg_rewrite ON pg_depend.objid = pg_rewrite.oid 
JOIN pg_class as dependent_view ON pg_rewrite.ev_class = dependent_view.oid 
JOIN pg_class as source_table ON pg_depend.refobjid = source_table.oid 
JOIN pg_namespace dependent_ns ON dependent_view.relnamespace = dependent_ns.oid 
JOIN pg_namespace source_ns ON source_table.relnamespace = source_ns.oid 
WHERE 
    source_ns.nspname = 'public'
    AND source_table.relname = 'customers'
    AND pg_depend.deptype = 'n';
```

---

**Document Status**: DRAFT  
**Next Step**: Confirm engine capabilities with ScratchBird team  
**Priority**: HIGH (blocks safe DROP operations)
