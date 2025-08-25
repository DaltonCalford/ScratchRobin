#include "mysql_catalog.h"
#include <QRegularExpression>

namespace scratchrobin {

// MySQL System Catalog Implementation

QString MySQLCatalog::getSystemDatabasesQuery() {
    return "SELECT "
           "schema_name as database_name, "
           "default_character_set_name as charset, "
           "default_collation_name as collation, "
           "sql_path, "
           "schema_comment as comment "
           "FROM information_schema.schemata "
           "WHERE schema_name IN ('information_schema', 'mysql', 'performance_schema', 'sys') "
           "ORDER BY schema_name";
}

QString MySQLCatalog::getUserDatabasesQuery() {
    return "SELECT "
           "schema_name as database_name, "
           "default_character_set_name as charset, "
           "default_collation_name as collation, "
           "sql_path, "
           "schema_comment as comment "
           "FROM information_schema.schemata "
           "WHERE schema_name NOT IN ('information_schema', 'mysql', 'performance_schema', 'sys', 'test') "
           "ORDER BY schema_name";
}

QString MySQLCatalog::getDatabaseInfoQuery(const QString& database) {
    QString query = "SELECT "
                   "DATABASE() as current_database, "
                   "VERSION() as version_info, "
                   "@@version_comment as version_comment, "
                   "@@version_compile_machine as compile_machine, "
                   "@@version_compile_os as compile_os, "
                   "@@hostname as hostname, "
                   "@@port as port, "
                   "@@socket as socket, "
                   "@@basedir as basedir, "
                   "@@datadir as datadir, "
                   "@@tmpdir as tmpdir, "
                   "@@character_set_server as server_charset, "
                   "@@collation_server as server_collation, "
                   "@@time_zone as time_zone, "
                   "@@system_time_zone as system_time_zone, "
                   "@@max_connections as max_connections, "
                   "@@max_user_connections as max_user_connections";

    if (!database.isEmpty()) {
        query += QString(", (SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '%1') as table_count").arg(database);
        query += QString(", (SELECT SUM(data_length + index_length) FROM information_schema.tables WHERE table_schema = '%1') as database_size").arg(database);
    }

    return query;
}

QString MySQLCatalog::getSchemasQuery() {
    return "SELECT "
           "schema_name, "
           "default_character_set_name as charset, "
           "default_collation_name as collation, "
           "sql_path "
           "FROM information_schema.schemata "
           "ORDER BY schema_name";
}

QString MySQLCatalog::getTablesQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "t.table_schema, "
                   "t.table_name, "
                   "t.table_type, "
                   "t.engine, "
                   "t.version, "
                   "t.row_format, "
                   "t.table_rows, "
                   "t.avg_row_length, "
                   "t.data_length, "
                   "t.max_data_length, "
                   "t.index_length, "
                   "t.data_free, "
                   "t.auto_increment, "
                   "t.create_time, "
                   "t.update_time, "
                   "t.check_time, "
                   "t.table_collation, "
                   "t.checksum, "
                   "t.create_options, "
                   "t.table_comment, "
                   "t.max_index_length, "
                   "t.temporary, "
                   "(t.data_length + t.index_length) as total_size_mb "
                   "FROM information_schema.tables t ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("t.table_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("t.table_schema = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY t.table_schema, t.table_name";
    return query;
}

QString MySQLCatalog::getViewsQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "v.table_schema, "
                   "v.table_name, "
                   "v.view_definition, "
                   "v.check_option, "
                   "v.is_updatable, "
                   "v.definer, "
                   "v.security_type, "
                   "v.character_set_client, "
                   "v.collation_connection, "
                   "v.view_definition_utf8, "
                   "t.table_comment, "
                   "t.create_time, "
                   "t.update_time "
                   "FROM information_schema.views v "
                   "LEFT JOIN information_schema.tables t ON v.table_schema = t.table_schema AND v.table_name = t.table_name ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("v.table_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("v.table_schema = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY v.table_schema, v.table_name";
    return query;
}

QString MySQLCatalog::getColumnsQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "c.table_schema, "
                   "c.table_name, "
                   "c.column_name, "
                   "c.ordinal_position, "
                   "c.column_default, "
                   "c.is_nullable, "
                   "c.data_type, "
                   "c.character_maximum_length, "
                   "c.character_octet_length, "
                   "c.numeric_precision, "
                   "c.numeric_scale, "
                   "c.datetime_precision, "
                   "c.character_set_name, "
                   "c.collation_name, "
                   "c.column_type, "
                   "c.column_key, "
                   "c.extra, "
                   "c.privileges, "
                   "c.column_comment, "
                   "c.generation_expression, "
                   "c.is_generated "
                   "FROM information_schema.columns c ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("c.table_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("c.table_schema = '%1'").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("c.table_name = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY c.table_schema, c.table_name, c.ordinal_position";
    return query;
}

QString MySQLCatalog::getIndexesQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "s.table_schema, "
                   "s.table_name, "
                   "s.index_name, "
                   "s.column_name, "
                   "s.collation, "
                   "s.cardinality, "
                   "s.sub_part, "
                   "s.packed, "
                   "s.nullable, "
                   "s.index_type, "
                   "s.comment, "
                   "s.index_comment, "
                   "CASE WHEN s.index_name = 'PRIMARY' THEN 'YES' ELSE 'NO' END as is_primary, "
                   "GROUP_CONCAT(s.column_name ORDER BY s.seq_in_index) as index_columns, "
                   "COUNT(*) as column_count, "
                   "CASE WHEN s.non_unique = 0 THEN 'YES' ELSE 'NO' END as is_unique "
                   "FROM information_schema.statistics s ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("s.table_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("s.table_schema = '%1'").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("s.table_name = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "GROUP BY s.table_schema, s.table_name, s.index_name "
             "ORDER BY s.table_schema, s.table_name, s.index_name, s.seq_in_index";
    return query;
}

QString MySQLCatalog::getConstraintsQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "tc.table_schema, "
                   "tc.table_name, "
                   "tc.constraint_name, "
                   "tc.constraint_type, "
                   "kcu.column_name, "
                   "kcu.referenced_table_schema, "
                   "kcu.referenced_table_name, "
                   "kcu.referenced_column_name, "
                   "rc.delete_rule, "
                   "rc.update_rule, "
                   "tc.enforced "
                   "FROM information_schema.table_constraints tc "
                   "LEFT JOIN information_schema.key_column_usage kcu ON "
                   "tc.constraint_name = kcu.constraint_name AND "
                   "tc.table_schema = kcu.table_schema AND "
                   "tc.table_name = kcu.table_name "
                   "LEFT JOIN information_schema.referential_constraints rc ON "
                   "tc.constraint_name = rc.constraint_name AND "
                   "tc.table_schema = rc.constraint_schema ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("tc.table_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("tc.table_schema = '%1'").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("tc.table_name = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY tc.table_schema, tc.table_name, tc.constraint_name";
    return query;
}

QString MySQLCatalog::getForeignKeysQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "kcu.constraint_name, "
                   "kcu.table_schema, "
                   "kcu.table_name, "
                   "kcu.column_name, "
                   "kcu.referenced_table_schema, "
                   "kcu.referenced_table_name, "
                   "kcu.referenced_column_name, "
                   "rc.delete_rule, "
                   "rc.update_rule, "
                   "rc.match_option "
                   "FROM information_schema.key_column_usage kcu "
                   "INNER JOIN information_schema.referential_constraints rc ON "
                   "kcu.constraint_name = rc.constraint_name AND "
                   "kcu.table_schema = rc.constraint_schema "
                   "WHERE kcu.referenced_table_name IS NOT NULL ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("kcu.table_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("kcu.table_schema = '%1'").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("kcu.table_name = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "AND " + conditions.join(" AND ");
    }

    query += "ORDER BY kcu.table_schema, kcu.table_name, kcu.constraint_name";
    return query;
}

QString MySQLCatalog::getStoredProceduresQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "r.routine_schema, "
                   "r.routine_name, "
                   "r.routine_type, "
                   "r.data_type, "
                   "r.character_maximum_length, "
                   "r.character_octet_length, "
                   "r.numeric_precision, "
                   "r.numeric_scale, "
                   "r.datetime_precision, "
                   "r.character_set_name, "
                   "r.collation_name, "
                   "r.routine_body, "
                   "r.routine_definition, "
                   "r.external_name, "
                   "r.external_language, "
                   "r.parameter_style, "
                   "r.is_deterministic, "
                   "r.sql_data_access, "
                   "r.sql_path, "
                   "r.security_type, "
                   "r.created, "
                   "r.last_altered, "
                   "r.routine_comment, "
                   "r.definer "
                   "FROM information_schema.routines r "
                   "WHERE r.routine_type = 'PROCEDURE' ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("r.routine_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("r.routine_schema = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "AND " + conditions.join(" AND ");
    }

    query += "ORDER BY r.routine_schema, r.routine_name";
    return query;
}

QString MySQLCatalog::getFunctionsQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "r.routine_schema, "
                   "r.routine_name, "
                   "r.routine_type, "
                   "r.data_type, "
                   "r.character_maximum_length, "
                   "r.character_octet_length, "
                   "r.numeric_precision, "
                   "r.numeric_scale, "
                   "r.datetime_precision, "
                   "r.character_set_name, "
                   "r.collation_name, "
                   "r.routine_body, "
                   "r.routine_definition, "
                   "r.external_name, "
                   "r.external_language, "
                   "r.parameter_style, "
                   "r.is_deterministic, "
                   "r.sql_data_access, "
                   "r.sql_path, "
                   "r.security_type, "
                   "r.created, "
                   "r.last_altered, "
                   "r.routine_comment, "
                   "r.definer "
                   "FROM information_schema.routines r "
                   "WHERE r.routine_type = 'FUNCTION' ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("r.routine_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("r.routine_schema = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "AND " + conditions.join(" AND ");
    }

    query += "ORDER BY r.routine_schema, r.routine_name";
    return query;
}

QString MySQLCatalog::getTriggersQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "t.trigger_schema, "
                   "t.trigger_name, "
                   "t.event_manipulation, "
                   "t.event_object_schema, "
                   "t.event_object_table, "
                   "t.action_statement, "
                   "t.action_timing, "
                   "t.action_reference_old_table, "
                   "t.action_reference_new_table, "
                   "t.action_reference_old_row, "
                   "t.action_reference_new_row, "
                   "t.created, "
                   "t.sql_mode, "
                   "t.definer, "
                   "t.character_set_client, "
                   "t.collation_connection, "
                   "t.database_collation, "
                   "t.action_order "
                   "FROM information_schema.triggers t ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("t.trigger_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("t.trigger_schema = '%1'").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("t.event_object_table = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY t.trigger_schema, t.event_object_table, t.trigger_name";
    return query;
}

QString MySQLCatalog::getEventsQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "e.event_schema, "
                   "e.event_name, "
                   "e.definer, "
                   "e.time_zone, "
                   "e.event_body, "
                   "e.event_definition, "
                   "e.event_type, "
                   "e.execute_at, "
                   "e.interval_value, "
                   "e.interval_field, "
                   "e.sql_mode, "
                   "e.starts, "
                   "e.ends, "
                   "e.status, "
                   "e.on_completion, "
                   "e.created, "
                   "e.last_executed, "
                   "e.last_altered, "
                   "e.last_error, "
                   "e.originator, "
                   "e.character_set_client, "
                   "e.collation_connection, "
                   "e.database_collation, "
                   "e.event_comment "
                   "FROM information_schema.events e ";

    QStringList conditions;
    if (!database.isEmpty()) {
        conditions << QString("e.event_schema = '%1'").arg(database);
    }
    if (!schema.isEmpty()) {
        conditions << QString("e.event_schema = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY e.event_schema, e.event_name";
    return query;
}

QString MySQLCatalog::getServerInfoQuery() {
    return "SELECT "
           "VERSION() as version_string, "
           "@@version_comment as version_comment, "
           "@@version_compile_machine as compile_machine, "
           "@@version_compile_os as compile_os, "
           "@@hostname as hostname, "
           "@@port as port, "
           "@@socket as socket, "
           "@@basedir as basedir, "
           "@@datadir as datadir, "
           "@@tmpdir as tmpdir, "
           "@@character_set_server as server_charset, "
           "@@collation_server as server_collation, "
           "@@time_zone as time_zone, "
           "@@system_time_zone as system_time_zone, "
           "@@max_connections as max_connections, "
           "@@max_user_connections as max_user_connections, "
           "@@wait_timeout as wait_timeout, "
           "@@interactive_timeout as interactive_timeout, "
           "@@query_cache_size as query_cache_size, "
           "@@innodb_buffer_pool_size as innodb_buffer_pool_size, "
           "@@innodb_log_file_size as innodb_log_file_size, "
           "@@innodb_log_files_in_group as innodb_log_files_in_group, "
           "@@key_buffer_size as key_buffer_size, "
           "@@table_open_cache as table_open_cache, "
           "@@thread_cache_size as thread_cache_size, "
           "@@binlog_format as binlog_format, "
           "@@sql_mode as sql_mode, "
           "@@optimizer_switch as optimizer_switch, "
           "@@have_ssl as ssl_support, "
           "@@have_openssl as openssl_support";
}

QString MySQLCatalog::getDatabasePropertiesQuery() {
    return "SELECT "
           "schema_name, "
           "default_character_set_name, "
           "default_collation_name, "
           "sql_path, "
           "schema_comment "
           "FROM information_schema.schemata "
           "ORDER BY schema_name";
}

QString MySQLCatalog::getSecurityInfoQuery() {
    return "SELECT "
           "u.User, "
           "u.Host, "
           "u.authentication_string, "
           "u.password_expired, "
           "u.password_last_changed, "
           "u.password_lifetime, "
           "u.account_locked, "
           "u.Select_priv, "
           "u.Insert_priv, "
           "u.Update_priv, "
           "u.Delete_priv, "
           "u.Create_priv, "
           "u.Drop_priv, "
           "u.Reload_priv, "
           "u.Shutdown_priv, "
           "u.Process_priv, "
           "u.File_priv, "
           "u.Grant_priv, "
           "u.References_priv, "
           "u.Index_priv, "
           "u.Alter_priv, "
           "u.Show_db_priv, "
           "u.Super_priv, "
           "u.Create_tmp_table_priv, "
           "u.Lock_tables_priv, "
           "u.Execute_priv, "
           "u.Repl_slave_priv, "
           "u.Repl_client_priv, "
           "u.Create_view_priv, "
           "u.Show_view_priv, "
           "u.Create_routine_priv, "
           "u.Alter_routine_priv, "
           "u.Create_user_priv, "
           "u.Event_priv, "
           "u.Trigger_priv, "
           "u.Create_tablespace_priv, "
           "u.ssl_type, "
           "u.ssl_cipher, "
           "u.x509_issuer, "
           "u.x509_subject, "
           "u.max_questions, "
           "u.max_updates, "
           "u.max_connections, "
           "u.max_user_connections, "
           "u.plugin, "
           "u.authentication_string, "
           "u.password_expired, "
           "u.is_role, "
           "u.default_role, "
           "u.max_statement_time "
           "FROM mysql.user u "
           "ORDER BY u.User, u.Host";
}

QString MySQLCatalog::getEngineInfoQuery() {
    return "SELECT "
           "engine, "
           "support, "
           "comment, "
           "transactions, "
           "xa, "
           "savepoints "
           "FROM information_schema.engines "
           "ORDER BY engine";
}

QString MySQLCatalog::getStorageEnginesQuery() {
    return "SHOW STORAGE ENGINES";
}

QString MySQLCatalog::getCharsetInfoQuery() {
    return "SELECT "
           "character_set_name, "
           "default_collate_name, "
           "description, "
           "maxlen "
           "FROM information_schema.character_sets "
           "ORDER BY character_set_name";
}

QString MySQLCatalog::getProcessListQuery() {
    return "SELECT "
           "id, "
           "user, "
           "host, "
           "db, "
           "command, "
           "time, "
           "state, "
           "info, "
           "time_ms, "
           "stage, "
           "max_stage, "
           "progress, "
           "memory_used, "
           "examined_rows, "
           "query_id "
           "FROM information_schema.processlist "
           "ORDER BY time DESC";
}

QString MySQLCatalog::getPerformanceCountersQuery() {
    return "SELECT "
           "variable_name, "
           "variable_value "
           "FROM performance_schema.global_status "
           "ORDER BY variable_name";
}

QString MySQLCatalog::getQueryStatsQuery() {
    return "SELECT "
           "digest, "
           "digest_text, "
           "count_star, "
           "sum_timer_wait, "
           "min_timer_wait, "
           "avg_timer_wait, "
           "max_timer_wait, "
           "sum_lock_time, "
           "sum_errors, "
           "sum_warnings, "
           "sum_rows_affected, "
           "sum_rows_sent, "
           "sum_rows_examined, "
           "sum_created_tmp_disk_tables, "
           "sum_created_tmp_tables, "
           "sum_select_full_join, "
           "sum_select_full_range_join, "
           "sum_select_range, "
           "sum_select_range_check, "
           "sum_select_scan, "
           "sum_sort_merge_passes, "
           "sum_sort_range, "
           "sum_sort_rows, "
           "sum_sort_scan, "
           "sum_no_index_used, "
           "sum_no_good_index_used, "
           "first_seen, "
           "last_seen "
           "FROM performance_schema.events_statements_summary_by_digest "
           "ORDER BY sum_timer_wait DESC "
           "LIMIT 100";
}

QString MySQLCatalog::getIndexStatsQuery() {
    return "SELECT "
           "object_schema, "
           "object_name, "
           "index_name, "
           "count_read, "
           "count_fetch, "
           "count_insert, "
           "count_update, "
           "count_delete, "
           "sum_timer_wait, "
           "sum_timer_read, "
           "sum_timer_fetch, "
           "sum_timer_insert, "
           "sum_timer_update, "
           "sum_timer_delete, "
           "avg_timer_wait, "
           "avg_timer_read, "
           "avg_timer_fetch, "
           "avg_timer_insert, "
           "avg_timer_update, "
           "avg_timer_delete "
           "FROM performance_schema.table_io_waits_summary_by_index_usage "
           "WHERE count_read > 0 "
           "ORDER BY sum_timer_wait DESC";
}

QString MySQLCatalog::getTableStatsQuery() {
    return "SELECT "
           "object_schema, "
           "object_name, "
           "count_read, "
           "count_write, "
           "count_fetch, "
           "count_insert, "
           "count_update, "
           "count_delete, "
           "sum_timer_wait, "
           "sum_timer_read, "
           "sum_timer_write, "
           "sum_timer_fetch, "
           "sum_timer_insert, "
           "sum_timer_update, "
           "sum_timer_delete, "
           "avg_timer_wait, "
           "avg_timer_read, "
           "avg_timer_write, "
           "avg_timer_fetch, "
           "avg_timer_insert, "
           "avg_timer_update, "
           "avg_timer_delete "
           "FROM performance_schema.table_io_waits_summary_by_table "
           "ORDER BY sum_timer_wait DESC";
}

QString MySQLCatalog::getInnoDBStatsQuery() {
    return "SELECT "
           "name, "
           "count, "
           "status "
           "FROM information_schema.innodb_metrics "
           "WHERE status = 'enabled' "
           "ORDER BY name";
}

QString MySQLCatalog::getReplicationStatusQuery() {
    return "SHOW MASTER STATUS";
}

QString MySQLCatalog::getSlaveStatusQuery() {
    return "SHOW SLAVE STATUS\\G";
}

QString MySQLCatalog::getBinaryLogStatusQuery() {
    return "SHOW BINARY LOGS";
}

QString MySQLCatalog::getGlobalVariablesQuery() {
    return "SHOW GLOBAL VARIABLES";
}

QString MySQLCatalog::getSessionVariablesQuery() {
    return "SHOW SESSION VARIABLES";
}

QString MySQLCatalog::getSysSchemaSummaryQuery() {
    return "SELECT "
           "* "
           "FROM sys.host_summary "
           "ORDER BY statements DESC";
}

QString MySQLCatalog::getInnoDBMetricsQuery() {
    return "SELECT "
           "name, "
           "count, "
           "status "
           "FROM information_schema.innodb_metrics "
           "WHERE status = 'enabled'";
}

QString MySQLCatalog::getFulltextIndexQuery() {
    return "SELECT "
           "table_schema, "
           "table_name, "
           "column_name, "
           "index_name "
           "FROM information_schema.statistics "
           "WHERE index_type = 'FULLTEXT'";
}

QStringList MySQLCatalog::getSystemTableList() {
    return QStringList() << "user" << "db" << "host" << "tables_priv" << "columns_priv"
                        << "procs_priv" << "proxies_priv" << "event" << "func" << "general_log"
                        << "help_category" << "help_keyword" << "help_relation" << "help_topic"
                        << "innodb_index_stats" << "innodb_table_stats" << "ndb_binlog_index"
                        << "plugin" << "proc" << "procs_priv" << "servers" << "slave_master_info"
                        << "slave_relay_log_info" << "slave_worker_info" << "slow_log" << "time_zone"
                        << "time_zone_leap_second" << "time_zone_name" << "time_zone_transition"
                        << "time_zone_transition_type";
}

QStringList MySQLCatalog::getSystemViewList() {
    return QStringList() << "information_schema.tables" << "information_schema.columns"
                        << "information_schema.views" << "information_schema.routines"
                        << "information_schema.triggers" << "information_schema.key_column_usage"
                        << "information_schema.table_constraints" << "information_schema.schemata"
                        << "information_schema.engines" << "information_schema.plugins"
                        << "information_schema.character_sets" << "information_schema.collations"
                        << "information_schema.collation_character_set_applicability"
                        << "information_schema.column_privileges" << "information_schema.key_caches"
                        << "information_schema.parameters" << "information_schema.partitions"
                        << "information_schema.profiling" << "information_schema.processlist"
                        << "information_schema.referential_constraints" << "information_schema.session_status"
                        << "information_schema.session_variables" << "information_schema.statistics"
                        << "information_schema.tablespaces" << "information_schema.table_constraints"
                        << "information_schema.table_privileges" << "information_schema.user_privileges"
                        << "information_schema.variables";
}

QStringList MySQLCatalog::getInformationSchemaViewList() {
    return QStringList() << "TABLES" << "COLUMNS" << "VIEWS" << "ROUTINES" << "TRIGGERS"
                        << "KEY_COLUMN_USAGE" << "TABLE_CONSTRAINTS" << "SCHEMATA" << "ENGINES"
                        << "PLUGINS" << "CHARACTER_SETS" << "COLLATIONS" << "COLLATION_CHARACTER_SET_APPLICABILITY"
                        << "COLUMN_PRIVILEGES" << "KEY_CACHES" << "PARAMETERS" << "PARTITIONS" << "PROFILING"
                        << "PROCESSLIST" << "REFERENTIAL_CONSTRAINTS" << "SESSION_STATUS" << "SESSION_VARIABLES"
                        << "STATISTICS" << "TABLESPACES" << "TABLE_CONSTRAINTS" << "TABLE_PRIVILEGES"
                        << "USER_PRIVILEGES" << "VARIABLES";
}

QStringList MySQLCatalog::getPerformanceSchemaViewList() {
    return QStringList() << "accounts" << "cond_instances" << "events_stages_current"
                        << "events_stages_history" << "events_stages_history_long"
                        << "events_stages_summary_by_account_by_event_name"
                        << "events_stages_summary_by_host_by_event_name"
                        << "events_stages_summary_by_thread_by_event_name"
                        << "events_stages_summary_by_user_by_event_name"
                        << "events_stages_summary_global_by_event_name"
                        << "events_statements_current" << "events_statements_history"
                        << "events_statements_history_long" << "events_statements_summary_by_account_by_event_name"
                        << "events_statements_summary_by_digest" << "events_statements_summary_by_host_by_event_name"
                        << "events_statements_summary_by_thread_by_event_name"
                        << "events_statements_summary_by_user_by_event_name"
                        << "events_statements_summary_global_by_event_name"
                        << "events_transactions_current" << "events_transactions_history"
                        << "events_transactions_history_long" << "events_waits_current"
                        << "events_waits_history" << "events_waits_history_long"
                        << "events_waits_summary_by_account_by_event_name"
                        << "events_waits_summary_by_host_by_event_name"
                        << "events_waits_summary_by_instance" << "events_waits_summary_by_thread_by_event_name"
                        << "events_waits_summary_by_user_by_event_name"
                        << "events_waits_summary_global_by_event_name"
                        << "events_waits_summary_global_by_event_name" << "file_instances"
                        << "file_summary_by_event_name" << "file_summary_by_instance"
                        << "host_cache" << "hosts" << "memory_summary_by_account_by_event_name"
                        << "memory_summary_by_host_by_event_name" << "memory_summary_by_thread_by_event_name"
                        << "memory_summary_by_user_by_event_name" << "memory_summary_global_by_current_bytes"
                        << "memory_summary_global_by_current_bytes" << "memory_summary_global_total"
                        << "metadata_locks" << "mutex_instances" << "objects_summary_global_by_type"
                        << "performance_timers" << "prepared_statements_instances"
                        << "replication_applier_configuration" << "replication_applier_status"
                        << "replication_applier_status_by_coordinator"
                        << "replication_applier_status_by_worker" << "replication_connection_configuration"
                        << "replication_connection_status" << "replication_group_member_stats"
                        << "replication_group_members" << "rwlock_instances" << "session_account_connect_attrs"
                        << "session_connect_attrs" << "setup_actors" << "setup_consumers" << "setup_instruments"
                        << "setup_objects" << "setup_timers" << "socket_instances" << "socket_summary_by_event_name"
                        << "socket_summary_by_instance" << "status_by_account" << "status_by_host"
                        << "status_by_thread" << "status_by_user" << "table_handles" << "table_io_waits_summary_by_index_usage"
                        << "table_io_waits_summary_by_table" << "table_lock_waits_summary_by_table"
                        << "threads" << "user_variables_by_thread" << "users" << "variables_by_thread"
                        << "variables_info";
}

QStringList MySQLCatalog::getMySQLSchemaTableList() {
    return QStringList() << "user" << "db" << "host" << "tables_priv" << "columns_priv"
                        << "procs_priv" << "proxies_priv" << "event" << "func" << "general_log"
                        << "help_category" << "help_keyword" << "help_relation" << "help_topic"
                        << "innodb_index_stats" << "innodb_table_stats" << "ndb_binlog_index"
                        << "plugin" << "proc" << "procs_priv" << "servers" << "slave_master_info"
                        << "slave_relay_log_info" << "slave_worker_info" << "slow_log" << "time_zone"
                        << "time_zone_leap_second" << "time_zone_name" << "time_zone_transition"
                        << "time_zone_transition_type";
}

QStringList MySQLCatalog::getSysSchemaViewList() {
    return QStringList() << "version" << "innodb_buffer_stats_by_schema" << "innodb_buffer_stats_by_table"
                        << "innodb_lock_waits" << "innodb_lock_holds" << "host_summary" << "host_summary_by_file_io"
                        << "host_summary_by_file_io_type" << "host_summary_by_stages" << "host_summary_by_statement_type"
                        << "host_summary_by_statement_latency" << "io_by_thread_by_latency" << "io_global_by_file_by_bytes"
                        << "io_global_by_file_by_latency" << "io_global_by_wait_by_bytes" << "io_global_by_wait_by_latency"
                        << "latest_file_io" << "memory_by_host_by_current_bytes" << "memory_by_thread_by_current_bytes"
                        << "memory_by_user_by_current_bytes" << "memory_global_by_current_bytes" << "memory_global_total"
                        << "processlist" << "ps_check_lost_instrumentation" << "schema_auto_increment_columns"
                        << "schema_foreign_keys" << "schema_index_statistics" << "schema_object_overview"
                        << "schema_redundant_indexes" << "schema_table_lock_waits" << "schema_table_statistics"
                        << "schema_table_statistics_with_buffer" << "schema_tables_with_full_table_scans"
                        << "schema_unused_indexes" << "session" << "statement_analysis" << "statements_with_errors_or_warnings"
                        << "statements_with_full_table_scans" << "statements_with_runtimes_in_95th_percentile"
                        << "statements_with_sorting" << "statements_with_temp_tables" << "user_summary"
                        << "user_summary_by_file_io" << "user_summary_by_file_io_type" << "user_summary_by_stages"
                        << "user_summary_by_statement_type" << "user_summary_by_statement_latency" << "wait_classes_global_by_avg_latency"
                        << "wait_classes_global_by_latency" << "waits_by_host_by_latency" << "waits_by_user_by_latency"
                        << "waits_global_by_latency" << "x$host_summary" << "x$host_summary_by_file_io" << "x$host_summary_by_file_io_type"
                        << "x$host_summary_by_stages" << "x$host_summary_by_statement_type" << "x$host_summary_by_statement_latency"
                        << "x$innodb_buffer_stats_by_schema" << "x$innodb_buffer_stats_by_table" << "x$innodb_lock_waits"
                        << "x$io_by_thread_by_latency" << "x$io_global_by_file_by_bytes" << "x$io_global_by_file_by_latency"
                        << "x$io_global_by_wait_by_bytes" << "x$io_global_by_wait_by_latency" << "x$latest_file_io"
                        << "x$memory_by_host_by_current_bytes" << "x$memory_by_thread_by_current_bytes"
                        << "x$memory_by_user_by_current_bytes" << "x$memory_global_by_current_bytes" << "x$memory_global_total"
                        << "x$processlist" << "x$ps_check_lost_instrumentation" << "x$schema_auto_increment_columns"
                        << "x$schema_foreign_keys" << "x$schema_index_statistics" << "x$schema_object_overview"
                        << "x$schema_redundant_indexes" << "x$schema_table_lock_waits" << "x$schema_table_statistics"
                        << "x$schema_table_statistics_with_buffer" << "x$schema_tables_with_full_table_scans"
                        << "x$schema_unused_indexes" << "x$session" << "x$statement_analysis" << "x$statements_with_errors_or_warnings"
                        << "x$statements_with_full_table_scans" << "x$statements_with_runtimes_in_95th_percentile"
                        << "x$statements_with_sorting" << "x$statements_with_temp_tables" << "x$user_summary"
                        << "x$user_summary_by_file_io" << "x$user_summary_by_file_io_type" << "x$user_summary_by_stages"
                        << "x$user_summary_by_statement_type" << "x$user_summary_by_statement_latency" << "x$wait_classes_global_by_avg_latency"
                        << "x$wait_classes_global_by_latency" << "x$waits_by_host_by_latency" << "x$waits_by_user_by_latency"
                        << "x$waits_global_by_latency";
}

QString MySQLCatalog::formatObjectName(const QString& schema, const QString& object, const QString& database) {
    QString result;
    if (!database.isEmpty()) {
        result = database + ".";
    }
    if (!schema.isEmpty()) {
        result += "`" + schema + "`.";
    }
    result += "`" + object + "`";
    return result;
}

QString MySQLCatalog::escapeIdentifier(const QString& identifier) {
    return "`" + identifier + "`";
}

QString MySQLCatalog::getVersionSpecificQuery(const QString& baseQuery, int majorVersion, int minorVersion) {
    // For now, return the base query. In the future, this could modify queries
    // based on version-specific features
    return baseQuery;
}

QString MySQLCatalog::getVersionInfoQuery() {
    return "SELECT VERSION() as version_string, @@version_comment as version_comment, "
           "@@version_compile_machine as compile_machine, @@version_compile_os as compile_os";
}

QString MySQLCatalog::getFeatureSupportQuery(const QString& feature) {
    QString query = "SELECT VERSION() as version, "
                   "CASE WHEN @@version >= '%1' THEN 'SUPPORTED' ELSE 'NOT_SUPPORTED' END as support_status";

    // Define minimum versions for features
    QMap<QString, QString> featureVersions;
    featureVersions["JSON"] = "5.7.0";           // MySQL 5.7
    featureVersions["CTE"] = "8.0.0";            // MySQL 8.0
    featureVersions["WINDOW_FUNCTIONS"] = "8.0.0"; // MySQL 8.0
    featureVersions["INVISIBLE_INDEXES"] = "8.0.0"; // MySQL 8.0
    featureVersions["EXPRESSION_INDEXES"] = "8.0.0"; // MySQL 8.0
    featureVersions["DESCENDING_INDEXES"] = "8.0.0"; // MySQL 8.0
    featureVersions["PERFORMANCE_SCHEMA"] = "5.5.0"; // MySQL 5.5
    featureVersions["PARTITIONING"] = "5.1.0";    // MySQL 5.1
    featureVersions["FULLTEXT"] = "3.23.23";      // MySQL 3.23.23

    return query.arg(featureVersions.value(feature, "5.1.0"));
}

QString MySQLCatalog::getEngineSupportQuery() {
    return "SELECT engine, support, comment FROM information_schema.engines";
}

QString MySQLCatalog::getPluginSupportQuery() {
    return "SELECT plugin_name, plugin_version, plugin_status, plugin_type, "
           "plugin_type_version, plugin_library, plugin_library_version, "
           "plugin_author, plugin_description, plugin_license, load_option "
           "FROM information_schema.plugins";
}

} // namespace scratchrobin
