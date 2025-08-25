#include "mariadb_features.h"
#include <QRegularExpression>

namespace scratchrobin {

// MariaDB Data Types Implementation
const QStringList MariaDBDataTypes::NUMERIC_TYPES = {
    "tinyint", "smallint", "mediumint", "int", "integer", "bigint",
    "decimal", "dec", "numeric", "float", "double", "real", "bit",
    "serial", "bigserial"
};

const QStringList MariaDBDataTypes::STRING_TYPES = {
    "char", "varchar", "tinytext", "text", "mediumtext", "longtext",
    "binary", "varbinary", "tinyblob", "blob", "mediumblob", "longblob",
    "enum", "set"
};

const QStringList MariaDBDataTypes::DATE_TIME_TYPES = {
    "date", "datetime", "timestamp", "time", "year"
};

const QStringList MariaDBDataTypes::BINARY_TYPES = {
    "binary", "varbinary", "tinyblob", "blob", "mediumblob", "longblob"
};

const QStringList MariaDBDataTypes::BOOLEAN_TYPES = {
    "bool", "boolean"
};

const QStringList MariaDBDataTypes::SPATIAL_TYPES = {
    "geometry", "point", "linestring", "polygon", "multipoint",
    "multilinestring", "multipolygon", "geometrycollection"
};

const QStringList MariaDBDataTypes::JSON_TYPES = {
    "json"
};

const QStringList MariaDBDataTypes::DYNAMIC_COLUMNS_TYPES = {
    "dynamic"
};

const QStringList MariaDBDataTypes::UUID_TYPES = {
    "uuid"
};

const QStringList MariaDBDataTypes::IP_TYPES = {
    "inet4", "inet6"
};

const QStringList MariaDBDataTypes::SEQUENCE_TYPES = {
    "serial", "bigserial"
};

QStringList MariaDBDataTypes::getAllDataTypes() {
    QStringList allTypes;
    allTypes << NUMERIC_TYPES << STRING_TYPES << DATE_TIME_TYPES
             << BINARY_TYPES << BOOLEAN_TYPES << SPATIAL_TYPES
             << JSON_TYPES << DYNAMIC_COLUMNS_TYPES << UUID_TYPES
             << IP_TYPES << SEQUENCE_TYPES;
    return allTypes;
}

// MariaDB SQL Features Implementation
const QStringList MariaDBSQLFeatures::KEYWORDS = {
    "ACCESSIBLE", "ADD", "ALL", "ALTER", "ANALYZE", "AND", "AS", "ASC", "ASENSITIVE",
    "BEFORE", "BETWEEN", "BIGINT", "BINARY", "BLOB", "BOTH", "BY", "CALL", "CASCADE",
    "CASE", "CHANGE", "CHAR", "CHARACTER", "CHECK", "COLLATE", "COLUMN", "CONDITION",
    "CONSTRAINT", "CONTINUE", "CONVERT", "CREATE", "CROSS", "CURRENT_DATE", "CURRENT_TIME",
    "CURRENT_TIMESTAMP", "CURRENT_USER", "CURSOR", "DATABASE", "DATABASES", "DAY_HOUR",
    "DAY_MICROSECOND", "DAY_MINUTE", "DAY_SECOND", "DEC", "DECIMAL", "DECLARE",
    "DEFAULT", "DELAYED", "DELETE", "DESC", "DESCRIBE", "DETERMINISTIC", "DISTINCT",
    "DISTINCTROW", "DIV", "DOUBLE", "DROP", "DUAL", "EACH", "ELSE", "ELSEIF", "ENCLOSED",
    "ESCAPED", "EXISTS", "EXIT", "EXPLAIN", "FALSE", "FETCH", "FLOAT", "FOR", "FORCE",
    "FOREIGN", "FROM", "FULLTEXT", "GRANT", "GROUP", "HAVING", "HIGH_PRIORITY",
    "HOUR_MICROSECOND", "HOUR_MINUTE", "HOUR_SECOND", "IF", "IGNORE", "IN", "INDEX",
    "INFILE", "INNER", "INOUT", "INSENSITIVE", "INSERT", "INT", "INTEGER", "INTERVAL",
    "INTO", "IS", "ITERATE", "JOIN", "KEY", "KEYS", "KILL", "LEADING", "LEAVE", "LEFT",
    "LIKE", "LIMIT", "LINEAR", "LINES", "LOAD", "LOCALTIME", "LOCALTIMESTAMP", "LOCK",
    "LONG", "LONGBLOB", "LONGTEXT", "LOOP", "LOW_PRIORITY", "MATCH", "MEDIUMBLOB",
    "MEDIUMINT", "MEDIUMTEXT", "MIDDLEINT", "MINUTE_MICROSECOND", "MINUTE_SECOND",
    "MOD", "MODIFIES", "NATURAL", "NOT", "NO_WRITE_TO_BINLOG", "NULL", "NUMERIC",
    "ON", "OPTIMIZE", "OPTION", "OPTIONALLY", "OR", "ORDER", "OUT", "OUTER", "OUTFILE",
    "PRECISION", "PRIMARY", "PROCEDURE", "PURGE", "RANGE", "READ", "READS", "REAL",
    "REFERENCES", "REGEXP", "RELEASE", "RENAME", "REPEAT", "REPLACE", "REQUIRE",
    "RESTRICT", "RETURN", "REVOKE", "RIGHT", "RLIKE", "SCHEMA", "SCHEMAS", "SECOND_MICROSECOND",
    "SELECT", "SENSITIVE", "SEPARATOR", "SET", "SHOW", "SMALLINT", "SOME", "SONAME",
    "SPATIAL", "SPECIFIC", "SQL", "SQLEXCEPTION", "SQLSTATE", "SQLWARNING", "SQL_BIG_RESULT",
    "SQL_CALC_FOUND_ROWS", "SQL_SMALL_RESULT", "SSL", "STARTING", "STRAIGHT_JOIN",
    "TABLE", "TERMINATED", "THEN", "TINYBLOB", "TINYINT", "TINYTEXT", "TO", "TRAILING",
    "TRIGGER", "TRUE", "UNDO", "UNION", "UNIQUE", "UNLOCK", "UNSIGNED", "UPDATE", "USAGE",
    "USE", "USING", "UTC_DATE", "UTC_TIME", "UTC_TIMESTAMP", "VALUES", "VARBINARY",
    "VARCHAR", "VARYING", "WHEN", "WHERE", "WHILE", "WITH", "WRITE", "XOR", "YEAR_MONTH",
    "ZEROFILL"
};

const QStringList MariaDBSQLFeatures::FUNCTIONS = {
    // String functions
    "ASCII", "BIN", "BIT_LENGTH", "CHAR", "CHAR_LENGTH", "CHARACTER_LENGTH",
    "CONCAT", "CONCAT_WS", "ELT", "EXPORT_SET", "FIELD", "FIND_IN_SET", "FORMAT",
    "FROM_BASE64", "HEX", "INSTR", "LCASE", "LEFT", "LENGTH", "LOAD_FILE", "LOCATE",
    "LOWER", "LPAD", "LTRIM", "MAKE_SET", "MID", "OCT", "OCTET_LENGTH", "ORD",
    "POSITION", "QUOTE", "REPEAT", "REPLACE", "REVERSE", "RIGHT", "RPAD", "RTRIM",
    "SOUNDEX", "SPACE", "STRCMP", "SUBSTR", "SUBSTRING", "SUBSTRING_INDEX", "TO_BASE64",
    "TRIM", "UCASE", "UNHEX", "UPPER", "WEIGHT_STRING",

    // Numeric functions
    "ABS", "ACOS", "ASIN", "ATAN", "ATAN2", "CEIL", "CEILING", "CONV", "COS", "COT",
    "CRC32", "DEGREES", "DIV", "EXP", "FLOOR", "GREATEST", "LEAST", "LN", "LOG", "LOG10",
    "LOG2", "MOD", "PI", "POW", "POWER", "RADIANS", "RAND", "ROUND", "SIGN", "SIN",
    "SQRT", "TAN", "TRUNCATE",

    // Date/Time functions
    "ADDDATE", "ADDTIME", "CONVERT_TZ", "CURDATE", "CURRENT_DATE", "CURRENT_TIME",
    "CURRENT_TIMESTAMP", "CURTIME", "DATE", "DATE_ADD", "DATE_FORMAT", "DATE_SUB",
    "DATEDIFF", "DAY", "DAYNAME", "DAYOFMONTH", "DAYOFWEEK", "DAYOFYEAR", "EXTRACT",
    "FROM_DAYS", "FROM_UNIXTIME", "GET_FORMAT", "HOUR", "LAST_DAY", "LOCALTIME",
    "LOCALTIMESTAMP", "MAKEDATE", "MAKETIME", "MICROSECOND", "MINUTE", "MONTH",
    "MONTHNAME", "NOW", "PERIOD_ADD", "PERIOD_DIFF", "QUARTER", "SECOND", "SEC_TO_TIME",
    "STR_TO_DATE", "SUBDATE", "SUBTIME", "SYSDATE", "TIME", "TIME_FORMAT", "TIME_TO_SEC",
    "TIMEDIFF", "TIMESTAMP", "TIMESTAMPADD", "TIMESTAMPDIFF", "TO_DAYS", "TO_SECONDS",
    "UNIX_TIMESTAMP", "UTC_DATE", "UTC_TIME", "UTC_TIMESTAMP", "WEEK", "WEEKDAY",
    "WEEKOFYEAR", "YEAR", "YEARWEEK",

    // Aggregate functions
    "AVG", "BIT_AND", "BIT_OR", "BIT_XOR", "COUNT", "GROUP_CONCAT", "MAX", "MIN",
    "STD", "STDDEV", "STDDEV_POP", "STDDEV_SAMP", "SUM", "VAR_POP", "VAR_SAMP", "VARIANCE",

    // JSON functions (MariaDB 10.2+)
    "JSON_ARRAY", "JSON_ARRAY_APPEND", "JSON_ARRAY_INSERT", "JSON_COMPACT", "JSON_CONTAINS",
    "JSON_CONTAINS_PATH", "JSON_DEPTH", "JSON_EXTRACT", "JSON_INSERT", "JSON_KEYS",
    "JSON_LENGTH", "JSON_MERGE", "JSON_MERGE_PATCH", "JSON_MERGE_PRESERVE", "JSON_OBJECT",
    "JSON_PRETTY", "JSON_QUOTE", "JSON_REMOVE", "JSON_REPLACE", "JSON_SEARCH", "JSON_SET",
    "JSON_TYPE", "JSON_UNQUOTE", "JSON_VALID",

    // Dynamic column functions
    "COLUMN_ADD", "COLUMN_CHECK", "COLUMN_CREATE", "COLUMN_DELETE", "COLUMN_EXISTS",
    "COLUMN_GET", "COLUMN_JSON", "COLUMN_LIST",

    // Other functions
    "AES_DECRYPT", "AES_ENCRYPT", "COMPRESS", "DECODE", "DES_DECRYPT", "DES_ENCRYPT",
    "ENCODE", "ENCRYPT", "MD5", "OLD_PASSWORD", "PASSWORD", "SHA", "SHA1", "SHA2",
    "UNCOMPRESS", "UNCOMPRESSED_LENGTH", "UUID", "UUID_SHORT"
};

const QStringList MariaDBSQLFeatures::OPERATORS = {
    // Comparison operators
    "=", ">", "<", ">=", "<=", "<>", "!=", "!<", "!>",

    // Arithmetic operators
    "+", "-", "*", "/", "%", "DIV", "MOD",

    // Logical operators
    "AND", "&&", "OR", "||", "NOT", "!", "XOR",

    // Bit operators
    "&", "|", "^", "~", "<<", ">>",

    // String operators
    "LIKE", "NOT LIKE", "REGEXP", "NOT REGEXP", "RLIKE", "NOT RLIKE",

    // Set operators
    "UNION", "UNION ALL", "INTERSECT", "EXCEPT", "MINUS",

    // Assignment operator
    ":=",

    // Special operators
    ".", "->", "->>", "IN", "NOT IN", "BETWEEN", "NOT BETWEEN", "IS", "IS NOT",
    "NULL", "NOT NULL", "EXISTS", "NOT EXISTS"
};

const QStringList MariaDBSQLFeatures::RESERVED_WORDS = {
    "ACCESSIBLE", "ADD", "ALL", "ALTER", "ANALYZE", "AND", "AS", "ASC", "ASENSITIVE",
    "BEFORE", "BETWEEN", "BIGINT", "BINARY", "BLOB", "BOTH", "BY", "CALL", "CASCADE",
    "CASE", "CHANGE", "CHAR", "CHARACTER", "CHECK", "COLLATE", "COLUMN", "CONDITION",
    "CONSTRAINT", "CONTINUE", "CONVERT", "CREATE", "CROSS", "CURRENT_DATE", "CURRENT_TIME",
    "CURRENT_TIMESTAMP", "CURRENT_USER", "CURSOR", "DATABASE", "DATABASES", "DAY_HOUR",
    "DAY_MICROSECOND", "DAY_MINUTE", "DAY_SECOND", "DEC", "DECIMAL", "DECLARE",
    "DEFAULT", "DELAYED", "DELETE", "DESC", "DESCRIBE", "DETERMINISTIC", "DISTINCT",
    "DISTINCTROW", "DIV", "DOUBLE", "DROP", "DUAL", "EACH", "ELSE", "ELSEIF", "ENCLOSED",
    "ESCAPED", "EXISTS", "EXIT", "EXPLAIN", "FALSE", "FETCH", "FLOAT", "FOR", "FORCE",
    "FOREIGN", "FROM", "FULLTEXT", "GRANT", "GROUP", "HAVING", "HIGH_PRIORITY",
    "HOUR_MICROSECOND", "HOUR_MINUTE", "HOUR_SECOND", "IF", "IGNORE", "IN", "INDEX",
    "INFILE", "INNER", "INOUT", "INSENSITIVE", "INSERT", "INT", "INTEGER", "INTERVAL",
    "INTO", "IS", "ITERATE", "JOIN", "KEY", "KEYS", "KILL", "LEADING", "LEAVE", "LEFT",
    "LIKE", "LIMIT", "LINEAR", "LINES", "LOAD", "LOCALTIME", "LOCALTIMESTAMP", "LOCK",
    "LONG", "LONGBLOB", "LONGTEXT", "LOOP", "LOW_PRIORITY", "MATCH", "MEDIUMBLOB",
    "MEDIUMINT", "MEDIUMTEXT", "MIDDLEINT", "MINUTE_MICROSECOND", "MINUTE_SECOND",
    "MOD", "MODIFIES", "NATURAL", "NOT", "NO_WRITE_TO_BINLOG", "NULL", "NUMERIC",
    "ON", "OPTIMIZE", "OPTION", "OPTIONALLY", "OR", "ORDER", "OUT", "OUTER", "OUTFILE",
    "PRECISION", "PRIMARY", "PROCEDURE", "PURGE", "RANGE", "READ", "READS", "REAL",
    "REFERENCES", "REGEXP", "RELEASE", "RENAME", "REPEAT", "REPLACE", "REQUIRE",
    "RESTRICT", "RETURN", "REVOKE", "RIGHT", "RLIKE", "SCHEMA", "SCHEMAS", "SECOND_MICROSECOND",
    "SELECT", "SENSITIVE", "SEPARATOR", "SET", "SHOW", "SMALLINT", "SOME", "SONAME",
    "SPATIAL", "SPECIFIC", "SQL", "SQLEXCEPTION", "SQLSTATE", "SQLWARNING", "SQL_BIG_RESULT",
    "SQL_CALC_FOUND_ROWS", "SQL_SMALL_RESULT", "SSL", "STARTING", "STRAIGHT_JOIN",
    "TABLE", "TERMINATED", "THEN", "TINYBLOB", "TINYINT", "TINYTEXT", "TO", "TRAILING",
    "TRIGGER", "TRUE", "UNDO", "UNION", "UNIQUE", "UNLOCK", "UNSIGNED", "UPDATE", "USAGE",
    "USE", "USING", "UTC_DATE", "UTC_TIME", "UTC_TIMESTAMP", "VALUES", "VARBINARY",
    "VARCHAR", "VARYING", "WHEN", "WHERE", "WHILE", "WITH", "WRITE", "XOR", "YEAR_MONTH",
    "ZEROFILL"
};

const QStringList MariaDBSQLFeatures::CTE_FEATURES = {
    "WITH", "RECURSIVE", "AS"
};

const QStringList MariaDBSQLFeatures::WINDOW_FEATURES = {
    "OVER", "PARTITION BY", "ORDER BY", "ROWS", "RANGE", "UNBOUNDED PRECEDING",
    "UNBOUNDED FOLLOWING", "CURRENT ROW", "PRECEDING", "FOLLOWING"
};

const QStringList MariaDBSQLFeatures::JSON_FEATURES = {
    "JSON_ARRAY", "JSON_OBJECT", "JSON_EXTRACT", "JSON_CONTAINS", "JSON_SET",
    "JSON_INSERT", "JSON_REPLACE", "JSON_REMOVE", "JSON_SEARCH", "JSON_VALID"
};

const QStringList MariaDBSQLFeatures::SPATIAL_FEATURES = {
    "GEOMETRY", "POINT", "LINESTRING", "POLYGON", "MULTIPOINT", "MULTILINESTRING",
    "MULTIPOLYGON", "GEOMETRYCOLLECTION", "ST_AsText", "ST_GeomFromText", "ST_Contains",
    "ST_Distance", "ST_Intersects", "ST_Buffer", "ST_Area", "ST_Length"
};

const QStringList MariaDBSQLFeatures::SEQUENCE_FEATURES = {
    "CREATE SEQUENCE", "ALTER SEQUENCE", "DROP SEQUENCE", "NEXTVAL", "CURRVAL"
};

const QStringList MariaDBSQLFeatures::DYNAMIC_FEATURES = {
    "COLUMN_ADD", "COLUMN_CHECK", "COLUMN_CREATE", "COLUMN_DELETE", "COLUMN_EXISTS",
    "COLUMN_GET", "COLUMN_JSON", "COLUMN_LIST"
};

const QStringList MariaDBSQLFeatures::VIRTUAL_FEATURES = {
    "VIRTUAL", "PERSISTENT", "GENERATED ALWAYS AS"
};

const QStringList MariaDBSQLFeatures::PARTITION_FEATURES = {
    "PARTITION BY", "RANGE", "LIST", "HASH", "KEY", "SUBPARTITION"
};

const QStringList MariaDBSQLFeatures::GTID_FEATURES = {
    "GTID", "MASTER_GTID_WAIT", "WAIT_FOR_EXECUTED_GTID_SET"
};

const QStringList MariaDBSQLFeatures::REPLICATION_FEATURES = {
    "SHOW SLAVE STATUS", "SHOW MASTER STATUS", "CHANGE MASTER TO", "START SLAVE",
    "STOP SLAVE", "RESET SLAVE", "FLUSH LOGS"
};

const QStringList MariaDBSQLFeatures::PERFORMANCE_FEATURES = {
    "performance_schema", "SHOW ENGINE PERFORMANCE_SCHEMA STATUS",
    "SELECT * FROM performance_schema.events_statements_current"
};

// MariaDB Objects Implementation
const QStringList MariaDBObjects::SYSTEM_DATABASES = {
    "information_schema", "mysql", "performance_schema", "sys"
};

const QStringList MariaDBObjects::SYSTEM_SCHEMAS = {
    "information_schema", "mysql", "performance_schema", "sys"
};

const QStringList MariaDBObjects::INFORMATION_SCHEMA_VIEWS = {
    "TABLES", "COLUMNS", "VIEWS", "ROUTINES", "TRIGGERS", "KEY_COLUMN_USAGE",
    "TABLE_CONSTRAINTS", "SCHEMATA", "ENGINES", "PLUGINS", "CHARACTER_SETS",
    "COLLATIONS", "COLLATION_CHARACTER_SET_APPLICABILITY", "COLUMN_PRIVILEGES",
    "KEY_CACHES", "PARAMETERS", "PARTITIONS", "PROFILING", "PROCESSLIST",
    "REFERENTIAL_CONSTRAINTS", "SCHEMA_PRIVILEGES", "SESSION_STATUS",
    "SESSION_VARIABLES", "STATISTICS", "TABLESPACES", "TABLE_CONSTRAINTS",
    "TABLE_PRIVILEGES", "USER_PRIVILEGES", "VARIABLES"
};

const QStringList MariaDBObjects::PERFORMANCE_SCHEMA_VIEWS = {
    "accounts", "cond_instances", "events_stages_current", "events_stages_history",
    "events_stages_history_long", "events_stages_summary_by_account_by_event_name",
    "events_stages_summary_by_host_by_event_name", "events_stages_summary_by_thread_by_event_name",
    "events_stages_summary_by_user_by_event_name", "events_stages_summary_global_by_event_name",
    "events_statements_current", "events_statements_history", "events_statements_history_long",
    "events_statements_summary_by_account_by_event_name", "events_statements_summary_by_digest",
    "events_statements_summary_by_host_by_event_name", "events_statements_summary_by_thread_by_event_name",
    "events_statements_summary_by_user_by_event_name", "events_statements_summary_global_by_event_name",
    "events_transactions_current", "events_transactions_history", "events_transactions_history_long",
    "events_waits_current", "events_waits_history", "events_waits_history_long",
    "events_waits_summary_by_account_by_event_name", "events_waits_summary_by_host_by_event_name",
    "events_waits_summary_by_instance", "events_waits_summary_by_thread_by_event_name",
    "events_waits_summary_by_user_by_event_name", "events_waits_summary_global_by_event_name",
    "events_waits_summary_global_by_event_name", "file_instances", "file_summary_by_event_name",
    "file_summary_by_instance", "host_cache", "hosts", "memory_summary_by_account_by_event_name",
    "memory_summary_by_host_by_event_name", "memory_summary_by_thread_by_event_name",
    "memory_summary_by_user_by_event_name", "memory_summary_global_by_event_name",
    "metadata_locks", "mutex_instances", "objects_summary_global_by_type",
    "performance_timers", "prepared_statements_instances", "replication_applier_configuration",
    "replication_applier_status", "replication_applier_status_by_coordinator",
    "replication_applier_status_by_worker", "replication_connection_configuration",
    "replication_connection_status", "replication_group_member_stats", "replication_group_members",
    "rwlock_instances", "session_account_connect_attrs", "session_connect_attrs",
    "setup_actors", "setup_consumers", "setup_instruments", "setup_objects", "setup_timers",
    "socket_instances", "socket_summary_by_event_name", "socket_summary_by_instance",
    "status_by_account", "status_by_host", "status_by_thread", "status_by_user",
    "table_handles", "table_io_waits_summary_by_index_usage", "table_io_waits_summary_by_table",
    "table_lock_waits_summary_by_table", "threads", "user_variables_by_thread",
    "users", "variables_by_thread", "variables_info"
};

const QStringList MariaDBObjects::MYSQL_SCHEMA_TABLES = {
    "user", "db", "host", "tables_priv", "columns_priv", "procs_priv", "proxies_priv",
    "event", "func", "general_log", "help_category", "help_keyword", "help_relation",
    "help_topic", "innodb_index_stats", "innodb_table_stats", "ndb_binlog_index",
    "plugin", "proc", "procs_priv", "servers", "slave_master_info", "slave_relay_log_info",
    "slave_worker_info", "slow_log", "time_zone", "time_zone_leap_second", "time_zone_name",
    "time_zone_transition", "time_zone_transition_type"
};

const QStringList MariaDBObjects::BUILTIN_FUNCTIONS = {
    "ABS", "ACOS", "ADDDATE", "ADDTIME", "AES_DECRYPT", "AES_ENCRYPT", "ANY_VALUE",
    "ASCII", "ASIN", "ATAN", "ATAN2", "AVG", "BENCHMARK", "BIN", "BINARY", "BIT_AND",
    "BIT_COUNT", "BIT_LENGTH", "BIT_OR", "BIT_XOR", "CAST", "CEIL", "CEILING",
    "CHAR", "CHAR_LENGTH", "CHARACTER_LENGTH", "CHARSET", "COALESCE", "COERCIBILITY",
    "COLLATION", "COMPRESS", "CONCAT", "CONCAT_WS", "CONNECTION_ID", "CONV", "CONVERT",
    "CONVERT_TZ", "COS", "COT", "COUNT", "CRC32", "CUME_DIST", "CURDATE", "CURRENT_DATE",
    "CURRENT_ROLE", "CURRENT_TIME", "CURRENT_TIMESTAMP", "CURRENT_USER", "CURTIME",
    "DATABASE", "DATE", "DATE_ADD", "DATE_FORMAT", "DATE_SUB", "DATEDIFF", "DAY",
    "DAYNAME", "DAYOFMONTH", "DAYOFWEEK", "DAYOFYEAR", "DECODE", "DEFAULT", "DEGREES",
    "DENSE_RANK", "DES_DECRYPT", "DES_ENCRYPT", "DIV", "ELT", "ENCODE", "ENCRYPT",
    "EXP", "EXPORT_SET", "EXTRACT", "FIELD", "FIND_IN_SET", "FIRST_VALUE", "FLOOR",
    "FORMAT", "FOUND_ROWS", "FROM_BASE64", "FROM_DAYS", "FROM_UNIXTIME", "GET_FORMAT",
    "GREATEST", "GROUP_CONCAT", "HEX", "HOUR", "IF", "IFNULL", "IN", "INET_ATON",
    "INET_NTOA", "INET6_ATON", "INET6_NTOA", "INSERT", "INSTR", "INTERVAL", "IS_FREE_LOCK",
    "IS_IPV4", "IS_IPV4_COMPAT", "IS_IPV4_MAPPED", "IS_IPV6", "IS_NOT_NULL", "IS_NULL",
    "IS_USED_LOCK", "LAG", "LAST_VALUE", "LCASE", "LEAD", "LEAST", "LEFT", "LENGTH",
    "LN", "LOAD_FILE", "LOCALTIME", "LOCALTIMESTAMP", "LOCATE", "LOG", "LOG10", "LOG2",
    "LOWER", "LPAD", "LTRIM", "MAKE_SET", "MASTER_POS_WAIT", "MATCH", "MAX", "MD5",
    "MICROSECOND", "MID", "MIN", "MINUTE", "MOD", "MONTH", "MONTHNAME", "NAME_CONST",
    "NOT", "NOW", "NULLIF", "OCT", "OCTET_LENGTH", "OLD_PASSWORD", "ORD", "PERCENT_RANK",
    "PERIOD_ADD", "PERIOD_DIFF", "PI", "POSITION", "POW", "POWER", "QUARTER", "QUOTE",
    "RADIANS", "RAND", "RANK", "REGEXP_INSTR", "REGEXP_REPLACE", "REGEXP_SUBSTR",
    "RELEASE_LOCK", "REPEAT", "REPLACE", "REVERSE", "RIGHT", "ROUND", "ROW_COUNT",
    "ROW_NUMBER", "RPAD", "RTRIM", "SCHEMA", "SECOND", "SEC_TO_TIME", "SESSION_USER",
    "SHA", "SHA1", "SHA2", "SIGN", "SIN", "SLEEP", "SOUNDEX", "SPACE", "SQRT", "STD",
    "STDDEV", "STDDEV_POP", "STDDEV_SAMP", "STR_TO_DATE", "STRCMP", "SUBDATE", "SUBSTR",
    "SUBSTRING", "SUBSTRING_INDEX", "SUBTIME", "SUM", "SYSDATE", "SYSTEM_USER", "TAN",
    "TIME", "TIME_FORMAT", "TIME_TO_SEC", "TIMEDIFF", "TIMESTAMP", "TIMESTAMPADD",
    "TIMESTAMPDIFF", "TO_BASE64", "TO_DAYS", "TO_SECONDS", "TRIM", "TRUNCATE", "UCASE",
    "UNCOMPRESS", "UNCOMPRESSED_LENGTH", "UNHEX", "UNIX_TIMESTAMP", "UPDATEXML", "UPPER",
    "USER", "UTC_DATE", "UTC_TIME", "UTC_TIMESTAMP", "UUID", "UUID_SHORT", "VALUES",
    "VAR_POP", "VAR_SAMP", "VARIANCE", "VERSION", "WEEK", "WEEKDAY", "WEEKOFYEAR",
    "WEIGHT_STRING", "YEAR", "YEARWEEK"
};

const QStringList MariaDBObjects::BUILTIN_OPERATORS = {
    "=", ":=", "!=", "<>", "<", "<=", ">", ">=", "IS", "IS NOT", "IS NULL", "IS NOT NULL",
    "AND", "&&", "OR", "||", "NOT", "!", "XOR", "BETWEEN", "NOT BETWEEN", "IN", "NOT IN",
    "EXISTS", "NOT EXISTS", "LIKE", "NOT LIKE", "REGEXP", "NOT REGEXP", "RLIKE", "NOT RLIKE",
    "SOUNDS LIKE", "+", "-", "*", "/", "%", "DIV", "MOD", "&", "|", "^", "~", "<<", ">>",
    "UNION", "UNION ALL", "INTERSECT", "EXCEPT"
};

const QStringList MariaDBObjects::SEQUENCE_OBJECTS = {
    "CREATE SEQUENCE", "ALTER SEQUENCE", "DROP SEQUENCE", "NEXT VALUE FOR", "LASTVAL"
};

const QStringList MariaDBObjects::DYNAMIC_COLUMN_FUNCTIONS = {
    "COLUMN_ADD", "COLUMN_CHECK", "COLUMN_CREATE", "COLUMN_DELETE", "COLUMN_EXISTS",
    "COLUMN_GET", "COLUMN_JSON", "COLUMN_LIST"
};

const QStringList MariaDBObjects::JSON_FUNCTIONS = {
    "JSON_ARRAY", "JSON_ARRAY_APPEND", "JSON_ARRAY_INSERT", "JSON_COMPACT", "JSON_CONTAINS",
    "JSON_CONTAINS_PATH", "JSON_DEPTH", "JSON_EXTRACT", "JSON_INSERT", "JSON_KEYS",
    "JSON_LENGTH", "JSON_MERGE", "JSON_MERGE_PATCH", "JSON_MERGE_PRESERVE", "JSON_OBJECT",
    "JSON_PRETTY", "JSON_QUOTE", "JSON_REMOVE", "JSON_REPLACE", "JSON_SEARCH", "JSON_SET",
    "JSON_TYPE", "JSON_UNQUOTE", "JSON_VALID"
};

const QStringList MariaDBObjects::SPATIAL_FUNCTIONS = {
    "ST_AsText", "ST_GeomFromText", "ST_Contains", "ST_Distance", "ST_Intersects",
    "ST_Buffer", "ST_Area", "ST_Length", "ST_ConvexHull", "ST_Crosses", "ST_Difference",
    "ST_Disjoint", "ST_Envelope", "ST_Equals", "ST_GeometryType", "ST_Intersection",
    "ST_IsSimple", "ST_IsValid", "ST_Overlaps", "ST_Simplify", "ST_SymDifference",
    "ST_Touches", "ST_Union", "ST_Within"
};

const QStringList MariaDBObjects::UUID_FUNCTIONS = {
    "UUID", "UUID_SHORT", "SYS_GUID"
};

const QStringList MariaDBObjects::ENCRYPTION_FUNCTIONS = {
    "AES_DECRYPT", "AES_ENCRYPT", "DES_DECRYPT", "DES_ENCRYPT", "ENCODE", "DECODE",
    "ENCRYPT", "MD5", "OLD_PASSWORD", "PASSWORD", "SHA", "SHA1", "SHA2"
};

// MariaDB Config Implementation
const QStringList MariaDBConfig::CONFIG_PARAMETERS = {
    "autocommit", "big_tables", "binlog_format", "binlog_row_image", "character_set_client",
    "character_set_connection", "character_set_database", "character_set_filesystem",
    "character_set_results", "character_set_server", "character_set_system", "collation_connection",
    "collation_database", "collation_server", "completion_type", "concurrent_insert",
    "connect_timeout", "datadir", "default_storage_engine", "default_tmp_storage_engine",
    "delay_key_write", "delayed_insert_limit", "delayed_insert_timeout", "delayed_queue_size",
    "div_precision_increment", "engine_condition_pushdown", "event_scheduler", "expire_logs_days",
    "flush", "flush_time", "foreign_key_checks", "ft_boolean_syntax", "ft_max_word_len",
    "ft_min_word_len", "ft_query_expansion_limit", "ft_stopword_file", "general_log",
    "general_log_file", "group_concat_max_len", "have_community_features", "have_dynamic_loading",
    "have_geometry", "have_openssl", "have_profiling", "have_query_cache", "have_rtree_keys",
    "have_ssl", "have_statement_timeout", "have_symlink", "hostname", "init_connect",
    "init_file", "init_slave", "innodb_adaptive_flushing", "innodb_adaptive_hash_index",
    "innodb_adaptive_max_sleep_delay", "innodb_additional_mem_pool_size", "innodb_autoextend_increment",
    "innodb_autoinc_lock_mode", "innodb_buffer_pool_instances", "innodb_buffer_pool_size",
    "innodb_change_buffering", "innodb_checksums", "innodb_commit_concurrency", "innodb_concurrency_tickets",
    "innodb_data_file_path", "innodb_data_home_dir", "innodb_doublewrite", "innodb_fast_shutdown",
    "innodb_file_format", "innodb_file_format_check", "innodb_file_format_max", "innodb_file_per_table",
    "innodb_flush_log_at_timeout", "innodb_flush_log_at_trx_commit", "innodb_flush_method",
    "innodb_force_load_corrupted", "innodb_force_recovery", "innodb_io_capacity", "innodb_large_prefix",
    "innodb_lock_wait_timeout", "innodb_log_buffer_size", "innodb_log_file_size", "innodb_log_files_in_group",
    "innodb_log_group_home_dir", "innodb_max_dirty_pages_pct", "innodb_max_purge_lag",
    "innodb_mirrored_log_groups", "innodb_old_blocks_pct", "innodb_old_blocks_time",
    "innodb_open_files", "innodb_print_all_deadlocks", "innodb_purge_batch_size",
    "innodb_purge_threads", "innodb_random_read_ahead", "innodb_read_ahead_threshold",
    "innodb_read_io_threads", "innodb_replication_delay", "innodb_rollback_on_timeout",
    "innodb_spin_wait_delay", "innodb_stats_auto_recalc", "innodb_stats_method",
    "innodb_stats_on_metadata", "innodb_stats_persistent", "innodb_stats_sample_pages",
    "innodb_stats_transient_sample_pages", "innodb_strict_mode", "innodb_support_xa",
    "innodb_sync_spin_loops", "innodb_table_locks", "innodb_thread_concurrency",
    "innodb_thread_sleep_delay", "innodb_use_native_aio", "innodb_use_sys_malloc",
    "innodb_version", "innodb_write_io_threads", "interactive_timeout", "join_buffer_size",
    "key_buffer_size", "key_cache_age_threshold", "key_cache_block_size", "key_cache_division_limit",
    "language", "large_files_support", "large_page_size", "large_pages", "lc_messages",
    "lc_messages_dir", "lc_time_names", "license", "local_infile", "locked_in_memory",
    "log", "log_bin", "log_bin_basename", "log_bin_index", "log_bin_trust_function_creators",
    "log_error", "log_queries_not_using_indexes", "log_slave_updates", "log_slow_queries",
    "log_slow_time", "long_query_time", "low_priority_updates", "lower_case_file_system",
    "lower_case_table_names", "max_allowed_packet", "max_binlog_cache_size", "max_binlog_size",
    "max_binlog_stmt_cache_size", "max_connections", "max_connect_errors", "max_delayed_threads",
    "max_error_count", "max_heap_table_size", "max_insert_delayed_threads", "max_join_size",
    "max_length_for_sort_data", "max_prepared_stmt_count", "max_relay_log_size", "max_seeks_for_key",
    "max_sort_length", "max_sp_recursion_depth", "max_tmp_tables", "max_user_connections",
    "max_write_lock_count", "min_examined_row_limit", "myisam_data_pointer_size", "myisam_max_sort_file_size",
    "myisam_mmap_size", "myisam_recover_options", "myisam_repair_threads", "myisam_sort_buffer_size",
    "myisam_stats_method", "myisam_use_mmap", "named_pipe", "net_buffer_length", "net_read_timeout",
    "net_retry_count", "net_write_timeout", "new", "old", "old_alter_table", "old_passwords",
    "optimizer_prune_level", "optimizer_search_depth", "optimizer_switch", "pid_file", "plugin_dir",
    "port", "preload_buffer_size", "profiling", "profiling_history_size", "protocol_version",
    "query_alloc_block_size", "query_cache_limit", "query_cache_min_res_unit", "query_cache_size",
    "query_cache_type", "query_cache_wlock_invalidate", "query_prealloc_size", "range_alloc_block_size",
    "read_buffer_size", "read_only", "read_rnd_buffer_size", "relay_log", "relay_log_basename",
    "relay_log_index", "relay_log_info_file", "relay_log_purge", "relay_log_recovery",
    "relay_log_space_limit", "replicate_do_db", "replicate_do_table", "replicate_ignore_db",
    "replicate_ignore_table", "replicate_wild_do_table", "replicate_wild_ignore_table",
    "report_host", "report_password", "report_port", "report_user", "rpl_recovery_rank",
    "secure_auth", "secure_file_priv", "server_id", "shared_memory", "shared_memory_base_name",
    "skip_external_locking", "skip_name_resolve", "skip_networking", "skip_show_database",
    "slave_compressed_protocol", "slave_exec_mode", "slave_load_tmpdir", "slave_max_allowed_packet",
    "slave_net_timeout", "slave_skip_errors", "slave_transaction_retries", "slow_launch_time",
    "slow_query_log", "slow_query_log_file", "socket", "sort_buffer_size", "sql_auto_is_null",
    "sql_big_selects", "sql_big_tables", "sql_buffer_result", "sql_log_bin", "sql_log_off",
    "sql_log_update", "sql_low_priority_updates", "sql_max_join_size", "sql_mode", "sql_notes",
    "sql_quote_show_create", "sql_safe_updates", "sql_select_limit", "sql_slave_skip_counter",
    "sql_warnings", "ssl_ca", "ssl_capath", "ssl_cert", "ssl_cipher", "ssl_key", "storage_engine",
    "sync_binlog", "sync_frm", "sync_master_info", "sync_relay_log", "sync_relay_log_info",
    "system_time_zone", "table_definition_cache", "table_open_cache", "thread_cache_size",
    "thread_concurrency", "thread_handling", "thread_stack", "time_format", "time_zone",
    "timed_mutexes", "timestamp", "tmp_table_size", "tmpdir", "transaction_alloc_block_size",
    "transaction_isolation", "transaction_prealloc_size", "tx_isolation", "unique_checks",
    "updatable_views_with_limit", "version", "version_comment", "version_compile_machine",
    "version_compile_os", "wait_timeout", "warning_count"
};

const QStringList MariaDBConfig::CONFIG_CATEGORIES = {
    "General", "Memory", "Storage Engines", "Security", "Connections", "Logging",
    "Replication", "Performance", "InnoDB", "MyISAM", "Query Cache", "MySQL"
};

const QStringList MariaDBConfig::STORAGE_ENGINES = {
    "InnoDB", "MyISAM", "MEMORY", "CSV", "ARCHIVE", "BLACKHOLE", "MERGE", "FEDERATED",
    "Aria", "XtraDB", "TokuDB", "RocksDB", "MyRocks", "Spider", "CONNECT", "S3"
};

const QStringList MariaDBConfig::ISOLATION_LEVELS = {
    "READ UNCOMMITTED", "READ COMMITTED", "REPEATABLE READ", "SERIALIZABLE"
};

const QStringList MariaDBConfig::CHARACTER_SETS = {
    "utf8", "utf8mb4", "utf8mb3", "latin1", "latin2", "ascii", "binary", "cp1250",
    "cp1251", "cp1256", "cp1257", "cp850", "cp852", "cp866", "cp932", "eucjpms",
    "euckr", "gb2312", "gbk", "geostd8", "greek", "hebrew", "hp8", "keybcs2",
    "koi8r", "koi8u", "macce", "macroman", "sjis", "swe7", "tis620", "ucs2",
    "ujis", "utf16", "utf16le", "utf32"
};

const QStringList MariaDBConfig::COLLATIONS = {
    "utf8_general_ci", "utf8_bin", "utf8_unicode_ci", "utf8mb4_general_ci",
    "utf8mb4_bin", "utf8mb4_unicode_ci", "latin1_swedish_ci", "latin1_general_ci",
    "ascii_general_ci", "binary"
};

const QStringList MariaDBConfig::SQL_MODES = {
    "STRICT_TRANS_TABLES", "ERROR_FOR_DIVISION_BY_ZERO", "NO_AUTO_CREATE_USER",
    "NO_AUTO_VALUE_ON_ZERO", "NO_BACKSLASH_ESCAPES", "NO_DIR_IN_CREATE",
    "NO_ENGINE_SUBSTITUTION", "NO_FIELD_OPTIONS", "NO_KEY_OPTIONS",
    "NO_TABLE_OPTIONS", "NO_UNSIGNED_SUBTRACTION", "NO_ZERO_DATE", "NO_ZERO_IN_DATE",
    "ONLY_FULL_GROUP_BY", "PIPES_AS_CONCAT", "REAL_AS_FLOAT", "STRICT_ALL_TABLES"
};

const QStringList MariaDBConfig::TIME_ZONES = {
    "SYSTEM", "UTC", "GMT", "EST", "CST", "MST", "PST", "+00:00", "-05:00", "-08:00"
};

const QStringList MariaDBConfig::LOG_LEVELS = {
    "ERROR", "WARNING", "NOTE", "INFO"
};

const QStringList MariaDBConfig::OPTIMIZER_SWITCHES = {
    "index_merge", "index_merge_union", "index_merge_sort_union", "index_merge_intersection",
    "engine_condition_pushdown", "index_condition_pushdown", "mrr", "mrr_cost_based",
    "block_nested_loop", "batched_key_access", "materialization", "semijoin", "loosescan",
    "firstmatch", "duplicateweedout", "subquery_materialization_cost_based", "use_index_extensions",
    "condition_fanout_filter", "derived_merge", "derived_with_keys", "prefer_ordering_index",
    "subquery_to_derived", "join_cache_incremental", "join_cache_hashed", "join_cache_bka",
    "join_cache_bka_unique", "table_elimination", "extended_keys", "exists_to_in", "orderby_uses_equalities",
    "condition_pushdown_for_derived", "split_materialized", "condition_pushdown_for_subquery",
    "rowid_filter", "condition_pushdown_from_having"
};

// MariaDB Feature Detector Implementation

bool MariaDBFeatureDetector::isFeatureSupported(DatabaseType dbType, const QString& feature) {
    if (dbType != DatabaseType::MYSQL && dbType != DatabaseType::MARIADB) {
        return false;
    }

    return getSupportedFeatures(dbType).contains(feature, Qt::CaseInsensitive);
}

QStringList MariaDBFeatureDetector::getSupportedFeatures(DatabaseType dbType) {
    if (dbType != DatabaseType::MYSQL && dbType != DatabaseType::MARIADB) {
        return QStringList();
    }

    QStringList features;
    features << "CTE" << "WINDOW_FUNCTIONS" << "JSON" << "SPATIAL" << "SEQUENCES"
             << "DYNAMIC_COLUMNS" << "VIRTUAL_COLUMNS" << "PARTITIONING" << "GTID"
             << "PERFORMANCE_SCHEMA" << "REPLICATION" << "ENCRYPTION";

    return features;
}

QString MariaDBFeatureDetector::getVersionSpecificSyntax(DatabaseType dbType, const QString& version) {
    if (dbType != DatabaseType::MYSQL && dbType != DatabaseType::MARIADB) {
        return QString();
    }

    // Parse version and return appropriate syntax
    QRegularExpression versionRegex("(\\d+)\\.(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match = versionRegex.match(version);

    if (match.hasMatch()) {
        int major = match.captured(1).toInt();
        int minor = match.captured(2).toInt();

        // MariaDB 10.2+ (JSON, window functions)
        if (major >= 10 && minor >= 2) {
            return "10.2+";
        }
        // MariaDB 10.0+ (sequences, virtual columns)
        else if (major >= 10) {
            return "10.0+";
        }
        // MariaDB 5.5+
        else if (major >= 5 && minor >= 5) {
            return "5.5+";
        }
    }

    return "5.1"; // Default fallback
}

bool MariaDBFeatureDetector::supportsJSON(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsSequences(DatabaseType dbType) {
    return dbType == DatabaseType::MARIADB; // MariaDB-specific feature
}

bool MariaDBFeatureDetector::supportsVirtualColumns(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsDynamicColumns(DatabaseType dbType) {
    return dbType == DatabaseType::MARIADB; // MariaDB-specific feature
}

bool MariaDBFeatureDetector::supportsWindowFunctions(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsCTEs(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsSpatial(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsPartitioning(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsGTID(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsPerformanceSchema(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsReplication(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsEncryption(DatabaseType dbType) {
    return dbType == DatabaseType::MYSQL || dbType == DatabaseType::MARIADB;
}

bool MariaDBFeatureDetector::supportsFeatureByVersion(const QString& version, const QString& feature) {
    QRegularExpression versionRegex("(\\d+)\\.(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match = versionRegex.match(version);

    if (!match.hasMatch()) {
        return false;
    }

    int major = match.captured(1).toInt();
    int minor = match.captured(2).toInt();
    QString minVersion = getMinimumVersionForFeature(feature);

    if (minVersion.isEmpty()) {
        return true; // Feature supported in all versions
    }

    QRegularExpressionMatch minMatch = versionRegex.match(minVersion);
    if (minMatch.hasMatch()) {
        int minMajor = minMatch.captured(1).toInt();
        int minMinor = minMatch.captured(2).toInt();
        return (major > minMajor) || (major == minMajor && minor >= minMinor);
    }

    return false;
}

QString MariaDBFeatureDetector::getMinimumVersionForFeature(const QString& feature) {
    static QMap<QString, QString> featureVersions = {
        {"JSON", "10.2.0"},           // MariaDB 10.2
        {"WINDOW_FUNCTIONS", "10.2.0"}, // MariaDB 10.2
        {"SEQUENCES", "10.3.0"},      // MariaDB 10.3
        {"DYNAMIC_COLUMNS", "5.5.0"}, // MariaDB 5.5
        {"VIRTUAL_COLUMNS", "5.2.0"}, // MariaDB 5.2
        {"PARTITIONING", "5.1.0"},    // MariaDB 5.1
        {"GTID", "10.0.0"},           // MariaDB 10.0
        {"CTE", "10.2.0"}             // MariaDB 10.2
    };

    return featureVersions.value(feature.toUpper(), "");
}

} // namespace scratchrobin
