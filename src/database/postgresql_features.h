#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include "database_driver_manager.h"

namespace scratchrobin {

// PostgreSQL-specific data types
struct PostgreSQLDataTypes {
    // Standard SQL types with PostgreSQL extensions
    static const QStringList NUMERIC_TYPES;
    static const QStringList STRING_TYPES;
    static const QStringList DATE_TIME_TYPES;
    static const QStringList BINARY_TYPES;
    static const QStringList BOOLEAN_TYPES;

    // PostgreSQL-specific types
    static const QStringList ARRAY_TYPES;
    static const QStringList JSON_TYPES;
    static const QStringList GEOMETRIC_TYPES;
    static const QStringList NETWORK_TYPES;
    static const QStringList TEXT_SEARCH_TYPES;
    static const QStringList RANGE_TYPES;
    static const QStringList UUID_TYPES;
    static const QStringList HSTORE_TYPES;

    // Get all PostgreSQL data types
    static QStringList getAllDataTypes();
};

// PostgreSQL-specific SQL features
struct PostgreSQLSQLFeatures {
    static const QStringList KEYWORDS;
    static const QStringList FUNCTIONS;
    static const QStringList OPERATORS;
    static const QStringList RESERVED_WORDS;

    // Advanced SQL constructs
    static const QStringList CTE_FEATURES;        // WITH, RECURSIVE
    static const QStringList WINDOW_FEATURES;     // OVER, PARTITION BY, ORDER BY
    static const QStringList ARRAY_FEATURES;      // [], ARRAY[], unnest(), etc.
    static const QStringList JSON_FEATURES;       // ->, ->>, #>, #>>, etc.
    static const QStringList TEXT_SEARCH_FEATURES; // @@, to_tsvector(), etc.
};

// PostgreSQL-specific database objects
struct PostgreSQLObjects {
    static const QStringList SYSTEM_SCHEMAS;
    static const QStringList CATALOG_TABLES;
    static const QStringList INFORMATION_SCHEMA_VIEWS;
    static const QStringList BUILTIN_FUNCTIONS;
    static const QStringList BUILTIN_OPERATORS;
};

// PostgreSQL-specific configuration
struct PostgreSQLConfig {
    static const QStringList CONFIG_PARAMETERS;
    static const QStringList CONFIG_CATEGORIES;
    static const QStringList LOG_LEVELS;
    static const QStringList ISOLATION_LEVELS;
};

// PostgreSQL feature detection and support
class PostgreSQLFeatureDetector {
public:
    static bool isFeatureSupported(DatabaseType dbType, const QString& feature);
    static QStringList getSupportedFeatures(DatabaseType dbType);
    static QString getVersionSpecificSyntax(DatabaseType dbType, const QString& version);

    // Detect PostgreSQL-specific capabilities
    static bool supportsArrays(DatabaseType dbType);
    static bool supportsJSON(DatabaseType dbType);
    static bool supportsHStore(DatabaseType dbType);
    static bool supportsGeometry(DatabaseType dbType);
    static bool supportsTextSearch(DatabaseType dbType);
    static bool supportsRanges(DatabaseType dbType);
    static bool supportsCTEs(DatabaseType dbType);
    static bool supportsWindowFunctions(DatabaseType dbType);
    static bool supportsInheritance(DatabaseType dbType);
    static bool supportsPartitioning(DatabaseType dbType);

    // Feature support checking
    static bool supportsFeatureByVersion(const QString& version, const QString& feature);
    static QString getMinimumVersionForFeature(const QString& feature);
};

} // namespace scratchrobin
