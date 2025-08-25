#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <memory>
#include <vector>
#include "database_driver_manager.h"

namespace scratchrobin {

// MSSQL System Catalog and Metadata Support
class MSSQLCatalog {
public:
    // System database queries
    static QString getSystemDatabasesQuery();
    static QString getUserDatabasesQuery();
    static QString getDatabaseInfoQuery(const QString& database = "");

    // Schema and object queries
    static QString getSchemasQuery();
    static QString getTablesQuery(const QString& schema = "");
    static QString getViewsQuery(const QString& schema = "");
    static QString getColumnsQuery(const QString& table = "", const QString& schema = "");
    static QString getIndexesQuery(const QString& table = "", const QString& schema = "");
    static QString getConstraintsQuery(const QString& table = "", const QString& schema = "");
    static QString getForeignKeysQuery(const QString& table = "", const QString& schema = "");

    // Advanced object queries
    static QString getStoredProceduresQuery(const QString& schema = "");
    static QString getFunctionsQuery(const QString& schema = "");
    static QString getTriggersQuery(const QString& table = "", const QString& schema = "");
    static QString getSequencesQuery(const QString& schema = "");

    // System information queries
    static QString getServerInfoQuery();
    static QString getDatabasePropertiesQuery();
    static QString getSecurityInfoQuery();
    static QString getBackupHistoryQuery();
    static QString getJobInfoQuery();

    // Performance and monitoring queries
    static QString getActiveConnectionsQuery();
    static QString getPerformanceCountersQuery();
    static QString getQueryStatsQuery();
    static QString getIndexUsageQuery();
    static QString getMissingIndexesQuery();

    // Metadata discovery
    static QStringList getSystemTableList();
    static QStringList getSystemViewList();
    static QStringList getInformationSchemaViewList();
    static QStringList getDynamicManagementViewList();

    // Utility functions
    static QString formatObjectName(const QString& schema, const QString& object);
    static QString escapeIdentifier(const QString& identifier);
    static QString getVersionSpecificQuery(const QString& baseQuery, int majorVersion);

    // Feature detection queries
    static QString getVersionInfoQuery();
    static QString getFeatureSupportQuery(const QString& feature);
};

// MSSQL Query Builder for common operations
class MSSQLQueryBuilder {
public:
    // DDL operations
    static QString buildCreateTableQuery(const QString& tableName, const QList<QPair<QString, QString>>& columns,
                                       const QString& schema = "dbo");
    static QString buildAlterTableQuery(const QString& tableName, const QString& operation,
                                      const QString& schema = "dbo");
    static QString buildDropTableQuery(const QString& tableName, const QString& schema = "dbo");

    // Index operations
    static QString buildCreateIndexQuery(const QString& indexName, const QString& tableName,
                                       const QStringList& columns, bool unique = false,
                                       const QString& schema = "dbo");
    static QString buildDropIndexQuery(const QString& indexName, const QString& tableName,
                                     const QString& schema = "dbo");

    // Constraint operations
    static QString buildAddConstraintQuery(const QString& constraintName, const QString& tableName,
                                         const QString& constraintType, const QString& definition,
                                         const QString& schema = "dbo");
    static QString buildDropConstraintQuery(const QString& constraintName, const QString& tableName,
                                          const QString& schema = "dbo");

    // Data operations
    static QString buildSelectQuery(const QStringList& columns, const QString& tableName,
                                  const QString& whereClause = "", const QString& orderBy = "",
                                  int limit = -1, const QString& schema = "dbo");
    static QString buildInsertQuery(const QString& tableName, const QStringList& columns,
                                  const QString& schema = "dbo");
    static QString buildUpdateQuery(const QString& tableName, const QStringList& columns,
                                  const QString& whereClause, const QString& schema = "dbo");
    static QString buildDeleteQuery(const QString& tableName, const QString& whereClause,
                                  const QString& schema = "dbo");

    // Utility functions
    static QString formatColumnList(const QStringList& columns);
    static QString formatValueList(const QVariantList& values);
    static QString buildWhereClause(const QMap<QString, QVariant>& conditions);
};

// MSSQL Data Type Mapper
class MSSQLDataTypeMapper {
public:
    // Map MSSQL types to generic types
    static QString mapToGenericType(const QString& mssqlType);
    static QString mapFromGenericType(const QString& genericType);

    // Type information queries
    static QString getTypeInfoQuery();
    static QString getColumnTypeQuery(const QString& table, const QString& column,
                                    const QString& schema = "dbo");

    // Type validation
    static bool isValidDataType(const QString& typeName);
    static bool isNumericType(const QString& typeName);
    static bool isStringType(const QString& typeName);
    static bool isDateTimeType(const QString& typeName);
    static bool isSpatialType(const QString& typeName);

    // Type conversion functions
    static QString getCastFunction(const QString& fromType, const QString& toType);
    static QString getConvertFunction(const QString& fromType, const QString& toType);
};

// MSSQL Security Manager
class MSSQLSecurityManager {
public:
    // User and role management
    static QString getUsersQuery();
    static QString getRolesQuery();
    static QString getPermissionsQuery(const QString& object = "", const QString& user = "");
    static QString getLoginInfoQuery();

    // Security operations
    static QString buildCreateUserQuery(const QString& username, const QString& password = "",
                                      const QString& defaultSchema = "dbo");
    static QString buildGrantPermissionQuery(const QString& permission, const QString& object,
                                           const QString& user, const QString& objectType = "OBJECT");
    static QString buildRevokePermissionQuery(const QString& permission, const QString& object,
                                            const QString& user, const QString& objectType = "OBJECT");

    // Audit queries
    static QString getAuditInfoQuery();
    static QString getLoginHistoryQuery();
};

// MSSQL Backup and Restore Manager
class MSSQLBackupManager {
public:
    // Backup operations
    static QString buildBackupDatabaseQuery(const QString& database, const QString& filename,
                                          const QString& backupType = "FULL");
    static QString buildBackupLogQuery(const QString& database, const QString& filename);

    // Restore operations
    static QString buildRestoreDatabaseQuery(const QString& database, const QString& filename,
                                           bool withReplace = false);

    // Backup management
    static QString getBackupDevicesQuery();
    static QString getBackupSetsQuery(const QString& database = "");
    static QString getRestoreHistoryQuery(const QString& database = "");
};

} // namespace scratchrobin
