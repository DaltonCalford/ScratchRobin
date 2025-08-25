#include "mssql_features.h"
#include <QRegularExpression>

namespace scratchrobin {

// MSSQL Data Types Implementation
const QStringList MSSQLDataTypes::NUMERIC_TYPES = {
    "bigint", "int", "smallint", "tinyint", "decimal", "numeric",
    "float", "real", "money", "smallmoney", "bit"
};

const QStringList MSSQLDataTypes::STRING_TYPES = {
    "char", "varchar", "text", "nchar", "nvarchar", "ntext"
};

const QStringList MSSQLDataTypes::DATE_TIME_TYPES = {
    "datetime", "datetime2", "smalldatetime", "date", "time",
    "datetimeoffset", "timestamp"
};

const QStringList MSSQLDataTypes::BINARY_TYPES = {
    "binary", "varbinary", "image"
};

const QStringList MSSQLDataTypes::BOOLEAN_TYPES = {
    "bit"
};

const QStringList MSSQLDataTypes::MONETARY_TYPES = {
    "money", "smallmoney"
};

const QStringList MSSQLDataTypes::UNIQUEIDENTIFIER_TYPES = {
    "uniqueidentifier"
};

const QStringList MSSQLDataTypes::SPATIAL_TYPES = {
    "geometry", "geography"
};

const QStringList MSSQLDataTypes::HIERARCHY_TYPES = {
    "hierarchyid"
};

const QStringList MSSQLDataTypes::XML_TYPES = {
    "xml"
};

QStringList MSSQLDataTypes::getAllDataTypes() {
    QStringList allTypes;
    allTypes << NUMERIC_TYPES << STRING_TYPES << DATE_TIME_TYPES
             << BINARY_TYPES << BOOLEAN_TYPES << MONETARY_TYPES
             << UNIQUEIDENTIFIER_TYPES << SPATIAL_TYPES
             << HIERARCHY_TYPES << XML_TYPES;
    return allTypes;
}

// MSSQL SQL Features Implementation
const QStringList MSSQLSQLFeatures::KEYWORDS = {
    "ADD", "ALL", "ALTER", "AND", "ANY", "AS", "ASC", "AUTHORIZATION",
    "BACKUP", "BEGIN", "BETWEEN", "BREAK", "BROWSE", "BULK", "BY", "CASCADE",
    "CASE", "CHECK", "CHECKPOINT", "CLOSE", "CLUSTERED", "COALESCE", "COLLATE",
    "COLUMN", "COMMIT", "COMPUTE", "CONSTRAINT", "CONTAINS", "CONTAINSTABLE",
    "CONTINUE", "CONVERT", "CREATE", "CROSS", "CURRENT", "CURRENT_DATE",
    "CURRENT_TIME", "CURRENT_TIMESTAMP", "CURRENT_USER", "CURSOR", "DATABASE",
    "DBCC", "DEALLOCATE", "DECLARE", "DEFAULT", "DELETE", "DENY", "DESC",
    "DISK", "DISTINCT", "DISTRIBUTED", "DOUBLE", "DROP", "DUMP", "ELSE",
    "END", "ERRLVL", "ESCAPE", "EXCEPT", "EXEC", "EXECUTE", "EXISTS",
    "EXIT", "EXPRESSION", "EXTERNAL", "FETCH", "FILE", "FILLFACTOR", "FOR",
    "FOREIGN", "FREETEXT", "FREETEXTTABLE", "FROM", "FULL", "FUNCTION", "GOTO",
    "GRANT", "GROUP", "HAVING", "HOLDLOCK", "IDENTITY", "IDENTITYCOL",
    "IDENTITY_INSERT", "IF", "IN", "INDEX", "INNER", "INSERT", "INSTEAD",
    "INTERSECT", "INTO", "IS", "JOIN", "KEY", "KILL", "LEFT", "LIKE",
    "LINENO", "LOAD", "MERGE", "NATIONAL", "NOCHECK", "NONCLUSTERED",
    "NOT", "NULL", "NULLIF", "OF", "OFF", "OFFSETS", "ON", "OPEN",
    "OPENDATASOURCE", "OPENQUERY", "OPENROWSET", "OPENXML", "OPTION", "OR",
    "ORDER", "OUTER", "OVER", "PERCENT", "PIVOT", "PLAN", "PRECISION",
    "PRIMARY", "PRINT", "PROC", "PROCEDURE", "PUBLIC", "RAISERROR", "READ",
    "READTEXT", "RECONFIGURE", "REFERENCES", "REPLICATION", "RESTORE",
    "RESTRICT", "RETURN", "REVERT", "REVOKE", "RIGHT", "ROLLBACK", "ROWCOUNT",
    "ROWGUIDCOL", "RULE", "SAVE", "SCHEMA", "SECURITYAUDIT", "SELECT",
    "SEMANTICKEYPHRASETABLE", "SEMANTICSIMILARITYDETAILSTABLE",
    "SEMANTICSIMILARITYTABLE", "SESSION_USER", "SET", "SETUSER", "SHUTDOWN",
    "SOME", "STATISTICS", "SYSTEM_USER", "TABLE", "TABLESAMPLE", "TEXTSIZE",
    "THEN", "TO", "TOP", "TRAN", "TRANSACTION", "TRIGGER", "TRUNCATE",
    "TSEQUAL", "UNION", "UNIQUE", "UNPIVOT", "UPDATE", "UPDATETEXT", "USE",
    "USER", "VALUES", "VARYING", "VIEW", "WAITFOR", "WHEN", "WHERE", "WHILE",
    "WITH", "WITHIN", "WRITETEXT"
};

const QStringList MSSQLSQLFeatures::FUNCTIONS = {
    // String functions
    "ASCII", "CHAR", "CHARINDEX", "CONCAT", "DATALENGTH", "DIFFERENCE",
    "FORMAT", "LEFT", "LEN", "LOWER", "LTRIM", "NCHAR", "PATINDEX",
    "QUOTENAME", "REPLACE", "REPLICATE", "REVERSE", "RIGHT", "RTRIM",
    "SOUNDEX", "SPACE", "STR", "STUFF", "SUBSTRING", "UNICODE", "UPPER",

    // Date functions
    "DATEADD", "DATEDIFF", "DATEFROMPARTS", "DATENAME", "DATEPART",
    "DAY", "GETDATE", "GETUTCDATE", "MONTH", "SMALLDATETIMEFROMPARTS",
    "SYSDATETIME", "SYSDATETIMEOFFSET", "SYSUTCDATETIME", "TIMEFROMPARTS",
    "YEAR",

    // Math functions
    "ABS", "ACOS", "ASIN", "ATAN", "ATN2", "CEILING", "COS", "COT",
    "DEGREES", "EXP", "FLOOR", "LOG", "LOG10", "PI", "POWER", "RADIANS",
    "RAND", "ROUND", "SIGN", "SIN", "SQRT", "TAN",

    // Aggregate functions
    "AVG", "CHECKSUM_AGG", "COUNT", "COUNT_BIG", "GROUPING", "GROUPING_ID",
    "MAX", "MIN", "STDEV", "STDEVP", "SUM", "VAR", "VARP", "STRING_AGG",

    // Ranking functions
    "DENSE_RANK", "NTILE", "RANK", "ROW_NUMBER",

    // JSON functions (SQL Server 2016+)
    "ISJSON", "JSON_VALUE", "JSON_QUERY", "JSON_MODIFY",

    // Other functions
    "CAST", "CONVERT", "COALESCE", "IIF", "ISNULL", "NULLIF", "CHOOSE"
};

const QStringList MSSQLSQLFeatures::OPERATORS = {
    // Comparison operators
    "=", ">", "<", ">=", "<=", "<>", "!=", "!>", "!<",

    // Arithmetic operators
    "+", "-", "*", "/", "%",

    // Logical operators
    "AND", "OR", "NOT", "EXISTS", "BETWEEN", "IN", "LIKE", "IS",

    // String operators
    "+", "%", "LIKE", "ESCAPE",

    // Set operators
    "UNION", "UNION ALL", "EXCEPT", "INTERSECT",

    // Assignment operator
    "=",

    // Special operators
    "::", ".", "->", "->>", "#>", "#>>", "@@", "<@", "@>", "<#", "#"
};

const QStringList MSSQLSQLFeatures::RESERVED_WORDS = {
    "ADD", "ALL", "ALTER", "AND", "ANY", "AS", "ASC", "AUTHORIZATION",
    "BACKUP", "BEGIN", "BETWEEN", "BREAK", "BROWSE", "BULK", "BY", "CASCADE",
    "CASE", "CHECK", "CHECKPOINT", "CLOSE", "CLUSTERED", "COALESCE", "COLLATE",
    "COLUMN", "COMMIT", "COMPUTE", "CONSTRAINT", "CONTAINS", "CONTAINSTABLE",
    "CONTINUE", "CONVERT", "CREATE", "CROSS", "CURRENT", "CURRENT_DATE",
    "CURRENT_TIME", "CURRENT_TIMESTAMP", "CURRENT_USER", "CURSOR", "DATABASE",
    "DBCC", "DEALLOCATE", "DECLARE", "DEFAULT", "DELETE", "DENY", "DESC",
    "DISK", "DISTINCT", "DISTRIBUTED", "DOUBLE", "DROP", "DUMP", "ELSE",
    "END", "ERRLVL", "ESCAPE", "EXCEPT", "EXEC", "EXECUTE", "EXISTS",
    "EXIT", "EXPRESSION", "EXTERNAL", "FETCH", "FILE", "FILLFACTOR", "FOR",
    "FOREIGN", "FREETEXT", "FREETEXTTABLE", "FROM", "FULL", "FUNCTION", "GOTO",
    "GRANT", "GROUP", "HAVING", "HOLDLOCK", "IDENTITY", "IDENTITYCOL",
    "IDENTITY_INSERT", "IF", "IN", "INDEX", "INNER", "INSERT", "INSTEAD",
    "INTERSECT", "INTO", "IS", "JOIN", "KEY", "KILL", "LEFT", "LIKE",
    "LINENO", "LOAD", "MERGE", "NATIONAL", "NOCHECK", "NONCLUSTERED",
    "NOT", "NULL", "NULLIF", "OF", "OFF", "OFFSETS", "ON", "OPEN",
    "OPENDATASOURCE", "OPENQUERY", "OPENROWSET", "OPENXML", "OPTION", "OR",
    "ORDER", "OUTER", "OVER", "PERCENT", "PIVOT", "PLAN", "PRECISION",
    "PRIMARY", "PRINT", "PROC", "PROCEDURE", "PUBLIC", "RAISERROR", "READ",
    "READTEXT", "RECONFIGURE", "REFERENCES", "REPLICATION", "RESTORE",
    "RESTRICT", "RETURN", "REVERT", "REVOKE", "RIGHT", "ROLLBACK", "ROWCOUNT",
    "ROWGUIDCOL", "RULE", "SAVE", "SCHEMA", "SECURITYAUDIT", "SELECT",
    "SEMANTICKEYPHRASETABLE", "SEMANTICSIMILARITYDETAILSTABLE",
    "SEMANTICSIMILARITYTABLE", "SESSION_USER", "SET", "SETUSER", "SHUTDOWN",
    "SOME", "STATISTICS", "SYSTEM_USER", "TABLE", "TABLESAMPLE", "TEXTSIZE",
    "THEN", "TO", "TOP", "TRAN", "TRANSACTION", "TRIGGER", "TRUNCATE",
    "TSEQUAL", "UNION", "UNIQUE", "UNPIVOT", "UPDATE", "UPDATETEXT", "USE",
    "USER", "VALUES", "VARYING", "VIEW", "WAITFOR", "WHEN", "WHERE", "WHILE",
    "WITH", "WITHIN", "WRITETEXT"
};

const QStringList MSSQLSQLFeatures::CTE_FEATURES = {
    "WITH", "RECURSIVE", "AS", "UNION ALL"
};

const QStringList MSSQLSQLFeatures::WINDOW_FEATURES = {
    "OVER", "PARTITION BY", "ORDER BY", "ROWS", "RANGE", "UNBOUNDED PRECEDING",
    "UNBOUNDED FOLLOWING", "CURRENT ROW", "PRECEDING", "FOLLOWING"
};

const QStringList MSSQLSQLFeatures::PIVOT_FEATURES = {
    "PIVOT", "UNPIVOT", "FOR", "IN"
};

const QStringList MSSQLSQLFeatures::MERGE_FEATURES = {
    "MERGE", "USING", "WHEN MATCHED", "WHEN NOT MATCHED", "THEN"
};

const QStringList MSSQLSQLFeatures::SEQUENCE_FEATURES = {
    "CREATE SEQUENCE", "NEXT VALUE FOR", "ALTER SEQUENCE", "DROP SEQUENCE"
};

const QStringList MSSQLSQLFeatures::XML_FEATURES = {
    "XML", "nodes()", "value()", "query()", "exist()", "modify()", "nodes()"
};

const QStringList MSSQLSQLFeatures::SPATIAL_FEATURES = {
    "GEOMETRY", "GEOGRAPHY", "STGeomFromText", "STAsText", "STDistance",
    "STContains", "STIntersects", "STBuffer", "STArea", "STLength"
};

const QStringList MSSQLSQLFeatures::HIERARCHY_FEATURES = {
    "HIERARCHYID", "GetAncestor", "GetDescendant", "GetLevel", "GetRoot",
    "IsDescendantOf", "Parse", "ToString"
};

const QStringList MSSQLSQLFeatures::JSON_FEATURES = {
    "ISJSON", "JSON_VALUE", "JSON_QUERY", "JSON_MODIFY", "OPENJSON", "FOR JSON"
};

const QStringList MSSQLSQLFeatures::STRING_AGG_FEATURES = {
    "STRING_AGG", "WITHIN GROUP"
};

const QStringList MSSQLSQLFeatures::OFFSET_FETCH_FEATURES = {
    "OFFSET", "FETCH", "NEXT", "ROWS", "ONLY"
};

// MSSQL Objects Implementation
const QStringList MSSQLObjects::SYSTEM_DATABASES = {
    "master", "model", "msdb", "tempdb", "distribution"
};

const QStringList MSSQLObjects::SYSTEM_SCHEMAS = {
    "dbo", "guest", "INFORMATION_SCHEMA", "sys", "db_owner", "db_accessadmin",
    "db_securityadmin", "db_ddladmin", "db_backupoperator", "db_datareader",
    "db_datawriter", "db_denydatareader", "db_denydatawriter"
};

const QStringList MSSQLObjects::SYSTEM_VIEWS = {
    "sys.databases", "sys.tables", "sys.columns", "sys.indexes", "sys.objects",
    "sys.schemas", "sys.types", "sys.procedures", "sys.views", "sys.triggers",
    "sys.foreign_keys", "sys.key_constraints", "sys.check_constraints",
    "sys.default_constraints", "sys.sequences", "sys.partitions"
};

const QStringList MSSQLObjects::SYSTEM_PROCEDURES = {
    "sp_help", "sp_helpdb", "sp_helprole", "sp_helpserver", "sp_helprotect",
    "sp_helpuser", "sp_addlogin", "sp_adduser", "sp_changedbowner", "sp_dropuser"
};

const QStringList MSSQLObjects::INFORMATION_SCHEMA_VIEWS = {
    "INFORMATION_SCHEMA.TABLES", "INFORMATION_SCHEMA.COLUMNS",
    "INFORMATION_SCHEMA.VIEWS", "INFORMATION_SCHEMA.ROUTINES",
    "INFORMATION_SCHEMA.KEY_COLUMN_USAGE", "INFORMATION_SCHEMA.TABLE_CONSTRAINTS"
};

const QStringList MSSQLObjects::BUILTIN_FUNCTIONS = {
    // Configuration functions
    "@@CONNECTIONS", "@@CPU_BUSY", "@@ERROR", "@@IDLE", "@@IO_BUSY",
    "@@PACKET_ERRORS", "@@PACK_RECEIVED", "@@PACK_SENT", "@@TIMETICKS",
    "@@TOTAL_ERRORS", "@@TOTAL_READ", "@@TOTAL_WRITE", "@@VERSION",

    // Metadata functions
    "APP_NAME", "ASSEMBLYPROPERTY", "COL_LENGTH", "COL_NAME", "COLUMNPROPERTY",
    "DATABASE_PRINCIPAL_ID", "DATABASEPROPERTY", "DATABASEPROPERTYEX",
    "DB_ID", "DB_NAME", "FILE_ID", "FILE_NAME", "FILEGROUP_ID", "FILEGROUP_NAME",
    "FILEGROUPPROPERTY", "FILEPROPERTY", "FULLTEXTCATALOGPROPERTY",
    "FULLTEXTSERVICEPROPERTY", "INDEX_COL", "INDEXPROPERTY", "NEXT VALUE FOR",
    "OBJECT_DEFINITION", "OBJECT_ID", "OBJECT_NAME", "OBJECT_SCHEMA_NAME",
    "OBJECTPROPERTY", "OBJECTPROPERTYEX", "ORIGINAL_DB_NAME", "PARSENAME",
    "SCHEMA_ID", "SCHEMA_NAME", "SCOPE_IDENTITY", "SERVERPROPERTY",
    "STATS_DATE", "TYPE_ID", "TYPE_NAME", "TYPEPROPERTY"
};

const QStringList MSSQLObjects::BUILTIN_OPERATORS = {
    // Comparison operators
    "=", ">", "<", ">=", "<=", "<>", "!=", "!>", "!<",

    // Arithmetic operators
    "+", "-", "*", "/", "%",

    // Logical operators
    "ALL", "AND", "ANY", "BETWEEN", "EXISTS", "IN", "LIKE", "NOT", "OR", "SOME",

    // String concatenation
    "+",

    // Unary operators
    "+", "-", "~",

    // Assignment operator
    "=",

    // Special operators
    "::", ".", "->", "->>", "#>", "#>>", "@@", "<@", "@>", "<#", "#"
};

const QStringList MSSQLObjects::DMV_VIEWS = {
    "sys.dm_exec_connections", "sys.dm_exec_sessions", "sys.dm_exec_requests",
    "sys.dm_exec_query_stats", "sys.dm_exec_query_plan", "sys.dm_exec_sql_text",
    "sys.dm_os_performance_counters", "sys.dm_os_wait_stats", "sys.dm_os_memory_objects",
    "sys.dm_db_index_usage_stats", "sys.dm_db_missing_index_details",
    "sys.dm_db_missing_index_groups", "sys.dm_db_missing_index_group_stats"
};

const QStringList MSSQLObjects::DMF_FUNCTIONS = {
    "sys.dm_exec_sql_text", "sys.dm_exec_query_plan", "sys.dm_exec_query_stats",
    "sys.dm_db_index_physical_stats", "sys.dm_db_index_operational_stats"
};

const QStringList MSSQLObjects::SYSTEM_TABLES = {
    "sys.sysobjects", "sys.syscolumns", "sys.sysindexes", "sys.systypes",
    "sys.sysusers", "sys.sysdatabases", "sys.syslogins", "sys.sysprocesses"
};

// MSSQL Config Implementation
const QStringList MSSQLConfig::CONFIG_PARAMETERS = {
    "backup compression default", "blocked process threshold", "clr enabled",
    "cost threshold for parallelism", "cursor threshold", "Database Mail XPs",
    "default full-text language", "default language", "default trace enabled",
    "disallow results from triggers", "fill factor (%)", "index create memory (KB)",
    "in-doubt xact resolution", "lightweight pooling", "locks", "max degree of parallelism",
    "max full-text crawl range", "max server memory (MB)", "max text repl size (B)",
    "max worker threads", "media retention", "min memory per query (KB)",
    "min server memory (MB)", "nested triggers", "network packet size (B)",
    "Ole Automation Procedures", "open objects", "optimize for ad hoc workloads",
    "PH timeout (s)", "precompute rank", "priority boost", "query governor cost limit",
    "query wait (s)", "recovery interval (min)", "remote access", "remote admin connections",
    "remote login timeout (s)", "remote proc trans", "remote query timeout (s)",
    "Replication XPs", "scan for startup procs", "server trigger recursion",
    "set working set size", "show advanced options", "SMO and DMO XPs", "SQL Mail XPs",
    "transform noise words", "two digit year cutoff", "user connections",
    "user options", "xp_cmdshell"
};

const QStringList MSSQLConfig::CONFIG_CATEGORIES = {
    "Database Settings", "Memory", "Processors", "Security", "Connections",
    "Advanced", "Filestream", "Backup/Restore", "Replication", "Full-text Search"
};

const QStringList MSSQLConfig::RECOVERY_MODELS = {
    "Simple", "Full", "Bulk-logged"
};

const QStringList MSSQLConfig::COMPATIBILITY_LEVELS = {
    "80", "90", "100", "110", "120", "130", "140", "150"
};

const QStringList MSSQLConfig::ISOLATION_LEVELS = {
    "Read uncommitted", "Read committed", "Repeatable read", "Serializable",
    "Snapshot", "Read committed snapshot"
};

const QStringList MSSQLConfig::COLLATION_NAMES = {
    "SQL_Latin1_General_CP1_CI_AS", "Latin1_General_CI_AS", "SQL_Latin1_General_CP1_CS_AS",
    "Latin1_General_CS_AS", "Chinese_PRC_CI_AS", "Japanese_CI_AS", "Korean_Wansung_CI_AS"
};

const QStringList MSSQLConfig::AUTHENTICATION_MODES = {
    "Windows Authentication", "SQL Server and Windows Authentication"
};

const QStringList MSSQLConfig::CONNECTION_ENCRYPTION_OPTIONS = {
    "Optional", "Required", "Strict"
};

// MSSQL Feature Detector Implementation
bool MSSQLFeatureDetector::isFeatureSupported(DatabaseType dbType, const QString& feature) {
    if (dbType != DatabaseType::SQLSERVER && dbType != DatabaseType::MSSQL) {
        return false;
    }

    // Get version info and check feature support
    // For now, return true for basic features
    return getSupportedFeatures(dbType).contains(feature, Qt::CaseInsensitive);
}

QStringList MSSQLFeatureDetector::getSupportedFeatures(DatabaseType dbType) {
    if (dbType != DatabaseType::SQLSERVER && dbType != DatabaseType::MSSQL) {
        return QStringList();
    }

    QStringList features;
    features << "CTE" << "WINDOW_FUNCTIONS" << "PIVOT" << "MERGE" << "XML"
             << "SPATIAL" << "HIERARCHY" << "SEQUENCES" << "JSON" << "STRING_AGG"
             << "OFFSET_FETCH" << "FILESTREAM" << "IN_MEMORY_OLTP" << "COLUMNSTORE"
             << "TEMPORAL_TABLES" << "GRAPH_DATABASE";

    return features;
}

QString MSSQLFeatureDetector::getVersionSpecificSyntax(DatabaseType dbType, const QString& version) {
    if (dbType != DatabaseType::SQLSERVER && dbType != DatabaseType::MSSQL) {
        return QString();
    }

    // Parse version and return appropriate syntax
    QRegularExpression versionRegex("(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match = versionRegex.match(version);

    if (match.hasMatch()) {
        int major = match.captured(1).toInt();
        int minor = match.captured(2).toInt();

        // SQL Server 2016+ (version 13.0)
        if (major >= 13) {
            return "2016+";
        }
        // SQL Server 2014 (version 12.0)
        else if (major >= 12) {
            return "2014";
        }
        // SQL Server 2012 (version 11.0)
        else if (major >= 11) {
            return "2012";
        }
        // SQL Server 2008/2008R2 (version 10.0/10.5)
        else if (major >= 10) {
            return "2008";
        }
    }

    return "2005"; // Default fallback
}

bool MSSQLFeatureDetector::supportsXML(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsSpatial(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsHierarchy(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsJSON(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsCTEs(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsWindowFunctions(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsPivot(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsMerge(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsSequences(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsStringAgg(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsOffsetFetch(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsFilestream(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsInMemory(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsColumnstore(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsTemporal(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsGraph(DatabaseType dbType) {
    return dbType == DatabaseType::SQLSERVER || dbType == DatabaseType::MSSQL;
}

bool MSSQLFeatureDetector::supportsFeatureByVersion(const QString& version, const QString& feature) {
    QRegularExpression versionRegex("(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match = versionRegex.match(version);

    if (!match.hasMatch()) {
        return false;
    }

    int major = match.captured(1).toInt();
    QString minVersion = getMinimumVersionForFeature(feature);

    if (minVersion.isEmpty()) {
        return true; // Feature supported in all versions
    }

    QRegularExpressionMatch minMatch = versionRegex.match(minVersion);
    if (minMatch.hasMatch()) {
        int minMajor = minMatch.captured(1).toInt();
        return major >= minMajor;
    }

    return false;
}

QString MSSQLFeatureDetector::getMinimumVersionForFeature(const QString& feature) {
    static QMap<QString, QString> featureVersions = {
        {"JSON", "13.0.0.0"},           // SQL Server 2016
        {"STRING_AGG", "13.0.0.0"},     // SQL Server 2016
        {"TEMPORAL_TABLES", "13.0.0.0"}, // SQL Server 2016
        {"IN_MEMORY_OLTP", "12.0.0.0"}, // SQL Server 2014
        {"COLUMNSTORE", "11.0.0.0"},    // SQL Server 2012
        {"OFFSET_FETCH", "11.0.0.0"},   // SQL Server 2012
        {"SEQUENCES", "11.0.0.0"},      // SQL Server 2012
        {"SPATIAL", "10.0.0.0"},        // SQL Server 2008
        {"HIERARCHY", "10.0.0.0"},      // SQL Server 2008
        {"XML", "9.0.0.0"},             // SQL Server 2005
        {"CTE", "9.0.0.0"},             // SQL Server 2005
        {"PIVOT", "9.0.0.0"}            // SQL Server 2005
    };

    return featureVersions.value(feature.toUpper(), "");
}

} // namespace scratchrobin
