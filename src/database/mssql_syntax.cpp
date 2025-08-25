#include "mssql_syntax.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

namespace scratchrobin {

// MSSQL Syntax Patterns Implementation

const QStringList MSSQLSyntaxPatterns::RESERVED_KEYWORDS = {
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

const QStringList MSSQLSyntaxPatterns::NON_RESERVED_KEYWORDS = {
    "ABORT", "ABORT_AFTER_WAIT", "ABSOLUTE", "ACCELERATED_DATABASE_RECOVERY",
    "ACCESS", "ACTION", "ACTIVATE", "ACTIVE", "ADD", "ADDRESS", "AES_128",
    "AES_192", "AES_256", "AFFINITY", "AFTER", "AGGREGATE", "ALGORITHM",
    "ALL_CONSTRAINTS", "ALL_ERRORMSGS", "ALL_INDEXES", "ALL_LEVELS", "ALLOW_CONNECTIONS",
    "ALLOW_MULTIPLE_EVENT_LOSS", "ALLOW_SINGLE_EVENT_LOSS", "ANONYMOUS", "APPEND",
    "APPLICATION", "APPLY", "ARITHABORT", "ARITHIGNORE", "ASSEMBLY", "ASYMMETRIC",
    "ASYNCHRONOUS_COMMIT", "ATOMIC", "ATTACH", "ATTACH_REBUILD_LOG", "AUDIT",
    "AUDIT_GUID", "AUTHENTICATION", "AUTHENTICATOR", "AUTO", "AUTO_CLEANUP",
    "AUTO_CLOSE", "AUTO_CREATE_STATISTICS", "AUTO_DROP", "AUTO_SHRINK",
    "AUTO_UPDATE_STATISTICS", "AUTOMATED_BACKUP_PREFERENCE", "AUTOMATIC",
    "AVAILABILITY", "AVAILABILITY_MODE", "BACKUP_PRIORITY", "BEFORE", "BEGIN_DIALOG",
    "BIGINT", "BINARY", "BINDING", "BIT", "BLOCKERS", "BLOCKING_HIERARCHY",
    "BLOCKSIZE", "BOUNDING_BOX", "BREAK", "BROKER", "BROKER_INSTANCE", "BULK",
    "BULK_LOGGED", "CACHE", "CALLED", "CALLER", "CAP_CPU_PERCENT", "CASCADE",
    "CATALOG", "CATCH", "CERTIFICATE", "CHANGE", "CHANGES", "CHAR", "CHARACTER",
    "CHECK_POLICY", "CHECK_EXPIRATION", "CHECKPOINT", "CHECKSUM", "CLASSIFIER",
    "CLEANUP", "CLEANUP_POLICY", "CLUSTER", "CLUSTERED", "CODEPAGE", "COLLATION",
    "COLLECTION", "COLUMN_ENCRYPTION_KEY", "COLUMN_MASTER_KEY", "COMMITTED",
    "COMPATIBILITY_LEVEL", "COMPRESSION", "CONCAT", "CONCAT_NULL_YIELDS_NULL",
    "CONFIGURATION", "CONNECT", "CONNECTION", "CONSTRAINT", "CONTAINMENT", "CONTENT",
    "CONTEXT", "CONTINUE_AFTER_ERROR", "CONTRACT", "CONVERSATION", "COOKIE",
    "COPY_ONLY", "CREATE", "CREDENTIAL", "CRYPTOGRAPHIC", "CUBE", "CURSOR_CLOSE_ON_COMMIT",
    "CURSOR_DEFAULT", "DATA", "DATA_COMPRESSION", "DATABASE", "DATABASE_MIRRORING",
    "DATE", "DATE_CORRELATION_OPTIMIZATION", "DATEFIRST", "DATELAST", "DATENAME",
    "DATEPART", "DAYS", "DB_CHAINING", "DB_FAILOVER", "DEADLOCK_PRIORITY", "DECRYPTION",
    "DEFAULT_DATABASE", "DEFAULT_FULLTEXT_LANGUAGE", "DEFAULT_LANGUAGE", "DEFAULT_SCHEMA",
    "DEFINITION", "DELAY", "DELAYED_DURABILITY", "DELETE", "DENSITY_VECTOR", "DES",
    "DESCRIPTION", "DESX", "DIAGNOSTIC", "DIALOG", "DIFFERENTIAL", "DIRECTORY",
    "DISABLE", "DISABLE_BROKER", "DISABLED", "DISK", "DISTRIBUTION", "DROP_EXISTING",
    "DTC_SUPPORT", "DYNAMIC", "ELEMENTS", "EMERGENCY", "EMPTY", "ENABLE", "ENABLE_BROKER",
    "ENCRYPTED", "ENCRYPTION", "ENCRYPTION_TYPE", "ENDPOINT", "ERROR", "ERROR_BROKER_CONVERSATIONS",
    "ESTIMATEONLY", "EVENT", "EVENT_RETENTION_MODE", "EXECUTABLE", "EXPIREDATE",
    "EXPIRYDATE", "EXPLICIT", "EXTENDED_LOGICAL_CHECKS", "EXTENSION", "EXTERNAL_ACCESS",
    "FAIL_OPERATION", "FAILOVER", "FAILURE_CONDITION_LEVEL", "FAST", "FAST_FORWARD",
    "FIELD_TERMINATOR", "FILEGROUP", "FILEGROWTH", "FILENAME", "FILEPATH", "FILESTREAM",
    "FILTER", "FIRE_TRIGGERS", "FIRST", "FIRSTROW", "FITS", "FOLLOWING", "FORCE",
    "FORCE_FAILOVER_ALLOW_DATA_LOSS", "FORCED", "FORMAT", "FORMATFILE", "FORMSOF",
    "FORWARD_ONLY", "FULLSCAN", "FULLTEXT", "FUNCTION", "GENERATED", "GEOGRAPHY",
    "GEOMETRY", "GET", "GETDATE", "GLOBAL", "GO", "GOTO", "GOVERNOR", "HASH",
    "HASHED", "HEALTH_CHECK_TIMEOUT", "HEAP", "HIERARCHYID", "HIGH", "HONOR_BROKER_PRIORITY",
    "HOURS", "IDENTITY", "IGNORE_CONSTRAINTS", "IGNORE_DUP_KEY", "IGNORE_TRIGGERS",
    "IMMEDIATE", "IMPERSONATE", "IMPORTANCE", "INCLUDE", "INCREMENTAL", "INCREMENT",
    "INDEX", "INDEXDEFRAG", "INFINITE", "INIT", "INITIATOR", "INPUT", "INSENSITIVE",
    "INSERT", "INSTEAD", "IO", "IP", "ISOLATION", "JOB", "JSON", "KEEP", "KEEP_CDC",
    "KEEP_NULLS", "KEEP_REPLICATION", "KEEPDEFAULTS", "KEEPFIXED", "KEEPIDENTITY",
    "KEY_PATH", "KEY_STORE_PROVIDER_NAME", "KILL", "LANGUAGE", "LAST", "LASTROW",
    "LEVEL", "LIFETIME", "LISTENER", "LISTENER_IP", "LISTENER_PORT", "LOAD", "LOCAL",
    "LOCATION", "LOCK_ESCALATION", "LOCK_TIMEOUT", "LOG", "LOGFILE", "LOGICAL",
    "LOGIN", "LOOP", "LOW", "MANUAL", "MARK", "MASKED", "MASTER", "MATCH", "MAX_MEMORY",
    "MAX_MEMORY_PERCENT", "MAXDOP", "MAXRECURSION", "MAXSIZE", "MEDIUM", "MEMORY_OPTIMIZED",
    "MESSAGE", "MIN_MEMORY", "MIN_MEMORY_PERCENT", "MINUTES", "MIRROR", "MIRROR_ADDRESS",
    "MIXED_PAGE_ALLOCATION", "MODE", "MODEL", "MODIFY", "MOVE", "MULTI_USER", "MUST_CHANGE",
    "NAME", "NESTED_TRIGGERS", "NEW_ACCOUNT", "NEW_BROKER", "NEW_PASSWORD", "NEXT",
    "NO", "NO_BROWSETABLE", "NO_CHECKSUM", "NO_COMPRESSION", "NO_EVENT_LOSS", "NO_INFOMSGS",
    "NO_TRUNCATE", "NO_WAIT", "NODE", "NON_TRANSACTED_ACCESS", "NUMERIC_ROUNDABORT",
    "OBJECT", "OFFLINE", "OFFSET", "OLD_ACCOUNT", "OLD_PASSWORD", "ONLINE", "ONLY",
    "OPEN_EXISTING", "OPTIMISTIC", "OPTIMIZE", "OUT", "OUTPUT", "OWNER", "PAGE",
    "PARAMETERIZATION", "PARTITION", "PARTITIONS", "PARTNER", "PASSWORD", "PATH",
    "PAUSE", "PERCENT", "PERMISSION_SET", "PERSISTED", "PLATFORM", "POLICY",
    "PRECEDING", "PRECISION", "PRIORITY", "PRIVATE", "PRIVATE_KEY", "PRIVILEGES",
    "PROCEDURE", "PROPERTY", "PROVIDER", "PROVIDER_KEY_NAME", "QUERY", "QUEUE",
    "QUOTED_IDENTIFIER", "RANGE", "RANK", "RC2", "RC4", "READ_COMMITTED_SNAPSHOT",
    "READ_ONLY", "READ_WRITE", "READCOMMITTED", "READCOMMITTEDLOCK", "READPAST",
    "READUNCOMMITTED", "READWRITE", "REBUILD", "RECEIVE", "RECOMPILE", "RECOVERY",
    "RECURSIVE", "RECOVERY", "RECURSIVE_TRIGGERS", "REFERENCES", "REGENERATE",
    "RELATED_CONVERSATION", "RELATED_CONVERSATION_GROUP", "RELOAD", "REMOTE",
    "REMOTE_PROC_TRANSACTIONS", "REMOTE_SERVICE_NAME", "REMOVE", "REORGANIZE",
    "REPEATABLE", "REPLICA", "REPLICATION", "REQUIRED", "RESAMPLE", "RESEED",
    "RESOURCE", "RESTART", "RESTRICT", "RESTRICTED_USER", "RESULT", "RESUME",
    "RETAINDAYS", "RETENTION", "RETURN", "RETURNS", "REVERT", "REVOKE", "REWIND",
    "ROBUST", "ROLE", "ROLLBACK", "ROLLUP", "ROOT", "ROUTE", "ROW", "ROWGUIDCOL",
    "ROWS", "ROWSET", "RULE", "SAFE", "SAFETY", "SAMPLE", "SCHEMA", "SCHEME",
    "SCROLL", "SCROLL_LOCKS", "SEARCH", "SECONDARY", "SECURITY", "SECURITY_LOG",
    "SEED", "SELF", "SEND", "SENT", "SEQUENCE", "SERIALIZABLE", "SERVER", "SERVICE",
    "SERVICE_BROKER", "SERVICE_NAME", "SESSION", "SESSION_TIMEOUT", "SET",
    "SETS", "SETUSER", "SHOWPLAN", "SIGNATURE", "SINGLE_BLOB", "SINGLE_CLOB",
    "SINGLE_NCLOB", "SINGLE_USER", "SIZE", "SMALLINT", "SNAPSHOT", "SOAP",
    "SORT_IN_TEMPDB", "SOURCE", "SPATIAL", "SPLIT", "SQL", "SQL_VARIANT",
    "STANDBY", "START", "START_DATE", "STARTED", "STARTUP_STATE", "STATISTICS",
    "STATE", "STATIC", "STATISTICAL_SEMANTICS", "STATUS", "STOP", "STOPPED",
    "STOP_ON_ERROR", "SUPPORTED", "SUSPEND", "SWITCH", "SYMMETRIC", "SYNCHRONOUS_COMMIT",
    "SYNONYM", "SYSTEM", "TABLE", "TAKE", "TAPE", "TARGET", "TCP", "TEXT",
    "TIMEOUT", "TIMER", "TINYINT", "TO", "TRACE", "TRACKING", "TRANSACTION",
    "TRANSFER", "TRIGGER", "TRIPLE_DES", "TRUNCATE", "TSEQUAL", "TSQL",
    "TWO_DIGIT_YEAR_CUTOFF", "TYPE", "TYPE_WARNING", "UNBOUNDED", "UNCOMMITTED",
    "UNION", "UNIQUE", "UNKNOWN", "UNLIMITED", "UNLOAD", "UNSAFE", "URL",
    "USED", "USE_TYPE_DEFAULT", "USING", "VALIDATION", "VALUE", "VARBINARY",
    "VARCHAR", "VARYING", "VERBOSELOGGING", "VERIFYONLY", "VERSION", "VIEW",
    "VIEWS", "WAIT", "WAITFOR", "WEEK", "WEIGHT", "WELL_FORMED_XML", "WHEN",
    "WINDOWS", "WITH", "WITHIN", "WITHOUT", "WITNESS", "WORK", "WORKLOAD",
    "XML", "XMLDATA", "XMLSCHEMA", "XSINIL", "ZONE"
};

const QStringList MSSQLSyntaxPatterns::DATA_TYPES = {
    "bigint", "int", "smallint", "tinyint", "decimal", "numeric", "float", "real",
    "money", "smallmoney", "bit", "char", "varchar", "text", "nchar", "nvarchar",
    "ntext", "datetime", "datetime2", "smalldatetime", "date", "time", "datetimeoffset",
    "timestamp", "binary", "varbinary", "image", "cursor", "sql_variant", "table",
    "uniqueidentifier", "geometry", "geography", "hierarchyid", "xml"
};

const QStringList MSSQLSyntaxPatterns::BUILTIN_FUNCTIONS = {
    // String functions
    "ASCII", "CHAR", "CHARINDEX", "CONCAT", "DATALENGTH", "DIFFERENCE", "FORMAT",
    "LEFT", "LEN", "LOWER", "LTRIM", "NCHAR", "PATINDEX", "QUOTENAME", "REPLACE",
    "REPLICATE", "REVERSE", "RIGHT", "RTRIM", "SOUNDEX", "SPACE", "STR", "STUFF",
    "SUBSTRING", "UNICODE", "UPPER",

    // Date functions
    "DATEADD", "DATEDIFF", "DATEFROMPARTS", "DATENAME", "DATEPART", "DAY", "GETDATE",
    "GETUTCDATE", "MONTH", "SMALLDATETIMEFROMPARTS", "SYSDATETIME", "SYSDATETIMEOFFSET",
    "SYSUTCDATETIME", "TIMEFROMPARTS", "YEAR",

    // Math functions
    "ABS", "ACOS", "ASIN", "ATAN", "ATN2", "CEILING", "COS", "COT", "DEGREES",
    "EXP", "FLOOR", "LOG", "LOG10", "PI", "POWER", "RADIANS", "RAND", "ROUND",
    "SIGN", "SIN", "SQRT", "TAN",

    // Aggregate functions
    "AVG", "CHECKSUM_AGG", "COUNT", "COUNT_BIG", "GROUPING", "GROUPING_ID",
    "MAX", "MIN", "STDEV", "STDEVP", "SUM", "VAR", "VARP", "STRING_AGG",

    // Ranking functions
    "DENSE_RANK", "NTILE", "RANK", "ROW_NUMBER",

    // JSON functions
    "ISJSON", "JSON_VALUE", "JSON_QUERY", "JSON_MODIFY", "OPENJSON",

    // Configuration functions
    "@@CONNECTIONS", "@@CPU_BUSY", "@@ERROR", "@@IDLE", "@@IO_BUSY", "@@PACKET_ERRORS",
    "@@PACK_RECEIVED", "@@PACK_SENT", "@@TIMETICKS", "@@TOTAL_ERRORS", "@@TOTAL_READ",
    "@@TOTAL_WRITE", "@@VERSION",

    // Metadata functions
    "APP_NAME", "ASSEMBLYPROPERTY", "COL_LENGTH", "COL_NAME", "COLUMNPROPERTY",
    "DATABASE_PRINCIPAL_ID", "DATABASEPROPERTY", "DATABASEPROPERTYEX", "DB_ID",
    "DB_NAME", "FILE_ID", "FILE_NAME", "FILEGROUP_ID", "FILEGROUP_NAME", "FILEGROUPPROPERTY",
    "FILEPROPERTY", "FULLTEXTCATALOGPROPERTY", "FULLTEXTSERVICEPROPERTY", "INDEX_COL",
    "INDEXPROPERTY", "NEXT VALUE FOR", "OBJECT_DEFINITION", "OBJECT_ID", "OBJECT_NAME",
    "OBJECT_SCHEMA_NAME", "OBJECTPROPERTY", "OBJECTPROPERTYEX", "ORIGINAL_DB_NAME",
    "PARSENAME", "SCHEMA_ID", "SCHEMA_NAME", "SCOPE_IDENTITY", "SERVERPROPERTY",
    "STATS_DATE", "TYPE_ID", "TYPE_NAME", "TYPEPROPERTY"
};

const QStringList MSSQLSyntaxPatterns::OPERATORS = {
    // Comparison operators
    "=", ">", "<", ">=", "<=", "<>", "!=", "!>", "!<",

    // Arithmetic operators
    "+", "-", "*", "/", "%",

    // Logical operators
    "ALL", "AND", "ANY", "BETWEEN", "EXISTS", "IN", "LIKE", "NOT", "OR", "SOME",

    // String operators
    "+", "%", "LIKE", "ESCAPE",

    // Set operators
    "UNION", "UNION ALL", "EXCEPT", "INTERSECT",

    // Assignment operator
    "=",

    // Special operators
    "::", ".", "->", "->>", "#>", "#>>", "@@", "<@", "@>", "<#", "#"
};

const QString MSSQLSyntaxPatterns::SINGLE_LINE_COMMENT = "--";
const QString MSSQLSyntaxPatterns::MULTI_LINE_COMMENT_START = "/*";
const QString MSSQLSyntaxPatterns::MULTI_LINE_COMMENT_END = "*/";
const QString MSSQLSyntaxPatterns::STRING_LITERAL = "'[^']*'";
const QString MSSQLSyntaxPatterns::IDENTIFIER = "[a-zA-Z_][a-zA-Z0-9_]*";
const QString MSSQLSyntaxPatterns::BRACKETED_IDENTIFIER = "\\[[^\\]]*\\]";
const QString MSSQLSyntaxPatterns::NUMBER_LITERAL = "\\b\\d+\\.?\\d*\\b";
const QString MSSQLSyntaxPatterns::VARIABLE = "@[a-zA-Z_][a-zA-Z0-9_]*";
const QString MSSQLSyntaxPatterns::TEMP_TABLE = "#[a-zA-Z_][a-zA-Z0-9_]*";
const QString MSSQLSyntaxPatterns::SYSTEM_OBJECT = "sys\\.[a-zA-Z_][a-zA-Z0-9_]*";

QStringList MSSQLSyntaxPatterns::getAllKeywords() {
    QStringList allKeywords;
    allKeywords << RESERVED_KEYWORDS << NON_RESERVED_KEYWORDS;
    return allKeywords;
}

QList<MSSQLSyntaxElement> MSSQLSyntaxPatterns::getAllSyntaxElements() {
    QList<MSSQLSyntaxElement> elements;

    // Add keywords
    for (const QString& keyword : RESERVED_KEYWORDS) {
        elements << MSSQLSyntaxElement(keyword, "\\b" + keyword + "\\b", "Reserved keyword", true, false, false, false);
    }

    for (const QString& keyword : NON_RESERVED_KEYWORDS) {
        elements << MSSQLSyntaxElement(keyword, "\\b" + keyword + "\\b", "Non-reserved keyword", true, false, false, false);
    }

    // Add data types
    for (const QString& dataType : DATA_TYPES) {
        elements << MSSQLSyntaxElement(dataType, "\\b" + dataType + "\\b", "Data type", false, false, false, true);
    }

    // Add functions
    for (const QString& function : BUILTIN_FUNCTIONS) {
        elements << MSSQLSyntaxElement(function, "\\b" + function + "\\b", "Built-in function", false, true, false, false);
    }

    // Add operators
    for (const QString& op : OPERATORS) {
        elements << MSSQLSyntaxElement(op, "\\b" + QRegularExpression::escape(op) + "\\b", "Operator", false, false, true, false);
    }

    // Add other patterns
    elements << MSSQLSyntaxElement("Single-line comment", SINGLE_LINE_COMMENT + ".*", "Single-line comment");
    elements << MSSQLSyntaxElement("Multi-line comment", MULTI_LINE_COMMENT_START + ".*?" + MULTI_LINE_COMMENT_END, "Multi-line comment");
    elements << MSSQLSyntaxElement("String literal", STRING_LITERAL, "String literal");
    elements << MSSQLSyntaxElement("Identifier", IDENTIFIER, "Regular identifier");
    elements << MSSQLSyntaxElement("Bracketed identifier", BRACKETED_IDENTIFIER, "Bracketed identifier");
    elements << MSSQLSyntaxElement("Number", NUMBER_LITERAL, "Numeric literal");
    elements << MSSQLSyntaxElement("Variable", VARIABLE, "Local variable");
    elements << MSSQLSyntaxElement("Temporary table", TEMP_TABLE, "Temporary table");
    elements << MSSQLSyntaxElement("System object", SYSTEM_OBJECT, "System object");

    return elements;
}

// MSSQL Parser Implementation

QList<MSSQLSyntaxElement> MSSQLParser::parseSQL(const QString& sql) {
    QList<MSSQLSyntaxElement> elements;
    QList<MSSQLSyntaxElement> patterns = MSSQLSyntaxPatterns::getAllSyntaxElements();

    for (const MSSQLSyntaxElement& pattern : patterns) {
        QRegularExpression regex(pattern.pattern, QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = regex.globalMatch(sql);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            MSSQLSyntaxElement element = pattern;
            element.name = match.captured();
            elements << element;
        }
    }

    return elements;
}

bool MSSQLParser::validateSQLSyntax(const QString& sql, QStringList& errors, QStringList& warnings) {
    errors.clear();
    warnings.clear();

    // Basic validation using MSSQLSyntaxValidator
    return MSSQLSyntaxValidator::validateSyntax(sql, errors, warnings);
}

QStringList MSSQLParser::extractTableNames(const QString& sql) {
    QStringList tableNames;
    QRegularExpression tableRegex("\\bFROM\\s+([\\w\\.\\[\\]]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = tableRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString tableName = match.captured(1);
        if (!tableNames.contains(tableName)) {
            tableNames << tableName;
        }
    }

    // Also check JOIN clauses
    QRegularExpression joinRegex("\\bJOIN\\s+([\\w\\.\\[\\]]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator joinIt = joinRegex.globalMatch(sql);

    while (joinIt.hasNext()) {
        QRegularExpressionMatch match = joinIt.next();
        QString tableName = match.captured(1);
        if (!tableNames.contains(tableName)) {
            tableNames << tableName;
        }
    }

    return tableNames;
}

QStringList MSSQLParser::extractColumnNames(const QString& sql) {
    QStringList columnNames;

    // Extract from SELECT clause
    QRegularExpression selectRegex("\\bSELECT\\s+(.*?)\\s+FROM\\s+", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch selectMatch = selectRegex.match(sql);

    if (selectMatch.hasMatch()) {
        QString selectClause = selectMatch.captured(1);
        QRegularExpression columnRegex("([\\w\\.\\[\\]]+)");
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(selectClause);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString columnName = match.captured(1);
            if (!columnName.contains("SELECT") && !columnName.contains("FROM") && !columnNames.contains(columnName)) {
                columnNames << columnName;
            }
        }
    }

    return columnNames;
}

QStringList MSSQLParser::extractFunctionNames(const QString& sql) {
    QStringList functionNames;
    QStringList functions = MSSQLSyntaxPatterns::BUILTIN_FUNCTIONS;

    for (const QString& function : functions) {
        if (sql.contains(function, Qt::CaseInsensitive)) {
            functionNames << function;
        }
    }

    return functionNames;
}

QStringList MSSQLParser::extractVariableNames(const QString& sql) {
    QStringList variableNames;
    QRegularExpression varRegex(MSSQLSyntaxPatterns::VARIABLE);
    QRegularExpressionMatchIterator it = varRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString variableName = match.captured();
        if (!variableNames.contains(variableName)) {
            variableNames << variableName;
        }
    }

    return variableNames;
}

QString MSSQLParser::formatSQL(const QString& sql) {
    return MSSQLCodeFormatter::formatCode(sql);
}

QStringList MSSQLParser::getCompletionSuggestions(const QString& partialText, const QString& context) {
    return MSSQLIntelliSense::getCompletions(partialText, partialText.length());
}

bool MSSQLParser::needsQuoting(const QString& identifier) {
    // Check if identifier contains special characters or is a reserved word
    QRegularExpression specialCharRegex("[^a-zA-Z0-9_]");
    return specialCharRegex.match(identifier).hasMatch() ||
           MSSQLSyntaxPatterns::RESERVED_KEYWORDS.contains(identifier, Qt::CaseInsensitive) ||
           MSSQLSyntaxPatterns::NON_RESERVED_KEYWORDS.contains(identifier, Qt::CaseInsensitive);
}

QString MSSQLParser::escapeIdentifier(const QString& identifier) {
    if (needsQuoting(identifier)) {
        return "[" + identifier + "]";
    }
    return identifier;
}

bool MSSQLParser::parseCreateTable(const QString& sql, QString& tableName, QStringList& columns) {
    QRegularExpression createTableRegex("\\bCREATE\\s+TABLE\\s+([\\w\\.\\[\\]]+)\\s*\\((.*)\\)",
                                       QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = createTableRegex.match(sql);

    if (match.hasMatch()) {
        tableName = match.captured(1);
        QString columnsDef = match.captured(2);

        // Parse column definitions (simplified)
        QRegularExpression columnRegex("([\\w\\[\\]]+)\\s+([\\w\\[\\]]+)[^,]*");
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(columnsDef);

        while (it.hasNext()) {
            QRegularExpressionMatch columnMatch = it.next();
            columns << columnMatch.captured(1) + " " + columnMatch.captured(2);
        }

        return true;
    }

    return false;
}

bool MSSQLParser::parseCreateIndex(const QString& sql, QString& indexName, QString& tableName, QStringList& columns) {
    QRegularExpression createIndexRegex("\\bCREATE\\s+(?:UNIQUE\\s+)?(?:CLUSTERED\\s+|NONCLUSTERED\\s+)?INDEX\\s+([\\w\\[\\]]+)\\s+ON\\s+([\\w\\.\\[\\]]+)\\s*\\(([^)]+)\\)",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = createIndexRegex.match(sql);

    if (match.hasMatch()) {
        indexName = match.captured(1);
        tableName = match.captured(2);
        QString columnsDef = match.captured(3);

        QRegularExpression columnRegex("([\\w\\[\\]]+)");
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(columnsDef);

        while (it.hasNext()) {
            QRegularExpressionMatch columnMatch = it.next();
            columns << columnMatch.captured(1);
        }

        return true;
    }

    return false;
}

bool MSSQLParser::parseCreateView(const QString& sql, QString& viewName, QString& definition) {
    QRegularExpression createViewRegex("\\bCREATE\\s+VIEW\\s+([\\w\\.\\[\\]]+)\\s+AS\\s+(.*)",
                                      QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = createViewRegex.match(sql);

    if (match.hasMatch()) {
        viewName = match.captured(1);
        definition = match.captured(2).trimmed();
        return true;
    }

    return false;
}

bool MSSQLParser::parseSelectStatement(const QString& sql, QStringList& columns, QStringList& tables, QString& whereClause) {
    // Extract columns
    columns = extractColumnNames(sql);

    // Extract tables
    tables = extractTableNames(sql);

    // Extract WHERE clause
    QRegularExpression whereRegex("\\bWHERE\\s+(.*?)(\\bORDER\\s+BY\\b|\\bGROUP\\s+BY\\b|\\bHAVING\\b|$)",
                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch whereMatch = whereRegex.match(sql);

    if (whereMatch.hasMatch()) {
        whereClause = whereMatch.captured(1).trimmed();
    }

    return !columns.isEmpty() || !tables.isEmpty();
}

// MSSQL Query Analyzer Implementation

void MSSQLQueryAnalyzer::analyzeQuery(const QString& sql, QStringList& issues, QStringList& suggestions) {
    issues.clear();
    suggestions.clear();

    // Check for common issues
    if (hasSelectStar(sql)) {
        issues << "Using SELECT * is not recommended for production code";
        suggestions << "Specify explicit column names in SELECT clause";
    }

    if (hasCartesianProduct(sql)) {
        issues << "Query may produce Cartesian product";
        suggestions << "Verify JOIN conditions are correct";
    }

    if (hasImplicitConversion(sql)) {
        issues << "Implicit data type conversion may cause performance issues";
        suggestions << "Use explicit CAST/CONVERT functions";
    }

    if (usesFunctionsInWhere(sql)) {
        issues << "Using functions in WHERE clause may prevent index usage";
        suggestions << "Avoid functions on indexed columns in WHERE clause";
    }

    if (hasSuboptimalLike(sql)) {
        issues << "LIKE pattern without wildcard at start may still be slow";
        suggestions << "Consider full-text search for text pattern matching";
    }

    if (hasUnnecessaryJoins(sql)) {
        issues << "Query may have unnecessary JOIN operations";
        suggestions << "Review JOIN conditions and remove unused tables";
    }
}

int MSSQLQueryAnalyzer::estimateComplexity(const QString& sql) {
    int complexity = 1;

    // Count keywords that indicate complexity
    QStringList complexityKeywords = {"JOIN", "UNION", "GROUP BY", "ORDER BY", "HAVING", "DISTINCT", "EXISTS", "IN", "NOT IN"};
    for (const QString& keyword : complexityKeywords) {
        QRegularExpression regex("\\b" + keyword + "\\b", QRegularExpression::CaseInsensitiveOption);
        complexity += regex.globalMatch(sql).size();
    }

    // Count subqueries
    QRegularExpression subqueryRegex("\\(\\s*SELECT\\s+", QRegularExpression::CaseInsensitiveOption);
    complexity += subqueryRegex.globalMatch(sql).size() * 2;

    // Count CTEs
    QRegularExpression cteRegex("\\bWITH\\s+\\w+\\s+AS\\s*\\(", QRegularExpression::CaseInsensitiveOption);
    complexity += cteRegex.globalMatch(sql).size() * 3;

    return complexity;
}

QStringList MSSQLQueryAnalyzer::checkBestPractices(const QString& sql) {
    QStringList suggestions;

    // Check for NOLOCK hint usage
    if (sql.contains("NOLOCK", Qt::CaseInsensitive)) {
        suggestions << "Consider using READ COMMITTED SNAPSHOT isolation level instead of NOLOCK hints";
    }

    // Check for SELECT * usage
    if (hasSelectStar(sql)) {
        suggestions << "Avoid using SELECT * in production code";
    }

    // Check for implicit transactions
    if (sql.contains("BEGIN TRAN", Qt::CaseInsensitive) && !sql.contains("COMMIT", Qt::CaseInsensitive)) {
        suggestions << "Ensure all transactions are properly committed or rolled back";
    }

    // Check for proper indexing hints
    if (sql.contains("INDEX=", Qt::CaseInsensitive)) {
        suggestions << "Index hints should only be used for troubleshooting, not in production";
    }

    return suggestions;
}

QStringList MSSQLQueryAnalyzer::suggestIndexes(const QString& sql) {
    QStringList suggestions;

    // Analyze WHERE clause for potential indexes
    QRegularExpression whereRegex("\\bWHERE\\s+(.*?)(\\bORDER\\s+BY\\b|\\bGROUP\\s+BY\\b|$)",
                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch whereMatch = whereRegex.match(sql);

    if (whereMatch.hasMatch()) {
        QString whereClause = whereMatch.captured(1);
        QStringList conditions = whereClause.split(QRegularExpression("\\b(AND|OR)\\b"), Qt::SkipEmptyParts);

        for (const QString& condition : conditions) {
            if (condition.contains("=") || condition.contains("LIKE") || condition.contains("BETWEEN")) {
                QRegularExpression columnRegex("([\\w\\.\\[\\]]+)\\s*[=<>]");
                QRegularExpressionMatch columnMatch = columnRegex.match(condition);
                if (columnMatch.hasMatch()) {
                    QString column = columnMatch.captured(1);
                    suggestions << QString("Consider creating an index on column: %1").arg(column);
                }
            }
        }
    }

    return suggestions;
}

QStringList MSSQLQueryAnalyzer::checkSecurityIssues(const QString& sql) {
    QStringList issues;

    // Check for SQL injection vulnerabilities
    if (sql.contains("EXEC", Qt::CaseInsensitive) || sql.contains("EXECUTE", Qt::CaseInsensitive)) {
        issues << "Dynamic SQL execution detected - ensure proper parameterization";
    }

    // Check for system table access without proper filtering
    if (sql.contains("sys.objects", Qt::CaseInsensitive) && !sql.contains("WHERE", Qt::CaseInsensitive)) {
        issues << "Accessing system tables without WHERE clause may expose sensitive information";
    }

    // Check for xp_cmdshell usage
    if (sql.contains("xp_cmdshell", Qt::CaseInsensitive)) {
        issues << "xp_cmdshell usage detected - this can be a security risk";
    }

    return issues;
}

bool MSSQLQueryAnalyzer::hasSelectStar(const QString& sql) {
    QRegularExpression selectStarRegex("\\bSELECT\\s+\\*", QRegularExpression::CaseInsensitiveOption);
    return selectStarRegex.match(sql).hasMatch();
}

bool MSSQLQueryAnalyzer::hasImplicitConversion(const QString& sql) {
    // Look for patterns that might cause implicit conversions
    return sql.contains("CONVERT", Qt::CaseInsensitive) == false &&
           (sql.contains("varchar", Qt::CaseInsensitive) && sql.contains("int", Qt::CaseInsensitive));
}

bool MSSQLQueryAnalyzer::hasCartesianProduct(const QString& sql) {
    QRegularExpression fromRegex("\\bFROM\\s+([\\w\\.\\[\\]]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression joinRegex("\\bJOIN\\s+", QRegularExpression::CaseInsensitiveOption);

    int tableCount = fromRegex.globalMatch(sql).size();
    int joinCount = joinRegex.globalMatch(sql).size();

    return tableCount > 1 && (tableCount - 1) > joinCount;
}

bool MSSQLQueryAnalyzer::usesFunctionsInWhere(const QString& sql) {
    QRegularExpression whereRegex("\\bWHERE\\s+(.*?)(\\bORDER\\s+BY\\b|\\bGROUP\\s+BY\\b|$)",
                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch whereMatch = whereRegex.match(sql);

    if (whereMatch.hasMatch()) {
        QString whereClause = whereMatch.captured(1);
        QStringList functions = MSSQLSyntaxPatterns::BUILTIN_FUNCTIONS;

        for (const QString& function : functions) {
            if (whereClause.contains(function, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }

    return false;
}

bool MSSQLQueryAnalyzer::hasSuboptimalLike(const QString& sql) {
    QRegularExpression likeRegex("\\bLIKE\\s+['\"][^%]", QRegularExpression::CaseInsensitiveOption);
    return likeRegex.match(sql).hasMatch();
}

bool MSSQLQueryAnalyzer::hasUnnecessaryJoins(const QString& sql) {
    // Simple heuristic: if we have many tables but few conditions, suggest review
    QStringList tables = MSSQLParser::extractTableNames(sql);
    QRegularExpression conditionRegex("\\bWHERE\\s+", QRegularExpression::CaseInsensitiveOption);

    return tables.size() > 3 && conditionRegex.match(sql).hasMatch() == false;
}

} // namespace scratchrobin
