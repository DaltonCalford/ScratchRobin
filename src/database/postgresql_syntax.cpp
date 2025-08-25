#include "postgresql_syntax.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

namespace scratchrobin {

// PostgreSQL Syntax Patterns Implementation

const QStringList PostgreSQLSyntaxPatterns::RESERVED_KEYWORDS = {
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

const QStringList PostgreSQLSyntaxPatterns::NON_RESERVED_KEYWORDS = {
    "AUTHID", "BINARY_DOUBLE", "BINARY_FLOAT", "BLOB", "BODY", "BREADTH", "CLOB",
    "CONTAINS", "CONTEXT", "COST", "CROSS", "CUBE", "CURRENT_PATH", "CURRENT_TRANSFORM_GROUP_FOR_TYPE",
    "CYCLE", "DATA", "DEBUG", "DEPTH", "DETERMINISTIC", "DOCUMENT", "EACH", "ELEMENT",
    "EMPTY", "ENCODING", "ERROR", "EXCEPTION", "EXCLUDING", "FINAL", "FIRST", "FOLLOWING",
    "FORALL", "FORCE", "GENERAL", "GENERATED", "GOTO", "GROUPS", "HASH", "IGNORE",
    "INDEXED", "INDICATOR", "INFINITE", "INSTANTIABLE", "INSTEAD", "ISOLATION", "JAVA",
    "LANGUAGE", "LAST", "LEADING", "LENGTH", "LEVEL", "LIBRARY", "LIKE2", "LIKE4",
    "LIKEC", "LIMIT", "LOCAL", "LOG", "MAP", "MATCHED", "MAXVALUE", "MEASURES", "MEMBER",
    "MERGE", "MINUS", "MOD", "MULTISET", "NAME", "NAN", "NATURAL", "NAV", "NCHAR_CS",
    "NCLOB", "NESTED", "NOAUDIT", "NOCOMPRESS", "NOCOPY", "NOPARALLEL", "NOVALIDATE",
    "NOWAIT", "NULLS", "NUMBER", "OBJECT", "ONLY", "OPAQUE", "OPEN", "OPTIMIZER",
    "ORACLE", "ORDER", "ORGANIZATION", "OTHERS", "OUT", "PACKAGE", "PARALLEL", "PARAMETERS",
    "PARTITION", "PASCAL", "PCTFREE", "PCTUSED", "PRECISION", "PRESENT", "PRIOR",
    "PROCEDURE", "RAISE", "RANGE", "RAW", "RECORD", "REF", "REJECT", "RESPECT",
    "RESTRICT_REFERENCES", "RESULT", "RETURN", "REVERSE", "ROLLBACK", "ROLLUP",
    "SAMPLE", "SAVE", "SCHEMA", "SEGMENT", "SELF", "SEQUENCE", "SEQUENTIAL", "SIBLINGS",
    "SINGLE", "SIZE", "SPACE", "SPATIAL", "SPECIFICATION", "START", "STATIC", "STATISTICS",
    "STRING", "STRUCTURE", "SUBMULTISET", "SUBPARTITION", "SUBSTITUTABLE", "SUCCESSFUL",
    "SYNONYM", "THE", "THEN", "TIMEZONE_ABBR", "TIMEZONE_HOUR", "TIMEZONE_MINUTE",
    "TIMEZONE_REGION", "TRAILING", "TRANSACTION", "TYPE", "UNDER", "UNION", "UNIQUE",
    "UNKNOWN", "UNLIMITED", "UNPIVOT", "UNTIL", "USE", "USING", "VALIDATE", "VALUE",
    "VARYING", "WHEN", "WHENEVER", "WHERE", "WITH", "WITHIN", "XMLAGG", "XMLATTRIBUTES",
    "XMLCAST", "XMLCOLATTVAL", "XMLELEMENT", "XMLEXISTS", "XMLFOREST", "XMLNAMESPACES",
    "XMLPARSE", "XMLPI", "XMLQUERY", "XMLROOT", "XMLSERIALIZE", "XMLTABLE", "YEAR",
    "YES", "ZONE"
};

const QStringList PostgreSQLSyntaxPatterns::DATA_TYPES = {
    "smallint", "integer", "bigint", "decimal", "numeric", "real", "double precision",
    "serial", "bigserial", "smallserial", "money", "float", "float4", "float8",
    "character", "char", "character varying", "varchar", "text", "name", "bytea",
    "timestamp", "timestamp with time zone", "timestamptz", "timestamp without time zone",
    "date", "time", "time with time zone", "timetz", "time without time zone", "interval",
    "bit", "bit varying", "varbit", "boolean", "bool", "point", "line", "lseg", "box",
    "path", "polygon", "circle", "cidr", "inet", "macaddr", "macaddr8", "tsvector",
    "tsquery", "int4range", "int8range", "numrange", "tsrange", "tstzrange", "daterange",
    "uuid", "json", "jsonb", "hstore", "ARRAY", "ENUM"
};

const QStringList PostgreSQLSyntaxPatterns::BUILTIN_FUNCTIONS = {
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
    "length", "ln", "localtime", "localtimestamp", "locate", "log", "lower", "lpad",
    "ltrim", "make_date", "make_interval", "make_time", "make_timestamp", "make_timestamptz",
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

const QStringList PostgreSQLSyntaxPatterns::OPERATORS = {
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

const QString PostgreSQLSyntaxPatterns::SINGLE_LINE_COMMENT = "--";
const QString PostgreSQLSyntaxPatterns::MULTI_LINE_COMMENT_START = "/*";
const QString PostgreSQLSyntaxPatterns::MULTI_LINE_COMMENT_END = "*/";
const QString PostgreSQLSyntaxPatterns::STRING_LITERAL = "'[^']*'";
const QString PostgreSQLSyntaxPatterns::IDENTIFIER = "[a-zA-Z_][a-zA-Z0-9_]*";
const QString PostgreSQLSyntaxPatterns::QUOTED_IDENTIFIER = "\"[^\"]*\"";
const QString PostgreSQLSyntaxPatterns::NUMBER_LITERAL = "\\b\\d+\\.?\\d*\\b";
const QString PostgreSQLSyntaxPatterns::VARIABLE = "\\$[0-9]+|\\$[a-zA-Z_][a-zA-Z0-9_]*";
const QString PostgreSQLSyntaxPatterns::SYSTEM_VARIABLE = "current_setting\\([^)]+\\)|pg_settings\\.[a-zA-Z_][a-zA-Z0-9_]*";
const QString PostgreSQLSyntaxPatterns::ARRAY_LITERAL = "'\\{[^}]*\\}'|ARRAY\\[[^\\]]*\\]";
const QString PostgreSQLSyntaxPatterns::JSON_LITERAL = "'\\{[^}]*\\}'::json|json_build_object\\([^)]*\\)|json_build_array\\([^)]*\\)";

QStringList PostgreSQLSyntaxPatterns::getAllKeywords() {
    QStringList allKeywords;
    allKeywords << RESERVED_KEYWORDS << NON_RESERVED_KEYWORDS;
    return allKeywords;
}

QList<PostgreSQLSyntaxElement> PostgreSQLSyntaxPatterns::getAllSyntaxElements() {
    QList<PostgreSQLSyntaxElement> elements;

    // Add keywords
    for (const QString& keyword : RESERVED_KEYWORDS) {
        elements << PostgreSQLSyntaxElement(keyword, "\\b" + keyword + "\\b", "Reserved keyword", true, false, false, false);
    }

    for (const QString& keyword : NON_RESERVED_KEYWORDS) {
        elements << PostgreSQLSyntaxElement(keyword, "\\b" + keyword + "\\b", "Non-reserved keyword", true, false, false, false);
    }

    // Add data types
    for (const QString& dataType : DATA_TYPES) {
        QString escapedType = dataType;
        escapedType.replace(" ", "\\s+");
        elements << PostgreSQLSyntaxElement(dataType, "\\b" + escapedType + "\\b", "Data type", false, false, false, true);
    }

    // Add functions
    for (const QString& function : BUILTIN_FUNCTIONS) {
        elements << PostgreSQLSyntaxElement(function, "\\b" + function + "\\b", "Built-in function", false, true, false, false);
    }

    // Add operators
    for (const QString& op : OPERATORS) {
        elements << PostgreSQLSyntaxElement(op, "\\b" + QRegularExpression::escape(op) + "\\b", "Operator", false, false, true, false);
    }

    // Add other patterns
    elements << PostgreSQLSyntaxElement("Single-line comment", "--.*", "Single-line comment");
    elements << PostgreSQLSyntaxElement("Multi-line comment", "/\\*.*?\\*/", "Multi-line comment");
    elements << PostgreSQLSyntaxElement("String literal", STRING_LITERAL, "String literal");
    elements << PostgreSQLSyntaxElement("Identifier", IDENTIFIER, "Regular identifier");
    elements << PostgreSQLSyntaxElement("Quoted identifier", QUOTED_IDENTIFIER, "Quoted identifier");
    elements << PostgreSQLSyntaxElement("Number", NUMBER_LITERAL, "Numeric literal");
    elements << PostgreSQLSyntaxElement("Variable", VARIABLE, "Parameter/variable");
    elements << PostgreSQLSyntaxElement("Array literal", ARRAY_LITERAL, "Array literal");
    elements << PostgreSQLSyntaxElement("JSON literal", JSON_LITERAL, "JSON literal");

    return elements;
}

// PostgreSQL Parser Implementation

QList<PostgreSQLSyntaxElement> PostgreSQLParser::parseSQL(const QString& sql) {
    QList<PostgreSQLSyntaxElement> elements;
    QList<PostgreSQLSyntaxElement> patterns = PostgreSQLSyntaxPatterns::getAllSyntaxElements();

    for (const PostgreSQLSyntaxElement& pattern : patterns) {
        QRegularExpression regex(pattern.pattern, QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = regex.globalMatch(sql);

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            PostgreSQLSyntaxElement element = pattern;
            element.name = match.captured();
            elements << element;
        }
    }

    return elements;
}

bool PostgreSQLParser::validateSQLSyntax(const QString& sql, QStringList& errors, QStringList& warnings) {
    errors.clear();
    warnings.clear();

    // Basic validation using PostgreSQLSyntaxValidator
    return PostgreSQLSyntaxValidator::validateSyntax(sql, errors, warnings);
}

QStringList PostgreSQLParser::extractTableNames(const QString& sql) {
    QStringList tableNames;
    QRegularExpression tableRegex("\\bFROM\\s+([\"`]?[\\w\\.]+[\"`]?)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = tableRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString tableName = match.captured(1);
        if (!tableNames.contains(tableName)) {
            tableNames << tableName;
        }
    }

    // Also check JOIN clauses
    QRegularExpression joinRegex("\\bJOIN\\s+([\"`]?[\\w\\.]+[\"`]?)", QRegularExpression::CaseInsensitiveOption);
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

QStringList PostgreSQLParser::extractColumnNames(const QString& sql) {
    QStringList columnNames;

    // Extract from SELECT clause
    QRegularExpression selectRegex("\\bSELECT\\s+(.*?)\\s+FROM\\s+", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch selectMatch = selectRegex.match(sql);

    if (selectMatch.hasMatch()) {
        QString selectClause = selectMatch.captured(1);
        QRegularExpression columnRegex("([\"`]?[\\w\\.]+[\"`]?)");
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

QStringList PostgreSQLParser::extractFunctionNames(const QString& sql) {
    QStringList functionNames;
    QStringList functions = PostgreSQLSyntaxPatterns::BUILTIN_FUNCTIONS;

    for (const QString& function : functions) {
        if (sql.contains(function, Qt::CaseInsensitive)) {
            functionNames << function;
        }
    }

    return functionNames;
}

QStringList PostgreSQLParser::extractVariableNames(const QString& sql) {
    QStringList variableNames;
    QRegularExpression varRegex("\\$[0-9]+|\\$[a-zA-Z_][a-zA-Z0-9_]*");
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

QStringList PostgreSQLParser::extractSchemaNames(const QString& sql) {
    QStringList schemaNames;

    // Extract schema-qualified names
    QRegularExpression schemaRegex("([\"`]?[\\w]+[\"`]?)\\.[\"`]?[\\w]+[\"`]?", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = schemaRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString schemaName = match.captured(1);
        if (!schemaNames.contains(schemaName) && schemaName != "pg_catalog" && schemaName != "information_schema") {
            schemaNames << schemaName;
        }
    }

    return schemaNames;
}

QString PostgreSQLParser::formatSQL(const QString& sql) {
    return PostgreSQLCodeFormatter::formatCode(sql);
}

QStringList PostgreSQLParser::getCompletionSuggestions(const QString& partialText, const QString& context) {
    return PostgreSQLIntelliSense::getCompletions(partialText, partialText.length());
}

bool PostgreSQLParser::needsQuoting(const QString& identifier) {
    // Check if identifier contains special characters or is a reserved word
    QRegularExpression specialCharRegex("[^a-zA-Z0-9_]");
    return specialCharRegex.match(identifier).hasMatch() ||
           PostgreSQLSyntaxPatterns::RESERVED_KEYWORDS.contains(identifier, Qt::CaseInsensitive) ||
           PostgreSQLSyntaxPatterns::NON_RESERVED_KEYWORDS.contains(identifier, Qt::CaseInsensitive);
}

QString PostgreSQLParser::escapeIdentifier(const QString& identifier) {
    if (needsQuoting(identifier)) {
        return "\"" + identifier + "\"";
    }
    return identifier;
}

QString PostgreSQLParser::buildCreateTableQuery(const QString& tableName, const QList<QPair<QString, QString>>& columns, const QString& schema, const QString& database) {
    QString query = "CREATE TABLE ";

    if (!schema.isEmpty()) {
        query += escapeIdentifier(schema) + ".";
    }

    query += escapeIdentifier(tableName) + " (\n";

    QStringList columnDefinitions;
    for (const auto& column : columns) {
        columnDefinitions << "    " + escapeIdentifier(column.first) + " " + column.second;
    }

    query += columnDefinitions.join(",\n");
    query += "\n);";

    return query;
}

bool PostgreSQLParser::parseCreateTable(const QString& sql, QString& tableName, QStringList& columns, QString& options) {
    QRegularExpression createTableRegex("\\bCREATE\\s+TABLE\\s+([\"`]?[\\w\\.]+[\"`]?)\\s*\\((.*)\\)",
                                       QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = createTableRegex.match(sql);

    if (match.hasMatch()) {
        tableName = match.captured(1);
        QString columnsDef = match.captured(2);

        // Parse column definitions (simplified)
        QRegularExpression columnRegex("([\"`]?[\\w]+[\"`]?)\\s+([\\w\\[\\]\\s]+)[^,]*");
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(columnsDef);

        while (it.hasNext()) {
            QRegularExpressionMatch columnMatch = it.next();
            columns << columnMatch.captured(1) + " " + columnMatch.captured(2);
        }

        return true;
    }

    return false;
}

bool PostgreSQLParser::parseCreateIndex(const QString& sql, QString& indexName, QString& tableName, QStringList& columns) {
    QRegularExpression createIndexRegex("\\bCREATE\\s+(?:UNIQUE\\s+)?(?:FULLTEXT\\s+|SPATIAL\\s+)?INDEX\\s+([\"`]?[\\w]+[\"`]?)\\s+ON\\s+([\"`]?[\\w\\.]+[\"`]?)\\s*\\(([^)]+)\\)",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = createIndexRegex.match(sql);

    if (match.hasMatch()) {
        indexName = match.captured(1);
        tableName = match.captured(2);
        QString columnsDef = match.captured(3);

        QRegularExpression columnRegex("([\"`]?[\\w]+[\"`]?)");
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(columnsDef);

        while (it.hasNext()) {
            QRegularExpressionMatch columnMatch = it.next();
            columns << columnMatch.captured(1);
        }

        return true;
    }

    return false;
}

bool PostgreSQLParser::parseCreateView(const QString& sql, QString& viewName, QString& definition) {
    QRegularExpression createViewRegex("\\bCREATE\\s+(?:OR\\s+REPLACE\\s+)?VIEW\\s+([\"`]?[\\w\\.]+[\"`]?)\\s+AS\\s+(.*)",
                                      QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = createViewRegex.match(sql);

    if (match.hasMatch()) {
        viewName = match.captured(1);
        definition = match.captured(2).trimmed();
        return true;
    }

    return false;
}

bool PostgreSQLParser::parseCreateFunction(const QString& sql, QString& functionName, QStringList& parameters, QString& returnType) {
    QRegularExpression createFunctionRegex("\\bCREATE\\s+(?:OR\\s+REPLACE\\s+)?FUNCTION\\s+([\"`]?[\\w\\.]+[\"`]?)\\s*\\(([^)]*)\\)\\s+RETURNS\\s+([^\\s;]+)",
                                          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = createFunctionRegex.match(sql);

    if (match.hasMatch()) {
        functionName = match.captured(1);
        QString paramsStr = match.captured(2);
        returnType = match.captured(3);

        // Parse parameters
        QRegularExpression paramRegex("([\"`]?[\\w]+[\"`]?)\\s+([^,\\s]+)");
        QRegularExpressionMatchIterator it = paramRegex.globalMatch(paramsStr);

        while (it.hasNext()) {
            QRegularExpressionMatch paramMatch = it.next();
            parameters << paramMatch.captured(1) + " " + paramMatch.captured(2);
        }

        return true;
    }

    return false;
}

bool PostgreSQLParser::parseCreateExtension(const QString& sql, QString& extensionName, QString& version) {
    QRegularExpression createExtensionRegex("\\bCREATE\\s+EXTENSION\\s+([\"`]?[\\w]+[\"`]?)\\s*(?:WITH\\s+)?(?:VERSION\\s+([\"`]?[\\w\\.]+[\"`]?)\\s*)?",
                                           QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = createExtensionRegex.match(sql);

    if (match.hasMatch()) {
        extensionName = match.captured(1);
        version = match.captured(2);
        return true;
    }

    return false;
}

bool PostgreSQLParser::parseSelectStatement(const QString& sql, QStringList& columns, QStringList& tables, QString& whereClause) {
    // Extract columns
    columns = extractColumnNames(sql);

    // Extract tables
    tables = extractTableNames(sql);

    // Extract WHERE clause
    QRegularExpression whereRegex("\\bWHERE\\s+(.*?)(\\bORDER\\s+BY\\b|\\bGROUP\\s+BY\\b|\\bHAVING\\b|\\bLIMIT\\b|\\bOFFSET\\b|\\bFETCH\\b|$)",
                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch whereMatch = whereRegex.match(sql);

    if (whereMatch.hasMatch()) {
        whereClause = whereMatch.captured(1).trimmed();
    }

    return !columns.isEmpty() || !tables.isEmpty();
}

bool PostgreSQLParser::parseArrayLiteral(const QString& sql, QStringList& elements) {
    QRegularExpression arrayRegex("'\\{([^}]*)\\}'|ARRAY\\[(.*)\\]");
    QRegularExpressionMatch match = arrayRegex.match(sql);

    if (match.hasMatch()) {
        QString arrayContent = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
        elements = arrayContent.split(",");
        for (QString& element : elements) {
            element = element.trimmed();
        }
        return true;
    }

    return false;
}

bool PostgreSQLParser::parseJsonPath(const QString& sql, QStringList& pathElements) {
    QRegularExpression jsonPathRegex("->'([^']+)'|->\"([^\"]+)\"|#>>\\{([^}]+)\\}");
    QRegularExpressionMatchIterator it = jsonPathRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString pathElement = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
        if (pathElement.isEmpty()) {
            pathElement = match.captured(3);
        }
        if (!pathElement.isEmpty()) {
            pathElements << pathElement;
        }
    }

    return !pathElements.isEmpty();
}

// PostgreSQL Query Analyzer Implementation

void PostgreSQLQueryAnalyzer::analyzeQuery(const QString& sql, QStringList& issues, QStringList& suggestions) {
    issues.clear();
    suggestions.clear();

    // Check for common issues
    if (hasSelectStar(sql)) {
        issues << "Using SELECT * is not recommended for production code";
        suggestions << "Specify explicit column names in SELECT clause";
    }

    if (hasImplicitJoins(sql)) {
        issues << "Using implicit joins (comma-separated tables) is not recommended";
        suggestions << "Use explicit JOIN syntax";
    }

    if (hasCartesianProduct(sql)) {
        issues << "Query may produce Cartesian product";
        suggestions << "Verify JOIN conditions are correct";
    }

    if (usesFunctionsInWhere(sql)) {
        issues << "Using functions in WHERE clause may prevent index usage";
        suggestions << "Avoid functions on indexed columns in WHERE clause";
    }

    if (hasSuboptimalLike(sql)) {
        issues << "LIKE pattern without wildcard at start may still be slow";
        suggestions << "Consider full-text search for text pattern matching";
    }

    if (hasMissingIndexes(sql)) {
        issues << "Query may benefit from additional indexes";
        suggestions << "Consider adding indexes on frequently queried columns";
    }

    if (hasSeqScanInsteadOfIndex(sql)) {
        issues << "Query may be performing sequential scans instead of using indexes";
        suggestions << "Review query structure and available indexes";
    }

    if (hasHighCostOperations(sql)) {
        issues << "Query contains potentially expensive operations";
        suggestions << "Consider query optimization or restructuring";
    }
}

int PostgreSQLQueryAnalyzer::estimateComplexity(const QString& sql) {
    int complexity = 1;

    // Count keywords that indicate complexity
    QStringList complexityKeywords = {"JOIN", "UNION", "GROUP BY", "ORDER BY", "HAVING", "DISTINCT", "EXISTS", "IN", "NOT IN", "ANY", "ALL", "CTE", "WINDOW"};
    for (const QString& keyword : complexityKeywords) {
        QRegularExpression regex("\\b" + keyword + "\\b", QRegularExpression::CaseInsensitiveOption);
        auto matches = regex.globalMatch(sql);
        int count = 0;
        while (matches.hasNext()) {
            matches.next();
            count++;
        }
        complexity += count;
    }

    // Count subqueries
    QRegularExpression subqueryRegex("\\(\\s*SELECT\\s+", QRegularExpression::CaseInsensitiveOption);
    auto subqueryMatches = subqueryRegex.globalMatch(sql);
    int subqueryCount = 0;
    while (subqueryMatches.hasNext()) {
        subqueryMatches.next();
        subqueryCount++;
    }
    complexity += subqueryCount * 2;

    // Count CTEs
    QRegularExpression cteRegex("\\bWITH\\s+\\w+\\s+AS\\s*\\(", QRegularExpression::CaseInsensitiveOption);
    auto cteMatches = cteRegex.globalMatch(sql);
    int cteCount = 0;
    while (cteMatches.hasNext()) {
        cteMatches.next();
        cteCount++;
    }
    complexity += cteCount * 3;

    // Count window functions
    QRegularExpression windowRegex("\\bOVER\\s*\\(", QRegularExpression::CaseInsensitiveOption);
    auto windowMatches = windowRegex.globalMatch(sql);
    int windowCount = 0;
    while (windowMatches.hasNext()) {
        windowMatches.next();
        windowCount++;
    }
    complexity += windowCount * 2;

    // Count array operations
    QRegularExpression arrayRegex("ARRAY\\[[^\\]]*\\]|'\\{[^}]*\\}'", QRegularExpression::CaseInsensitiveOption);
    auto arrayMatches = arrayRegex.globalMatch(sql);
    int arrayCount = 0;
    while (arrayMatches.hasNext()) {
        arrayMatches.next();
        arrayCount++;
    }
    complexity += arrayCount;

    // Count JSON operations
    QRegularExpression jsonRegex("->|->>|#>|#>>|@>|\\?\\?|\\?&|\\?\\|", QRegularExpression::CaseInsensitiveOption);
    auto jsonMatches = jsonRegex.globalMatch(sql);
    int jsonCount = 0;
    while (jsonMatches.hasNext()) {
        jsonMatches.next();
        jsonCount++;
    }
    complexity += jsonCount;

    return complexity;
}

QStringList PostgreSQLQueryAnalyzer::checkBestPractices(const QString& sql) {
    QStringList suggestions;

    // Check for SELECT * usage
    if (hasSelectStar(sql)) {
        suggestions << "Avoid using SELECT * in production code";
    }

    // Check for implicit transactions
    if (sql.contains("BEGIN", Qt::CaseInsensitive) && !sql.contains("COMMIT", Qt::CaseInsensitive)) {
        suggestions << "Ensure all transactions are properly committed or rolled back";
    }

    // Check for proper schema qualification
    if (sql.contains("FROM", Qt::CaseInsensitive) && !sql.contains(".", Qt::CaseInsensitive)) {
        suggestions << "Consider using schema-qualified table names for clarity and performance";
    }

    // Check for LIMIT without ORDER BY
    if (sql.contains("LIMIT", Qt::CaseInsensitive) && !sql.contains("ORDER BY", Qt::CaseInsensitive)) {
        suggestions << "Using LIMIT without ORDER BY may return unpredictable results";
    }

    return suggestions;
}

QStringList PostgreSQLQueryAnalyzer::suggestIndexes(const QString& sql) {
    QStringList suggestions;

    // Analyze WHERE clause for potential indexes
    QRegularExpression whereRegex("\\bWHERE\\s+(.*?)(\\bORDER\\s+BY\\b|\\bGROUP\\s+BY\\b|\\bHAVING\\b|\\bLIMIT\\b|\\bOFFSET\\b|\\bFETCH\\b|$)",
                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch whereMatch = whereRegex.match(sql);

    if (whereMatch.hasMatch()) {
        QString whereClause = whereMatch.captured(1);
        QStringList conditions = whereClause.split(QRegularExpression("\\b(AND|OR)\\b"), Qt::SkipEmptyParts);

        for (const QString& condition : conditions) {
            if (condition.contains("=") || condition.contains("LIKE") || condition.contains("BETWEEN")) {
                QRegularExpression columnRegex("([\"`]?[\\w\\.]+[\"`]?)\\s*[=<>]");
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

QStringList PostgreSQLQueryAnalyzer::checkSecurityIssues(const QString& sql) {
    QStringList issues;

    // Check for SQL injection vulnerabilities
    if (sql.contains("EXEC", Qt::CaseInsensitive) || sql.contains("EXECUTE", Qt::CaseInsensitive)) {
        issues << "Dynamic SQL execution detected - ensure proper parameterization";
    }

    // Check for system catalog access without proper filtering
    if (sql.contains("pg_catalog", Qt::CaseInsensitive) && !sql.contains("WHERE", Qt::CaseInsensitive)) {
        issues << "Accessing system catalogs without WHERE clause may expose sensitive information";
    }

    // Check for privilege escalation
    if (sql.contains("SET ROLE", Qt::CaseInsensitive) || sql.contains("SET SESSION AUTHORIZATION", Qt::CaseInsensitive)) {
        issues << "Role switching detected - ensure proper security controls";
    }

    return issues;
}

QStringList PostgreSQLQueryAnalyzer::checkPostgreSQLSpecificIssues(const QString& sql) {
    QStringList issues;

    // Check for implicit casting issues
    if (sql.contains("::", Qt::CaseInsensitive)) {
        issues << "Explicit casting detected - ensure proper data type handling";
    }

    // Check for array operations without proper bounds checking
    if (sql.contains("ARRAY[", Qt::CaseInsensitive) && !sql.contains("COALESCE", Qt::CaseInsensitive)) {
        issues << "Array operations detected - consider bounds checking";
    }

    // Check for JSON operations without error handling
    if (sql.contains("->", Qt::CaseInsensitive) && !sql.contains("COALESCE", Qt::CaseInsensitive)) {
        issues << "JSON operations detected - consider error handling for missing keys";
    }

    return issues;
}

QStringList PostgreSQLQueryAnalyzer::suggestPostgreSQLOptimizations(const QString& sql) {
    QStringList suggestions;

    // Suggest partial indexes for filtered queries
    if (sql.contains("WHERE", Qt::CaseInsensitive) && sql.contains("deleted = false", Qt::CaseInsensitive)) {
        suggestions << "Consider using partial indexes for soft delete patterns";
    }

    // Suggest covering indexes
    if (sql.contains("SELECT", Qt::CaseInsensitive) && sql.contains("ORDER BY", Qt::CaseInsensitive)) {
        suggestions << "Consider covering indexes that include ORDER BY columns";
    }

    // Suggest expression indexes
    if (sql.contains("UPPER(", Qt::CaseInsensitive) || sql.contains("LOWER(", Qt::CaseInsensitive)) {
        suggestions << "Consider expression indexes for frequently used functions";
    }

    return suggestions;
}

bool PostgreSQLQueryAnalyzer::hasSelectStar(const QString& sql) {
    QRegularExpression selectStarRegex("\\bSELECT\\s+\\*", QRegularExpression::CaseInsensitiveOption);
    return selectStarRegex.match(sql).hasMatch();
}

bool PostgreSQLQueryAnalyzer::hasImplicitJoins(const QString& sql) {
    QRegularExpression implicitJoinRegex("\\bFROM\\s+[^,]+,[^,]+", QRegularExpression::CaseInsensitiveOption);
    return implicitJoinRegex.match(sql).hasMatch();
}

bool PostgreSQLQueryAnalyzer::hasCartesianProduct(const QString& sql) {
    QRegularExpression fromRegex("\\bFROM\\s+([\"`]?[\\w\\.]+[\"`]?)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression joinRegex("\\bJOIN\\s+", QRegularExpression::CaseInsensitiveOption);

    auto tableMatches = fromRegex.globalMatch(sql);
    int tableCount = 0;
    while (tableMatches.hasNext()) {
        tableMatches.next();
        tableCount++;
    }
    auto joinMatches = joinRegex.globalMatch(sql);
    int joinCount = 0;
    while (joinMatches.hasNext()) {
        joinMatches.next();
        joinCount++;
    }

    return tableCount > 1 && (tableCount - 1) > joinCount;
}

bool PostgreSQLQueryAnalyzer::hasUnnecessaryJoins(const QString& sql) {
    // Check for joins that don't contribute to the result
    return sql.contains("LEFT JOIN", Qt::CaseInsensitive) && !sql.contains("WHERE", Qt::CaseInsensitive);
}

bool PostgreSQLQueryAnalyzer::usesFunctionsInWhere(const QString& sql) {
    QRegularExpression whereRegex("\\bWHERE\\s+(.*?)(\\bORDER\\s+BY\\b|\\bGROUP\\s+BY\\b|\\bHAVING\\b|\\bLIMIT\\b|$)",
                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch whereMatch = whereRegex.match(sql);

    if (whereMatch.hasMatch()) {
        QString whereClause = whereMatch.captured(1);
        QStringList functions = PostgreSQLSyntaxPatterns::BUILTIN_FUNCTIONS;

        for (const QString& function : functions) {
            if (whereClause.contains(function, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }

    return false;
}

bool PostgreSQLQueryAnalyzer::hasSuboptimalLike(const QString& sql) {
    QRegularExpression likeRegex("\\bLIKE\\s+['\"][^%]", QRegularExpression::CaseInsensitiveOption);
    return likeRegex.match(sql).hasMatch();
}

bool PostgreSQLQueryAnalyzer::hasMissingIndexes(const QString& sql) {
    // Look for patterns that typically benefit from indexes
    return sql.contains("WHERE", Qt::CaseInsensitive) && sql.contains("ORDER BY", Qt::CaseInsensitive);
}

bool PostgreSQLQueryAnalyzer::hasSeqScanInsteadOfIndex(const QString& sql) {
    // This would typically be determined from EXPLAIN output
    return sql.contains("LIKE", Qt::CaseInsensitive) && !sql.contains("GIN", Qt::CaseInsensitive);
}

bool PostgreSQLQueryAnalyzer::hasHighCostOperations(const QString& sql) {
    // Check for operations that are typically expensive
    return sql.contains("DISTINCT", Qt::CaseInsensitive) || sql.contains("GROUP BY", Qt::CaseInsensitive) ||
           sql.contains("ORDER BY", Qt::CaseInsensitive) || sql.contains("UNION", Qt::CaseInsensitive);
}

// PostgreSQL Code Formatter Implementation

QString PostgreSQLCodeFormatter::formatCode(const QString& sql, int indentSize) {
    QString formatted = sql;

    // Basic formatting steps
    formatted = formatKeywords(formatted, true);
    formatted = addNewlines(formatted);
    formatted = indentCode(formatted, indentSize);
    formatted = formatArrayLiterals(formatted);
    formatted = formatJsonPaths(formatted);
    formatted = formatCTEs(formatted);
    formatted = formatWindowFunctions(formatted);

    return formatted;
}

QString PostgreSQLCodeFormatter::compressCode(const QString& sql) {
    QString compressed = sql;

    // Remove extra whitespace
    compressed = compressed.simplified();

    // Remove comments
    QRegularExpression commentRegex("--.*$", QRegularExpression::MultilineOption);
    compressed = compressed.replace(commentRegex, "");

    QRegularExpression multiLineCommentRegex("/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption);
    compressed = compressed.replace(multiLineCommentRegex, " ");

    return compressed.simplified();
}

QString PostgreSQLCodeFormatter::expandCode(const QString& sql) {
    return formatCode(sql);
}

QString PostgreSQLCodeFormatter::convertCase(const QString& sql, bool upperKeywords, bool upperFunctions) {
    QString result = sql;

    if (upperKeywords) {
        result = formatKeywords(result, true);
    }

    if (upperFunctions) {
        result = formatFunctions(result, true);
    }

    return result;
}

QString PostgreSQLCodeFormatter::formatArrayLiterals(const QString& sql) {
    QString formatted = sql;

    // Format array literals
    QRegularExpression arrayRegex("'\\{([^}]*)\\}'");
    QRegularExpressionMatchIterator it = arrayRegex.globalMatch(formatted);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString arrayContent = match.captured(1);
        arrayContent = arrayContent.replace(",", ", ");
        formatted = formatted.replace(match.captured(0), "'{" + arrayContent + "}'");
    }

    return formatted;
}

QString PostgreSQLCodeFormatter::formatJsonPaths(const QString& sql) {
    // Basic JSON path formatting - could be enhanced
    return sql;
}

QString PostgreSQLCodeFormatter::formatCTEs(const QString& sql) {
    QString formatted = sql;

    // Format CTEs with proper indentation
    QRegularExpression cteRegex("\\bWITH\\s+([\\w\\s,]+)\\s+AS\\s*\\(", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = cteRegex.globalMatch(formatted);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString cteDefinition = match.captured(0);
        cteDefinition = cteDefinition.replace("WITH", "WITH\n    ");
        cteDefinition = cteDefinition.replace("AS", "\nAS");
        formatted = formatted.replace(match.captured(0), cteDefinition);
    }

    return formatted;
}

QString PostgreSQLCodeFormatter::formatWindowFunctions(const QString& sql) {
    QString formatted = sql;

    // Format window functions
    QRegularExpression windowRegex("\\bOVER\\s*\\(", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = windowRegex.globalMatch(formatted);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        formatted = formatted.replace(match.captured(0), "\n    OVER (");
    }

    return formatted;
}

QString PostgreSQLCodeFormatter::indentCode(const QString& sql, int indentSize) {
    QString indented = sql;
    QString indent = QString(" ").repeated(indentSize);

    // Basic indentation logic
    QStringList lines = indented.split("\n");
    int currentIndent = 0;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();

        // Decrease indent for closing keywords
        if (line.contains("END", Qt::CaseInsensitive) || line.contains(")", Qt::CaseInsensitive)) {
            currentIndent = qMax(0, currentIndent - 1);
        }

        lines[i] = QString(" ").repeated(currentIndent * indentSize) + line;

        // Increase indent for opening keywords
        if (line.contains("BEGIN", Qt::CaseInsensitive) || line.contains("(", Qt::CaseInsensitive)) {
            currentIndent++;
        }
    }

    return lines.join("\n");
}

QString PostgreSQLCodeFormatter::formatKeywords(const QString& sql, bool uppercase) {
    QString result = sql;

    QStringList keywords = PostgreSQLSyntaxPatterns::getAllKeywords();
    for (const QString& keyword : keywords) {
        QRegularExpression regex("\\b" + keyword + "\\b", QRegularExpression::CaseInsensitiveOption);
        QString replacement = uppercase ? keyword.toUpper() : keyword.toLower();
        result = result.replace(regex, replacement);
    }

    return result;
}

QString PostgreSQLCodeFormatter::formatFunctions(const QString& sql, bool uppercase) {
    QString result = sql;

    QStringList functions = PostgreSQLSyntaxPatterns::BUILTIN_FUNCTIONS;
    for (const QString& function : functions) {
        QRegularExpression regex("\\b" + function + "\\b", QRegularExpression::CaseInsensitiveOption);
        QString replacement = uppercase ? function.toUpper() : function.toLower();
        result = result.replace(regex, replacement);
    }

    return result;
}

QString PostgreSQLCodeFormatter::addNewlines(const QString& sql) {
    QString result = sql;

    // Add newlines after certain keywords
    QStringList newlineKeywords = {"SELECT", "FROM", "WHERE", "ORDER BY", "GROUP BY", "HAVING", "LIMIT", "OFFSET"};
    for (const QString& keyword : newlineKeywords) {
        QRegularExpression regex("\\b" + keyword + "\\b", QRegularExpression::CaseInsensitiveOption);
        result = result.replace(regex, "\n" + keyword);
    }

    return result;
}

QString PostgreSQLCodeFormatter::alignClauses(const QString& sql) {
    // Basic clause alignment - could be enhanced
    return sql;
}

// PostgreSQL Syntax Validator Implementation

bool PostgreSQLSyntaxValidator::validateSyntax(const QString& sql, QStringList& errors, QStringList& warnings) {
    errors.clear();
    warnings.clear();

    // Basic validation
    if (hasUnclosedComments(sql)) {
        errors << "Unclosed comment detected";
    }

    if (hasUnclosedStrings(sql)) {
        errors << "Unclosed string literal detected";
    }

    if (hasUnclosedBrackets(sql)) {
        errors << "Unclosed bracket detected";
    }

    if (hasInvalidArraySyntax(sql)) {
        errors << "Invalid array syntax detected";
    }

    if (hasInvalidJsonSyntax(sql)) {
        errors << "Invalid JSON syntax detected";
    }

    // Check for deprecated features
    warnings << checkDeprecatedFeatures(sql);

    return errors.isEmpty();
}

bool PostgreSQLSyntaxValidator::validateIdentifiers(const QString& sql, QStringList& errors) {
    // Check for invalid identifiers
    QRegularExpression invalidIdRegex("[^a-zA-Z0-9_]\"");
    QRegularExpressionMatchIterator it = invalidIdRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        if (!PostgreSQLSyntaxPatterns::RESERVED_KEYWORDS.contains(match.captured(), Qt::CaseInsensitive)) {
            errors << "Invalid identifier: " + match.captured();
        }
    }

    return errors.isEmpty();
}

bool PostgreSQLSyntaxValidator::validateDataTypes(const QString& sql, QStringList& errors) {
    QStringList dataTypes = PostgreSQLSyntaxPatterns::DATA_TYPES;

    for (const QString& dataType : dataTypes) {
        if (!sql.contains(dataType, Qt::CaseInsensitive)) {
            continue;
        }

        // Check for valid data type usage
        QString escapedType = dataType;
        escapedType.replace(" ", "\\s+");
        QRegularExpression typeRegex("\\b" + escapedType + "\\b", QRegularExpression::CaseInsensitiveOption);
        if (!typeRegex.match(sql).hasMatch()) {
            errors << "Invalid data type usage: " + dataType;
        }
    }

    return errors.isEmpty();
}

bool PostgreSQLSyntaxValidator::validateFunctions(const QString& sql, QStringList& errors) {
    QStringList functions = PostgreSQLSyntaxPatterns::BUILTIN_FUNCTIONS;

    for (const QString& function : functions) {
        if (sql.contains(function, Qt::CaseInsensitive)) {
            // Check for proper function syntax
            QRegularExpression funcRegex("\\b" + function + "\\s*\\(", QRegularExpression::CaseInsensitiveOption);
            if (!funcRegex.match(sql).hasMatch()) {
                errors << "Invalid function call: " + function;
            }
        }
    }

    return errors.isEmpty();
}

bool PostgreSQLSyntaxValidator::validateOperators(const QString& sql, QStringList& errors) {
    QStringList operators = PostgreSQLSyntaxPatterns::OPERATORS;

    for (const QString& op : operators) {
        if (sql.contains(op)) {
            // Basic operator validation - could be enhanced
            if (sql.count(op) % 2 != 0 && (op == "(" || op == ")")) {
                errors << "Unmatched operator: " + op;
            }
        }
    }

    return errors.isEmpty();
}

bool PostgreSQLSyntaxValidator::validatePostgreSQLExtensions(const QString& sql, QStringList& errors, QStringList& warnings) {
    // Validate array syntax
    warnings << validateArraySyntax(sql);

    // Validate JSON syntax
    warnings << validateJsonSyntax(sql);

    // Validate full-text search syntax
    warnings << validateFulltextSyntax(sql);

    return errors.isEmpty();
}

QStringList PostgreSQLSyntaxValidator::checkDeprecatedFeatures(const QString& sql) {
    QStringList warnings;

    // Check for deprecated features
    if (sql.contains("=>", Qt::CaseInsensitive)) {
        warnings << "Old-style => syntax for arrays is deprecated, use ARRAY[] syntax";
    }

    if (sql.contains("SET LOCAL", Qt::CaseInsensitive)) {
        warnings << "SET LOCAL may have limited scope in some contexts";
    }

    return warnings;
}

QStringList PostgreSQLSyntaxValidator::validateArraySyntax(const QString& sql) {
    QStringList warnings;

    QRegularExpression arrayRegex("'\\{[^}]*\\}'|ARRAY\\[[^\\]]*\\]");
    QRegularExpressionMatchIterator it = arrayRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString arrayLiteral = match.captured();

        // Check for proper array syntax
        if (arrayLiteral.startsWith("'") && !arrayLiteral.contains("{")) {
            warnings << "Invalid array literal syntax";
        }
    }

    return warnings;
}

QStringList PostgreSQLSyntaxValidator::validateJsonSyntax(const QString& sql) {
    QStringList warnings;

    // Check for JSON operators
    QStringList jsonOps = {"->", "->>", "#>", "#>>", "@>", "<@", "?", "?|", "?&"};
    for (const QString& op : jsonOps) {
        if (sql.contains(op)) {
            // Basic validation - could be enhanced
            if (sql.count(op) % 2 != 0) {
                warnings << "Potentially unmatched JSON operator: " + op;
            }
        }
    }

    return warnings;
}

QStringList PostgreSQLSyntaxValidator::validateFulltextSyntax(const QString& sql) {
    QStringList warnings;

    if (sql.contains("@@", Qt::CaseInsensitive)) {
        // Check for proper full-text search syntax
        if (!sql.contains("to_tsvector", Qt::CaseInsensitive) && !sql.contains("plainto_tsquery", Qt::CaseInsensitive)) {
            warnings << "Full-text search operator @@ used without proper vector or query functions";
        }
    }

    return warnings;
}

bool PostgreSQLSyntaxValidator::hasUnclosedComments(const QString& sql) {
    int commentCount = sql.count("/*") - sql.count("*/");
    return commentCount != 0;
}

bool PostgreSQLSyntaxValidator::hasUnclosedStrings(const QString& sql) {
    int singleQuoteCount = sql.count("'") - sql.count("\\'");
    int doubleQuoteCount = sql.count("\"") - sql.count("\\\"");

    // Account for escaped quotes
    QRegularExpression escapedSingleQuote("\\\\'");
    QRegularExpression escapedDoubleQuote("\\\\\"");

    auto singleQuoteMatches = escapedSingleQuote.globalMatch(sql);
    int singleEscapedCount = 0;
    while (singleQuoteMatches.hasNext()) {
        singleQuoteMatches.next();
        singleEscapedCount++;
    }
    singleQuoteCount -= singleEscapedCount;

    auto doubleQuoteMatches = escapedDoubleQuote.globalMatch(sql);
    int doubleEscapedCount = 0;
    while (doubleQuoteMatches.hasNext()) {
        doubleQuoteMatches.next();
        doubleEscapedCount++;
    }
    doubleQuoteCount -= doubleEscapedCount;

    return singleQuoteCount % 2 != 0 || doubleQuoteCount % 2 != 0;
}

bool PostgreSQLSyntaxValidator::hasUnclosedBrackets(const QString& sql) {
    int openBracketCount = sql.count("(");
    int closeBracketCount = sql.count(")");

    return openBracketCount != closeBracketCount;
}

bool PostgreSQLSyntaxValidator::hasInvalidArraySyntax(const QString& sql) {
    // Check for mismatched array brackets
    QRegularExpression arrayRegex("'\\{[^}]*\\}'|ARRAY\\[[^\\]]*\\]");
    QRegularExpressionMatchIterator it = arrayRegex.globalMatch(sql);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString arrayLiteral = match.captured();

        if (arrayLiteral.startsWith("ARRAY[") && !arrayLiteral.endsWith("]")) {
            return true;
        }

        if (arrayLiteral.startsWith("'") && !arrayLiteral.endsWith("}'")) {
            return true;
        }
    }

    return false;
}

bool PostgreSQLSyntaxValidator::hasInvalidJsonSyntax(const QString& sql) {
    // Basic JSON syntax validation - could be enhanced with proper JSON parsing
    return sql.contains("json", Qt::CaseInsensitive) && !sql.contains("'", Qt::CaseInsensitive);
}

// PostgreSQL IntelliSense Implementation

QStringList PostgreSQLIntelliSense::getCompletions(const QString& text, int cursorPosition) {
    QStringList suggestions;

    // Get current context
    QString context = getCurrentContext(text, cursorPosition);
    QString currentWord = getCurrentWord(text, cursorPosition);

    // Provide suggestions based on context
    if (context == "SELECT") {
        suggestions << getKeywordSuggestions();
        suggestions << getFunctionSuggestions();
        suggestions << getColumnSuggestions("", currentWord);
    } else if (context == "FROM") {
        suggestions << getTableSuggestions(currentWord);
        suggestions << getSchemaSuggestions(currentWord);
    } else if (context == "WHERE" || context == "ON") {
        suggestions << getColumnSuggestions("", currentWord);
        suggestions << getFunctionSuggestions();
        suggestions << getOperatorSuggestions();
    } else if (context == "CREATE") {
        suggestions << "TABLE" << "INDEX" << "VIEW" << "FUNCTION" << "SEQUENCE" << "EXTENSION";
    } else {
        suggestions << getKeywordSuggestions();
        suggestions << getFunctionSuggestions();
        suggestions << getTableSuggestions();
        suggestions << getSchemaSuggestions();
    }

    // Filter suggestions based on current word
    if (!currentWord.isEmpty()) {
        QStringList filtered;
        for (const QString& suggestion : suggestions) {
            if (suggestion.startsWith(currentWord, Qt::CaseInsensitive)) {
                filtered << suggestion;
            }
        }
        suggestions = filtered;
    }

    return suggestions;
}

QStringList PostgreSQLIntelliSense::getContextSuggestions(const QString& text, int cursorPosition) {
    return getCompletions(text, cursorPosition);
}

QStringList PostgreSQLIntelliSense::getTableSuggestions(const QString& partialName) {
    QStringList suggestions;
    // This would typically query the database for table names
    // For now, return common table name patterns
    suggestions << "users" << "products" << "orders" << "customers" << "categories";
    return suggestions;
}

QStringList PostgreSQLIntelliSense::getColumnSuggestions(const QString& tableName, const QString& partialName) {
    QStringList suggestions;
    // This would typically query the database for column names
    // For now, return common column name patterns
    suggestions << "id" << "name" << "email" << "created_at" << "updated_at" << "user_id";
    return suggestions;
}

QStringList PostgreSQLIntelliSense::getKeywordSuggestions(const QString& partialName) {
    return PostgreSQLSyntaxPatterns::getAllKeywords();
}

QStringList PostgreSQLIntelliSense::getFunctionSuggestions(const QString& partialName) {
    QStringList suggestions = PostgreSQLSyntaxPatterns::BUILTIN_FUNCTIONS;
    suggestions << getArraySuggestions() << getJsonSuggestions();
    return suggestions;
}

QStringList PostgreSQLIntelliSense::getOperatorSuggestions() {
    return PostgreSQLSyntaxPatterns::OPERATORS;
}

QStringList PostgreSQLIntelliSense::getSchemaSuggestions(const QString& partialName) {
    QStringList suggestions;
    // This would typically query the database for schema names
    suggestions << "public" << "pg_catalog" << "information_schema";
    return suggestions;
}

QStringList PostgreSQLIntelliSense::getArraySuggestions() {
    QStringList suggestions;
    suggestions << "ARRAY_AGG" << "ARRAY_APPEND" << "ARRAY_CAT" << "ARRAY_DIMS" << "ARRAY_FILL"
                << "ARRAY_LENGTH" << "ARRAY_LOWER" << "ARRAY_NDIMS" << "ARRAY_POSITION"
                << "ARRAY_POSITIONS" << "ARRAY_PREPEND" << "ARRAY_REMOVE" << "ARRAY_REPLACE"
                << "ARRAY_TO_JSON" << "ARRAY_TO_STRING" << "ARRAY_UPPER" << "CARDINALITY"
                << "STRING_TO_ARRAY" << "UNNEST";
    return suggestions;
}

QStringList PostgreSQLIntelliSense::getJsonSuggestions() {
    QStringList suggestions;
    suggestions << "JSON_AGG" << "JSON_ARRAY_ELEMENTS" << "JSON_ARRAY_ELEMENTS_TEXT"
                << "JSON_ARRAY_LENGTH" << "JSON_BUILD_ARRAY" << "JSON_BUILD_OBJECT"
                << "JSON_EACH" << "JSON_EACH_TEXT" << "JSON_EXTRACT_PATH"
                << "JSON_EXTRACT_PATH_TEXT" << "JSON_OBJECT" << "JSON_OBJECT_AGG"
                << "JSON_OBJECT_KEYS" << "JSON_POPULATE_RECORD" << "JSON_POPULATE_RECORDSET"
                << "JSON_TO_RECORD" << "JSON_TO_RECORDSET" << "JSON_TYPEOF" << "ROW_TO_JSON"
                << "TO_JSON" << "TO_JSONB";
    return suggestions;
}

QString PostgreSQLIntelliSense::getCurrentContext(const QString& text, int cursorPosition) {
    QString beforeCursor = text.left(cursorPosition).toUpper();

    // Determine context based on recent keywords
    if (beforeCursor.contains("SELECT")) {
        return "SELECT";
    } else if (beforeCursor.contains("FROM")) {
        return "FROM";
    } else if (beforeCursor.contains("WHERE")) {
        return "WHERE";
    } else if (beforeCursor.contains("CREATE")) {
        return "CREATE";
    } else if (beforeCursor.contains("JOIN") || beforeCursor.contains("ON")) {
        return "ON";
    }

    return "GENERAL";
}

QString PostgreSQLIntelliSense::getCurrentWord(const QString& text, int cursorPosition) {
    QString beforeCursor = text.left(cursorPosition);
    QRegularExpression wordRegex("[\\w]+$");
    QRegularExpressionMatch match = wordRegex.match(beforeCursor);

    if (match.hasMatch()) {
        return match.captured();
    }

    return QString();
}

// PostgreSQL Script Executor Implementation

bool PostgreSQLScriptExecutor::executeScript(const QString& script, QStringList& results, QStringList& errors) {
    results.clear();
    errors.clear();

    QStringList statements = parseScript(script);

    for (const QString& statement : statements) {
        QString result;
        QString errorMsg = errors.isEmpty() ? QString() : errors.last();
        if (!executeStatement(statement, QStringList() << result, errorMsg)) {
            errors << "Failed to execute statement: " + statement.left(50) + "...";
        } else {
            results << result;
        }
    }

    return errors.isEmpty();
}

bool PostgreSQLScriptExecutor::executeStatement(const QString& statement, QStringList& results, QString& error) {
    // This would typically execute the statement against the database
    // For now, return a mock success
    results << "Statement executed successfully";
    return true;
}

bool PostgreSQLScriptExecutor::executeBatch(const QStringList& statements, QStringList& results, QStringList& errors, bool useTransaction) {
    results.clear();
    errors.clear();

    for (const QString& statement : statements) {
        QString result;
        QString errorMsg = errors.isEmpty() ? QString() : errors.last();
        if (!executeStatement(statement, QStringList() << result, errorMsg)) {
            errors << "Failed to execute statement: " + statement.left(50) + "...";
            if (useTransaction) {
                // Rollback would happen here
                break;
            }
        } else {
            results << result;
        }
    }

    return errors.isEmpty();
}

QStringList PostgreSQLScriptExecutor::parseScript(const QString& script) {
    return splitStatements(script);
}

bool PostgreSQLScriptExecutor::executeWithCopy(const QString& copyCommand, const QString& data, QString& error) {
    // This would execute a COPY command with data
    // For now, return mock success
    return true;
}

bool PostgreSQLScriptExecutor::executeWithCursor(const QString& sql, int fetchSize, QStringList& results, QString& error) {
    // This would execute a query with cursor-based fetching
    // For now, return mock success
    results << "Query executed with cursor";
    return true;
}

QStringList PostgreSQLScriptExecutor::splitStatements(const QString& script) {
    QStringList statements;
    QString currentStatement;
    bool inString = false;
    bool inComment = false;
    QChar stringChar;
    int commentLevel = 0;

    for (int i = 0; i < script.length(); ++i) {
        QChar c = script[i];

        if (inComment) {
            if (c == '*' && i + 1 < script.length() && script[i + 1] == '/') {
                inComment = false;
                i++; // Skip next character
            }
            currentStatement += c;
            continue;
        }

        if (inString) {
            if (c == stringChar && (i == 0 || script[i - 1] != '\\')) {
                inString = false;
            }
            currentStatement += c;
            continue;
        }

        // Check for start of comment
        if (c == '/' && i + 1 < script.length() && script[i + 1] == '*') {
            inComment = true;
            currentStatement += c;
            continue;
        }

        // Check for start of string
        if (c == '\'' || c == '"') {
            inString = true;
            stringChar = c;
            currentStatement += c;
            continue;
        }

        // Check for statement terminator
        if (c == ';') {
            currentStatement += c;
            statements << currentStatement.trimmed();
            currentStatement.clear();
            continue;
        }

        currentStatement += c;
    }

    // Add remaining statement if any
    if (!currentStatement.trimmed().isEmpty()) {
        statements << currentStatement.trimmed();
    }

    return statements;
}

bool PostgreSQLScriptExecutor::isCompleteStatement(const QString& statement) {
    // Basic check for complete statement
    return statement.trimmed().endsWith(";");
}

QString PostgreSQLScriptExecutor::cleanStatement(const QString& statement) {
    QString cleaned = statement.trimmed();

    // Remove trailing semicolon
    if (cleaned.endsWith(";")) {
        cleaned = cleaned.left(cleaned.length() - 1);
    }

    return cleaned;
}

} // namespace scratchrobin
