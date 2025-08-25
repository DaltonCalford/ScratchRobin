#include "postgresql_features.h"
#include <QRegularExpression>

namespace scratchrobin {

// PostgreSQL Data Types Implementation
const QStringList PostgreSQLDataTypes::NUMERIC_TYPES = {
    "smallint", "integer", "bigint", "decimal", "numeric", "real", "double precision",
    "serial", "bigserial", "smallserial", "money", "float", "float4", "float8"
};

const QStringList PostgreSQLDataTypes::STRING_TYPES = {
    "character", "char", "character varying", "varchar", "text", "name", "bytea"
};

const QStringList PostgreSQLDataTypes::DATE_TIME_TYPES = {
    "timestamp", "timestamp with time zone", "timestamptz",
    "timestamp without time zone", "date", "time", "time with time zone",
    "timetz", "time without time zone", "interval"
};

const QStringList PostgreSQLDataTypes::BINARY_TYPES = {
    "bytea", "bit", "bit varying", "varbit"
};

const QStringList PostgreSQLDataTypes::BOOLEAN_TYPES = {
    "boolean", "bool"
};

const QStringList PostgreSQLDataTypes::ARRAY_TYPES = {
    "smallint[]", "integer[]", "bigint[]", "decimal[]", "numeric[]", "real[]",
    "double precision[]", "text[]", "varchar[]", "char[]", "boolean[]", "date[]",
    "timestamp[]", "timestamptz[]", "bytea[]", "json[]", "jsonb[]"
};

const QStringList PostgreSQLDataTypes::JSON_TYPES = {
    "json", "jsonb"
};

const QStringList PostgreSQLDataTypes::GEOMETRIC_TYPES = {
    "point", "line", "lseg", "box", "path", "polygon", "circle"
};

const QStringList PostgreSQLDataTypes::NETWORK_TYPES = {
    "cidr", "inet", "macaddr", "macaddr8"
};

const QStringList PostgreSQLDataTypes::TEXT_SEARCH_TYPES = {
    "tsvector", "tsquery"
};

const QStringList PostgreSQLDataTypes::RANGE_TYPES = {
    "int4range", "int8range", "numrange", "tsrange", "tstzrange", "daterange"
};

const QStringList PostgreSQLDataTypes::UUID_TYPES = {
    "uuid"
};

const QStringList PostgreSQLDataTypes::HSTORE_TYPES = {
    "hstore"
};

QStringList PostgreSQLDataTypes::getAllDataTypes() {
    QStringList allTypes;
    allTypes << NUMERIC_TYPES << STRING_TYPES << DATE_TIME_TYPES
             << BINARY_TYPES << BOOLEAN_TYPES << ARRAY_TYPES
             << JSON_TYPES << GEOMETRIC_TYPES << NETWORK_TYPES
             << TEXT_SEARCH_TYPES << RANGE_TYPES << UUID_TYPES
             << HSTORE_TYPES;
    return allTypes;
}

// PostgreSQL SQL Features Implementation
const QStringList PostgreSQLSQLFeatures::KEYWORDS = {
    "ABORT", "ABSOLUTE", "ACCESS", "ACTION", "ADD", "ADMIN", "AFTER", "AGGREGATE",
    "ALL", "ALSO", "ALTER", "ALWAYS", "ANALYSE", "ANALYZE", "AND", "ANY", "ARRAY",
    "AS", "ASC", "ASSERTION", "ASSIGNMENT", "ASYMMETRIC", "AT", "AUTHORIZATION",
    "BACKWARD", "BEFORE", "BEGIN", "BETWEEN", "BIGINT", "BIGSERIAL", "BINARY",
    "BIT", "BOOLEAN", "BOTH", "BY", "CACHE", "CALLED", "CASCADE", "CASCADED",
    "CASE", "CAST", "CATALOG", "CHAIN", "CHAR", "CHARACTER", "CHARACTERISTICS",
    "CHECK", "CHECKPOINT", "CLASS", "CLOSE", "CLUSTER", "COALESCE", "COLLATE",
    "COLLATION", "COLUMN", "COMMENT", "COMMENTS", "COMMIT", "COMMITTED", "CONCURRENTLY",
    "CONFIGURATION", "CONNECTION", "CONSTRAINT", "CONSTRAINTS", "CONTENT", "CONTINUE",
    "CONVERSION", "COPY", "COST", "CREATE", "CROSS", "CSV", "CUBE", "CURRENT",
    "CURRENT_CATALOG", "CURRENT_DATE", "CURRENT_ROLE", "CURRENT_SCHEMA", "CURRENT_TIME",
    "CURRENT_TIMESTAMP", "CURRENT_USER", "CURSOR", "CYCLE", "DATA", "DATABASE",
    "DAY", "DEALLOCATE", "DEC", "DECIMAL", "DECLARE", "DEFAULT", "DEFAULTS",
    "DEFERRABLE", "DEFERRED", "DEFINER", "DELETE", "DELIMITER", "DELIMITERS",
    "DESC", "DICTIONARY", "DISABLE", "DISCARD", "DISTINCT", "DO", "DOCUMENT",
    "DOMAIN", "DOUBLE", "DROP", "EACH", "ELSE", "ENABLE", "ENCODING", "ENCRYPTED",
    "END", "ENUM", "ESCAPE", "EVENT", "EXCEPT", "EXCLUDE", "EXCLUDING", "EXCLUSIVE",
    "EXECUTE", "EXISTS", "EXPLAIN", "EXTENSION", "EXTERNAL", "EXTRACT", "FALSE",
    "FAMILY", "FETCH", "FILTER", "FIRST", "FLOAT", "FOLLOWING", "FOR", "FORCE",
    "FOREIGN", "FORWARD", "FREEZE", "FROM", "FULL", "FUNCTION", "FUNCTIONS", "GLOBAL",
    "GRANT", "GRANTED", "GREATEST", "GROUP", "GROUPING", "HANDLER", "HAVING",
    "HEADER", "HOLD", "HOUR", "IDENTITY", "IF", "ILIKE", "IMMEDIATE", "IMMUTABLE",
    "IMPLICIT", "IMPORT", "IN", "INCLUDING", "INCREMENT", "INDEX", "INDEXES",
    "INHERIT", "INHERITS", "INITIALLY", "INLINE", "INNER", "INOUT", "INPUT",
    "INSENSITIVE", "INSERT", "INSTEAD", "INT", "INTEGER", "INTERSECT", "INTERVAL",
    "INTO", "INVOKER", "IS", "ISNULL", "ISOLATION", "JOIN", "KEY", "LABEL",
    "LANGUAGE", "LARGE", "LAST", "LATERAL", "LEADING", "LEAKPROOF", "LEAST",
    "LEFT", "LEVEL", "LIKE", "LIMIT", "LISTEN", "LOAD", "LOCAL", "LOCALTIME",
    "LOCALTIMESTAMP", "LOCATION", "LOCK", "LOCKED", "LOGGED", "MAPPING", "MATCH",
    "MATERIALIZED", "MAXVALUE", "MINUTE", "MINVALUE", "MODE", "MONTH", "MOVE",
    "NAME", "NAMES", "NATIONAL", "NATURAL", "NCHAR", "NEXT", "NO", "NONE", "NOT",
    "NOTHING", "NOTIFY", "NOTNULL", "NOWAIT", "NULL", "NULLIF", "NULLS", "NUMERIC",
    "OBJECT", "OF", "OFF", "OFFSET", "OIDS", "ON", "ONLY", "OPERATOR", "OPTION",
    "OPTIONS", "OR", "ORDER", "ORDINALITY", "OUT", "OUTER", "OVER", "OVERLAPS",
    "OVERLAY", "OWNED", "OWNER", "PARSER", "PARTIAL", "PARTITION", "PASSING",
    "PASSWORD", "PLACING", "PLANS", "POLICY", "POSITION", "PRECEDING", "PRECISION",
    "PREPARE", "PREPARED", "PRESERVE", "PRIMARY", "PRIOR", "PRIVILEGES", "PROCEDURAL",
    "PROCEDURE", "PROGRAM", "QUOTE", "RANGE", "READ", "REAL", "REASSIGN", "RECHECK",
    "RECURSIVE", "REF", "REFERENCES", "REFRESH", "REINDEX", "RELATIVE", "RELEASE",
    "RENAME", "REPEATABLE", "REPLACE", "REPLICA", "RESET", "RESTART", "RESTRICT",
    "RETURNING", "RETURNS", "REVOKE", "RIGHT", "ROLE", "ROLLBACK", "ROLLUP", "ROW",
    "ROWS", "RULE", "SAVEPOINT", "SCHEMA", "SCROLL", "SEARCH", "SECOND", "SECURITY",
    "SELECT", "SEQUENCE", "SEQUENCES", "SERIALIZABLE", "SERVER", "SESSION", "SESSION_USER",
    "SET", "SETOF", "SETS", "SHARE", "SHOW", "SIMILAR", "SIMPLE", "SMALLINT",
    "SMALLSERIAL", "SNAPSHOT", "SOME", "SQL", "STABLE", "STANDALONE", "START",
    "STATEMENT", "STATISTICS", "STDIN", "STDOUT", "STORAGE", "STRICT", "STRIP",
    "SUBSTRING", "SYMMETRIC", "SYSID", "SYSTEM", "TABLE", "TABLES", "TABLESPACE",
    "TEMP", "TEMPLATE", "TEMPORARY", "TEXT", "THEN", "TIME", "TIMESTAMP", "TO",
    "TRAILING", "TRANSACTION", "TRANSFORM", "TREAT", "TRIGGER", "TRIM", "TRUE",
    "TRUNCATE", "TRUSTED", "TYPE", "TYPES", "UESCAPE", "UNBOUNDED", "UNCOMMITTED",
    "UNENCRYPTED", "UNION", "UNIQUE", "UNKNOWN", "UNLISTEN", "UNLOGGED", "UNTIL",
    "UPDATE", "USER", "USING", "VACUUM", "VALID", "VALIDATE", "VALIDATOR", "VALUE",
    "VALUES", "VARBIT", "VARCHAR", "VARIADIC", "VARYING", "VERBOSE", "VERSION",
    "VIEW", "VIEWS", "VOLATILE", "WHEN", "WHERE", "WHITESPACE", "WINDOW", "WITH",
    "WITHIN", "WITHOUT", "WORK", "WRAPPER", "WRITE", "XML", "XMLATTRIBUTES",
    "XMLCONCAT", "XMLELEMENT", "XMLEXISTS", "XMLFOREST", "XMLPARSE", "XMLPI",
    "XMLROOT", "XMLSERIALIZE", "YEAR", "YES", "ZONE", "ZEROFILL"
};

const QStringList PostgreSQLSQLFeatures::FUNCTIONS = {
    // String functions
    "ASCII", "BIT_LENGTH", "BTRIM", "CHAR_LENGTH", "CHARACTER_LENGTH", "CHR",
    "CONCAT", "CONCAT_WS", "CONVERT", "DECODE", "ENCODE", "FORMAT", "INITCAP",
    "LEFT", "LENGTH", "LOWER", "LPAD", "LTRIM", "MD5", "POSITION", "QUOTE_IDENT",
    "QUOTE_LITERAL", "QUOTE_NULLABLE", "REPEAT", "REPLACE", "REVERSE", "RIGHT",
    "RPAD", "RTRIM", "SPLIT_PART", "STRPOS", "SUBSTR", "SUBSTRING", "TO_ASCII",
    "TO_HEX", "TRANSLATE", "TRIM", "UPPER",

    // Numeric functions
    "ABS", "ACOS", "ASIN", "ATAN", "ATAN2", "CEIL", "CEILING", "COS", "COT",
    "DEGREES", "DIV", "EXP", "FLOOR", "LN", "LOG", "MOD", "PI", "POWER", "RADIANS",
    "RANDOM", "ROUND", "SIGN", "SIN", "SQRT", "TAN", "TRUNC", "WIDTH_BUCKET",

    // Date/Time functions
    "AGE", "CLOCK_TIMESTAMP", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP",
    "DATE_PART", "DATE_TRUNC", "EXTRACT", "ISFINITE", "JUSTIFY_DAYS", "JUSTIFY_HOURS",
    "JUSTIFY_INTERVAL", "LOCALTIME", "LOCALTIMESTAMP", "MAKE_DATE", "MAKE_INTERVAL",
    "MAKE_TIME", "MAKE_TIMESTAMP", "MAKE_TIMESTAMPTZ", "NOW", "STATEMENT_TIMESTAMP",
    "TIMEZONE", "TO_CHAR", "TO_DATE", "TO_NUMBER", "TO_TIMESTAMP", "TRANSACTION_TIMESTAMP",

    // Array functions
    "ARRAY_AGG", "ARRAY_APPEND", "ARRAY_CAT", "ARRAY_DIMS", "ARRAY_FILL", "ARRAY_LENGTH",
    "ARRAY_LOWER", "ARRAY_NDIMS", "ARRAY_POSITION", "ARRAY_POSITIONS", "ARRAY_PREPEND",
    "ARRAY_REMOVE", "ARRAY_REPLACE", "ARRAY_TO_JSON", "ARRAY_TO_STRING", "ARRAY_UPPER",
    "CARDINALITY", "STRING_TO_ARRAY", "UNNEST",

    // JSON functions
    "JSON_AGG", "JSON_ARRAY_ELEMENTS", "JSON_ARRAY_ELEMENTS_TEXT", "JSON_ARRAY_LENGTH",
    "JSON_BUILD_ARRAY", "JSON_BUILD_OBJECT", "JSON_EACH", "JSON_EACH_TEXT", "JSON_EXTRACT_PATH",
    "JSON_EXTRACT_PATH_TEXT", "JSON_OBJECT", "JSON_OBJECT_AGG", "JSON_OBJECT_KEYS",
    "JSON_POPULATE_RECORD", "JSON_POPULATE_RECORDSET", "JSON_TO_RECORD", "JSON_TO_RECORDSET",
    "JSON_TYPEOF", "ROW_TO_JSON", "TO_JSON", "TO_JSONB",

    // Text search functions
    "PLAINTO_TSQUERY", "PHRASETO_TSQUERY", "QUERYTREE", "SETWEIGHT", "STRIP", "TO_TSQUERY",
    "TO_TSVECTOR", "TS_HEADLINE", "TS_RANK", "TS_RANK_CD", "TS_REWRITE", "TS_STAT",
    "TS_TOKEN_TYPE", "TSVECTOR_UPDATE_TRIGGER", "TSVECTOR_UPDATE_TRIGGER_COLUMN",

    // Window functions
    "ROW_NUMBER", "RANK", "DENSE_RANK", "PERCENT_RANK", "CUME_DIST", "NTILE", "LAG", "LEAD",
    "FIRST_VALUE", "LAST_VALUE", "NTH_VALUE", "COUNT", "SUM", "AVG", "MIN", "MAX",

    // Aggregate functions
    "COUNT", "SUM", "AVG", "MIN", "MAX", "STRING_AGG", "ARRAY_AGG", "BOOL_AND", "BOOL_OR",
    "EVERY", "BIT_AND", "BIT_OR", "XMLAGG",

    // Other functions
    "COALESCE", "NULLIF", "GREATEST", "LEAST", "CASE", "CAST", "EXISTS", "IN", "GENERATE_SERIES",
    "GENERATE_SUBSCRIPTS", "CURRENT_CATALOG", "CURRENT_SCHEMA", "CURRENT_USER", "SESSION_USER"
};

const QStringList PostgreSQLSQLFeatures::OPERATORS = {
    // Comparison operators
    "=", ">", "<", ">=", "<=", "<>", "!=", "!<", "!>",

    // Arithmetic operators
    "+", "-", "*", "/", "%", "^", "|/", "||/", "!",

    // String operators
    "||", "LIKE", "NOT LIKE", "ILIKE", "NOT ILIKE", "SIMILAR TO", "NOT SIMILAR TO",
    "POSIX", "~", "~*", "!~", "!~*", "@@",

    // Logical operators
    "AND", "OR", "NOT",

    // Array operators
    "[]", "[i:j]", "[i:j:k]", "&&", "@>", "<@", "#", "?", "?|", "?&",

    // JSON operators
    "->", "->>", "#>", "#>>", "@>", "<@", "?", "?|", "?&",

    // Text search operators
    "@@", "&&", "||", "!!", "@@", "<->", "<#>", "<@>",

    // Geometric operators
    "@", "@@", "##", "<@", "@>", "<<", ">>", "&<", "&>", "<<|", "|>>", "&<|", "|&>", "<@>",
    "<#>", "<<#", "#>>", "<<->", "<->", "<<<", ">>>", "&", "|", "#", "<->", "<#>", "@@",

    // Network operators
    "<<", ">>", "<<=", ">>=", "@>", "<@", "<@>", "<<|", "|>>", "&<|", "|&>", ">>=",

    // Range operators
    "@>", "<@", "<<", ">>", "&<", "&>", "-|-", "*", "+",

    // Other operators
    "IN", "NOT IN", "BETWEEN", "NOT BETWEEN", "IS", "IS NOT", "ISNULL", "NOTNULL",
    "EXISTS", "NOT EXISTS", "ANY", "ALL", "SOME", "DISTINCT FROM", "NOT DISTINCT FROM"
};

const QStringList PostgreSQLSQLFeatures::RESERVED_WORDS = {
    "ABORT", "ABSOLUTE", "ACCESS", "ACTION", "ADD", "ADMIN", "AFTER", "AGGREGATE",
    "ALL", "ALSO", "ALTER", "ALWAYS", "ANALYSE", "ANALYZE", "AND", "ANY", "ARRAY",
    "AS", "ASC", "ASSERTION", "ASSIGNMENT", "ASYMMETRIC", "AT", "AUTHORIZATION",
    "BACKWARD", "BEFORE", "BEGIN", "BETWEEN", "BIGINT", "BIGSERIAL", "BINARY",
    "BIT", "BOOLEAN", "BOTH", "BY", "CACHE", "CALLED", "CASCADE", "CASCADED",
    "CASE", "CAST", "CATALOG", "CHAIN", "CHAR", "CHARACTER", "CHARACTERISTICS",
    "CHECK", "CHECKPOINT", "CLASS", "CLOSE", "CLUSTER", "COALESCE", "COLLATE",
    "COLLATION", "COLUMN", "COMMENT", "COMMENTS", "COMMIT", "COMMITTED", "CONCURRENTLY",
    "CONFIGURATION", "CONNECTION", "CONSTRAINT", "CONSTRAINTS", "CONTENT", "CONTINUE",
    "CONVERSION", "COPY", "COST", "CREATE", "CROSS", "CSV", "CUBE", "CURRENT",
    "CURRENT_CATALOG", "CURRENT_DATE", "CURRENT_ROLE", "CURRENT_SCHEMA", "CURRENT_TIME",
    "CURRENT_TIMESTAMP", "CURRENT_USER", "CURSOR", "CYCLE", "DATA", "DATABASE",
    "DAY", "DEALLOCATE", "DEC", "DECIMAL", "DECLARE", "DEFAULT", "DEFAULTS",
    "DEFERRABLE", "DEFERRED", "DEFINER", "DELETE", "DELIMITER", "DELIMITERS",
    "DESC", "DICTIONARY", "DISABLE", "DISCARD", "DISTINCT", "DO", "DOCUMENT",
    "DOMAIN", "DOUBLE", "DROP", "EACH", "ELSE", "ENABLE", "ENCODING", "ENCRYPTED",
    "END", "ENUM", "ESCAPE", "EVENT", "EXCEPT", "EXCLUDE", "EXCLUDING", "EXCLUSIVE",
    "EXECUTE", "EXISTS", "EXPLAIN", "EXTENSION", "EXTERNAL", "EXTRACT", "FALSE",
    "FAMILY", "FETCH", "FILTER", "FIRST", "FLOAT", "FOLLOWING", "FOR", "FORCE",
    "FOREIGN", "FORWARD", "FREEZE", "FROM", "FULL", "FUNCTION", "FUNCTIONS", "GLOBAL",
    "GRANT", "GRANTED", "GREATEST", "GROUP", "GROUPING", "HANDLER", "HAVING",
    "HEADER", "HOLD", "HOUR", "IDENTITY", "IF", "ILIKE", "IMMEDIATE", "IMMUTABLE",
    "IMPLICIT", "IMPORT", "IN", "INCLUDING", "INCREMENT", "INDEX", "INDEXES",
    "INHERIT", "INHERITS", "INITIALLY", "INLINE", "INNER", "INOUT", "INPUT",
    "INSENSITIVE", "INSERT", "INSTEAD", "INT", "INTEGER", "INTERSECT", "INTERVAL",
    "INTO", "INVOKER", "IS", "ISNULL", "ISOLATION", "JOIN", "KEY", "LABEL",
    "LANGUAGE", "LARGE", "LAST", "LATERAL", "LEADING", "LEAKPROOF", "LEAST",
    "LEFT", "LEVEL", "LIKE", "LIMIT", "LISTEN", "LOAD", "LOCAL", "LOCALTIME",
    "LOCALTIMESTAMP", "LOCATION", "LOCK", "LOCKED", "LOGGED", "MAPPING", "MATCH",
    "MATERIALIZED", "MAXVALUE", "MINUTE", "MINVALUE", "MODE", "MONTH", "MOVE",
    "NAME", "NAMES", "NATIONAL", "NATURAL", "NCHAR", "NEXT", "NO", "NONE", "NOT",
    "NOTHING", "NOTIFY", "NOTNULL", "NOWAIT", "NULL", "NULLIF", "NULLS", "NUMERIC",
    "OBJECT", "OF", "OFF", "OFFSET", "OIDS", "ON", "ONLY", "OPERATOR", "OPTION",
    "OPTIONS", "OR", "ORDER", "ORDINALITY", "OUT", "OUTER", "OVER", "OVERLAPS",
    "OVERLAY", "OWNED", "OWNER", "PARSER", "PARTIAL", "PARTITION", "PASSING",
    "PASSWORD", "PLACING", "PLANS", "POLICY", "POSITION", "PRECEDING", "PRECISION",
    "PREPARE", "PREPARED", "PRESERVE", "PRIMARY", "PRIOR", "PRIVILEGES", "PROCEDURAL",
    "PROCEDURE", "PROGRAM", "QUOTE", "RANGE", "READ", "REAL", "REASSIGN", "RECHECK",
    "RECURSIVE", "REF", "REFERENCES", "REFRESH", "REINDEX", "RELATIVE", "RELEASE",
    "RENAME", "REPEATABLE", "REPLACE", "REPLICA", "RESET", "RESTART", "RESTRICT",
    "RETURNING", "RETURNS", "REVOKE", "RIGHT", "ROLE", "ROLLBACK", "ROLLUP", "ROW",
    "ROWS", "RULE", "SAVEPOINT", "SCHEMA", "SCROLL", "SEARCH", "SECOND", "SECURITY",
    "SELECT", "SEQUENCE", "SEQUENCES", "SERIALIZABLE", "SERVER", "SESSION", "SESSION_USER",
    "SET", "SETOF", "SETS", "SHARE", "SHOW", "SIMILAR", "SIMPLE", "SMALLINT",
    "SMALLSERIAL", "SNAPSHOT", "SOME", "SQL", "STABLE", "STANDALONE", "START",
    "STATEMENT", "STATISTICS", "STDIN", "STDOUT", "STORAGE", "STRICT", "STRIP",
    "SUBSTRING", "SYMMETRIC", "SYSID", "SYSTEM", "TABLE", "TABLES", "TABLESPACE",
    "TEMP", "TEMPLATE", "TEMPORARY", "TEXT", "THEN", "TIME", "TIMESTAMP", "TO",
    "TRAILING", "TRANSACTION", "TRANSFORM", "TREAT", "TRIGGER", "TRIM", "TRUE",
    "TRUNCATE", "TRUSTED", "TYPE", "TYPES", "UESCAPE", "UNBOUNDED", "UNCOMMITTED",
    "UNENCRYPTED", "UNION", "UNIQUE", "UNKNOWN", "UNLISTEN", "UNLOGGED", "UNTIL",
    "UPDATE", "USER", "USING", "VACUUM", "VALID", "VALIDATE", "VALIDATOR", "VALUE",
    "VALUES", "VARBIT", "VARCHAR", "VARIADIC", "VARYING", "VERBOSE", "VERSION",
    "VIEW", "VIEWS", "VOLATILE", "WHEN", "WHERE", "WHITESPACE", "WINDOW", "WITH",
    "WITHIN", "WITHOUT", "WORK", "WRAPPER", "WRITE", "XML", "XMLATTRIBUTES",
    "XMLCONCAT", "XMLELEMENT", "XMLEXISTS", "XMLFOREST", "XMLPARSE", "XMLPI",
    "XMLROOT", "XMLSERIALIZE", "YEAR", "YES", "ZONE", "ZEROFILL"
};

const QStringList PostgreSQLSQLFeatures::CTE_FEATURES = {
    "WITH", "RECURSIVE", "AS"
};

const QStringList PostgreSQLSQLFeatures::WINDOW_FEATURES = {
    "OVER", "PARTITION BY", "ORDER BY", "ROWS", "RANGE", "UNBOUNDED PRECEDING",
    "UNBOUNDED FOLLOWING", "CURRENT ROW", "PRECEDING", "FOLLOWING", "FIRST_VALUE",
    "LAST_VALUE", "NTH_VALUE", "ROW_NUMBER", "RANK", "DENSE_RANK", "PERCENT_RANK",
    "CUME_DIST", "NTILE", "LAG", "LEAD"
};

const QStringList PostgreSQLSQLFeatures::ARRAY_FEATURES = {
    "ARRAY", "[]", "[i:j]", "[i:j:k]", "ARRAY_AGG", "ARRAY_APPEND", "ARRAY_CAT",
    "ARRAY_DIMS", "ARRAY_FILL", "ARRAY_LENGTH", "ARRAY_LOWER", "ARRAY_NDIMS",
    "ARRAY_POSITION", "ARRAY_POSITIONS", "ARRAY_PREPEND", "ARRAY_REMOVE",
    "ARRAY_REPLACE", "ARRAY_TO_JSON", "ARRAY_TO_STRING", "ARRAY_UPPER",
    "CARDINALITY", "STRING_TO_ARRAY", "UNNEST"
};

const QStringList PostgreSQLSQLFeatures::JSON_FEATURES = {
    "JSON", "JSONB", "->", "->>", "#>", "#>>", "@>", "<@", "?", "?|", "?&",
    "JSON_AGG", "JSON_ARRAY_ELEMENTS", "JSON_ARRAY_ELEMENTS_TEXT", "JSON_ARRAY_LENGTH",
    "JSON_BUILD_ARRAY", "JSON_BUILD_OBJECT", "JSON_EACH", "JSON_EACH_TEXT",
    "JSON_EXTRACT_PATH", "JSON_EXTRACT_PATH_TEXT", "JSON_OBJECT", "JSON_OBJECT_AGG",
    "JSON_OBJECT_KEYS", "JSON_POPULATE_RECORD", "JSON_POPULATE_RECORDSET",
    "JSON_TO_RECORD", "JSON_TO_RECORDSET", "JSON_TYPEOF", "ROW_TO_JSON", "TO_JSON", "TO_JSONB"
};

const QStringList PostgreSQLSQLFeatures::TEXT_SEARCH_FEATURES = {
    "TSVECTOR", "TSQUERY", "@@", "&&", "||", "!!", "<->", "<#>", "<@>", "PLAINTO_TSQUERY",
    "PHRASETO_TSQUERY", "QUERYTREE", "SETWEIGHT", "STRIP", "TO_TSQUERY", "TO_TSVECTOR",
    "TS_HEADLINE", "TS_RANK", "TS_RANK_CD", "TS_REWRITE", "TS_STAT", "TS_TOKEN_TYPE",
    "TSVECTOR_UPDATE_TRIGGER", "TSVECTOR_UPDATE_TRIGGER_COLUMN"
};

// PostgreSQL Objects Implementation
const QStringList PostgreSQLObjects::SYSTEM_SCHEMAS = {
    "pg_catalog", "information_schema", "pg_toast", "pg_temp_1", "pg_toast_temp_1"
};

const QStringList PostgreSQLObjects::CATALOG_TABLES = {
    "pg_class", "pg_attribute", "pg_type", "pg_proc", "pg_namespace", "pg_database",
    "pg_index", "pg_constraint", "pg_trigger", "pg_operator", "pg_opclass", "pg_am",
    "pg_amop", "pg_amproc", "pg_language", "pg_rewrite", "pg_cast", "pg_conversion",
    "pg_aggregate", "pg_statistic", "pg_statistic_ext", "pg_foreign_table", "pg_foreign_server",
    "pg_foreign_data_wrapper", "pg_user_mapping", "pg_enum", "pg_extension", "pg_authid",
    "pg_auth_members", "pg_tablespace", "pg_shdepend", "pg_shdescription", "pg_ts_config",
    "pg_ts_config_map", "pg_ts_dict", "pg_ts_parser", "pg_ts_template", "pg_extension",
    "pg_available_extensions", "pg_available_extension_versions", "pg_config", "pg_cursors",
    "pg_file_settings", "pg_group", "pg_hba_file_rules", "pg_ident_file_mappings",
    "pg_indexes", "pg_locks", "pg_matviews", "pg_policies", "pg_prepared_statements",
    "pg_prepared_xacts", "pg_publication", "pg_publication_tables", "pg_replication_origin",
    "pg_replication_origin_status", "pg_replication_slots", "pg_roles", "pg_rules",
    "pg_seclabel", "pg_seclabels", "pg_sequences", "pg_settings", "pg_shadow",
    "pg_shmem_allocations", "pg_stats", "pg_stats_ext", "pg_subscription",
    "pg_subscription_rel", "pg_tables", "pg_timezone_abbrevs", "pg_timezone_names",
    "pg_transform", "pg_trigger", "pg_user", "pg_views", "pg_wait_events",
    "pg_wait_events_type"
};

const QStringList PostgreSQLObjects::INFORMATION_SCHEMA_VIEWS = {
    "information_schema_catalog_name", "applicable_roles", "administrable_role_authorizations",
    "attributes", "character_sets", "check_constraint_routine_usage", "check_constraints",
    "collations", "collation_character_set_applicability", "column_domain_usage",
    "column_privileges", "column_udt_usage", "columns", "constraint_column_usage",
    "constraint_table_usage", "data_type_privileges", "domain_constraints",
    "domain_udt_usage", "domains", "element_types", "enabled_roles", "foreign_data_wrapper_options",
    "foreign_data_wrappers", "foreign_server_options", "foreign_servers",
    "foreign_table_options", "foreign_tables", "information_schema_catalog_name",
    "key_column_usage", "parameters", "referential_constraints", "role_column_grants",
    "role_routine_grants", "role_table_grants", "role_udt_grants", "role_usage_grants",
    "routine_privileges", "routines", "schemata", "sequence_privileges", "sequences",
    "sql_features", "sql_implementation_info", "sql_languages", "sql_packages",
    "sql_parts", "sql_sizing", "sql_sizing_profiles", "table_constraints",
    "table_privileges", "tables", "transforms", "triggered_update_columns",
    "triggers", "udt_privileges", "usage_privileges", "user_defined_types",
    "user_mapping_options", "user_mappings", "view_column_usage", "view_routine_usage",
    "view_table_usage", "views"
};

const QStringList PostgreSQLObjects::BUILTIN_FUNCTIONS = {
    "abs", "acos", "age", "array_agg", "array_append", "array_cat", "array_dims",
    "array_fill", "array_length", "array_lower", "array_ndims", "array_position",
    "array_positions", "array_prepend", "array_remove", "array_replace", "array_to_json",
    "array_to_string", "array_upper", "ascii", "asin", "atan", "atan2", "avg", "bit_and",
    "bit_length", "bit_or", "bool_and", "bool_or", "btrim", "cardinality", "cbrt",
    "ceil", "ceiling", "char_length", "character_length", "chr", "clock_timestamp",
    "coalesce", "col_description", "concat", "concat_ws", "convert", "convert_from",
    "convert_to", "cos", "cot", "count", "cume_dist", "current_catalog", "current_database",
    "current_date", "current_query", "current_role", "current_schema", "current_setting",
    "current_time", "current_timestamp", "current_user", "currval", "cursor_to_xml",
    "cursor_to_xmlschema", "database_to_xml", "database_to_xml_and_xmlschema", "date_part",
    "date_trunc", "decode", "degrees", "dense_rank", "div", "encode", "enum_first",
    "enum_last", "enum_range", "every", "exp", "extract", "factorial", "first_value",
    "floor", "format", "frame_end", "frame_start", "generate_series", "generate_subscripts",
    "get_bit", "get_byte", "get_current_ts_config", "getcwd", "gcd", "gen_random_uuid",
    "greatest", "group_concat", "has_any_column_privilege", "has_column_privilege",
    "has_database_privilege", "has_foreign_data_wrapper_privilege", "has_function_privilege",
    "has_language_privilege", "has_schema_privilege", "has_sequence_privilege",
    "has_server_privilege", "has_table_privilege", "has_tablespace_privilege",
    "has_type_privilege", "host", "hostmask", "inet_client_addr", "inet_client_port",
    "inet_server_addr", "inet_server_port", "initcap", "is_array", "is_ipv4", "is_ipv6",
    "isfinite", "isinf", "isnan", "justify_days", "justify_hours", "justify_interval",
    "lag", "language_handler", "last_value", "lastval", "lcm", "lead", "least", "left",
    "length", "ln", "localtime", "localtimestamp", "log", "lower", "lpad", "ltrim",
    "make_date", "make_interval", "make_time", "make_timestamp", "make_timestamptz",
    "masklen", "max", "md5", "min", "mod", "netmask", "network", "nextval", "no_inherit",
    "now", "nth_value", "ntile", "nullif", "num_nonnulls", "num_nulls", "obj_description",
    "octet_length", "overlay", "parse_ident", "percent_rank", "percentile_cont",
    "percentile_disc", "pg_advisory_lock", "pg_advisory_lock_shared", "pg_advisory_unlock",
    "pg_advisory_unlock_all", "pg_advisory_unlock_shared", "pg_advisory_xact_lock",
    "pg_advisory_xact_lock_shared", "pg_backup_start_time", "pg_blocking_pids",
    "pg_cancel_backend", "pg_client_encoding", "pg_collation_is_visible", "pg_column_size",
    "pg_conf_load_time", "pg_control_checkpoint", "pg_control_init", "pg_control_recovery",
    "pg_control_system", "pg_conversion_is_visible", "pg_create_logical_replication_slot",
    "pg_create_physical_replication_slot", "pg_create_restore_point", "pg_current_logfile",
    "pg_current_snapshot", "pg_current_wal_flush_lsn", "pg_current_wal_insert_lsn",
    "pg_current_wal_lsn", "pg_current_xlog_flush_location", "pg_current_xlog_insert_location",
    "pg_current_xlog_location", "pg_database_size", "pg_describe_object", "pg_drop_replication_slot",
    "pg_export_snapshot", "pg_filenode_relation", "pg_function_is_visible", "pg_get_constraintdef",
    "pg_get_expr", "pg_get_function_arguments", "pg_get_function_identity_arguments",
    "pg_get_function_result", "pg_get_functiondef", "pg_get_indexdef", "pg_get_keywords",
    "pg_get_object_address", "pg_get_owned_sequence", "pg_get_ruledef", "pg_get_serial_sequence",
    "pg_get_triggerdef", "pg_get_userbyid", "pg_get_viewdef", "pg_has_role", "pg_identify_object",
    "pg_identify_object_as_address", "pg_index_column_has_property", "pg_index_has_property",
    "pg_indexam_has_property", "pg_indexes_size", "pg_is_in_backup", "pg_is_in_recovery",
    "pg_is_other_temp_schema", "pg_is_wal_replay_paused", "pg_last_committed_xact",
    "pg_last_xact_replay_timestamp", "pg_last_xlog_receive_location", "pg_last_xlog_replay_location",
    "pg_listening_channels", "pg_lock_status", "pg_logdir_ls", "pg_logical_slot_get_binary_changes",
    "pg_logical_slot_get_changes", "pg_logical_slot_peek_binary_changes", "pg_logical_slot_peek_changes",
    "pg_ls_archive_statusdir", "pg_ls_dir", "pg_ls_logdir", "pg_ls_waldir", "pg_my_temp_schema",
    "pg_notification_queue_usage", "pg_opclass_is_visible", "pg_operator_is_visible",
    "pg_options_to_table", "pg_postmaster_start_time", "pg_prepared_statement", "pg_prepared_statements",
    "pg_prepared_xact", "pg_prepared_xacts", "pg_read_binary_file", "pg_read_file", "pg_read_file_old",
    "pg_relation_filenode", "pg_relation_filepath", "pg_relation_size", "pg_reload_conf",
    "pg_replication_origin_create", "pg_replication_origin_drop", "pg_replication_origin_oid",
    "pg_replication_origin_progress", "pg_replication_origin_session_is_setup",
    "pg_replication_origin_session_progress", "pg_replication_origin_session_reset",
    "pg_replication_origin_session_setup", "pg_replication_origin_xact_reset",
    "pg_replication_origin_xact_setup", "pg_replication_slot_advance", "pg_rotate_logfile",
    "pg_safe_snapshot_blocking_pids", "pg_size_bytes", "pg_size_pretty", "pg_sleep",
    "pg_sleep_for", "pg_sleep_until", "pg_start_backup", "pg_stat_clear_snapshot",
    "pg_stat_file", "pg_stat_get_activity", "pg_stat_get_analyze_count", "pg_stat_get_archiver",
    "pg_stat_get_autovacuum_count", "pg_stat_get_backend_activity", "pg_stat_get_backend_activity_start",
    "pg_stat_get_backend_client_addr", "pg_stat_get_backend_client_port", "pg_stat_get_backend_dbid",
    "pg_stat_get_backend_idset", "pg_stat_get_backend_pid", "pg_stat_get_backend_start",
    "pg_stat_get_backend_userid", "pg_stat_get_backend_wait_event", "pg_stat_get_backend_wait_event_type",
    "pg_stat_get_backend_xact_start", "pg_stat_get_bgwriter_buf_written_checkpoints",
    "pg_stat_get_bgwriter_buf_written_clean", "pg_stat_get_bgwriter_maxwritten_clean",
    "pg_stat_get_bgwriter_requested_checkpoints", "pg_stat_get_bgwriter_stat_reset_time",
    "pg_stat_get_bgwriter_timed_checkpoints", "pg_stat_get_blocks_fetched", "pg_stat_get_blocks_hit",
    "pg_stat_get_buf_alloc", "pg_stat_get_buf_fsync_backend", "pg_stat_get_buf_written_backend",
    "pg_stat_get_checkpoint_sync_time", "pg_stat_get_checkpoint_write_time", "pg_stat_get_db_blk_read_time",
    "pg_stat_get_db_blk_write_time", "pg_stat_get_db_blocks_fetched", "pg_stat_get_db_blocks_hit",
    "pg_stat_get_db_conflict_all", "pg_stat_get_db_conflict_bufferpin", "pg_stat_get_db_conflict_lock",
    "pg_stat_get_db_conflict_snapshot", "pg_stat_get_db_conflict_startup_deadlock",
    "pg_stat_get_db_conflict_tablespace", "pg_stat_get_db_deadlocks", "pg_stat_get_db_numbackends",
    "pg_stat_get_db_stat_reset_time", "pg_stat_get_db_temp_bytes", "pg_stat_get_db_temp_files",
    "pg_stat_get_db_tuples_deleted", "pg_stat_get_db_tuples_fetched", "pg_stat_get_db_tuples_inserted",
    "pg_stat_get_db_tuples_returned", "pg_stat_get_db_tuples_updated", "pg_stat_get_db_xact_commit",
    "pg_stat_get_db_xact_rollback", "pg_stat_get_dead_tuples", "pg_stat_get_function_calls",
    "pg_stat_get_function_self_time", "pg_stat_get_function_total_time", "pg_stat_get_ins_since_vacuum",
    "pg_stat_get_last_analyze_time", "pg_stat_get_last_autovacuum", "pg_stat_get_last_vacuum",
    "pg_stat_get_live_tuples", "pg_stat_get_mod_since_analyze", "pg_stat_get_numscans",
    "pg_stat_get_tuples_deleted", "pg_stat_get_tuples_fetched", "pg_stat_get_tuples_hot_updated",
    "pg_stat_get_tuples_inserted", "pg_stat_get_tuples_returned", "pg_stat_get_tuples_updated",
    "pg_stat_get_vacuum_count", "pg_stat_get_wal_receiver", "pg_stat_get_wal_senders",
    "pg_stat_get_xact_blocks_fetched", "pg_stat_get_xact_blocks_hit", "pg_stat_get_xact_function_calls",
    "pg_stat_get_xact_function_self_time", "pg_stat_get_xact_function_total_time",
    "pg_stat_get_xact_numscans", "pg_stat_get_xact_tuples_deleted", "pg_stat_get_xact_tuples_fetched",
    "pg_stat_get_xact_tuples_hot_updated", "pg_stat_get_xact_tuples_inserted",
    "pg_stat_get_xact_tuples_returned", "pg_stat_get_xact_tuples_updated", "pg_stat_reset",
    "pg_stat_reset_shared", "pg_stat_reset_single_function", "pg_stat_reset_single_table",
    "pg_stop_backup", "pg_switch_wal", "pg_switch_xlog", "pg_table_is_visible", "pg_table_size",
    "pg_tablespace_databases", "pg_tablespace_size", "pg_terminate_backend", "pg_timezone_abbrevs",
    "pg_timezone_names", "pg_total_relation_size", "pg_trigger_depth", "pg_try_advisory_lock",
    "pg_try_advisory_lock_shared", "pg_try_advisory_xact_lock", "pg_try_advisory_xact_lock_shared",
    "pg_ts_config_is_visible", "pg_ts_dict_is_visible", "pg_ts_parser_is_visible",
    "pg_ts_template_is_visible", "pg_type_is_visible", "pg_typeof", "pg_wal_replay_pause",
    "pg_wal_replay_resume", "pg_walfile_name", "pg_walfile_name_offset", "pg_xact_commit_timestamp",
    "pg_xlog_replay_pause", "pg_xlog_replay_resume", "pi", "plainto_tsquery", "plpgsql_call_handler",
    "plpgsql_inline_handler", "plpgsql_validator", "position", "pow", "power", "pq_server_version",
    "pqgetssl", "pqhost", "pqhostaddr", "pqoptions", "pqpass", "pqport", "pquser", "query_to_xml",
    "query_to_xml_and_xmlschema", "query_to_xmlschema", "querytree", "quote_ident", "quote_literal",
    "quote_nullable", "radians", "random", "rank", "regexp_matches", "regexp_replace", "regexp_split_to_array",
    "regexp_split_to_table", "repeat", "replace", "reverse", "right", "round", "row_number",
    "row_security_active", "row_to_json", "rpad", "rtrim", "scale", "schema_to_xml", "schema_to_xml_and_xmlschema",
    "schema_to_xmlschema", "session_user", "set_bit", "set_byte", "set_config", "set_masklen",
    "setseed", "setval", "setweight", "sha224", "sha256", "sha384", "sha512", "shobj_description",
    "sign", "sin", "split_part", "sqrt", "statement_timestamp", "stddev", "stddev_pop", "stddev_samp",
    "string_agg", "string_to_array", "strip", "strpos", "substr", "substring", "sum", "table_to_xml",
    "table_to_xml_and_xmlschema", "table_to_xmlschema", "tan", "text", "to_ascii", "to_char", "to_date",
    "to_hex", "to_json", "to_jsonb", "to_number", "to_regclass", "to_regnamespace", "to_regoper",
    "to_regoperator", "to_regproc", "to_regprocedure", "to_regrole", "to_regtype", "to_timestamp",
    "to_tsquery", "to_tsvector", "transaction_timestamp", "translate", "trigger", "trim", "trim_scale",
    "trunc", "ts_headline", "ts_rank", "ts_rank_cd", "ts_rewrite", "ts_stat", "ts_token_type", "tsvector_to_array",
    "tsvector_update_trigger", "tsvector_update_trigger_column", "txid_current", "txid_current_snapshot",
    "txid_snapshot_xip", "txid_snapshot_xmax", "txid_snapshot_xmin", "txid_visible_in_snapshot", "unnest",
    "upper", "user", "var_pop", "var_samp", "variance", "version", "width_bucket", "xml", "xml_is_well_formed",
    "xml_is_well_formed_content", "xml_is_well_formed_document", "xmlagg", "xmlcomment", "xmlconcat",
    "xmlelement", "xmlexists", "xmlforest", "xmlparse", "xmlpi", "xmlroot", "xmlserialize", "xpath",
    "xpath_exists", "xslt_process"
};

const QStringList PostgreSQLObjects::BUILTIN_OPERATORS = {
    "=", ">", "<", ">=", "<=", "<>", "!=", "!<", "!>", "+", "-", "*", "/", "%", "^",
    "|/", "||/", "!", "@", "@@", "##", "<@", "@>", "<<", ">>", "&<", "&>", "<<|",
    "|>>", "&<|", "|&>", "<@>", "<#>", "<<#", "#>>", "<<->", "<->", "<<<", ">>>",
    "&", "|", "#", "<->", "<#>", "@@", "&&", "||", "!!", "@@", "<->", "<#>", "<@>",
    "?", "?|", "?&", "@>", "<@", "<@>", "<<|", "|>>", "&<|", "|&>", ">>=", "-|-",
    "*", "+", "IN", "NOT IN", "BETWEEN", "NOT BETWEEN", "IS", "IS NOT", "ISNULL",
    "NOTNULL", "EXISTS", "NOT EXISTS", "ANY", "ALL", "SOME", "DISTINCT FROM",
    "NOT DISTINCT FROM", "LIKE", "NOT LIKE", "ILIKE", "NOT ILIKE", "SIMILAR TO",
    "NOT SIMILAR TO", "POSIX", "~", "~*", "!~", "!~*", "@@", "[]", "[i:j]", "[i:j:k]",
    "->", "->>", "#>", "#>>", "@>", "<@", "?", "?|", "?&"
};

// PostgreSQL Config Implementation
const QStringList PostgreSQLConfig::CONFIG_PARAMETERS = {
    "allow_system_table_mods", "application_name", "archive_command", "archive_mode",
    "archive_timeout", "array_nulls", "authentication_timeout", "autovacuum",
    "autovacuum_analyze_scale_factor", "autovacuum_analyze_threshold", "autovacuum_freeze_max_age",
    "autovacuum_max_workers", "autovacuum_multixact_freeze_max_age", "autovacuum_naptime",
    "autovacuum_vacuum_cost_delay", "autovacuum_vacuum_cost_limit", "autovacuum_vacuum_scale_factor",
    "autovacuum_vacuum_threshold", "autovacuum_work_mem", "backend_flush_after", "bgwriter_delay",
    "bgwriter_flush_after", "bgwriter_lru_maxpages", "bgwriter_lru_multiplier", "block_size",
    "bonjour", "bonjour_name", "bytea_output", "checkpoint_completion_target", "checkpoint_flush_after",
    "checkpoint_segments", "checkpoint_timeout", "checkpoint_warning", "client_encoding",
    "client_min_messages", "cluster_name", "commit_delay", "commit_siblings", "config_file",
    "constraint_exclusion", "cpu_index_tuple_cost", "cpu_operator_cost", "cpu_tuple_cost",
    "cursor_tuple_fraction", "data_checksums", "data_directory", "data_sync_retry", "DateStyle",
    "db_user_namespace", "deadlock_timeout", "debug_assertions", "debug_pretty_print",
    "debug_print_parse", "debug_print_plan", "debug_print_rewritten", "debug_print_stats",
    "default_statistics_target", "default_tablespace", "default_text_search_config",
    "default_transaction_deferrable", "default_transaction_isolation", "default_transaction_read_only",
    "default_with_oids", "dynamic_library_path", "dynamic_shared_memory_type", "effective_cache_size",
    "effective_io_concurrency", "enable_bitmapscan", "enable_hashagg", "enable_hashjoin",
    "enable_indexonlyscan", "enable_indexscan", "enable_material", "enable_mergejoin",
    "enable_nestloop", "enable_seqscan", "enable_sort", "enable_tidscan", "escape_string_warning",
    "event_source", "exit_on_error", "external_pid_file", "extra_float_digits", "from_collapse_limit",
    "fsync", "full_page_writes", "geqo", "geqo_effort", "geqo_generations", "geqo_pool_size",
    "geqo_seed", "geqo_selection_bias", "geqo_threshold", "gin_fuzzy_search_limit",
    "gin_pending_list_limit", "hba_file", "hot_standby", "hot_standby_feedback", "huge_pages",
    "ident_file", "idle_in_transaction_session_timeout", "ignore_checksum_failure", "ignore_system_indexes",
    "integer_datetimes", "IntervalStyle", "join_collapse_limit", "krb_caseins_users",
    "krb_server_keyfile", "lc_collate", "lc_ctype", "lc_messages", "lc_monetary", "lc_numeric",
    "lc_time", "listen_addresses", "local_preload_libraries", "lock_timeout", "log_autovacuum_min_duration",
    "log_checkpoints", "log_connections", "log_destination", "log_directory", "log_disconnections",
    "log_duration", "log_error_verbosity", "log_executor_stats", "log_file_mode", "log_filename",
    "log_hostname", "log_line_prefix", "log_lock_waits", "log_min_duration_statement", "log_min_error_statement",
    "log_min_messages", "log_parser_stats", "log_planner_stats", "log_rotation_age", "log_rotation_size",
    "log_statement", "log_statement_stats", "log_temp_files", "log_timezone", "log_truncate_on_rotation",
    "logging_collector", "maintenance_work_mem", "max_connections", "max_files_per_process",
    "max_function_args", "max_identifier_length", "max_index_keys", "max_locks_per_transaction",
    "max_parallel_workers_per_gather", "max_pred_locks_per_transaction", "max_prepared_transactions",
    "max_replication_slots", "max_stack_depth", "max_standby_archive_delay", "max_standby_streaming_delay",
    "max_wal_senders", "max_worker_processes", "min_parallel_relation_size", "min_wal_size",
    "old_snapshot_threshold", "operator_precedence_warning", "parallel_setup_cost", "parallel_tuple_cost",
    "password_encryption", "port", "post_auth_delay", "pre_auth_delay", "quote_all_identifiers",
    "random_page_cost", "replacement_sort_tuples", "restart_after_crash", "row_security",
    "search_path", "segment_size", "seq_page_cost", "server_encoding", "server_version",
    "server_version_num", "session_preload_libraries", "session_replication_role", "shared_buffers",
    "shared_preload_libraries", "sql_inheritance", "ssl", "ssl_ca_file", "ssl_cert_file",
    "ssl_ciphers", "ssl_crl_file", "ssl_key_file", "ssl_prefer_server_ciphers", "standard_conforming_strings",
    "statement_timeout", "stats_temp_directory", "superuser_reserved_connections", "synchronize_seqscans",
    "synchronous_commit", "synchronous_standby_names", "syslog_facility", "syslog_ident",
    "syslog_sequence_numbers", "syslog_split_messages", "tcp_keepalives_count", "tcp_keepalives_idle",
    "tcp_keepalives_interval", "temp_buffers", "temp_file_limit", "temp_tablespaces", "TimeZone",
    "timezone_abbreviations", "trace_notify", "trace_sort", "track_activities", "track_counts",
    "track_functions", "track_io_timing", "track_wal_io_timing", "transaction_deferrable",
    "transaction_isolation", "transaction_read_only", "transform_null_equals", "unix_socket_directories",
    "unix_socket_group", "unix_socket_permissions", "update_process_title", "vacuum_cost_delay",
    "vacuum_cost_limit", "vacuum_cost_page_dirty", "vacuum_cost_page_hit", "vacuum_cost_page_miss",
    "vacuum_defer_cleanup_age", "vacuum_freeze_min_age", "vacuum_freeze_table_age", "vacuum_multixact_freeze_min_age",
    "vacuum_multixact_freeze_table_age", "wal_block_size", "wal_buffers", "wal_compression",
    "wal_keep_segments", "wal_level", "wal_log_hints", "wal_receiver_status_interval",
    "wal_receiver_timeout", "wal_retrieve_retry_interval", "wal_sender_timeout", "wal_sync_method",
    "wal_writer_delay", "work_mem", "xmlbinary", "xmloption", "zero_damaged_pages"
};

const QStringList PostgreSQLConfig::CONFIG_CATEGORIES = {
    "Autovacuum", "Client Connection Defaults", "Connections and Authentication",
    "Developer Options", "Error Handling", "File Locations", "Lock Management",
    "Memory", "Preset Options", "Query Planning", "Replication", "Reporting and Logging",
    "Resource Usage", "Statistics", "System Administration", "Version and Platform Compatibility",
    "WAL", "Write Ahead Log"
};

const QStringList PostgreSQLConfig::LOG_LEVELS = {
    "DEBUG5", "DEBUG4", "DEBUG3", "DEBUG2", "DEBUG1", "INFO", "NOTICE", "WARNING",
    "ERROR", "LOG", "FATAL", "PANIC"
};

const QStringList PostgreSQLConfig::ISOLATION_LEVELS = {
    "READ UNCOMMITTED", "READ COMMITTED", "REPEATABLE READ", "SERIALIZABLE"
};

// PostgreSQL Feature Detector Implementation

bool PostgreSQLFeatureDetector::isFeatureSupported(DatabaseType dbType, const QString& feature) {
    if (dbType != DatabaseType::POSTGRESQL) {
        return false;
    }

    return getSupportedFeatures(dbType).contains(feature, Qt::CaseInsensitive);
}

QStringList PostgreSQLFeatureDetector::getSupportedFeatures(DatabaseType dbType) {
    if (dbType != DatabaseType::POSTGRESQL) {
        return QStringList();
    }

    QStringList features;
    features << "CTE" << "WINDOW_FUNCTIONS" << "JSON" << "ARRAYS" << "GEOMETRY"
             << "TEXT_SEARCH" << "RANGES" << "UUID" << "HSTORE" << "INHERITANCE"
             << "PARTITIONING";

    return features;
}

QString PostgreSQLFeatureDetector::getVersionSpecificSyntax(DatabaseType dbType, const QString& version) {
    if (dbType != DatabaseType::POSTGRESQL) {
        return QString();
    }

    // Parse version and return appropriate syntax
    QRegularExpression versionRegex("(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match = versionRegex.match(version);

    if (match.hasMatch()) {
        int major = match.captured(1).toInt();
        int minor = match.captured(2).toInt();

        // PostgreSQL 13+ (advanced features)
        if (major >= 13) {
            return "13+";
        }
        // PostgreSQL 12+ (advanced partitioning, generated columns)
        else if (major >= 12) {
            return "12+";
        }
        // PostgreSQL 11+ (procedural languages, partitioning)
        else if (major >= 11) {
            return "11+";
        }
        // PostgreSQL 10+ (logical replication, partitioning)
        else if (major >= 10) {
            return "10+";
        }
        // PostgreSQL 9.6+
        else if (major == 9 && minor >= 6) {
            return "9.6+";
        }
        // PostgreSQL 9.5+
        else if (major == 9 && minor >= 5) {
            return "9.5+";
        }
        // PostgreSQL 9.4+
        else if (major == 9 && minor >= 4) {
            return "9.4+";
        }
        // PostgreSQL 9.3+
        else if (major == 9 && minor >= 3) {
            return "9.3+";
        }
        // PostgreSQL 9.2+
        else if (major == 9 && minor >= 2) {
            return "9.2+";
        }
        // PostgreSQL 9.1+
        else if (major == 9 && minor >= 1) {
            return "9.1+";
        }
    }

    return "9.0"; // Default fallback
}

bool PostgreSQLFeatureDetector::supportsArrays(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsJSON(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsHStore(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsGeometry(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsTextSearch(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsRanges(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsCTEs(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsWindowFunctions(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsInheritance(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsPartitioning(DatabaseType dbType) {
    return dbType == DatabaseType::POSTGRESQL;
}

bool PostgreSQLFeatureDetector::supportsFeatureByVersion(const QString& version, const QString& feature) {
    QRegularExpression versionRegex("(\\d+)\\.(\\d+)");
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

QString PostgreSQLFeatureDetector::getMinimumVersionForFeature(const QString& feature) {
    static QMap<QString, QString> featureVersions = {
        {"JSON", "9.2.0"},           // PostgreSQL 9.2
        {"CTE", "8.4.0"},            // PostgreSQL 8.4
        {"WINDOW_FUNCTIONS", "8.4.0"}, // PostgreSQL 8.4
        {"ARRAYS", "8.0.0"},         // PostgreSQL 8.0
        {"GEOMETRY", "8.0.0"},       // PostgreSQL 8.0
        {"TEXT_SEARCH", "8.3.0"},    // PostgreSQL 8.3
        {"RANGES", "9.2.0"},         // PostgreSQL 9.2
        {"UUID", "8.3.0"},           // PostgreSQL 8.3
        {"HSTORE", "8.4.0"},         // PostgreSQL 8.4 (contrib)
        {"INHERITANCE", "7.4.0"},    // PostgreSQL 7.4
        {"PARTITIONING", "10.0.0"}   // PostgreSQL 10.0 (declarative)
    };

    return featureVersions.value(feature.toUpper(), "");
}

} // namespace scratchrobin
