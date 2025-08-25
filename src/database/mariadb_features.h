#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include "database_driver_manager.h"

namespace scratchrobin {

// MariaDB-specific data types
struct MariaDBDataTypes {
    // Standard SQL types with MariaDB extensions
    static const QStringList NUMERIC_TYPES;
    static const QStringList STRING_TYPES;
    static const QStringList DATE_TIME_TYPES;
    static const QStringList BINARY_TYPES;
    static const QStringList BOOLEAN_TYPES;
    static const QStringList SPATIAL_TYPES;

    // MariaDB-specific types
    static const QStringList JSON_TYPES;
    static const QStringList DYNAMIC_COLUMNS_TYPES;
    static const QStringList UUID_TYPES;
    static const QStringList IP_TYPES;
    static const QStringList SEQUENCE_TYPES;

    // Get all MariaDB data types
    static QStringList getAllDataTypes();
};

// MariaDB-specific SQL features
struct MariaDBSQLFeatures {
    static const QStringList KEYWORDS;
    static const QStringList FUNCTIONS;
    static const QStringList OPERATORS;
    static const QStringList RESERVED_WORDS;

    // Advanced SQL constructs
    static const QStringList CTE_FEATURES;           // WITH, RECURSIVE
    static const QStringList WINDOW_FEATURES;        // OVER, PARTITION BY, ORDER BY
    static const QStringList JSON_FEATURES;          // JSON functions
    static const QStringList SPATIAL_FEATURES;       // Spatial data types and functions
    static const QStringList SEQUENCE_FEATURES;      // SEQUENCE objects
    static const QStringList DYNAMIC_FEATURES;       // Dynamic columns
    static const QStringList VIRTUAL_FEATURES;       // Virtual/Persistent columns
    static const QStringList PARTITION_FEATURES;     // Table partitioning
    static const QStringList GTID_FEATURES;          // Global Transaction IDs
    static const QStringList REPLICATION_FEATURES;   // Replication features
    static const QStringList PERFORMANCE_FEATURES;   // Performance schema features
};

// MariaDB-specific database objects
struct MariaDBObjects {
    static const QStringList SYSTEM_DATABASES;
    static const QStringList SYSTEM_SCHEMAS;
    static const QStringList INFORMATION_SCHEMA_VIEWS;
    static const QStringList PERFORMANCE_SCHEMA_VIEWS;
    static const QStringList MYSQL_SCHEMA_TABLES;
    static const QStringList BUILTIN_FUNCTIONS;
    static const QStringList BUILTIN_OPERATORS;

    // MariaDB-specific system objects
    static const QStringList SEQUENCE_OBJECTS;       // Sequence objects
    static const QStringList DYNAMIC_COLUMN_FUNCTIONS; // Dynamic column functions
    static const QStringList JSON_FUNCTIONS;         // JSON functions
    static const QStringList SPATIAL_FUNCTIONS;      // Spatial functions
    static const QStringList UUID_FUNCTIONS;         // UUID functions
    static const QStringList ENCRYPTION_FUNCTIONS;   // Encryption functions
};

// MariaDB-specific configuration
struct MariaDBConfig {
    static const QStringList CONFIG_PARAMETERS;
    static const QStringList CONFIG_CATEGORIES;
    static const QStringList STORAGE_ENGINES;
    static const QStringList ISOLATION_LEVELS;
    static const QStringList CHARACTER_SETS;
    static const QStringList COLLATIONS;

    // MariaDB specific settings
    static const QStringList SQL_MODES;
    static const QStringList TIME_ZONES;
    static const QStringList LOG_LEVELS;
    static const QStringList OPTIMIZER_SWITCHES;
};

// MariaDB feature detection and support
class MariaDBFeatureDetector {
public:
    static bool isFeatureSupported(DatabaseType dbType, const QString& feature);
    static QStringList getSupportedFeatures(DatabaseType dbType);
    static QString getVersionSpecificSyntax(DatabaseType dbType, const QString& version);

    // Detect MariaDB-specific capabilities
    static bool supportsJSON(DatabaseType dbType);
    static bool supportsSequences(DatabaseType dbType);
    static bool supportsVirtualColumns(DatabaseType dbType);
    static bool supportsDynamicColumns(DatabaseType dbType);
    static bool supportsWindowFunctions(DatabaseType dbType);
    static bool supportsCTEs(DatabaseType dbType);
    static bool supportsSpatial(DatabaseType dbType);
    static bool supportsPartitioning(DatabaseType dbType);
    static bool supportsGTID(DatabaseType dbType);
    static bool supportsPerformanceSchema(DatabaseType dbType);
    static bool supportsReplication(DatabaseType dbType);
    static bool supportsEncryption(DatabaseType dbType);

    // Version-specific feature detection
    static bool supportsFeatureByVersion(const QString& version, const QString& feature);
    static QString getMinimumVersionForFeature(const QString& feature);
};

} // namespace scratchrobin
