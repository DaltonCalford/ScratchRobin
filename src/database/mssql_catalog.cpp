#include "mssql_catalog.h"
#include <QRegularExpression>

namespace scratchrobin {

// MSSQL System Catalog Implementation

QString MSSQLCatalog::getSystemDatabasesQuery() {
    return "SELECT name, database_id, create_date, compatibility_level, "
           "collation_name, user_access_desc, state_desc, recovery_model_desc "
           "FROM sys.databases "
           "ORDER BY name";
}

QString MSSQLCatalog::getUserDatabasesQuery() {
    return "SELECT name, database_id, create_date, compatibility_level, "
           "collation_name, user_access_desc, state_desc, recovery_model_desc "
           "FROM sys.databases "
           "WHERE database_id > 4 "
           "ORDER BY name";
}

QString MSSQLCatalog::getDatabaseInfoQuery(const QString& database) {
    QString query = "SELECT "
                   "DB_NAME() as database_name, "
                   "@@VERSION as version_info, "
                   "SERVERPROPERTY('Edition') as edition, "
                   "SERVERPROPERTY('ProductVersion') as product_version, "
                   "SERVERPROPERTY('ProductLevel') as product_level, "
                   "SERVERPROPERTY('MachineName') as machine_name, "
                   "SERVERPROPERTY('ServerName') as server_name, "
                   "DATABASEPROPERTYEX('%1', 'Collation') as collation, "
                   "DATABASEPROPERTYEX('%1', 'Recovery') as recovery_model, "
                   "DATABASEPROPERTYEX('%1', 'Status') as status";
    return database.isEmpty() ? query.arg("DB_NAME()") : query.arg(database);
}

QString MSSQLCatalog::getSchemasQuery() {
    return "SELECT s.schema_id, s.name as schema_name, s.principal_id, "
           "p.name as owner_name, s.create_date, s.modify_date "
           "FROM sys.schemas s "
           "LEFT JOIN sys.database_principals p ON s.principal_id = p.principal_id "
           "ORDER BY s.name";
}

QString MSSQLCatalog::getTablesQuery(const QString& schema) {
    QString query = "SELECT "
                   "t.object_id, "
                   "s.name as schema_name, "
                   "t.name as table_name, "
                   "t.type_desc, "
                   "t.create_date, "
                   "t.modify_date, "
                   "t.is_ms_shipped, "
                   "t.is_published, "
                   "t.is_schema_published, "
                   "p.rows, "
                   "SUM(a.total_pages) * 8 / 1024 as total_space_mb "
                   "FROM sys.tables t "
                   "INNER JOIN sys.schemas s ON t.schema_id = s.schema_id "
                   "INNER JOIN sys.partitions p ON t.object_id = p.object_id AND p.index_id IN (0,1) "
                   "LEFT JOIN sys.allocation_units a ON p.partition_id = a.container_id "
                   "WHERE t.type = 'U' ";

    if (!schema.isEmpty()) {
        query += QString("AND s.name = '%1' ").arg(schema);
    }

    query += "GROUP BY t.object_id, s.name, t.name, t.type_desc, "
             "t.create_date, t.modify_date, t.is_ms_shipped, "
             "t.is_published, t.is_schema_published, p.rows "
             "ORDER BY s.name, t.name";

    return query;
}

QString MSSQLCatalog::getViewsQuery(const QString& schema) {
    QString query = "SELECT "
                   "v.object_id, "
                   "s.name as schema_name, "
                   "v.name as view_name, "
                   "v.create_date, "
                   "v.modify_date, "
                   "v.is_ms_shipped, "
                   "v.is_date_correlation_view, "
                   "OBJECT_DEFINITION(v.object_id) as definition, "
                   "CASE WHEN v.is_ms_shipped = 1 THEN 'System' "
                   "     WHEN OBJECTPROPERTY(v.object_id, 'IsIndexed') = 1 THEN 'Indexed' "
                   "     ELSE 'User' END as view_type "
                   "FROM sys.views v "
                   "INNER JOIN sys.schemas s ON v.schema_id = s.schema_id ";

    if (!schema.isEmpty()) {
        query += QString("WHERE s.name = '%1' ").arg(schema);
    }

    query += "ORDER BY s.name, v.name";
    return query;
}

QString MSSQLCatalog::getColumnsQuery(const QString& table, const QString& schema) {
    QString query = "SELECT "
                   "c.object_id, "
                   "OBJECT_NAME(c.object_id) as table_name, "
                   "s.name as schema_name, "
                   "c.name as column_name, "
                   "c.column_id, "
                   "TYPE_NAME(c.system_type_id) as data_type, "
                   "c.max_length, "
                   "c.precision, "
                   "c.scale, "
                   "c.is_nullable, "
                   "c.is_identity, "
                   "c.is_computed, "
                   "c.is_xml_document, "
                   "c.xml_collection_id, "
                   "c.default_object_id, "
                   "OBJECT_DEFINITION(c.default_object_id) as default_definition, "
                   "c.collation_name, "
                   "CASE WHEN pk.column_id IS NOT NULL THEN 'YES' ELSE 'NO' END as is_primary_key, "
                   "CASE WHEN fk.parent_column_id IS NOT NULL THEN 'YES' ELSE 'NO' END as is_foreign_key "
                   "FROM sys.columns c "
                   "INNER JOIN sys.objects o ON c.object_id = o.object_id "
                   "INNER JOIN sys.schemas s ON o.schema_id = s.schema_id "
                   "LEFT JOIN (SELECT ic.object_id, ic.column_id FROM sys.index_columns ic "
                   "           INNER JOIN sys.indexes i ON ic.object_id = i.object_id AND ic.index_id = i.index_id "
                   "           WHERE i.is_primary_key = 1) pk ON c.object_id = pk.object_id AND c.column_id = pk.column_id "
                   "LEFT JOIN sys.foreign_key_columns fk ON c.object_id = fk.parent_object_id AND c.column_id = fk.parent_column_id "
                   "WHERE o.type IN ('U', 'V') "; // Tables and Views

    if (!table.isEmpty()) {
        query += QString("AND o.name = '%1' ").arg(table);
    }
    if (!schema.isEmpty()) {
        query += QString("AND s.name = '%1' ").arg(schema);
    }

    query += "ORDER BY c.object_id, c.column_id";
    return query;
}

QString MSSQLCatalog::getIndexesQuery(const QString& table, const QString& schema) {
    QString query = "SELECT "
                   "i.object_id, "
                   "i.index_id, "
                   "i.name as index_name, "
                   "OBJECT_NAME(i.object_id) as table_name, "
                   "s.name as schema_name, "
                   "i.type_desc, "
                   "i.is_unique, "
                   "i.is_primary_key, "
                   "i.is_unique_constraint, "
                   "i.fill_factor, "
                   "i.is_padded, "
                   "i.is_disabled, "
                   "i.ignore_dup_key, "
                   "STUFF((SELECT ', ' + c.name "
                   "       FROM sys.index_columns ic "
                   "       INNER JOIN sys.columns c ON ic.object_id = c.object_id AND ic.column_id = c.column_id "
                   "       WHERE ic.object_id = i.object_id AND ic.index_id = i.index_id "
                   "       ORDER BY ic.key_ordinal "
                   "       FOR XML PATH('')), 1, 2, '') as index_columns, "
                   "STUFF((SELECT ', ' + c.name "
                   "       FROM sys.index_columns ic "
                   "       INNER JOIN sys.columns c ON ic.object_id = c.object_id AND ic.column_id = c.column_id "
                   "       WHERE ic.object_id = i.object_id AND ic.index_id = i.index_id AND ic.is_included_column = 1 "
                   "       ORDER BY ic.key_ordinal "
                   "       FOR XML PATH('')), 1, 2, '') as included_columns "
                   "FROM sys.indexes i "
                   "INNER JOIN sys.objects o ON i.object_id = o.object_id "
                   "INNER JOIN sys.schemas s ON o.schema_id = s.schema_id "
                   "WHERE i.type > 0 "; // Exclude heaps

    if (!table.isEmpty()) {
        query += QString("AND o.name = '%1' ").arg(table);
    }
    if (!schema.isEmpty()) {
        query += QString("AND s.name = '%1' ").arg(schema);
    }

    query += "ORDER BY o.name, i.name";
    return query;
}

QString MSSQLCatalog::getConstraintsQuery(const QString& table, const QString& schema) {
    QString query = "SELECT "
                   "c.object_id, "
                   "c.name as constraint_name, "
                   "OBJECT_NAME(c.parent_object_id) as table_name, "
                   "s.name as schema_name, "
                   "c.type_desc, "
                   "c.definition, "
                   "c.create_date, "
                   "c.modify_date, "
                   "c.is_ms_shipped, "
                   "c.is_published, "
                   "c.is_schema_published "
                   "FROM sys.check_constraints c "
                   "INNER JOIN sys.objects o ON c.parent_object_id = o.object_id "
                   "INNER JOIN sys.schemas s ON o.schema_id = s.schema_id ";

    if (!table.isEmpty()) {
        query += QString("WHERE o.name = '%1' ").arg(table);
        if (!schema.isEmpty()) {
            query += QString("AND s.name = '%1' ").arg(schema);
        }
    } else if (!schema.isEmpty()) {
        query += QString("WHERE s.name = '%1' ").arg(schema);
    }

    query += "UNION ALL "
             "SELECT "
             "d.object_id, "
             "d.name as constraint_name, "
             "OBJECT_NAME(d.parent_object_id) as table_name, "
             "s.name as schema_name, "
             "d.type_desc, "
             "NULL as definition, "
             "d.create_date, "
             "d.modify_date, "
             "d.is_ms_shipped, "
             "d.is_published, "
             "d.is_schema_published "
             "FROM sys.default_constraints d "
             "INNER JOIN sys.objects o ON d.parent_object_id = o.object_id "
             "INNER JOIN sys.schemas s ON o.schema_id = s.schema_id ";

    if (!table.isEmpty()) {
        query += QString("WHERE o.name = '%1' ").arg(table);
        if (!schema.isEmpty()) {
            query += QString("AND s.name = '%1' ").arg(schema);
        }
    } else if (!schema.isEmpty()) {
        query += QString("WHERE s.name = '%1' ").arg(schema);
    }

    query += "ORDER BY table_name, constraint_name";
    return query;
}

QString MSSQLCatalog::getForeignKeysQuery(const QString& table, const QString& schema) {
    QString query = "SELECT "
                   "fk.object_id, "
                   "fk.name as fk_name, "
                   "OBJECT_NAME(fk.parent_object_id) as parent_table, "
                   "s1.name as parent_schema, "
                   "OBJECT_NAME(fk.referenced_object_id) as referenced_table, "
                   "s2.name as referenced_schema, "
                   "fk.delete_referential_action_desc, "
                   "fk.update_referential_action_desc, "
                   "fk.is_ms_shipped, "
                   "fk.is_published, "
                   "fk.is_schema_published, "
                   "STUFF((SELECT ', ' + COL_NAME(fkc.parent_object_id, fkc.parent_column_id) "
                   "       FROM sys.foreign_key_columns fkc "
                   "       WHERE fkc.constraint_object_id = fk.object_id "
                   "       ORDER BY fkc.constraint_column_id "
                   "       FOR XML PATH('')), 1, 2, '') as parent_columns, "
                   "STUFF((SELECT ', ' + COL_NAME(fkc.referenced_object_id, fkc.referenced_column_id) "
                   "       FROM sys.foreign_key_columns fkc "
                   "       WHERE fkc.constraint_object_id = fk.object_id "
                   "       ORDER BY fkc.constraint_column_id "
                   "       FOR XML PATH('')), 1, 2, '') as referenced_columns "
                   "FROM sys.foreign_keys fk "
                   "INNER JOIN sys.objects o1 ON fk.parent_object_id = o1.object_id "
                   "INNER JOIN sys.schemas s1 ON o1.schema_id = s1.schema_id "
                   "INNER JOIN sys.objects o2 ON fk.referenced_object_id = o2.object_id "
                   "INNER JOIN sys.schemas s2 ON o2.schema_id = s2.schema_id ";

    if (!table.isEmpty()) {
        query += QString("WHERE o1.name = '%1' ").arg(table);
        if (!schema.isEmpty()) {
            query += QString("AND s1.name = '%1' ").arg(schema);
        }
    } else if (!schema.isEmpty()) {
        query += QString("WHERE s1.name = '%1' ").arg(schema);
    }

    query += "ORDER BY parent_table, fk_name";
    return query;
}

QString MSSQLCatalog::getStoredProceduresQuery(const QString& schema) {
    QString query = "SELECT "
                   "p.object_id, "
                   "p.name as procedure_name, "
                   "s.name as schema_name, "
                   "p.create_date, "
                   "p.modify_date, "
                   "p.is_ms_shipped, "
                   "OBJECT_DEFINITION(p.object_id) as definition, "
                   "CASE WHEN p.type = 'P' THEN 'Stored Procedure' "
                   "     WHEN p.type = 'PC' THEN 'CLR Stored Procedure' "
                   "     ELSE 'Other' END as procedure_type "
                   "FROM sys.procedures p "
                   "INNER JOIN sys.schemas s ON p.schema_id = s.schema_id ";

    if (!schema.isEmpty()) {
        query += QString("WHERE s.name = '%1' ").arg(schema);
    }

    query += "ORDER BY s.name, p.name";
    return query;
}

QString MSSQLCatalog::getFunctionsQuery(const QString& schema) {
    QString query = "SELECT "
                   "f.object_id, "
                   "f.name as function_name, "
                   "s.name as schema_name, "
                   "f.create_date, "
                   "f.modify_date, "
                   "f.is_ms_shipped, "
                   "OBJECT_DEFINITION(f.object_id) as definition, "
                   "CASE WHEN f.type = 'FN' THEN 'Scalar Function' "
                   "     WHEN f.type = 'IF' THEN 'Inline Table Function' "
                   "     WHEN f.type = 'TF' THEN 'Table Function' "
                   "     WHEN f.type = 'FS' THEN 'CLR Scalar Function' "
                   "     WHEN f.type = 'FT' THEN 'CLR Table Function' "
                   "     ELSE 'Other' END as function_type "
                   "FROM sys.objects f "
                   "INNER JOIN sys.schemas s ON f.schema_id = s.schema_id "
                   "WHERE f.type IN ('FN', 'IF', 'TF', 'FS', 'FT') ";

    if (!schema.isEmpty()) {
        query += QString("AND s.name = '%1' ").arg(schema);
    }

    query += "ORDER BY s.name, f.name";
    return query;
}

QString MSSQLCatalog::getTriggersQuery(const QString& table, const QString& schema) {
    QString query = "SELECT "
                   "t.object_id, "
                   "t.name as trigger_name, "
                   "OBJECT_NAME(t.parent_id) as table_name, "
                   "s.name as schema_name, "
                   "t.create_date, "
                   "t.modify_date, "
                   "t.is_ms_shipped, "
                   "OBJECT_DEFINITION(t.object_id) as definition, "
                   "CASE WHEN t.is_instead_of_trigger = 1 THEN 'INSTEAD OF' ELSE 'AFTER' END as trigger_type, "
                   "STUFF((SELECT ', ' + type_desc FROM sys.trigger_events te WHERE te.object_id = t.object_id FOR XML PATH('')), 1, 2, '') as trigger_events "
                   "FROM sys.triggers t "
                   "INNER JOIN sys.objects o ON t.parent_id = o.object_id "
                   "INNER JOIN sys.schemas s ON o.schema_id = s.schema_id ";

    if (!table.isEmpty()) {
        query += QString("WHERE o.name = '%1' ").arg(table);
        if (!schema.isEmpty()) {
            query += QString("AND s.name = '%1' ").arg(schema);
        }
    } else if (!schema.isEmpty()) {
        query += QString("WHERE s.name = '%1' ").arg(schema);
    }

    query += "ORDER BY o.name, t.name";
    return query;
}

QString MSSQLCatalog::getSequencesQuery(const QString& schema) {
    QString query = "SELECT "
                   "s.object_id, "
                   "s.name as sequence_name, "
                   "SCHEMA_NAME(s.schema_id) as schema_name, "
                   "s.create_date, "
                   "s.modify_date, "
                   "TYPE_NAME(s.system_type_id) as data_type, "
                   "s.start_value, "
                   "s.increment, "
                   "s.minimum_value, "
                   "s.maximum_value, "
                   "s.is_cycling, "
                   "s.is_exhausted, "
                   "s.cache_size "
                   "FROM sys.sequences s ";

    if (!schema.isEmpty()) {
        query += QString("WHERE SCHEMA_NAME(s.schema_id) = '%1' ").arg(schema);
    }

    query += "ORDER BY s.schema_id, s.name";
    return query;
}

QString MSSQLCatalog::getServerInfoQuery() {
    return "SELECT "
           "@@VERSION as version_info, "
           "@@SERVERNAME as server_name, "
           "SERVERPROPERTY('MachineName') as machine_name, "
           "SERVERPROPERTY('InstanceName') as instance_name, "
           "SERVERPROPERTY('Edition') as edition, "
           "SERVERPROPERTY('ProductVersion') as product_version, "
           "SERVERPROPERTY('ProductLevel') as product_level, "
           "SERVERPROPERTY('ProductUpdateLevel') as product_update_level, "
           "SERVERPROPERTY('Collation') as server_collation, "
           "@@LANGUAGE as language, "
           "@@MAX_CONNECTIONS as max_connections, "
           "@@LOCK_TIMEOUT as lock_timeout, "
           "SERVERPROPERTY('IsIntegratedSecurityOnly') as integrated_security_only";
}

QString MSSQLCatalog::getDatabasePropertiesQuery() {
    return "SELECT "
           "name as database_name, "
           "database_id, "
           "create_date, "
           "compatibility_level, "
           "collation_name, "
           "user_access_desc, "
           "state_desc, "
           "recovery_model_desc, "
           "page_verify_option_desc, "
           "is_auto_create_stats_on, "
           "is_auto_update_stats_on, "
           "is_auto_update_stats_async_on, "
           "is_ansi_null_default_on, "
           "is_ansi_nulls_on, "
           "is_ansi_padding_on, "
           "is_ansi_warnings_on, "
           "is_arithabort_on, "
           "is_concat_null_yields_null_on, "
           "is_numeric_roundabort_on, "
           "is_quoted_identifier_on, "
           "is_recursive_triggers_on, "
           "is_cursor_close_on_commit_on, "
           "is_local_cursor_default, "
           "is_fulltext_enabled, "
           "is_trustworthy_on, "
           "is_db_chaining_on, "
           "is_parameterization_forced, "
           "is_master_key_encrypted_by_server, "
           "is_published, "
           "is_subscribed, "
           "is_merge_published, "
           "is_distributor, "
           "is_sync_with_backup, "
           "service_broker_guid, "
           "snapshot_isolation_state_desc, "
           "is_read_committed_snapshot_on "
           "FROM sys.databases";
}

QString MSSQLCatalog::getSecurityInfoQuery() {
    return "SELECT "
           "p.principal_id, "
           "p.name, "
           "p.type_desc, "
           "p.default_schema_name, "
           "p.create_date, "
           "p.modify_date, "
           "p.owning_principal_id, "
           "p.is_fixed_role, "
           "CASE WHEN p.type = 'S' THEN l.name ELSE NULL END as login_name, "
           "CASE WHEN p.type = 'S' THEN l.is_disabled ELSE NULL END as login_disabled "
           "FROM sys.database_principals p "
           "LEFT JOIN sys.sql_logins l ON p.sid = l.sid "
           "ORDER BY p.type_desc, p.name";
}

QString MSSQLCatalog::getBackupHistoryQuery() {
    return "SELECT "
           "bs.database_name, "
           "bs.backup_start_date, "
           "bs.backup_finish_date, "
           "bs.backup_size / 1024 / 1024 as backup_size_mb, "
           "bs.compressed_backup_size / 1024 / 1024 as compressed_size_mb, "
           "bs.type as backup_type, "
           "bmf.physical_device_name, "
           "bs.description, "
           "bs.is_compressed "
           "FROM msdb.dbo.backupset bs "
           "INNER JOIN msdb.dbo.backupmediafamily bmf ON bs.media_set_id = bmf.media_set_id "
           "ORDER BY bs.backup_start_date DESC";
}

QString MSSQLCatalog::getJobInfoQuery() {
    return "SELECT "
           "j.job_id, "
           "j.name as job_name, "
           "j.description, "
           "j.date_created, "
           "j.date_modified, "
           "j.enabled, "
           "c.name as category_name, "
           "j.version_number, "
           "j.originating_server, "
           "j.start_step_id, "
           "j.owner_sid, "
           "suser_sname(j.owner_sid) as owner_name "
           "FROM msdb.dbo.sysjobs j "
           "INNER JOIN msdb.dbo.syscategories c ON j.category_id = c.category_id "
           "ORDER BY j.name";
}

QString MSSQLCatalog::getActiveConnectionsQuery() {
    return "SELECT "
           "s.session_id, "
           "s.login_name, "
           "s.host_name, "
           "s.program_name, "
           "s.client_interface_name, "
           "s.login_time, "
           "s.last_request_start_time, "
           "s.last_request_end_time, "
           "s.cpu_time, "
           "s.memory_usage, "
           "s.total_scheduled_time, "
           "s.total_elapsed_time, "
           "s.reads, "
           "s.writes, "
           "s.logical_reads, "
           "db.name as database_name, "
           "r.status, "
           "r.command, "
           "r.percent_complete, "
           "r.estimated_completion_time, "
           "r.total_elapsed_time as request_elapsed_time, "
           "r.cpu_time as request_cpu_time, "
           "r.reads as request_reads, "
           "r.writes as request_writes, "
           "r.logical_reads as request_logical_reads, "
           "st.text as sql_text, "
           "qp.query_plan "
           "FROM sys.dm_exec_sessions s "
           "LEFT JOIN sys.dm_exec_requests r ON s.session_id = r.session_id "
           "LEFT JOIN sys.databases db ON s.database_id = db.database_id "
           "OUTER APPLY sys.dm_exec_sql_text(r.sql_handle) st "
           "OUTER APPLY sys.dm_exec_query_plan(r.plan_handle) qp "
           "WHERE s.is_user_process = 1 "
           "ORDER BY s.session_id";
}

QString MSSQLCatalog::getPerformanceCountersQuery() {
    return "SELECT "
           "RTRIM(counter_name) as counter_name, "
           "RTRIM(instance_name) as instance_name, "
           "cntr_value, "
           "RTRIM(object_name) as object_name, "
           "cntr_type "
           "FROM sys.dm_os_performance_counters "
           "ORDER BY object_name, counter_name, instance_name";
}

QString MSSQLCatalog::getQueryStatsQuery() {
    return "SELECT TOP 100 "
           "qs.sql_handle, "
           "qs.execution_count, "
           "qs.total_worker_time, "
           "qs.total_elapsed_time, "
           "qs.total_logical_reads, "
           "qs.total_logical_writes, "
           "qs.total_physical_reads, "
           "qs.creation_time, "
           "qs.last_execution_time, "
           "qs.min_worker_time, "
           "qs.max_worker_time, "
           "qs.min_elapsed_time, "
           "qs.max_elapsed_time, "
           "qs.min_logical_reads, "
           "qs.max_logical_reads, "
           "qs.min_logical_writes, "
           "qs.max_logical_writes, "
           "qs.min_physical_reads, "
           "qs.max_physical_reads, "
           "st.text as sql_text, "
           "qp.query_plan "
           "FROM sys.dm_exec_query_stats qs "
           "CROSS APPLY sys.dm_exec_sql_text(qs.sql_handle) st "
           "CROSS APPLY sys.dm_exec_query_plan(qs.plan_handle) qp "
           "ORDER BY qs.total_worker_time DESC";
}

QString MSSQLCatalog::getIndexUsageQuery() {
    return "SELECT "
           "OBJECT_NAME(i.object_id) as table_name, "
           "i.name as index_name, "
           "i.type_desc as index_type, "
           "ius.user_seeks, "
           "ius.user_scans, "
           "ius.user_lookups, "
           "ius.user_updates, "
           "ius.system_seeks, "
           "ius.system_scans, "
           "ius.system_lookups, "
           "ius.system_updates, "
           "ius.last_user_seek, "
           "ius.last_user_scan, "
           "ius.last_user_lookup, "
           "ius.last_user_update, "
           "ius.last_system_seek, "
           "ius.last_system_scan, "
           "ius.last_system_lookup, "
           "ius.last_system_update "
           "FROM sys.indexes i "
           "LEFT JOIN sys.dm_db_index_usage_stats ius ON i.object_id = ius.object_id AND i.index_id = ius.index_id "
           "WHERE i.type > 0 "
           "ORDER BY OBJECT_NAME(i.object_id), i.name";
}

QString MSSQLCatalog::getMissingIndexesQuery() {
    return "SELECT TOP 50 "
           "dmig.index_handle, "
           "dmig.database_id, "
           "DB_NAME(dmig.database_id) as database_name, "
           "OBJECT_NAME(dmig.object_id, dmig.database_id) as table_name, "
           "dmig.equality_columns, "
           "dmig.inequality_columns, "
           "dmig.included_columns, "
           "dmigs.unique_compiles, "
           "dmigs.user_compiles, "
           "dmigs.avg_total_user_cost, "
           "dmigs.avg_user_impact, "
           "dmigs.last_user_seek, "
           "dmigs.system_seeks, "
           "dmigs.avg_system_impact, "
           "dmigs.last_system_seek "
           "FROM sys.dm_db_missing_index_groups dmg "
           "INNER JOIN sys.dm_db_missing_index_group_stats dmigs ON dmg.index_group_handle = dmigs.group_handle "
           "INNER JOIN sys.dm_db_missing_index_details dmig ON dmg.index_handle = dmig.index_handle "
           "ORDER BY dmigs.avg_user_impact DESC";
}

QStringList MSSQLCatalog::getSystemTableList() {
    return QStringList() << "sysobjects" << "syscolumns" << "systypes" << "sysindexes"
                        << "sysusers" << "sysdatabases" << "syslogins" << "sysprocesses"
                        << "sysfiles" << "sysfilegroups" << "sysmessages" << "sysconfigures";
}

QStringList MSSQLCatalog::getSystemViewList() {
    return QStringList() << "sys.databases" << "sys.tables" << "sys.columns" << "sys.indexes"
                        << "sys.objects" << "sys.schemas" << "sys.types" << "sys.procedures"
                        << "sys.views" << "sys.triggers" << "sys.foreign_keys" << "sys.key_constraints"
                        << "sys.check_constraints" << "sys.default_constraints" << "sys.sequences";
}

QStringList MSSQLCatalog::getInformationSchemaViewList() {
    return QStringList() << "INFORMATION_SCHEMA.TABLES" << "INFORMATION_SCHEMA.COLUMNS"
                        << "INFORMATION_SCHEMA.VIEWS" << "INFORMATION_SCHEMA.ROUTINES"
                        << "INFORMATION_SCHEMA.KEY_COLUMN_USAGE" << "INFORMATION_SCHEMA.TABLE_CONSTRAINTS"
                        << "INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS" << "INFORMATION_SCHEMA.CHECK_CONSTRAINTS";
}

QStringList MSSQLCatalog::getDynamicManagementViewList() {
    return QStringList() << "sys.dm_exec_connections" << "sys.dm_exec_sessions" << "sys.dm_exec_requests"
                        << "sys.dm_exec_query_stats" << "sys.dm_exec_query_plan" << "sys.dm_exec_sql_text"
                        << "sys.dm_os_performance_counters" << "sys.dm_os_wait_stats" << "sys.dm_os_memory_objects"
                        << "sys.dm_db_index_usage_stats" << "sys.dm_db_missing_index_details"
                        << "sys.dm_db_missing_index_groups" << "sys.dm_db_missing_index_group_stats";
}

QString MSSQLCatalog::formatObjectName(const QString& schema, const QString& object) {
    if (schema.isEmpty()) {
        return QString("[%1]").arg(object);
    } else {
        return QString("[%1].[%2]").arg(schema, object);
    }
}

QString MSSQLCatalog::escapeIdentifier(const QString& identifier) {
    return QString("[%1]").arg(identifier);
}

QString MSSQLCatalog::getVersionSpecificQuery(const QString& baseQuery, int majorVersion) {
    // For now, return the base query. In the future, this could modify queries
    // based on version-specific features
    return baseQuery;
}

QString MSSQLCatalog::getVersionInfoQuery() {
    return "SELECT @@VERSION as version_string, "
           "SERVERPROPERTY('ProductVersion') as product_version, "
           "SERVERPROPERTY('ProductLevel') as product_level, "
           "SERVERPROPERTY('ProductUpdateLevel') as product_update_level, "
           "SERVERPROPERTY('Edition') as edition";
}

QString MSSQLCatalog::getFeatureSupportQuery(const QString& feature) {
    QString query = "SELECT SERVERPROPERTY('ProductVersion') as version, "
                   "CASE WHEN SERVERPROPERTY('ProductVersion') >= '%1' THEN 'SUPPORTED' ELSE 'NOT_SUPPORTED' END as support_status";

    // Define minimum versions for features
    QMap<QString, QString> featureVersions;
    featureVersions["JSON"] = "13.0.0.0";     // SQL Server 2016
    featureVersions["STRING_AGG"] = "13.0.0.0"; // SQL Server 2016
    featureVersions["OFFSET_FETCH"] = "11.0.0.0"; // SQL Server 2012
    featureVersions["SEQUENCES"] = "11.0.0.0"; // SQL Server 2012
    featureVersions["SPATIAL"] = "10.0.0.0"; // SQL Server 2008

    return query.arg(featureVersions.value(feature, "9.0.0.0"));
}

} // namespace scratchrobin
