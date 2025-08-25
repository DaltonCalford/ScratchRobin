#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <memory>
#include <vector>
#include "database_driver_manager.h"
#include "postgresql_features.h"

namespace scratchrobin {

// PostgreSQL System Catalog and Metadata Support
class PostgreSQLCatalog {
public:
    // System database queries
    static QString getSystemDatabasesQuery();
    static QString getUserDatabasesQuery();
    static QString getDatabaseInfoQuery(const QString& database = "");

    // Schema and object queries
    static QString getSchemasQuery();
    static QString getTablesQuery(const QString& schema = "", const QString& database = "");
    static QString getViewsQuery(const QString& schema = "", const QString& database = "");
    static QString getColumnsQuery(const QString& table = "", const QString& schema = "", const QString& database = "");
    static QString getIndexesQuery(const QString& table = "", const QString& schema = "", const QString& database = "");
    static QString getConstraintsQuery(const QString& table = "", const QString& schema = "", const QString& database = "");
    static QString getForeignKeysQuery(const QString& table = "", const QString& schema = "", const QString& database = "");

    // Advanced object queries
    static QString getStoredProceduresQuery(const QString& schema = "", const QString& database = "");
    static QString getFunctionsQuery(const QString& schema = "", const QString& database = "");
    static QString getTriggersQuery(const QString& table = "", const QString& schema = "", const QString& database = "");
    static QString getSequencesQuery(const QString& schema = "", const QString& database = "");
    static QString getEventsQuery(const QString& schema = "", const QString& database = "");

    // System information queries
    static QString getServerInfoQuery();
    static QString getDatabasePropertiesQuery();
    static QString getSecurityInfoQuery();
    static QString getEngineInfoQuery();
    static QString getStorageEnginesQuery();
    static QString getCharsetInfoQuery();

    // Performance and monitoring queries
    static QString getProcessListQuery();
    static QString getPerformanceCountersQuery();
    static QString getQueryStatsQuery();
    static QString getIndexStatsQuery();
    static QString getTableStatsQuery();
    static QString getInnoDBStatsQuery();

    // Replication queries
    static QString getReplicationStatusQuery();
    static QString getSlaveStatusQuery();
    static QString getBinaryLogStatusQuery();

    // PostgreSQL-specific queries
    static QString getGlobalVariablesQuery();
    static QString getSessionVariablesQuery();
    static QString getSysSchemaSummaryQuery();
    static QString getInnoDBMetricsQuery();
    static QString getFulltextIndexQuery();

    // Metadata discovery
    static QStringList getSystemTableList();
    static QStringList getSystemViewList();
    static QStringList getInformationSchemaViewList();
    static QStringList getPerformanceSchemaViewList();
    static QStringList getMySQLSchemaTableList();
    static QStringList getSysSchemaViewList();

    // Utility functions
    static QString formatObjectName(const QString& schema, const QString& object, const QString& database = "");
    static QString escapeIdentifier(const QString& identifier);
    static QString getVersionSpecificQuery(const QString& baseQuery, int majorVersion, int minorVersion);

    // Feature detection queries
    static QString getVersionInfoQuery();
    static QString getFeatureSupportQuery(const QString& feature);
    static QString getEngineSupportQuery();
    static QString getPluginSupportQuery();
};

// PostgreSQL Query Builder for common operations
class PostgreSQLQueryBuilder {
public:
    // DDL operations
    static QString buildCreateTableQuery(const QString& tableName, const QList<QPair<QString, QString>>& columns,
                                       const QString& schema = "", const QString& database = "", const QString& engine = "InnoDB");
    static QString buildAlterTableQuery(const QString& tableName, const QString& operation,
                                      const QString& schema = "", const QString& database = "");
    static QString buildDropTableQuery(const QString& tableName, const QString& schema = "", const QString& database = "");
    static QString buildCreateDatabaseQuery(const QString& database, const QString& charset = "utf8mb4", const QString& collation = "utf8mb4_general_ci");
    static QString buildDropDatabaseQuery(const QString& database);

    // Index operations
    static QString buildCreateIndexQuery(const QString& indexName, const QString& tableName,
                                       const QStringList& columns, bool unique = false, bool invisible = false,
                                       const QString& schema = "", const QString& database = "");
    static QString buildDropIndexQuery(const QString& indexName, const QString& tableName,
                                     const QString& schema = "", const QString& database = "");
    static QString buildCreateExpressionIndexQuery(const QString& indexName, const QString& tableName,
                                                 const QString& expression, const QString& schema = "", const QString& database = "");

    // Constraint operations
    static QString buildAddConstraintQuery(const QString& constraintName, const QString& tableName,
                                         const QString& constraintType, const QString& definition,
                                         const QString& schema = "", const QString& database = "");
    static QString buildDropConstraintQuery(const QString& constraintName, const QString& tableName,
                                          const QString& schema = "", const QString& database = "");

    // Generated column operations
    static QString buildAddGeneratedColumnQuery(const QString& tableName, const QString& columnName,
                                              const QString& expression, const QString& dataType, bool stored = false,
                                              const QString& schema = "", const QString& database = "");

    // Data operations
    static QString buildSelectQuery(const QStringList& columns, const QString& tableName,
                                  const QString& whereClause = "", const QString& orderBy = "",
                                  int limit = -1, const QString& schema = "", const QString& database = "");
    static QString buildInsertQuery(const QString& tableName, const QStringList& columns,
                                  const QString& schema = "", const QString& database = "");
    static QString buildUpdateQuery(const QString& tableName, const QStringList& columns,
                                  const QString& whereClause, const QString& schema = "", const QString& database = "");
    static QString buildDeleteQuery(const QString& tableName, const QString& whereClause,
                                  const QString& schema = "", const QString& database = "");

    // Utility functions
    static QString formatColumnList(const QStringList& columns);
    static QString formatValueList(const QVariantList& values);
    static QString buildWhereClause(const QMap<QString, QVariant>& conditions);
    static QString getEngineSpecificOptions(const QString& engine);
};

// PostgreSQL Data Type Mapper
class PostgreSQLDataTypeMapper {
public:
    // Map PostgreSQL types to generic types
    static QString mapToGenericType(const QString& postgresqlType);
    static QString mapFromGenericType(const QString& genericType);

    // Type information queries
    static QString getTypeInfoQuery();
    static QString getColumnTypeQuery(const QString& table, const QString& column,
                                    const QString& schema = "", const QString& database = "");

    // Type validation
    static bool isValidDataType(const QString& typeName);
    static bool isNumericType(const QString& typeName);
    static bool isStringType(const QString& typeName);
    static bool isDateTimeType(const QString& typeName);
    static bool isSpatialType(const QString& typeName);
    static bool isJsonType(const QString& typeName);

    // Type conversion functions
    static QString getCastFunction(const QString& fromType, const QString& toType);
    static QString getConvertFunction(const QString& fromType, const QString& toType);
};

// PostgreSQL Security Manager
class PostgreSQLSecurityManager {
public:
    // User and role management
    static QString getUsersQuery();
    static QString getRolesQuery();
    static QString getPermissionsQuery(const QString& user = "", const QString& host = "");
    static QString getUserPrivilegesQuery(const QString& user = "", const QString& host = "");

    // Security operations
    static QString buildCreateUserQuery(const QString& username, const QString& host = "localhost",
                                      const QString& password = "");
    static QString buildGrantPermissionQuery(const QString& permission, const QString& object,
                                           const QString& user, const QString& host = "localhost");
    static QString buildRevokePermissionQuery(const QString& permission, const QString& object,
                                            const QString& user, const QString& host = "localhost");
    static QString buildAlterUserQuery(const QString& username, const QString& host = "localhost",
                                     const QString& newPassword = "");

    // Audit queries
    static QString getAuditInfoQuery();
    static QString getLoginHistoryQuery();
    static QString getSecuritySettingsQuery();
};

// PostgreSQL Backup and Restore Manager
class PostgreSQLBackupManager {
public:
    // Backup operations
    static QString buildMysqldumpCommand(const QString& database, const QString& filename,
                                       const QString& username, const QString& password,
                                       const QString& host = "localhost", int port = 3306);
    static QString buildXtrabackupCommand(const QString& database, const QString& backupDir,
                                        const QString& username, const QString& password,
                                        const QString& host = "localhost", int port = 3306);

    // Restore operations
    static QString buildRestoreCommand(const QString& filename, const QString& database,
                                     const QString& username, const QString& password,
                                     const QString& host = "localhost", int port = 3306);

    // Backup management
    static QString getBackupHistoryQuery();
    static QString getBackupStatusQuery();
};

// PostgreSQL Performance Monitor
class PostgreSQLPerformanceMonitor {
public:
    // Performance monitoring queries
    static QString getSlowQueriesQuery(int limit = 100);
    static QString getRunningQueriesQuery();
    static QString getLockWaitsQuery();
    static QString getDeadlocksQuery();
    static QString getMemoryUsageQuery();
    static QString getDiskIOQuery();
    static QString getConnectionStatsQuery();

    // Performance tuning queries
    static QString getIndexRecommendationsQuery();
    static QString getUnusedIndexesQuery();
    static QString getDuplicateIndexesQuery();
    static QString getTableFragmentationQuery();
};

// PostgreSQL Enterprise Features
class PostgreSQLEnterpriseManager {
public:
    // Enterprise feature detection
    static QString getEnterpriseFeaturesQuery();
    static QString getAuditLogQuery();
    static QString getThreadPoolQuery();
    static QString getFirewallQuery();

    // Enterprise-specific queries
    static QString getQueryRewriteQuery();
    static QString getTransparentDataEncryptionQuery();
};

} // namespace scratchrobin
