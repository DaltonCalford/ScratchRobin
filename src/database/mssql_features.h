#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include "database_driver_manager.h"

namespace scratchrobin {

// Microsoft SQL Server-specific data types
struct MSSQLDataTypes {
    // Standard SQL types with MSSQL extensions
    static const QStringList NUMERIC_TYPES;
    static const QStringList STRING_TYPES;
    static const QStringList DATE_TIME_TYPES;
    static const QStringList BINARY_TYPES;
    static const QStringList BOOLEAN_TYPES;
    static const QStringList MONETARY_TYPES;
    static const QStringList UNIQUEIDENTIFIER_TYPES;

    // SQL Server-specific types
    static const QStringList SPATIAL_TYPES;
    static const QStringList HIERARCHY_TYPES;
    static const QStringList XML_TYPES;

    // Get all MSSQL data types
    static QStringList getAllDataTypes();
};

// Microsoft SQL Server-specific SQL features
struct MSSQLSQLFeatures {
    static const QStringList KEYWORDS;
    static const QStringList FUNCTIONS;
    static const QStringList OPERATORS;
    static const QStringList RESERVED_WORDS;

    // Advanced SQL constructs
    static const QStringList CTE_FEATURES;           // WITH, RECURSIVE
    static const QStringList WINDOW_FEATURES;        // OVER, PARTITION BY, ORDER BY
    static const QStringList PIVOT_FEATURES;         // PIVOT, UNPIVOT
    static const QStringList MERGE_FEATURES;         // MERGE statement
    static const QStringList SEQUENCE_FEATURES;      // SEQUENCE objects
    static const QStringList XML_FEATURES;           // XML data type and methods
    static const QStringList SPATIAL_FEATURES;       // Geography and Geometry types
    static const QStringList HIERARCHY_FEATURES;     // HIERARCHYID type
    static const QStringList JSON_FEATURES;          // JSON functions (SQL Server 2016+)
    static const QStringList STRING_AGG_FEATURES;    // STRING_AGG function
    static const QStringList OFFSET_FETCH_FEATURES;  // OFFSET and FETCH clauses
};

// Microsoft SQL Server-specific database objects
struct MSSQLObjects {
    static const QStringList SYSTEM_DATABASES;
    static const QStringList SYSTEM_SCHEMAS;
    static const QStringList SYSTEM_VIEWS;
    static const QStringList SYSTEM_PROCEDURES;
    static const QStringList INFORMATION_SCHEMA_VIEWS;
    static const QStringList BUILTIN_FUNCTIONS;
    static const QStringList BUILTIN_OPERATORS;

    // SQL Server system objects
    static const QStringList DMV_VIEWS;              // Dynamic Management Views
    static const QStringList DMF_FUNCTIONS;          // Dynamic Management Functions
    static const QStringList SYSTEM_TABLES;          // System base tables
};

// Microsoft SQL Server-specific configuration
struct MSSQLConfig {
    static const QStringList CONFIG_PARAMETERS;
    static const QStringList CONFIG_CATEGORIES;
    static const QStringList RECOVERY_MODELS;
    static const QStringList COMPATIBILITY_LEVELS;
    static const QStringList ISOLATION_LEVELS;

    // SQL Server specific settings
    static const QStringList COLLATION_NAMES;
    static const QStringList AUTHENTICATION_MODES;
    static const QStringList CONNECTION_ENCRYPTION_OPTIONS;
};

// Microsoft SQL Server feature detection and support
class MSSQLFeatureDetector {
public:
    static bool isFeatureSupported(DatabaseType dbType, const QString& feature);
    static QStringList getSupportedFeatures(DatabaseType dbType);
    static QString getVersionSpecificSyntax(DatabaseType dbType, const QString& version);

    // Detect MSSQL-specific capabilities
    static bool supportsXML(DatabaseType dbType);
    static bool supportsSpatial(DatabaseType dbType);
    static bool supportsHierarchy(DatabaseType dbType);
    static bool supportsJSON(DatabaseType dbType);
    static bool supportsCTEs(DatabaseType dbType);
    static bool supportsWindowFunctions(DatabaseType dbType);
    static bool supportsPivot(DatabaseType dbType);
    static bool supportsMerge(DatabaseType dbType);
    static bool supportsSequences(DatabaseType dbType);
    static bool supportsStringAgg(DatabaseType dbType);
    static bool supportsOffsetFetch(DatabaseType dbType);
    static bool supportsFilestream(DatabaseType dbType);
    static bool supportsInMemory(DatabaseType dbType);
    static bool supportsColumnstore(DatabaseType dbType);
    static bool supportsTemporal(DatabaseType dbType);
    static bool supportsGraph(DatabaseType dbType);

    // Version-specific feature detection
    static bool supportsFeatureByVersion(const QString& version, const QString& feature);
    static QString getMinimumVersionForFeature(const QString& feature);
};

} // namespace scratchrobin
