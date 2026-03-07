#pragma once
#include <QString>
#include <QList>
#include <QHash>

namespace scratchrobin::backend {

/**
 * @brief ScratchBird V3 Object Type Registry
 * 
 * Complete list of first-class object families in ScratchBird V3 dialect.
 * This provides a single source of truth for object type metadata used
 * throughout the UI and backend.
 */

// ============================================================================
// Object Family Enum
// ============================================================================
enum class ObjectFamily {
    // Core relation objects
    TABLE,
    VIEW,
    MATERIALIZED_VIEW,
    CDC_TABLE,
    FOREIGN_TABLE,
    
    // Schema objects
    SCHEMA,
    DATABASE,
    DATABASE_CONNECTION,
    
    // Code objects
    FUNCTION,
    PROCEDURE,
    TRIGGER,
    PACKAGE,
    EXCEPTION,
    UDR,              // User Defined Routine
    
    // Type system
    DOMAIN,           // Global namespace
    TYPE,
    SEQUENCE,
    COLLATION,
    
    // Indexing
    INDEX,
    CONSTRAINT,
    
    // Security
    ROLE,
    USER,
    GROUP,
    POLICY,
    TOKEN,
    
    // Federation
    SERVER,
    FOREIGN_DATA_WRAPPER,
    SYNONYM,
    PUBLIC_SYNONYM,
    USER_MAPPING,
    
    // Administration
    TABLESPACE,
    FILESPACE,
    EXTENSION,
    
    // Operations
    JOB,
    CONNECTION_RULE,
    QUOTA_PROFILE,
    
    // Replication
    PUBLICATION,
    SUBSCRIPTION,
    REPLICATION_CHANNEL,
    EVENT,
    
    // Analytics
    CLUSTER,
    CUBE,
    
    // Unknown/Invalid
    UNKNOWN
};

// ============================================================================
// Object Type Metadata
// ============================================================================
struct ObjectTypeInfo {
    ObjectFamily family;
    QString name;                    // Canonical name (e.g., "TABLE")
    QString pluralName;              // Plural form (e.g., "TABLES")
    QString createKeyword;           // CREATE keyword
    QString showKeyword;             // SHOW keyword
    bool hasOwner;                   // Supports OWNER TO
    bool hasSchema;                  // Schema-scoped
    bool supportsComment;            // Supports COMMENT ON
    bool supportsPrivileges;         // Supports GRANT/REVOKE
    bool isGlobal;                   // Global namespace (e.g., DOMAIN)
    bool requiresParent;             // Requires parent object (INDEX, TRIGGER)
    ObjectFamily parentFamily;       // Parent object family if applicable
};

// ============================================================================
// Object Type Registry
// ============================================================================
class ObjectTypeRegistry {
public:
    static ObjectTypeRegistry& instance();
    
    // Lookup methods
    ObjectTypeInfo info(ObjectFamily family) const;
    ObjectTypeInfo info(const QString& name) const;
    ObjectFamily family(const QString& name) const;
    
    // List methods
    QList<ObjectTypeInfo> allTypes() const;
    QList<ObjectTypeInfo> schemaScopedTypes() const;
    QList<ObjectTypeInfo> databaseScopedTypes() const;
    QList<ObjectTypeInfo> globalTypes() const;
    QList<ObjectTypeInfo> parentedTypes() const;  // INDEX, TRIGGER
    
    // Validation
    bool isValid(const QString& name) const;
    bool isSchemaScoped(ObjectFamily family) const;
    bool isDatabaseScoped(ObjectFamily family) const;
    bool isGlobal(ObjectFamily family) const;
    
private:
    ObjectTypeRegistry();
    void registerType(const ObjectTypeInfo& info);
    
    QHash<ObjectFamily, ObjectTypeInfo> byFamily_;
    QHash<QString, ObjectFamily> byName_;
};

// ============================================================================
// DDL Templates
// ============================================================================
struct DdlTemplates {
    // CREATE templates
    static QString createTable();
    static QString createView();
    static QString createMaterializedView();
    static QString createTrigger();
    static QString createIndex();
    static QString createFunction();
    static QString createProcedure();
    static QString createSequence();
    static QString createUser();
    static QString createRole();
    static QString createSchema();
    static QString createPublication();
    static QString createSubscription();
    
    // ALTER templates
    static QString alterTable();
    static QString alterView();
    static QString alterTrigger();
    static QString alterIndex();
    static QString alterFunction();
    static QString alterProcedure();
    static QString alterSequence();
    static QString alterUser();
    static QString alterRole();
    
    // DROP templates
    static QString drop(ObjectFamily family, bool ifExists = true, bool cascade = false);
    
    // SHOW templates
    static QString showList(ObjectFamily family);
    static QString showCreate(ObjectFamily family);
    static QString showDetails(ObjectFamily family);
    
    // COMMENT template
    static QString commentOn(ObjectFamily family);
};

// ============================================================================
// Convenience Functions
// ============================================================================
inline QString toString(ObjectFamily family) {
    return ObjectTypeRegistry::instance().info(family).name;
}

inline ObjectFamily fromString(const QString& name) {
    return ObjectTypeRegistry::instance().family(name);
}

} // namespace scratchrobin::backend
