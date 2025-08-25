#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include "database_driver_manager.h"

namespace scratchrobin {

// MySQL-specific data types
struct MySQLDataTypes {
    // Standard SQL types with MySQL extensions
    static const QStringList NUMERIC_TYPES;
    static const QStringList STRING_TYPES;
    static const QStringList DATE_TIME_TYPES;
    static const QStringList BINARY_TYPES;
    static const QStringList BOOLEAN_TYPES;
    static const QStringList SPATIAL_TYPES;

    // MySQL-specific types
    static const QStringList JSON_TYPES;
    static const QStringList GENERATED_TYPES;

    // Get all MySQL data types
    static QStringList getAllDataTypes();
};

// MySQL-specific SQL features
struct MySQLSQLFeatures {
    static const QStringList KEYWORDS;
    static const QStringList FUNCTIONS;
    static const QStringList OPERATORS;
    static const QStringList RESERVED_WORDS;

    // Advanced SQL constructs
    static const QStringList CTE_FEATURES;           // WITH, RECURSIVE (MySQL 8.0+)
    static const QStringList WINDOW_FEATURES;        // OVER, PARTITION BY, ORDER BY (MySQL 8.0+)
    static const QStringList JSON_FEATURES;          // JSON functions (MySQL 5.7+)
    static const QStringList SPATIAL_FEATURES;       // Spatial data types and functions
    static const QStringList PARTITION_FEATURES;     // Table partitioning
    static const QStringList FULLTEXT_FEATURES;      // Full-text search
    static const QStringList GTID_FEATURES;          // Global Transaction IDs
    static const QStringList REPLICATION_FEATURES;   // Replication features
    static const QStringList PERFORMANCE_FEATURES;   // Performance schema features (MySQL 5.5+)
    static const QStringList INVISIBLE_INDEX_FEATURES; // Invisible indexes (MySQL 8.0+)
    static const QStringList EXPRESSION_INDEX_FEATURES; // Functional indexes (MySQL 8.0+)
    static const QStringList DESCENDING_INDEX_FEATURES; // Descending indexes (MySQL 8.0+)
};

// MySQL-specific database objects
struct MySQLObjects {
    static const QStringList SYSTEM_DATABASES;
    static const QStringList SYSTEM_SCHEMAS;
    static const QStringList INFORMATION_SCHEMA_VIEWS;
    static const QStringList PERFORMANCE_SCHEMA_VIEWS;
    static const QStringList MYSQL_SCHEMA_TABLES;
    static const QStringList BUILTIN_FUNCTIONS;
    static const QStringList BUILTIN_OPERATORS;

    // MySQL-specific system objects
    static const QStringList SYS_SCHEMA_VIEWS;       // sys schema views (MySQL 5.7+)
    static const QStringList INNODB_METRICS;         // InnoDB metrics
    static const QStringList FULLTEXT_INDEXES;       // Full-text index information
};

// MySQL-specific configuration
struct MySQLConfig {
    static const QStringList CONFIG_PARAMETERS;
    static const QStringList CONFIG_CATEGORIES;
    static const QStringList STORAGE_ENGINES;
    static const QStringList ISOLATION_LEVELS;
    static const QStringList CHARACTER_SETS;
    static const QStringList COLLATIONS;

    // MySQL specific settings
    static const QStringList SQL_MODES;
    static const QStringList TIME_ZONES;
    static const QStringList LOG_LEVELS;
    static const QStringList OPTIMIZER_SWITCHES;
    static const QStringList INNODB_CONFIGS;
};

// MySQL feature detection and support
class MySQLFeatureDetector {
public:
    static bool isFeatureSupported(DatabaseType dbType, const QString& feature);
    static QStringList getSupportedFeatures(DatabaseType dbType);
    static QString getVersionSpecificSyntax(DatabaseType dbType, const QString& version);

    // Detect MySQL-specific capabilities
    static bool supportsJSON(DatabaseType dbType);
    static bool supportsSequences(DatabaseType dbType);  // Note: MySQL doesn't have sequences like MariaDB
    static bool supportsVirtualColumns(DatabaseType dbType);
    static bool supportsWindowFunctions(DatabaseType dbType);
    static bool supportsCTEs(DatabaseType dbType);
    static bool supportsSpatial(DatabaseType dbType);
    static bool supportsPartitioning(DatabaseType dbType);
    static bool supportsGTID(DatabaseType dbType);
    static bool supportsPerformanceSchema(DatabaseType dbType);
    static bool supportsReplication(DatabaseType dbType);
    static bool supportsFulltextSearch(DatabaseType dbType);
    static bool supportsInvisibleIndexes(DatabaseType dbType);
    static bool supportsExpressionIndexes(DatabaseType dbType);

    // Version-specific feature detection
    static bool supportsFeatureByVersion(const QString& version, const QString& feature);
    static QString getMinimumVersionForFeature(const QString& feature);

    // MySQL-specific feature checks
    static bool isPerconaServer(const QString& version);
    static bool isMySQLCluster(const QString& version);
    static bool supportsEnterpriseFeatures(DatabaseType dbType);
};

} // namespace scratchrobin
