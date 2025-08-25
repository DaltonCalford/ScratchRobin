#include "postgresql_catalog.h"
#include <QRegularExpression>

namespace scratchrobin {

// PostgreSQL System Catalog Implementation

QString PostgreSQLCatalog::getSystemDatabasesQuery() {
    return "SELECT "
           "datname as database_name, "
           "datdba as owner_id, "
           "encoding as encoding_id, "
           "datcollate as collate_name, "
           "datctype as ctype_name, "
           "datistemplate as is_template, "
           "datallowconn as allow_connections, "
           "datconnlimit as connection_limit, "
           "datlastsysoid as last_sysoid, "
           "datfrozenxid as frozen_xid, "
           "datminmxid as min_mxid, "
           "dattablespace as tablespace_id, "
           "datacl as access_privileges "
           "FROM pg_database "
           "WHERE datistemplate = false OR datname = 'template0' "
           "ORDER BY datname";
}

QString PostgreSQLCatalog::getUserDatabasesQuery() {
    return "SELECT "
           "datname as database_name, "
           "datdba as owner_id, "
           "encoding as encoding_id, "
           "datcollate as collate_name, "
           "datctype as ctype_name, "
           "datistemplate as is_template, "
           "datallowconn as allow_connections, "
           "datconnlimit as connection_limit, "
           "datlastsysoid as last_sysoid, "
           "datfrozenxid as frozen_xid, "
           "datminmxid as min_mxid, "
           "dattablespace as tablespace_id, "
           "datacl as access_privileges, "
           "pg_size_pretty(pg_database_size(datname)) as size_pretty, "
           "pg_database_size(datname) as size_bytes "
           "FROM pg_database "
           "WHERE datistemplate = false AND datname NOT IN ('postgres', 'template0', 'template1') "
           "ORDER BY datname";
}

QString PostgreSQLCatalog::getDatabaseInfoQuery(const QString& database) {
    QString query = "SELECT "
                   "current_database() as current_database, "
                   "version() as version_info, "
                   "inet_server_addr() as server_address, "
                   "inet_server_port() as server_port, "
                   "current_setting('server_version') as server_version, "
                   "current_setting('server_version_num') as server_version_num, "
                   "current_setting('server_encoding') as server_encoding, "
                   "current_setting('client_encoding') as client_encoding, "
                   "current_setting('lc_collate') as lc_collate, "
                   "current_setting('lc_ctype') as lc_ctype, "
                   "current_setting('timezone') as timezone, "
                   "current_setting('shared_buffers') as shared_buffers, "
                   "current_setting('work_mem') as work_mem, "
                   "current_setting('maintenance_work_mem') as maintenance_work_mem, "
                   "current_setting('effective_cache_size') as effective_cache_size, "
                   "current_setting('max_connections') as max_connections, "
                   "current_setting('autovacuum') as autovacuum_enabled, "
                   "current_setting('log_statement') as log_statement, "
                   "current_setting('log_duration') as log_duration, "
                   "pg_size_pretty(pg_database_size(current_database())) as database_size";

    if (!database.isEmpty()) {
        query += QString(", (SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '%1') as table_count").arg(database);
    }

    return query;
}

QString PostgreSQLCatalog::getSchemasQuery() {
    return "SELECT "
           "n.nspname as schema_name, "
           "n.nspowner as owner_id, "
           "usename as owner_name, "
           "n.nspacl as access_privileges, "
           "obj_description(n.oid, 'pg_namespace') as description "
           "FROM pg_namespace n "
           "LEFT JOIN pg_user u ON n.nspowner = u.usesysid "
           "WHERE nspname NOT IN ('pg_catalog', 'pg_toast', 'information_schema') "
           "AND nspname NOT LIKE 'pg_temp_%' "
           "ORDER BY nspname";
}

QString PostgreSQLCatalog::getTablesQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "schemaname as schema_name, "
                   "tablename as table_name, "
                   "tableowner as owner_name, "
                   "tablespace, "
                   "hasindexes, "
                   "hasrules, "
                   "hastriggers, "
                   "rowsecurity, "
                   "pg_size_pretty(pg_total_relation_size(schemaname||'.'||tablename)) as total_size, "
                   "pg_size_pretty(pg_relation_size(schemaname||'.'||tablename)) as table_size, "
                   "pg_size_pretty(pg_total_relation_size(schemaname||'.'||tablename) - "
                   "pg_relation_size(schemaname||'.'||tablename)) as index_size, "
                   "n_tup_ins as inserts, "
                   "n_tup_upd as updates, "
                   "n_tup_del as deletes, "
                   "n_live_tup as live_tuples, "
                   "n_dead_tup as dead_tuples, "
                   "last_vacuum, "
                   "last_autovacuum, "
                   "last_analyze, "
                   "last_autoanalyze, "
                   "vacuum_count, "
                   "autovacuum_count, "
                   "analyze_count, "
                   "autoanalyze_count "
                   "FROM pg_stat_user_tables ";

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("schemaname = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY schemaname, tablename";
    return query;
}

QString PostgreSQLCatalog::getViewsQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "schemaname as schema_name, "
                   "viewname as view_name, "
                   "viewowner as owner_name, "
                   "definition as view_definition, "
                   "pg_get_viewdef(schemaname||'.'||viewname, true) as full_definition, "
                   "isinsertableinto, "
                   "isupdatable, "
                   "istriggerinsertableinto, "
                   "istriggerupdatable, "
                   "obj_description(v.oid, 'pg_class') as description "
                   "FROM pg_views v "
                   "LEFT JOIN pg_class c ON v.viewname = c.relname "
                   "LEFT JOIN pg_namespace n ON c.relnamespace = n.oid AND v.schemaname = n.nspname ";

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("schemaname = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY schemaname, viewname";
    return query;
}

QString PostgreSQLCatalog::getColumnsQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "table_schema, "
                   "table_name, "
                   "column_name, "
                   "ordinal_position, "
                   "column_default, "
                   "is_nullable, "
                   "data_type, "
                   "character_maximum_length, "
                   "character_octet_length, "
                   "numeric_precision, "
                   "numeric_scale, "
                   "datetime_precision, "
                   "udt_name, "
                   "udt_schema, "
                   "is_identity, "
                   "identity_generation, "
                   "identity_start, "
                   "identity_increment, "
                   "identity_maximum, "
                   "identity_minimum, "
                   "identity_cycle, "
                   "collation_name, "
                   "column_comment "
                   "FROM information_schema.columns ";

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("table_schema = '%1'").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("table_name = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY table_schema, table_name, ordinal_position";
    return query;
}

QString PostgreSQLCatalog::getIndexesQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "schemaname as schema_name, "
                   "tablename as table_name, "
                   "indexname as index_name, "
                   "indexdef as index_definition, "
                   "tablespace, "
                   "indexrelid::regclass as index_relation, "
                   "indrelid::regclass as table_relation, "
                   "indisunique as is_unique, "
                   "indisprimary as is_primary, "
                   "indisexclusion as is_exclusion, "
                   "indimmediate as is_immediate, "
                   "indisclustered as is_clustered, "
                   "indisvalid as is_valid, "
                   "indcheckxmin as check_xmin, "
                   "indisready as is_ready, "
                   "indislive as is_live, "
                   "indisreplident as is_replica_identity, "
                   "pg_size_pretty(pg_relation_size(indexrelid)) as index_size, "
                   "pg_relation_size(indexrelid) as index_size_bytes, "
                   "idx_scan as scans, "
                   "idx_tup_read as tuples_read, "
                   "idx_tup_fetch as tuples_fetched, "
                   "array_to_string(indkey, ',') as column_numbers, "
                   "array_to_string(indcollation, ',') as collation_oids, "
                   "array_to_string(indclass, ',') as operator_classes, "
                   "array_to_string(indoption, ',') as options "
                   "FROM pg_indexes i "
                   "LEFT JOIN pg_stat_user_indexes sui ON i.schemaname = sui.schemaname "
                   "AND i.tablename = sui.tablename AND i.indexname = sui.indexname "
                   "LEFT JOIN pg_index pi ON sui.indexrelid = pi.indexrelid ";

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("i.schemaname = '%1'").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("i.tablename = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY i.schemaname, i.tablename, i.indexname";
    return query;
}

QString PostgreSQLCatalog::getConstraintsQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "tc.table_schema, "
                   "tc.table_name, "
                   "tc.constraint_name, "
                   "tc.constraint_type, "
                   "kcu.column_name, "
                   "kcu.ordinal_position, "
                   "ccu.table_schema as referenced_schema, "
                   "ccu.table_name as referenced_table, "
                   "ccu.column_name as referenced_column, "
                   "rc.delete_rule, "
                   "rc.update_rule, "
                   "rc.match_option, "
                   "tc.is_deferrable, "
                   "tc.initially_deferred, "
                   "check_clause "
                   "FROM information_schema.table_constraints tc "
                   "LEFT JOIN information_schema.key_column_usage kcu ON "
                   "tc.constraint_name = kcu.constraint_name AND "
                   "tc.table_schema = kcu.table_schema AND "
                   "tc.table_name = kcu.table_name "
                   "LEFT JOIN information_schema.constraint_column_usage ccu ON "
                   "tc.constraint_name = ccu.constraint_name AND "
                   "tc.table_schema = ccu.table_schema "
                   "LEFT JOIN information_schema.referential_constraints rc ON "
                   "tc.constraint_name = rc.constraint_name AND "
                   "tc.table_schema = rc.constraint_schema "
                   "LEFT JOIN pg_constraint pc ON tc.constraint_name = pc.conname "
                   "AND pc.contype = CASE tc.constraint_type "
                   "WHEN 'PRIMARY KEY' THEN 'p' "
                   "WHEN 'FOREIGN KEY' THEN 'f' "
                   "WHEN 'UNIQUE' THEN 'u' "
                   "WHEN 'CHECK' THEN 'c' "
                   "WHEN 'EXCLUDE' THEN 'x' "
                   "END "
                   "LEFT JOIN pg_get_constraintdef(pc.oid) check_clause ON tc.constraint_type = 'CHECK' ";

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("tc.table_schema = '%1'").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("tc.table_name = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY tc.table_schema, tc.table_name, tc.constraint_name, kcu.ordinal_position";
    return query;
}

QString PostgreSQLCatalog::getForeignKeysQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "conname as constraint_name, "
                   "conrelid::regclass as table_name, "
                   "confrelid::regclass as referenced_table, "
                   "array_agg(a1.attname ORDER BY attnum) as columns, "
                   "array_agg(a2.attname ORDER BY attnum) as referenced_columns, "
                   "confupdtype as update_rule, "
                   "confdeltype as delete_rule, "
                   "confmatchtype as match_type, "
                   "condeferrable as is_deferrable, "
                   "condeferred as is_deferred "
                   "FROM pg_constraint c "
                   "JOIN pg_attribute a1 ON a1.attrelid = c.conrelid AND a1.attnum = ANY(c.conkey) "
                   "JOIN pg_attribute a2 ON a2.attrelid = c.confrelid AND a2.attnum = ANY(c.confkey) "
                   "WHERE c.contype = 'f' ";

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("c.connamespace = (SELECT oid FROM pg_namespace WHERE nspname = '%1')").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("conrelid::regclass::text = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "AND " + conditions.join(" AND ");
    }

    query += "GROUP BY conname, conrelid, confrelid, confupdtype, confdeltype, confmatchtype, condeferrable, condeferred "
             "ORDER BY conname";
    return query;
}

QString PostgreSQLCatalog::getStoredProceduresQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "n.nspname as schema_name, "
                   "p.proname as procedure_name, "
                   "pg_get_function_identity_arguments(p.oid) as arguments, "
                   "pg_get_functiondef(p.oid) as definition, "
                   "p.prokind as kind, "
                   "p.prosecdef as security_definer, "
                   "p.proleakproof as leak_proof, "
                   "p.proisstrict as strict, "
                   "p.proretset as returns_set, "
                   "p.provolatile as volatility, "
                   "p.procost as cost, "
                   "p.prorows as rows_estimate, "
                   "l.lanname as language, "
                   "p.proowner::regrole as owner, "
                   "obj_description(p.oid, 'pg_proc') as description "
                   "FROM pg_proc p "
                   "LEFT JOIN pg_namespace n ON p.pronamespace = n.oid "
                   "LEFT JOIN pg_language l ON p.prolang = l.oid "
                   "WHERE p.prokind = 'p' ";  // 'p' for procedures

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("n.nspname = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "AND " + conditions.join(" AND ");
    }

    query += "ORDER BY n.nspname, p.proname";
    return query;
}

QString PostgreSQLCatalog::getFunctionsQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "n.nspname as schema_name, "
                   "p.proname as function_name, "
                   "pg_get_function_identity_arguments(p.oid) as arguments, "
                   "pg_get_function_result(p.oid) as return_type, "
                   "pg_get_functiondef(p.oid) as definition, "
                   "p.prokind as kind, "
                   "p.prosecdef as security_definer, "
                   "p.proleakproof as leak_proof, "
                   "p.proisstrict as strict, "
                   "p.proretset as returns_set, "
                   "p.provolatile as volatility, "
                   "p.procost as cost, "
                   "p.prorows as rows_estimate, "
                   "l.lanname as language, "
                   "p.proowner::regrole as owner, "
                   "obj_description(p.oid, 'pg_proc') as description "
                   "FROM pg_proc p "
                   "LEFT JOIN pg_namespace n ON p.pronamespace = n.oid "
                   "LEFT JOIN pg_language l ON p.prolang = l.oid "
                   "WHERE p.prokind = 'f' ";  // 'f' for functions

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("n.nspname = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "AND " + conditions.join(" AND ");
    }

    query += "ORDER BY n.nspname, p.proname";
    return query;
}

QString PostgreSQLCatalog::getTriggersQuery(const QString& table, const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "tgname as trigger_name, "
                   "tgrelid::regclass as table_name, "
                   "tgfoid::regproc as function_name, "
                   "tgenabled as enabled, "
                   "tgtype as trigger_type, "
                   "tgattr as attribute_numbers, "
                   "tgargs as arguments, "
                   "tgqual as when_clause, "
                   "tgdeferrable as is_deferrable, "
                   "tginitdeferred as initially_deferred, "
                   "obj_description(t.oid, 'pg_trigger') as description "
                   "FROM pg_trigger t "
                   "LEFT JOIN pg_class c ON t.tgrelid = c.oid "
                   "LEFT JOIN pg_namespace n ON c.relnamespace = n.oid "
                   "WHERE t.tgisinternal = false ";

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("n.nspname = '%1'").arg(schema);
    }
    if (!table.isEmpty()) {
        conditions << QString("tgrelid::regclass::text = '%1'").arg(table);
    }

    if (!conditions.isEmpty()) {
        query += "AND " + conditions.join(" AND ");
    }

    query += "ORDER BY tgname";
    return query;
}

QString PostgreSQLCatalog::getSequencesQuery(const QString& schema, const QString& database) {
    QString query = "SELECT "
                   "schemaname as schema_name, "
                   "sequencename as sequence_name, "
                   "sequenceowner as owner_name, "
                   "data_type, "
                   "start_value, "
                   "min_value, "
                   "max_value, "
                   "increment_by, "
                   "cycle, "
                   "cache_size, "
                   "last_value "
                   "FROM pg_sequences ";

    QStringList conditions;
    if (!schema.isEmpty()) {
        conditions << QString("schemaname = '%1'").arg(schema);
    }

    if (!conditions.isEmpty()) {
        query += "WHERE " + conditions.join(" AND ");
    }

    query += "ORDER BY schemaname, sequencename";
    return query;
}

QString PostgreSQLCatalog::getEventsQuery(const QString& schema, const QString& database) {
    // PostgreSQL doesn't have events like MySQL, but it has scheduled jobs via pg_cron or similar
    return "SELECT "
           "jobname as event_name, "
           "jobid as event_id, "
           "username as owner, "
           "command as event_command, "
           "node_string as schedule, "
           "nextrun as next_run, "
           "lastrun as last_run, "
           "lastsuccess as last_success, "
           "thisrun as current_run, "
           "totalruns as total_runs, "
           "totalfailures as total_failures "
           "FROM cron.job ";
}

QString PostgreSQLCatalog::getServerInfoQuery() {
    return "SELECT "
           "version() as version_string, "
           "inet_server_addr() as server_address, "
           "inet_server_port() as server_port, "
           "current_setting('server_version') as server_version, "
           "current_setting('server_version_num') as server_version_num, "
           "current_setting('server_encoding') as server_encoding, "
           "current_setting('client_encoding') as client_encoding, "
           "current_setting('lc_collate') as lc_collate, "
           "current_setting('lc_ctype') as lc_ctype, "
           "current_setting('timezone') as timezone, "
           "current_setting('shared_buffers') as shared_buffers, "
           "current_setting('work_mem') as work_mem, "
           "current_setting('maintenance_work_mem') as maintenance_work_mem, "
           "current_setting('effective_cache_size') as effective_cache_size, "
           "current_setting('max_connections') as max_connections, "
           "current_setting('autovacuum') as autovacuum_enabled, "
           "current_setting('log_statement') as log_statement, "
           "current_setting('log_duration') as log_duration, "
           "pg_size_pretty(pg_database_size(current_database())) as database_size";
}

QString PostgreSQLCatalog::getDatabasePropertiesQuery() {
    return "SELECT "
           "datname as database_name, "
           "datdba::regrole as owner, "
           "encoding, "
           "datcollate as collate_name, "
           "datctype as ctype_name, "
           "datistemplate as is_template, "
           "datallowconn as allow_connections, "
           "datconnlimit as connection_limit, "
           "pg_size_pretty(pg_database_size(datname)) as size_pretty, "
           "pg_database_size(datname) as size_bytes "
           "FROM pg_database "
           "ORDER BY datname";
}

QString PostgreSQLCatalog::getSecurityInfoQuery() {
    return "SELECT "
           "usename as username, "
           "usesysid as user_id, "
           "usecreatedb as can_create_db, "
           "usesuper as is_superuser, "
           "userepl as can_replicate, "
           "usebypassrls as bypass_rls, "
           "passwd as password, "
           "valuntil as password_expires, "
           "useconfig as configuration "
           "FROM pg_user "
           "ORDER BY usename";
}

QString PostgreSQLCatalog::getEngineInfoQuery() {
    return "SELECT "
           "name as extension_name, "
           "default_version as default_version, "
           "installed_version, "
           "comment as description "
           "FROM pg_available_extensions "
           "ORDER BY name";
}

QString PostgreSQLCatalog::getStorageEnginesQuery() {
    return "SELECT "
           "spcname as tablespace_name, "
           "spcowner::regrole as owner, "
           "spcacl as access_privileges, "
           "pg_size_pretty(pg_tablespace_size(spcname)) as size_pretty, "
           "pg_tablespace_size(spcname) as size_bytes, "
           "spcoptions as options "
           "FROM pg_tablespace "
           "ORDER BY spcname";
}

QString PostgreSQLCatalog::getCharsetInfoQuery() {
    return "SELECT "
           "encoding, "
           "name as charset_name, "
           "description "
           "FROM pg_encoding "
           "ORDER BY encoding";
}

QString PostgreSQLCatalog::getProcessListQuery() {
    return "SELECT "
           "pid, "
           "datname as database_name, "
           "usename as username, "
           "client_addr as client_address, "
           "client_port, "
           "backend_start, "
           "query_start, "
           "state_change, "
           "wait_event_type, "
           "wait_event, "
           "state, "
           "query, "
           "backend_xid, "
           "backend_xmin, "
           "usesysid as user_id "
           "FROM pg_stat_activity "
           "ORDER BY pid";
}

QString PostgreSQLCatalog::getPerformanceCountersQuery() {
    return "SELECT "
           "name as counter_name, "
           "setting as current_value, "
           "boot_val as boot_value, "
           "reset_val as reset_value, "
           "unit as unit, "
           "short_desc as short_description, "
           "extra_desc as extra_description, "
           "min_val as minimum_value, "
           "max_val as maximum_value, "
           "enumvals as enum_values, "
           "boot_val as boot_value, "
           "reset_val as reset_value, "
           "source as source, "
           "sourceline as source_line "
           "FROM pg_settings "
           "ORDER BY name";
}

QString PostgreSQLCatalog::getQueryStatsQuery() {
    return "SELECT "
           "schemaname, "
           "tablename, "
           "attname as column_name, "
           "inherited, "
           "null_frac as null_fraction, "
           "avg_width as average_width, "
           "n_distinct as distinct_values, "
           "correlation, "
           "most_common_vals as most_common_values, "
           "most_common_freqs as most_common_frequencies, "
           "histogram_bounds as histogram_bounds, "
           "correlation as correlation, "
           "most_common_elems as most_common_elements, "
           "most_common_elem_freqs as most_common_element_frequencies, "
           "elem_count_histogram as element_count_histogram "
           "FROM pg_stats "
           "ORDER BY schemaname, tablename, attname";
}

QString PostgreSQLCatalog::getIndexStatsQuery() {
    return "SELECT "
           "schemaname, "
           "tablename, "
           "indexname, "
           "idx_scan as index_scans, "
           "idx_tup_read as tuples_read, "
           "idx_tup_fetch as tuples_fetched, "
           "pg_size_pretty(pg_relation_size(indexrelid)) as index_size, "
           "pg_relation_size(indexrelid) as index_size_bytes "
           "FROM pg_stat_user_indexes "
           "ORDER BY idx_scan DESC";
}

QString PostgreSQLCatalog::getTableStatsQuery() {
    return "SELECT "
           "schemaname, "
           "tablename, "
           "seq_scan as sequential_scans, "
           "seq_tup_read as sequential_tuples_read, "
           "idx_scan as index_scans, "
           "idx_tup_fetch as index_tuples_fetched, "
           "n_tup_ins as tuples_inserted, "
           "n_tup_upd as tuples_updated, "
           "n_tup_del as tuples_deleted, "
           "n_live_tup as live_tuples, "
           "n_dead_tup as dead_tuples, "
           "n_mod_since_analyze as modified_since_analyze, "
           "last_vacuum, "
           "last_autovacuum, "
           "last_analyze, "
           "last_autoanalyze, "
           "vacuum_count, "
           "autovacuum_count, "
           "analyze_count, "
           "autoanalyze_count, "
           "pg_size_pretty(pg_total_relation_size(schemaname||'.'||tablename)) as total_size "
           "FROM pg_stat_user_tables "
           "ORDER BY schemaname, tablename";
}

QString PostgreSQLCatalog::getInnoDBStatsQuery() {
    return "SELECT "
           "schemaname, "
           "tablename, "
           "n_tup_ins as inserts, "
           "n_tup_upd as updates, "
           "n_tup_del as deletes, "
           "n_live_tup as live_tuples, "
           "n_dead_tup as dead_tuples, "
           "last_vacuum, "
           "last_autovacuum, "
           "vacuum_count, "
           "autovacuum_count "
           "FROM pg_stat_user_tables "
           "ORDER BY n_live_tup DESC";
}

QString PostgreSQLCatalog::getReplicationStatusQuery() {
    return "SELECT "
           "client_addr as client_address, "
           "client_port, "
           "pid, "
           "usesysid as user_id, "
           "usename as username, "
           "application_name, "
           "client_addr, "
           "client_port, "
           "backend_start, "
           "backend_xmin, "
           "state, "
           "sent_lsn, "
           "write_lsn, "
           "flush_lsn, "
           "replay_lsn, "
           "write_lag, "
           "flush_lag, "
           "replay_lag, "
           "sync_priority, "
           "sync_state "
           "FROM pg_stat_replication";
}

QString PostgreSQLCatalog::getSlaveStatusQuery() {
    return "SELECT "
           "pid, "
           "usesysid as user_id, "
           "usename as username, "
           "application_name, "
           "client_addr, "
           "client_port, "
           "backend_start, "
           "backend_xmin, "
           "state, "
           "sent_lsn, "
           "write_lsn, "
           "flush_lsn, "
           "replay_lsn, "
           "write_lag, "
           "flush_lag, "
           "replay_lag, "
           "sync_priority, "
           "sync_state, "
           "reply_time "
           "FROM pg_stat_wal_receiver";
}

QString PostgreSQLCatalog::getBinaryLogStatusQuery() {
    return "SELECT "
           "slot_name, "
           "plugin, "
           "slot_type, "
           "datoid as database_id, "
           "datname as database, "
           "temporary, "
           "active, "
           "active_pid, "
           "xmin, "
           "catalog_xmin, "
           "restart_lsn, "
           "confirmed_flush_lsn, "
           "wal_status, "
           "safe_wal_size, "
           "two_phase "
           "FROM pg_replication_slots";
}

QString PostgreSQLCatalog::getGlobalVariablesQuery() {
    return "SHOW ALL";
}

QString PostgreSQLCatalog::getSessionVariablesQuery() {
    return "SELECT "
           "name, "
           "setting as value, "
           "short_desc as description "
           "FROM pg_settings "
           "ORDER BY name";
}

QString PostgreSQLCatalog::getSysSchemaSummaryQuery() {
    return "SELECT "
           "schemaname, "
           "tablename, "
           "seq_scan as sequential_scans, "
           "seq_tup_read as sequential_tuples_read, "
           "idx_scan as index_scans, "
           "idx_tup_fetch as index_tuples_fetched, "
           "n_tup_ins as inserts, "
           "n_tup_upd as updates, "
           "n_tup_del as deletes, "
           "n_live_tup as live_tuples, "
           "n_dead_tup as dead_tuples, "
           "pg_size_pretty(pg_total_relation_size(schemaname||'.'||tablename)) as total_size "
           "FROM pg_stat_user_tables "
           "ORDER BY seq_scan DESC "
           "LIMIT 20";
}

QString PostgreSQLCatalog::getInnoDBMetricsQuery() {
    return "SELECT "
           "schemaname, "
           "tablename, "
           "n_tup_ins as inserts, "
           "n_tup_upd as updates, "
           "n_tup_del as deletes, "
           "n_live_tup as live_tuples, "
           "n_dead_tup as dead_tuples, "
           "last_vacuum, "
           "last_autovacuum, "
           "vacuum_count, "
           "autovacuum_count "
           "FROM pg_stat_user_tables "
           "ORDER BY n_live_tup DESC";
}

QString PostgreSQLCatalog::getFulltextIndexQuery() {
    return "SELECT "
           "schemaname, "
           "tablename, "
           "indexname, "
           "pg_get_indexdef(indexrelid) as index_definition "
           "FROM pg_indexes "
           "WHERE indexdef LIKE '%@@%' OR indexdef LIKE '%to_tsvector%'";
}

QStringList PostgreSQLCatalog::getSystemTableList() {
    return QStringList() << "pg_class" << "pg_attribute" << "pg_type" << "pg_proc" << "pg_namespace"
                        << "pg_database" << "pg_index" << "pg_constraint" << "pg_trigger" << "pg_operator"
                        << "pg_opclass" << "pg_am" << "pg_amop" << "pg_amproc" << "pg_language" << "pg_rewrite"
                        << "pg_cast" << "pg_conversion" << "pg_aggregate" << "pg_statistic" << "pg_statistic_ext"
                        << "pg_foreign_table" << "pg_foreign_server" << "pg_foreign_data_wrapper" << "pg_user_mapping"
                        << "pg_enum" << "pg_extension" << "pg_authid" << "pg_auth_members" << "pg_tablespace"
                        << "pg_shdepend" << "pg_shdescription" << "pg_ts_config" << "pg_ts_config_map"
                        << "pg_ts_dict" << "pg_ts_parser" << "pg_ts_template" << "pg_extension" << "pg_available_extensions"
                        << "pg_available_extension_versions" << "pg_config" << "pg_cursors" << "pg_file_settings"
                        << "pg_group" << "pg_hba_file_rules" << "pg_ident_file_mappings" << "pg_indexes"
                        << "pg_locks" << "pg_matviews" << "pg_policies" << "pg_prepared_statements"
                        << "pg_prepared_xacts" << "pg_publication" << "pg_publication_tables" << "pg_replication_origin"
                        << "pg_replication_origin_status" << "pg_replication_slots" << "pg_roles" << "pg_rules"
                        << "pg_seclabel" << "pg_seclabels" << "pg_sequences" << "pg_settings" << "pg_shadow"
                        << "pg_shmem_allocations" << "pg_stats" << "pg_stats_ext" << "pg_subscription"
                        << "pg_subscription_rel" << "pg_tables" << "pg_timezone_abbrevs" << "pg_timezone_names"
                        << "pg_transform" << "pg_trigger" << "pg_user" << "pg_views" << "pg_wait_events"
                        << "pg_wait_events_type";
}

QStringList PostgreSQLCatalog::getSystemViewList() {
    return QStringList() << "pg_stat_activity" << "pg_stat_replication" << "pg_stat_wal_receiver"
                        << "pg_stat_subscription" << "pg_stat_ssl" << "pg_stat_gssapi" << "pg_stat_archiver"
                        << "pg_stat_bgwriter" << "pg_stat_checkpointer" << "pg_stat_database" << "pg_stat_database_conflicts"
                        << "pg_stat_user_functions" << "pg_stat_xact_user_functions" << "pg_stat_user_indexes"
                        << "pg_stat_user_tables" << "pg_stat_xact_user_tables" << "pg_statio_all_indexes"
                        << "pg_statio_all_sequences" << "pg_statio_all_tables" << "pg_statio_user_indexes"
                        << "pg_statio_user_sequences" << "pg_statio_user_tables" << "pg_stat_progress_analyze"
                        << "pg_stat_progress_basebackup" << "pg_stat_progress_cluster" << "pg_stat_progress_copy"
                        << "pg_stat_progress_create_index" << "pg_stat_progress_vacuum";
}

QStringList PostgreSQLCatalog::getInformationSchemaViewList() {
    return QStringList() << "information_schema_catalog_name" << "applicable_roles" << "administrable_role_authorizations"
                        << "attributes" << "character_sets" << "check_constraint_routine_usage" << "check_constraints"
                        << "collations" << "collation_character_set_applicability" << "column_domain_usage"
                        << "column_privileges" << "column_udt_usage" << "columns" << "constraint_column_usage"
                        << "constraint_table_usage" << "data_type_privileges" << "domain_constraints"
                        << "domain_udt_usage" << "domains" << "element_types" << "enabled_roles" << "foreign_data_wrapper_options"
                        << "foreign_data_wrappers" << "foreign_server_options" << "foreign_servers"
                        << "foreign_table_options" << "foreign_tables" << "information_schema_catalog_name"
                        << "key_column_usage" << "parameters" << "referential_constraints" << "role_column_grants"
                        << "role_routine_grants" << "role_table_grants" << "role_udt_grants" << "role_usage_grants"
                        << "routine_privileges" << "routines" << "schemata" << "sequence_privileges" << "sequences"
                        << "sql_features" << "sql_implementation_info" << "sql_languages" << "sql_packages"
                        << "sql_parts" << "sql_sizing" << "sql_sizing_profiles" << "table_constraints"
                        << "table_privileges" << "tables" << "transforms" << "triggered_update_columns"
                        << "triggers" << "udt_privileges" << "usage_privileges" << "user_defined_types"
                        << "user_mapping_options" << "user_mappings" << "view_column_usage" << "view_routine_usage"
                        << "view_table_usage" << "views";
}

QStringList PostgreSQLCatalog::getPerformanceSchemaViewList() {
    // PostgreSQL doesn't have a performance schema like MySQL, but we can provide similar functionality
    return QStringList() << "pg_stat_activity" << "pg_stat_replication" << "pg_stat_wal_receiver"
                        << "pg_stat_subscription" << "pg_stat_ssl" << "pg_stat_gssapi" << "pg_stat_archiver"
                        << "pg_stat_bgwriter" << "pg_stat_checkpointer" << "pg_stat_database" << "pg_stat_database_conflicts"
                        << "pg_stat_user_functions" << "pg_stat_xact_user_functions" << "pg_stat_user_indexes"
                        << "pg_stat_user_tables" << "pg_stat_xact_user_tables" << "pg_statio_all_indexes"
                        << "pg_statio_all_sequences" << "pg_statio_all_tables" << "pg_statio_user_indexes"
                        << "pg_statio_user_sequences" << "pg_statio_user_indexes" << "pg_statio_user_sequences"
                        << "pg_statio_user_tables" << "pg_stat_progress_analyze" << "pg_stat_progress_basebackup"
                        << "pg_stat_progress_cluster" << "pg_stat_progress_copy" << "pg_stat_progress_create_index"
                        << "pg_stat_progress_vacuum";
}

QStringList PostgreSQLCatalog::getMySQLSchemaTableList() {
    // PostgreSQL equivalent of MySQL schema tables
    return QStringList() << "pg_user" << "pg_database" << "pg_namespace" << "pg_class" << "pg_attribute"
                        << "pg_type" << "pg_proc" << "pg_index" << "pg_constraint" << "pg_trigger"
                        << "pg_operator" << "pg_opclass" << "pg_am" << "pg_amop" << "pg_amproc"
                        << "pg_language" << "pg_rewrite" << "pg_cast" << "pg_conversion" << "pg_aggregate"
                        << "pg_statistic" << "pg_statistic_ext" << "pg_foreign_table" << "pg_foreign_server"
                        << "pg_foreign_data_wrapper" << "pg_user_mapping" << "pg_enum" << "pg_extension"
                        << "pg_authid" << "pg_auth_members" << "pg_tablespace" << "pg_shdepend"
                        << "pg_shdescription" << "pg_ts_config" << "pg_ts_config_map" << "pg_ts_dict"
                        << "pg_ts_parser" << "pg_ts_template";
}

QStringList PostgreSQLCatalog::getSysSchemaViewList() {
    // PostgreSQL doesn't have a sys schema like MySQL, but we can provide similar views
    return QStringList() << "pg_stat_activity" << "pg_stat_replication" << "pg_stat_wal_receiver"
                        << "pg_stat_subscription" << "pg_stat_ssl" << "pg_stat_gssapi" << "pg_stat_archiver"
                        << "pg_stat_bgwriter" << "pg_stat_checkpointer" << "pg_stat_database" << "pg_stat_database_conflicts"
                        << "pg_stat_user_functions" << "pg_stat_xact_user_functions" << "pg_stat_user_indexes"
                        << "pg_stat_user_tables" << "pg_stat_xact_user_tables" << "pg_statio_all_indexes"
                        << "pg_statio_all_sequences" << "pg_statio_all_tables" << "pg_statio_user_indexes"
                        << "pg_statio_user_sequences" << "pg_statio_user_tables" << "pg_stat_progress_analyze"
                        << "pg_stat_progress_basebackup" << "pg_stat_progress_cluster" << "pg_stat_progress_copy"
                        << "pg_stat_progress_create_index" << "pg_stat_progress_vacuum";
}

QString PostgreSQLCatalog::formatObjectName(const QString& schema, const QString& object, const QString& database) {
    QString result;
    if (!database.isEmpty()) {
        result = database + ".";
    }
    if (!schema.isEmpty()) {
        result += "\"" + schema + "\".";
    }
    result += "\"" + object + "\"";
    return result;
}

QString PostgreSQLCatalog::escapeIdentifier(const QString& identifier) {
    return "\"" + identifier + "\"";
}

QString PostgreSQLCatalog::getVersionSpecificQuery(const QString& baseQuery, int majorVersion, int minorVersion) {
    // For now, return the base query. In the future, this could modify queries
    // based on version-specific features
    return baseQuery;
}

QString PostgreSQLCatalog::getVersionInfoQuery() {
    return "SELECT version() as version_string, current_setting('server_version') as server_version, "
           "current_setting('server_version_num') as server_version_num";
}

QString PostgreSQLCatalog::getFeatureSupportQuery(const QString& feature) {
    QString query = "SELECT version() as version, "
                   "CASE WHEN current_setting('server_version_num')::int >= %1 THEN 'SUPPORTED' ELSE 'NOT_SUPPORTED' END as support_status";

    // Define minimum versions for features
    QMap<QString, QString> featureVersions;
    featureVersions["JSON"] = "90200";           // PostgreSQL 9.2
    featureVersions["CTE"] = "80400";            // PostgreSQL 8.4
    featureVersions["WINDOW_FUNCTIONS"] = "80400"; // PostgreSQL 8.4
    featureVersions["ARRAYS"] = "80000";         // PostgreSQL 8.0
    featureVersions["GEOMETRY"] = "80000";       // PostgreSQL 8.0
    featureVersions["TEXT_SEARCH"] = "80300";    // PostgreSQL 8.3
    featureVersions["RANGES"] = "90200";         // PostgreSQL 9.2
    featureVersions["UUID"] = "80300";           // PostgreSQL 8.3
    featureVersions["HSTORE"] = "80400";         // PostgreSQL 8.4 (contrib)
    featureVersions["INHERITANCE"] = "70400";    // PostgreSQL 7.4
    featureVersions["PARTITIONING"] = "100000";  // PostgreSQL 10.0 (declarative)

    return query.arg(featureVersions.value(feature, "80000"));
}

QString PostgreSQLCatalog::getEngineSupportQuery() {
    return "SELECT name, default_version, installed_version, comment FROM pg_available_extensions ORDER BY name";
}

QString PostgreSQLCatalog::getPluginSupportQuery() {
    return "SELECT name, default_version, installed_version, comment FROM pg_available_extensions WHERE installed_version IS NOT NULL ORDER BY name";
}

} // namespace scratchrobin
