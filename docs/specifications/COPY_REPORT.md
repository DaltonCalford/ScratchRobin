# Specification Copy Report

This report compares ScratchRobin specs to ScratchBird `docs/specifications/` by file path and content hash.

## Summary

- Mirrored from ScratchBird: 1004
- ScratchRobin authored/divergent: 107
- Redirect stubs (excluded from authored list): 21

## Mirrored from ScratchBird

### Alpha Phase 2/

- `Alpha Phase 2/00-Implementation-Roadmap.md`
- `Alpha Phase 2/01-Architecture-Overview.md`
- `Alpha Phase 2/02-Clock-Synchronization-Specification.md`
- `Alpha Phase 2/03-Distributed-MVCC-Specification.md`
- `Alpha Phase 2/04-Replication-Protocol-Specification.md`
- `Alpha Phase 2/05-Wire-Protocol-Integration-Specification.md`
- `Alpha Phase 2/06-Ingestion-Layer.md`
- `Alpha Phase 2/07-OLAP-Tier.md`
- `Alpha Phase 2/08-Deployment-Guide.md`
- `Alpha Phase 2/09-Monitoring-Observability.md`
- `Alpha Phase 2/10-UDR-System-Specification.md`
- `Alpha Phase 2/11-Remote-Database-UDR-Specification.md`
- `Alpha Phase 2/11a-Connection-Pool-Implementation.md`
- `Alpha Phase 2/11b-PostgreSQL-Client-Implementation.md`
- `Alpha Phase 2/11c-MySQL-Client-Implementation.md`
- `Alpha Phase 2/11d-MSSQL-Client-Implementation.md`
- `Alpha Phase 2/11e-Firebird-Client-Implementation.md`
- `Alpha Phase 2/11f-ODBC-Client-Implementation.md`
- `Alpha Phase 2/11g-JDBC-Client-Implementation.md`
- `Alpha Phase 2/11h-Live-Migration-Emulated-Listener.md`
- `Alpha Phase 2/11i-ScratchBird-Client-Implementation.md`
- `Alpha Phase 2/Discussion_Notes.md`
- `Alpha Phase 2/README.md`

### Cluster Specification Work/

- `Cluster Specification Work/README.md`
- `Cluster Specification Work/SBCLUSTER-00-GUIDING-PRINCIPLES.md`
- `Cluster Specification Work/SBCLUSTER-01-CLUSTER-CONFIG-EPOCH.md`
- `Cluster Specification Work/SBCLUSTER-02-MEMBERSHIP-AND-IDENTITY.md`
- `Cluster Specification Work/SBCLUSTER-03-CA-POLICY.md`
- `Cluster Specification Work/SBCLUSTER-04-SECURITY-BUNDLE.md`
- `Cluster Specification Work/SBCLUSTER-05-SHARDING.md`
- `Cluster Specification Work/SBCLUSTER-06-DISTRIBUTED-QUERY.md`
- `Cluster Specification Work/SBCLUSTER-07-REPLICATION.md`
- `Cluster Specification Work/SBCLUSTER-08-BACKUP-AND-RESTORE.md`
- `Cluster Specification Work/SBCLUSTER-09-SCHEDULER.md`
- `Cluster Specification Work/SBCLUSTER-10-OBSERVABILITY.md`
- `Cluster Specification Work/SBCLUSTER-11-SHARD-MIGRATION-AND-REBALANCING.md`
- `Cluster Specification Work/SBCLUSTER-12-AUTOSCALING_AND_ELASTIC_LIFECYCLE.md`
- `Cluster Specification Work/SBCLUSTER-AI-HANDOFF.md`
- `Cluster Specification Work/SBCLUSTER-IMPLEMENTATION-BOUNDARY.md`
- `Cluster Specification Work/SBCLUSTER-NORMATIVE-LANGUAGE.md`
- `Cluster Specification Work/SBCLUSTER-SUMMARY.md`
- `Cluster Specification Work/SBCLUSTER-THREAT-MODEL.md`
- `Cluster Specification Work/sbsec_handoff_summary.md`
- `Cluster Specification Work/scratch_bird_cluster_architecture_security_specifications_draft.md`

### Security Design Specification/

- `Security Design Specification/00_SECURITY_SPEC_INDEX.md`
- `Security Design Specification/01_SECURITY_ARCHITECTURE.md`
- `Security Design Specification/02_IDENTITY_AUTHENTICATION.md`
- `Security Design Specification/03_AUTHORIZATION_MODEL.md`
- `Security Design Specification/04.01_KEY_LIFECYCLE_STATE_MACHINES.md`
- `Security Design Specification/04.02_KEY_MATERIAL_HANDLING_REQUIREMENTS.md`
- `Security Design Specification/04.03_NONCE_IV_SPECIFICATION.md`
- `Security Design Specification/04_ENCRYPTION_KEY_MANAGEMENT.md`
- `Security Design Specification/05.A_IPC_WIRE_FORMAT_AND_EXAMPLES.md`
- `Security Design Specification/05_IPC_SECURITY.md`
- `Security Design Specification/06.01_QUORUM_PROPOSAL_CANONICALIZATION_HASHING.md`
- `Security Design Specification/06.02_QUORUM_EVIDENCE_AND_AUDIT_COUPLING.md`
- `Security Design Specification/06_CLUSTER_SECURITY.md`
- `Security Design Specification/07_NETWORK_PRESENCE_BINDING.md`
- `Security Design Specification/08.01_AUDIT_EVENT_CANONICALIZATION.md`
- `Security Design Specification/08.02_AUDIT_CHAIN_VERIFICATION_CHECKPOINTS.md`
- `Security Design Specification/08_AUDIT_COMPLIANCE.md`
- `Security Design Specification/09_SECURITY_LEVELS.md`
- `Security Design Specification/10_RELEASE_INTEGRITY_PROVENANCE.md`
- `Security Design Specification/AUTH_CERTIFICATE_TLS.md`
- `Security Design Specification/AUTH_CORE_FRAMEWORK.md`
- `Security Design Specification/AUTH_ENTERPRISE_LDAP_KERBEROS.md`
- `Security Design Specification/AUTH_MODERN_OAUTH_MFA.md`
- `Security Design Specification/AUTH_PASSWORD_METHODS.md`
- `Security Design Specification/EXTERNAL_AUTHENTICATION_DESIGN.md`
- `Security Design Specification/README.md`
- `Security Design Specification/ROLE_COMPOSITION_AND_HIERARCHIES.md`
- `Security Design Specification/archive security work/00_SECURITY_SPEC_INDEX.md`
- `Security Design Specification/archive security work/01_SECURITY_ARCHITECTURE.md`
- `Security Design Specification/archive security work/02_IDENTITY_AUTHENTICATION.md`
- `Security Design Specification/archive security work/03_AUTHORIZATION_MODEL.md`
- `Security Design Specification/archive security work/04_ENCRYPTION_KEY_MANAGEMENT.md`
- `Security Design Specification/archive security work/05_IPC_SECURITY.md`
- `Security Design Specification/archive security work/06_CLUSTER_SECURITY.md`
- `Security Design Specification/archive security work/07_NETWORK_PRESENCE_BINDING.md`
- `Security Design Specification/archive security work/08_AUDIT_COMPLIANCE.md`
- `Security Design Specification/archive security work/09_SECURITY_LEVELS.md`
- `Security Design Specification/archive security work/Beta Task -Distributed Secret Sharing Implementation Specification.md`
- `Security Design Specification/archive security work/Engine Internal Security.md`
- `Security Design Specification/archive security work/SECURITY_IMPLIMENTATION_DETAILS.md`
- `Security Design Specification/archive security work/SECURITY_SYSTEM_SPECIFICATION.md`
- `Security Design Specification/archive security work/Security Hardening Guide.md`
- `Security Design Specification/archive security work/draft_security_architecture_specification.md`
- `Security Design Specification/contributor_security_rules.md`
- `Security Design Specification/sbsec_alpha_base.md`
- `Security Design Specification/supportability_contract.md`

### admin/

- `admin/README.md`
- `admin/SB_ADMIN_CLI_SPECIFICATION.md`
- `admin/SB_SERVER_NETWORK_CLI_SPECIFICATION.md`

### alpha_requirements/

- `alpha_requirements/optional/OBJECT_NAME_REGISTRY_MATERIALIZED_VIEW.md`
- `alpha_requirements/optional/README.md`

### api/

- `api/CONNECTION_POOLING_SPECIFICATION.md`
- `api/README.md`

### archive/

- `archive/AnalysisOfBestParsingStructures.md`
- `archive/DraftQueryOptimizationSpecification.md`
- `archive/README.md`
- `archive/Specification for a Multi-Generational Database Architecture.md`

### beta_requirements/

- `beta_requirements/00_DRIVERS_AND_INTEGRATIONS_INDEX.md`
- `beta_requirements/COMPLETION_STATUS.md`
- `beta_requirements/DIRECTORY_STRUCTURE_CREATED.md`
- `beta_requirements/README.md`
- `beta_requirements/ai-ml/README.md`
- `beta_requirements/applications/README.md`
- `beta_requirements/big-data-streaming/README.md`
- `beta_requirements/big-data-streaming/apache-kafka/README.md`
- `beta_requirements/big-data-streaming/apache-kafka/SPECIFICATION.md`
- `beta_requirements/builds/00_BUILD_REQUIREMENTS_INDEX.md`
- `beta_requirements/builds/01_LINUX_NATIVE.md`
- `beta_requirements/builds/02_WINDOWS_NATIVE.md`
- `beta_requirements/builds/03_MACOS_NATIVE.md`
- `beta_requirements/builds/10_LINUX_TO_WINDOWS.md`
- `beta_requirements/builds/11_LINUX_TO_MACOS.md`
- `beta_requirements/builds/12_WINDOWS_TO_LINUX.md`
- `beta_requirements/builds/20_APPIMAGE.md`
- `beta_requirements/builds/23_DEB.md`
- `beta_requirements/builds/24_RPM.md`
- `beta_requirements/builds/27_BREW.md`
- `beta_requirements/builds/30_DOCKER.md`
- `beta_requirements/builds/40_GITHUB_ACTIONS.md`
- `beta_requirements/builds/COMPLETE_BUILD_ENVIRONMENT_SETUP.md`
- `beta_requirements/cloud-container/README.md`
- `beta_requirements/cloud-container/docker/README.md`
- `beta_requirements/cloud-container/docker/SPECIFICATION.md`
- `beta_requirements/connectivity/README.md`
- `beta_requirements/connectivity/odbc/README.md`
- `beta_requirements/connectivity/odbc/SPECIFICATION.md`
- `beta_requirements/drivers/DRIVER_BASELINE_SPEC.md`
- `beta_requirements/drivers/README.md`
- `beta_requirements/drivers/cpp/API_REFERENCE.md`
- `beta_requirements/drivers/cpp/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/cpp/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/cpp/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/cpp/SPECIFICATION.md`
- `beta_requirements/drivers/cpp/TESTING_CRITERIA.md`
- `beta_requirements/drivers/dotnet-csharp/API_REFERENCE.md`
- `beta_requirements/drivers/dotnet-csharp/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/dotnet-csharp/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/dotnet-csharp/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/dotnet-csharp/README.md`
- `beta_requirements/drivers/dotnet-csharp/SPECIFICATION.md`
- `beta_requirements/drivers/dotnet-csharp/TESTING_CRITERIA.md`
- `beta_requirements/drivers/golang/API_REFERENCE.md`
- `beta_requirements/drivers/golang/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/golang/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/golang/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/golang/README.md`
- `beta_requirements/drivers/golang/SPECIFICATION.md`
- `beta_requirements/drivers/golang/TESTING_CRITERIA.md`
- `beta_requirements/drivers/java-jdbc/API_REFERENCE.md`
- `beta_requirements/drivers/java-jdbc/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/java-jdbc/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/java-jdbc/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/java-jdbc/README.md`
- `beta_requirements/drivers/java-jdbc/SPECIFICATION.md`
- `beta_requirements/drivers/java-jdbc/TESTING_CRITERIA.md`
- `beta_requirements/drivers/nodejs-typescript/API_REFERENCE.md`
- `beta_requirements/drivers/nodejs-typescript/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/nodejs-typescript/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/nodejs-typescript/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/nodejs-typescript/README.md`
- `beta_requirements/drivers/nodejs-typescript/SPECIFICATION.md`
- `beta_requirements/drivers/nodejs-typescript/TESTING_CRITERIA.md`
- `beta_requirements/drivers/pascal-delphi/API_REFERENCE.md`
- `beta_requirements/drivers/pascal-delphi/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/pascal-delphi/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/pascal-delphi/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/pascal-delphi/README.md`
- `beta_requirements/drivers/pascal-delphi/SPECIFICATION.md`
- `beta_requirements/drivers/pascal-delphi/TESTING_CRITERIA.md`
- `beta_requirements/drivers/php/API_REFERENCE.md`
- `beta_requirements/drivers/php/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/php/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/php/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/php/README.md`
- `beta_requirements/drivers/php/SPECIFICATION.md`
- `beta_requirements/drivers/php/TESTING_CRITERIA.md`
- `beta_requirements/drivers/python/API_REFERENCE.md`
- `beta_requirements/drivers/python/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/python/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/python/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/python/README.md`
- `beta_requirements/drivers/python/SPECIFICATION.md`
- `beta_requirements/drivers/python/TESTING_CRITERIA.md`
- `beta_requirements/drivers/r/API_REFERENCE.md`
- `beta_requirements/drivers/r/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/r/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/r/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/r/SPECIFICATION.md`
- `beta_requirements/drivers/r/TESTING_CRITERIA.md`
- `beta_requirements/drivers/ruby/API_REFERENCE.md`
- `beta_requirements/drivers/ruby/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/ruby/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/ruby/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/ruby/SPECIFICATION.md`
- `beta_requirements/drivers/ruby/TESTING_CRITERIA.md`
- `beta_requirements/drivers/rust/API_REFERENCE.md`
- `beta_requirements/drivers/rust/COMPATIBILITY_MATRIX.md`
- `beta_requirements/drivers/rust/IMPLEMENTATION_PLAN.md`
- `beta_requirements/drivers/rust/MIGRATION_GUIDE.md`
- `beta_requirements/drivers/rust/SPECIFICATION.md`
- `beta_requirements/drivers/rust/TESTING_CRITERIA.md`
- `beta_requirements/indexes/README.md`
- `beta_requirements/nosql/NOSQL_CATALOG_MODEL_SPEC.md`
- `beta_requirements/nosql/NOSQL_CONSOLIDATION_AND_GAP_MATRIX.md`
- `beta_requirements/nosql/NOSQL_ENGINE_TYPE_OVERVIEW.md`
- `beta_requirements/nosql/NOSQL_LANGUAGE_SPEC_TRACKER.md`
- `beta_requirements/nosql/NOSQL_SCHEMA_SPECIFICATION.md`
- `beta_requirements/nosql/NOSQL_STORAGE_STRUCTURES_REPORT.md`
- `beta_requirements/nosql/README.md`
- `beta_requirements/nosql/languages/README.md`
- `beta_requirements/nosql/languages/arangodb_aql/SPECIFICATION.md`
- `beta_requirements/nosql/languages/cassandra_cql/SPECIFICATION.md`
- `beta_requirements/nosql/languages/couchbase_n1ql/SPECIFICATION.md`
- `beta_requirements/nosql/languages/couchdb_mango/SPECIFICATION.md`
- `beta_requirements/nosql/languages/cypher/SPECIFICATION.md`
- `beta_requirements/nosql/languages/elasticsearch_dsl/SPECIFICATION.md`
- `beta_requirements/nosql/languages/flux/SPECIFICATION.md`
- `beta_requirements/nosql/languages/gremlin/SPECIFICATION.md`
- `beta_requirements/nosql/languages/hbase_shell/SPECIFICATION.md`
- `beta_requirements/nosql/languages/influxql/SPECIFICATION.md`
- `beta_requirements/nosql/languages/lucene_query_syntax/SPECIFICATION.md`
- `beta_requirements/nosql/languages/milvus_query/SPECIFICATION.md`
- `beta_requirements/nosql/languages/mongodb_mql/SPECIFICATION.md`
- `beta_requirements/nosql/languages/promql/SPECIFICATION.md`
- `beta_requirements/nosql/languages/redis_resp/SPECIFICATION.md`
- `beta_requirements/nosql/languages/sparql/SPECIFICATION.md`
- `beta_requirements/nosql/languages/weaviate_graphql/SPECIFICATION.md`
- `beta_requirements/optional/AUDIT_TEMPORAL_HISTORY_ARCHIVE.md`
- `beta_requirements/optional/README.md`
- `beta_requirements/optional/STORAGE_ENCODING_OPTIMIZATIONS.md`
- `beta_requirements/optional/TABLESPACE_SHRINK_COMPACTION.md`
- `beta_requirements/orms-frameworks/README.md`
- `beta_requirements/replication/BETA_REPLICATION_ARCHITECTURE_FINDINGS.md`
- `beta_requirements/replication/README.md`
- `beta_requirements/replication/REPLICATION_AND_SHADOW_PROTOCOLS.md`
- `beta_requirements/replication/WAL_IMPLEMENTATION.md`
- `beta_requirements/replication/uuidv7-optimized/00_BETA_REPLICATION_INDEX.md`
- `beta_requirements/replication/uuidv7-optimized/00_REPLICATION_INDEX.md`
- `beta_requirements/replication/uuidv7-optimized/01_CORE_ARCHITECTURE.md`
- `beta_requirements/replication/uuidv7-optimized/01_UUIDV8_HLC_ARCHITECTURE.md`
- `beta_requirements/replication/uuidv7-optimized/02_LEADERLESS_QUORUM_REPLICATION.md`
- `beta_requirements/replication/uuidv7-optimized/03_SCHEMA_DRIVEN_COLOCATION.md`
- `beta_requirements/replication/uuidv7-optimized/04_TIME_PARTITIONED_MERKLE_FOREST.md`
- `beta_requirements/replication/uuidv7-optimized/05_MGA_INTEGRATION.md`
- `beta_requirements/replication/uuidv7-optimized/06_IMPLEMENTATION_PHASES.md`
- `beta_requirements/replication/uuidv7-optimized/07_TESTING_STRATEGY.md`
- `beta_requirements/replication/uuidv7-optimized/08_MIGRATION_OPERATIONS.md`
- `beta_requirements/tools/README.md`

### catalog/

- `catalog/CATALOG_CORRECTION_PLAN.md`
- `catalog/COMPONENT_MODEL_AND_RESPONSIBILITIES.md`
- `catalog/README.md`
- `catalog/SCHEMA_PATH_RESOLUTION.md`
- `catalog/SCHEMA_PATH_SECURITY_DEFAULTS.md`
- `catalog/SYSTEM_CATALOG_STRUCTURE.md`

### compression/

- `compression/COMPRESSION_FRAMEWORK.md`
- `compression/README.md`

### core/

- `core/CACHE_AND_BUFFER_ARCHITECTURE.md`
- `core/CORE_IMPLEMENTATION_SPECS_SUMMARY.md`
- `core/ENGINE_CORE_UNIFIED_SPEC.md`
- `core/GIT_METADATA_INTEGRATION_SPECIFICATION.md`
- `core/IMPLEMENTATION_RECOMMENDATIONS.md`
- `core/INTERNAL_FUNCTIONS.md`
- `core/LIVE_MIGRATION_PASSTHROUGH_SPECIFICATION.md`
- `core/THREAD_SAFETY.md`
- `core/Y_VALVE_ARCHITECTURE.md`
- `core/design_limits.md`

### ddl/

- `ddl/02_DDL_STATEMENTS_OVERVIEW.md`
- `ddl/09_DDL_FOREIGN_DATA.md`
- `ddl/CASCADE_DROP_SPECIFICATION.md`
- `ddl/DDL_DATABASES.md`
- `ddl/DDL_EVENTS.md`
- `ddl/DDL_EXCEPTIONS.md`
- `ddl/DDL_FUNCTIONS.md`
- `ddl/DDL_INDEXES.md`
- `ddl/DDL_PACKAGES.md`
- `ddl/DDL_PROCEDURES.md`
- `ddl/DDL_ROLES_AND_GROUPS.md`
- `ddl/DDL_ROW_LEVEL_SECURITY.md`
- `ddl/DDL_SCHEMAS.md`
- `ddl/DDL_SEQUENCES.md`
- `ddl/DDL_TABLES.md`
- `ddl/DDL_TABLE_PARTITIONING.md`
- `ddl/DDL_TEMPORAL_TABLES.md`
- `ddl/DDL_TRIGGERS.md`
- `ddl/DDL_TYPES.md`
- `ddl/DDL_USER_DEFINED_RESOURCES.md`
- `ddl/DDL_VIEWS.md`
- `ddl/EXTRACT_AND_ALTER_ELEMENT.md`
- `ddl/README.md`

### deployment/

- `deployment/INSTALLATION_AND_BUILD_SPECIFICATION.md`
- `deployment/INSTALLER_FEATURES_AND_CONFIG_GENERATOR.md`
- `deployment/SYSTEMD_SERVICE_SPECIFICATION.md`
- `deployment/WINDOWS_CROSS_COMPILE_SPECIFICATION.md`

### dml/

- `dml/04_DML_STATEMENTS_OVERVIEW.md`
- `dml/DML_COPY.md`
- `dml/DML_DELETE.md`
- `dml/DML_INSERT.md`
- `dml/DML_MERGE.md`
- `dml/DML_SELECT.md`
- `dml/DML_UPDATE.md`
- `dml/DML_XML_JSON_TABLES.md`
- `dml/README.md`

### future/

- `future/C_API_IMPLEMENTATION_GUIDE.md`
- `future/C_API_SPECIFICATION.md`
- `future/ERROR_HANDLING.md`
- `future/README.md`

### indexes/

- `indexes/AdaptiveRadixTreeIndex.md`
- `indexes/AdvancedIndexes.md`
- `indexes/BloomFilterIndex.md`
- `indexes/COLUMNSTORE_SPEC.md`
- `indexes/CountMinSketchIndex.md`
- `indexes/FSTIndex.md`
- `indexes/GeohashS2Index.md`
- `indexes/HyperLogLogIndex.md`
- `indexes/INDEX_ARCHITECTURE.md`
- `indexes/INDEX_GC_PROTOCOL.md`
- `indexes/INDEX_IMPLEMENTATION_GUIDE.md`
- `indexes/INDEX_IMPLEMENTATION_SPEC.md`
- `indexes/IVFIndex.md`
- `indexes/InvertedIndex.md`
- `indexes/JSONPathIndex.md`
- `indexes/LSMTimeSeriesIndex.md`
- `indexes/LSM_TREE_ARCHITECTURE.md`
- `indexes/LSM_TREE_SPEC.md`
- `indexes/LearnedIndex.md`
- `indexes/QuadtreeOctreeIndex.md`
- `indexes/README.md`
- `indexes/SuffixIndex.md`
- `indexes/ZOrderIndex.md`
- `indexes/ZoneMapsIndex.md`

### network/

- `network/CONTROL_PLANE_PROTOCOL_SPEC.md`
- `network/DIALECT_AUTH_MAPPING_SPEC.md`
- `network/ENGINE_PARSER_IPC_CONTRACT.md`
- `network/NETWORK_LAYER_SPEC.md`
- `network/NETWORK_LISTENER_AND_PARSER_POOL_SPEC.md`
- `network/PARSER_AGENT_SPEC.md`
- `network/README.md`
- `network/WIRE_PROTOCOL_SPECIFICATIONS.md`
- `network/Y_VALVE_DESIGN_PRINCIPLES.md`
- `network/ipc_message_table.json`
- `network/ipc_message_table.yaml`

### operations/

- `operations/LISTENER_POOL_METRICS.md`
- `operations/MONITORING_DIALECT_MAPPINGS.md`
- `operations/MONITORING_SQL_VIEWS.md`
- `operations/OID_MAPPING_STRATEGY.md`
- `operations/PROMETHEUS_METRICS_REFERENCE.md`
- `operations/README.md`

### parser/

- `parser/01_SQL_DIALECT_OVERVIEW.md`
- `parser/05_PSQL_PROCEDURAL_LANGUAGE.md`
- `parser/08_PARSER_AND_DEVELOPER_EXPERIENCE.md`
- `parser/EMULATED_DATABASE_PARSER_SPECIFICATION.md`
- `parser/MYSQL_PARSER_SPECIFICATION.md`
- `parser/POSTGRESQL_PARSER_IMPLEMENTATION.md`
- `parser/POSTGRESQL_PARSER_SPECIFICATION.md`
- `parser/PSQL_CURSOR_HANDLES.md`
- `parser/README.md`
- `parser/SCRATCHBIRD_SQL_COMPLETE_BNF.md`
- `parser/SCRATCHBIRD_SQL_CORE_LANGUAGE.md`
- `parser/SCRATCHBIRD_UNIFIED_NOSQL_EXTENSIONS.md`
- `parser/ScratchBird Master Grammar Specification v2.0.md`
- `parser/ScratchBird SQL Language Specification - Master Document.md`

### query/

- `query/PARALLEL_EXECUTION_ARCHITECTURE.md`
- `query/QUERY_OPTIMIZER_SPEC.md`
- `query/README.md`

### reference/

- `reference/README.md`
- `reference/UUIDv7 Replication System Design.md`
- `reference/firebird/2_newtransactionfeatures.pdf`
- `reference/firebird/FirebirdReferenceDocument.md`
- `reference/firebird/README.md`
- `reference/firebird/firebird-50-language-reference.pdf`
- `reference/firebird/firebird-isql.pdf`
- `reference/firebird/firebird_docs_split/00_Preface_and_ToC.md`
- `reference/firebird/firebird_docs_split/01_About_Firebird_5.0.md`
- `reference/firebird/firebird_docs_split/02_SQL_Language_Structure.md`
- `reference/firebird/firebird_docs_split/03_Data_Types_and_Subtypes.md`
- `reference/firebird/firebird_docs_split/04_Common_Language_Elements.md`
- `reference/firebird/firebird_docs_split/05_DDL_Statements.md`
- `reference/firebird/firebird_docs_split/06_DML_Statements.md`
- `reference/firebird/firebird_docs_split/07_PSQL_Statements.md`
- `reference/firebird/firebird_docs_split/08_Built_in_Scalar_Functions.md`
- `reference/firebird/firebird_docs_split/09_Aggregate_Functions.md`
- `reference/firebird/firebird_docs_split/10_Window_Functions.md`
- `reference/firebird/firebird_docs_split/11_System_Packages.md`
- `reference/firebird/firebird_docs_split/12_Context_Variables.md`
- `reference/firebird/firebird_docs_split/13_Transaction_Control.md`
- `reference/firebird/firebird_docs_split/14_Security.md`
- `reference/firebird/firebird_docs_split/15_Management_Statements.md`
- `reference/firebird/firebird_docs_split/App_A_Supplementary_Info.md`
- `reference/firebird/firebird_docs_split/App_B_Exception_Codes.md`
- `reference/firebird/firebird_docs_split/App_C_Reserved_Words.md`
- `reference/firebird/firebird_docs_split/App_D_System_Tables.md`
- `reference/firebird/firebird_docs_split/App_E_Monitoring_Tables.md`
- `reference/firebird/firebird_docs_split/App_F_Security_Tables.md`
- `reference/firebird/firebird_docs_split/App_G_Plugin_Tables.md`
- `reference/firebird/firebird_docs_split/App_H_Charsets_and_Collations.md`
- `reference/firebird/firebird_docs_split/App_I_License.md`
- `reference/firebird/firebird_docs_split/App_J_Document_History.md`

### remote_database_udr/

- `remote_database_udr/01-CORE_TYPES.md`
- `remote_database_udr/02-CONNECTION_POOL.md`
- `remote_database_udr/03-POSTGRESQL_ADAPTER.md`
- `remote_database_udr/04-MYSQL_ADAPTER.md`
- `remote_database_udr/05-MSSQL_FIREBIRD_ADAPTERS.md`
- `remote_database_udr/06-QUERY_EXECUTION.md`
- `remote_database_udr/07-SCHEMA_INTROSPECTION.md`
- `remote_database_udr/08-SQL_SYNTAX.md`
- `remote_database_udr/09-MIGRATION_WORKFLOWS.md`
- `remote_database_udr/README.md`

### root/

- `BACKUP_AND_RESTORE.md`
- `BETA_SQL2023_IMPLEMENTATION_SPECIFICATION.md`
- `BETA_SQL_STANDARD_COMPLIANCE_SPECIFICATION.md`
- `DRIVER_CONFORMANCE_TEST_HARNESS.md`
- `DRIVER_STREAMING_AND_PAGING.md`
- `FIREBIRD_V2_FEATURE_PARITY_SPECIFICATION.md`
- `MEMORY_MANAGEMENT.md`
- `MYSQL_PARSER_IMPLEMENTATION_GAPS.md`
- `PARSER_REMAPPING_AND_IMPLEMENTATION_STRATEGY.md`
- `PERFORMANCE_BENCHMARKS.md`
- `POSTGRESQL_PARSER_IMPLEMENTATION_GAPS.md`
- `TEMPORARY_TABLES_SPECIFICATION.md`
- `V2_PARSER_FIREBIRD_ALIGNMENT_SPECIFICATION.md`
- `V2_PARSER_INDEX_TYPE_COMPLETENESS.md`
- `split_firebird_docs.py`

### sblr/

- `sblr/Appendix_A_SBLR_BYTECODE.md`
- `sblr/FIREBIRD_BLR_FIXTURES.md`
- `sblr/FIREBIRD_BLR_TO_SBLR_MAPPING.md`
- `sblr/FIREBIRD_TRANSACTION_MODEL_SPEC.md`
- `sblr/README.md`
- `sblr/SBLR_DOMAIN_PAYLOADS.md`
- `sblr/SBLR_EXECUTION_PERFORMANCE_ALPHA.md`
- `sblr/SBLR_EXECUTION_PERFORMANCE_BETA.md`
- `sblr/SBLR_EXECUTION_PERFORMANCE_RESEARCH.md`
- `sblr/SBLR_OPCODE_REGISTRY.md`

### scheduler/

- `scheduler/ALPHA_SCHEDULER_SPECIFICATION.md`
- `scheduler/README.md`
- `scheduler/SCHEDULER_JOB_RUNNER_CANONICAL_SPEC.md`

### scratchbird/

- `scratchbird/specifications/Alpha Phase 2/00-Implementation-Roadmap.md`
- `scratchbird/specifications/Alpha Phase 2/01-Architecture-Overview.md`
- `scratchbird/specifications/Alpha Phase 2/02-Clock-Synchronization-Specification.md`
- `scratchbird/specifications/Alpha Phase 2/03-Distributed-MVCC-Specification.md`
- `scratchbird/specifications/Alpha Phase 2/04-Replication-Protocol-Specification.md`
- `scratchbird/specifications/Alpha Phase 2/05-Wire-Protocol-Integration-Specification.md`
- `scratchbird/specifications/Alpha Phase 2/06-Ingestion-Layer.md`
- `scratchbird/specifications/Alpha Phase 2/07-OLAP-Tier.md`
- `scratchbird/specifications/Alpha Phase 2/08-Deployment-Guide.md`
- `scratchbird/specifications/Alpha Phase 2/09-Monitoring-Observability.md`
- `scratchbird/specifications/Alpha Phase 2/10-UDR-System-Specification.md`
- `scratchbird/specifications/Alpha Phase 2/11-Remote-Database-UDR-Specification.md`
- `scratchbird/specifications/Alpha Phase 2/11a-Connection-Pool-Implementation.md`
- `scratchbird/specifications/Alpha Phase 2/11b-PostgreSQL-Client-Implementation.md`
- `scratchbird/specifications/Alpha Phase 2/11c-MySQL-Client-Implementation.md`
- `scratchbird/specifications/Alpha Phase 2/11d-MSSQL-Client-Implementation.md`
- `scratchbird/specifications/Alpha Phase 2/11e-Firebird-Client-Implementation.md`
- `scratchbird/specifications/Alpha Phase 2/11f-ODBC-Client-Implementation.md`
- `scratchbird/specifications/Alpha Phase 2/11g-JDBC-Client-Implementation.md`
- `scratchbird/specifications/Alpha Phase 2/11h-Live-Migration-Emulated-Listener.md`
- `scratchbird/specifications/Alpha Phase 2/11i-ScratchBird-Client-Implementation.md`
- `scratchbird/specifications/Alpha Phase 2/Discussion_Notes.md`
- `scratchbird/specifications/Alpha Phase 2/README.md`
- `scratchbird/specifications/BACKUP_AND_RESTORE.md`
- `scratchbird/specifications/BETA_SQL2023_IMPLEMENTATION_SPECIFICATION.md`
- `scratchbird/specifications/BETA_SQL_STANDARD_COMPLIANCE_SPECIFICATION.md`
- `scratchbird/specifications/Cluster Specification Work/README.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-00-GUIDING-PRINCIPLES.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-01-CLUSTER-CONFIG-EPOCH.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-02-MEMBERSHIP-AND-IDENTITY.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-03-CA-POLICY.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-04-SECURITY-BUNDLE.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-05-SHARDING.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-06-DISTRIBUTED-QUERY.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-07-REPLICATION.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-08-BACKUP-AND-RESTORE.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-09-SCHEDULER.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-10-OBSERVABILITY.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-11-SHARD-MIGRATION-AND-REBALANCING.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-12-AUTOSCALING_AND_ELASTIC_LIFECYCLE.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-AI-HANDOFF.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-IMPLEMENTATION-BOUNDARY.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-NORMATIVE-LANGUAGE.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-SUMMARY.md`
- `scratchbird/specifications/Cluster Specification Work/SBCLUSTER-THREAT-MODEL.md`
- `scratchbird/specifications/Cluster Specification Work/sbsec_handoff_summary.md`
- `scratchbird/specifications/Cluster Specification Work/scratch_bird_cluster_architecture_security_specifications_draft.md`
- `scratchbird/specifications/DRIVER_CONFORMANCE_TEST_HARNESS.md`
- `scratchbird/specifications/DRIVER_STREAMING_AND_PAGING.md`
- `scratchbird/specifications/FIREBIRD_V2_FEATURE_PARITY_SPECIFICATION.md`
- `scratchbird/specifications/MEMORY_MANAGEMENT.md`
- `scratchbird/specifications/MYSQL_PARSER_IMPLEMENTATION_GAPS.md`
- `scratchbird/specifications/PARSER_REMAPPING_AND_IMPLEMENTATION_STRATEGY.md`
- `scratchbird/specifications/PERFORMANCE_BENCHMARKS.md`
- `scratchbird/specifications/POSTGRESQL_PARSER_IMPLEMENTATION_GAPS.md`
- `scratchbird/specifications/README.md`
- `scratchbird/specifications/Security Design Specification/00_SECURITY_SPEC_INDEX.md`
- `scratchbird/specifications/Security Design Specification/01_SECURITY_ARCHITECTURE.md`
- `scratchbird/specifications/Security Design Specification/02_IDENTITY_AUTHENTICATION.md`
- `scratchbird/specifications/Security Design Specification/03_AUTHORIZATION_MODEL.md`
- `scratchbird/specifications/Security Design Specification/04.01_KEY_LIFECYCLE_STATE_MACHINES.md`
- `scratchbird/specifications/Security Design Specification/04.02_KEY_MATERIAL_HANDLING_REQUIREMENTS.md`
- `scratchbird/specifications/Security Design Specification/04.03_NONCE_IV_SPECIFICATION.md`
- `scratchbird/specifications/Security Design Specification/04_ENCRYPTION_KEY_MANAGEMENT.md`
- `scratchbird/specifications/Security Design Specification/05.A_IPC_WIRE_FORMAT_AND_EXAMPLES.md`
- `scratchbird/specifications/Security Design Specification/05_IPC_SECURITY.md`
- `scratchbird/specifications/Security Design Specification/06.01_QUORUM_PROPOSAL_CANONICALIZATION_HASHING.md`
- `scratchbird/specifications/Security Design Specification/06.02_QUORUM_EVIDENCE_AND_AUDIT_COUPLING.md`
- `scratchbird/specifications/Security Design Specification/06_CLUSTER_SECURITY.md`
- `scratchbird/specifications/Security Design Specification/07_NETWORK_PRESENCE_BINDING.md`
- `scratchbird/specifications/Security Design Specification/08.01_AUDIT_EVENT_CANONICALIZATION.md`
- `scratchbird/specifications/Security Design Specification/08.02_AUDIT_CHAIN_VERIFICATION_CHECKPOINTS.md`
- `scratchbird/specifications/Security Design Specification/08_AUDIT_COMPLIANCE.md`
- `scratchbird/specifications/Security Design Specification/09_SECURITY_LEVELS.md`
- `scratchbird/specifications/Security Design Specification/10_RELEASE_INTEGRITY_PROVENANCE.md`
- `scratchbird/specifications/Security Design Specification/AUTH_CERTIFICATE_TLS.md`
- `scratchbird/specifications/Security Design Specification/AUTH_CORE_FRAMEWORK.md`
- `scratchbird/specifications/Security Design Specification/AUTH_ENTERPRISE_LDAP_KERBEROS.md`
- `scratchbird/specifications/Security Design Specification/AUTH_MODERN_OAUTH_MFA.md`
- `scratchbird/specifications/Security Design Specification/AUTH_PASSWORD_METHODS.md`
- `scratchbird/specifications/Security Design Specification/EXTERNAL_AUTHENTICATION_DESIGN.md`
- `scratchbird/specifications/Security Design Specification/README.md`
- `scratchbird/specifications/Security Design Specification/ROLE_COMPOSITION_AND_HIERARCHIES.md`
- `scratchbird/specifications/Security Design Specification/archive security work/00_SECURITY_SPEC_INDEX.md`
- `scratchbird/specifications/Security Design Specification/archive security work/01_SECURITY_ARCHITECTURE.md`
- `scratchbird/specifications/Security Design Specification/archive security work/02_IDENTITY_AUTHENTICATION.md`
- `scratchbird/specifications/Security Design Specification/archive security work/03_AUTHORIZATION_MODEL.md`
- `scratchbird/specifications/Security Design Specification/archive security work/04_ENCRYPTION_KEY_MANAGEMENT.md`
- `scratchbird/specifications/Security Design Specification/archive security work/05_IPC_SECURITY.md`
- `scratchbird/specifications/Security Design Specification/archive security work/06_CLUSTER_SECURITY.md`
- `scratchbird/specifications/Security Design Specification/archive security work/07_NETWORK_PRESENCE_BINDING.md`
- `scratchbird/specifications/Security Design Specification/archive security work/08_AUDIT_COMPLIANCE.md`
- `scratchbird/specifications/Security Design Specification/archive security work/09_SECURITY_LEVELS.md`
- `scratchbird/specifications/Security Design Specification/archive security work/Beta Task -Distributed Secret Sharing Implementation Specification.md`
- `scratchbird/specifications/Security Design Specification/archive security work/Engine Internal Security.md`
- `scratchbird/specifications/Security Design Specification/archive security work/SECURITY_IMPLIMENTATION_DETAILS.md`
- `scratchbird/specifications/Security Design Specification/archive security work/SECURITY_SYSTEM_SPECIFICATION.md`
- `scratchbird/specifications/Security Design Specification/archive security work/Security Hardening Guide.md`
- `scratchbird/specifications/Security Design Specification/archive security work/draft_security_architecture_specification.md`
- `scratchbird/specifications/Security Design Specification/contributor_security_rules.md`
- `scratchbird/specifications/Security Design Specification/sbsec_alpha_base.md`
- `scratchbird/specifications/Security Design Specification/supportability_contract.md`
- `scratchbird/specifications/TEMPORARY_TABLES_SPECIFICATION.md`
- `scratchbird/specifications/V2_PARSER_FIREBIRD_ALIGNMENT_SPECIFICATION.md`
- `scratchbird/specifications/V2_PARSER_INDEX_TYPE_COMPLETENESS.md`
- `scratchbird/specifications/admin/README.md`
- `scratchbird/specifications/admin/SB_ADMIN_CLI_SPECIFICATION.md`
- `scratchbird/specifications/admin/SB_SERVER_NETWORK_CLI_SPECIFICATION.md`
- `scratchbird/specifications/alpha_requirements/optional/OBJECT_NAME_REGISTRY_MATERIALIZED_VIEW.md`
- `scratchbird/specifications/alpha_requirements/optional/README.md`
- `scratchbird/specifications/api/CONNECTION_POOLING_SPECIFICATION.md`
- `scratchbird/specifications/api/README.md`
- `scratchbird/specifications/archive/AnalysisOfBestParsingStructures.md`
- `scratchbird/specifications/archive/DraftQueryOptimizationSpecification.md`
- `scratchbird/specifications/archive/README.md`
- `scratchbird/specifications/archive/Specification for a Multi-Generational Database Architecture.md`
- `scratchbird/specifications/beta_requirements/00_DRIVERS_AND_INTEGRATIONS_INDEX.md`
- `scratchbird/specifications/beta_requirements/COMPLETION_STATUS.md`
- `scratchbird/specifications/beta_requirements/DIRECTORY_STRUCTURE_CREATED.md`
- `scratchbird/specifications/beta_requirements/README.md`
- `scratchbird/specifications/beta_requirements/ai-ml/README.md`
- `scratchbird/specifications/beta_requirements/applications/README.md`
- `scratchbird/specifications/beta_requirements/big-data-streaming/README.md`
- `scratchbird/specifications/beta_requirements/big-data-streaming/apache-kafka/README.md`
- `scratchbird/specifications/beta_requirements/big-data-streaming/apache-kafka/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/builds/00_BUILD_REQUIREMENTS_INDEX.md`
- `scratchbird/specifications/beta_requirements/builds/01_LINUX_NATIVE.md`
- `scratchbird/specifications/beta_requirements/builds/02_WINDOWS_NATIVE.md`
- `scratchbird/specifications/beta_requirements/builds/03_MACOS_NATIVE.md`
- `scratchbird/specifications/beta_requirements/builds/10_LINUX_TO_WINDOWS.md`
- `scratchbird/specifications/beta_requirements/builds/11_LINUX_TO_MACOS.md`
- `scratchbird/specifications/beta_requirements/builds/12_WINDOWS_TO_LINUX.md`
- `scratchbird/specifications/beta_requirements/builds/20_APPIMAGE.md`
- `scratchbird/specifications/beta_requirements/builds/23_DEB.md`
- `scratchbird/specifications/beta_requirements/builds/24_RPM.md`
- `scratchbird/specifications/beta_requirements/builds/27_BREW.md`
- `scratchbird/specifications/beta_requirements/builds/30_DOCKER.md`
- `scratchbird/specifications/beta_requirements/builds/40_GITHUB_ACTIONS.md`
- `scratchbird/specifications/beta_requirements/builds/COMPLETE_BUILD_ENVIRONMENT_SETUP.md`
- `scratchbird/specifications/beta_requirements/cloud-container/README.md`
- `scratchbird/specifications/beta_requirements/cloud-container/docker/README.md`
- `scratchbird/specifications/beta_requirements/cloud-container/docker/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/connectivity/README.md`
- `scratchbird/specifications/beta_requirements/connectivity/odbc/README.md`
- `scratchbird/specifications/beta_requirements/connectivity/odbc/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/DRIVER_BASELINE_SPEC.md`
- `scratchbird/specifications/beta_requirements/drivers/README.md`
- `scratchbird/specifications/beta_requirements/drivers/cpp/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/cpp/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/cpp/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/cpp/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/cpp/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/cpp/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/dotnet-csharp/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/dotnet-csharp/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/dotnet-csharp/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/dotnet-csharp/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/dotnet-csharp/README.md`
- `scratchbird/specifications/beta_requirements/drivers/dotnet-csharp/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/dotnet-csharp/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/golang/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/golang/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/golang/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/golang/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/golang/README.md`
- `scratchbird/specifications/beta_requirements/drivers/golang/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/golang/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/java-jdbc/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/java-jdbc/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/java-jdbc/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/java-jdbc/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/java-jdbc/README.md`
- `scratchbird/specifications/beta_requirements/drivers/java-jdbc/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/java-jdbc/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/nodejs-typescript/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/nodejs-typescript/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/nodejs-typescript/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/nodejs-typescript/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/nodejs-typescript/README.md`
- `scratchbird/specifications/beta_requirements/drivers/nodejs-typescript/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/nodejs-typescript/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/pascal-delphi/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/pascal-delphi/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/pascal-delphi/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/pascal-delphi/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/pascal-delphi/README.md`
- `scratchbird/specifications/beta_requirements/drivers/pascal-delphi/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/pascal-delphi/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/php/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/php/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/php/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/php/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/php/README.md`
- `scratchbird/specifications/beta_requirements/drivers/php/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/php/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/python/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/python/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/python/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/python/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/python/README.md`
- `scratchbird/specifications/beta_requirements/drivers/python/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/python/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/r/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/r/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/r/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/r/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/r/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/r/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/ruby/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/ruby/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/ruby/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/ruby/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/ruby/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/ruby/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/drivers/rust/API_REFERENCE.md`
- `scratchbird/specifications/beta_requirements/drivers/rust/COMPATIBILITY_MATRIX.md`
- `scratchbird/specifications/beta_requirements/drivers/rust/IMPLEMENTATION_PLAN.md`
- `scratchbird/specifications/beta_requirements/drivers/rust/MIGRATION_GUIDE.md`
- `scratchbird/specifications/beta_requirements/drivers/rust/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/drivers/rust/TESTING_CRITERIA.md`
- `scratchbird/specifications/beta_requirements/indexes/README.md`
- `scratchbird/specifications/beta_requirements/nosql/NOSQL_CATALOG_MODEL_SPEC.md`
- `scratchbird/specifications/beta_requirements/nosql/NOSQL_CONSOLIDATION_AND_GAP_MATRIX.md`
- `scratchbird/specifications/beta_requirements/nosql/NOSQL_ENGINE_TYPE_OVERVIEW.md`
- `scratchbird/specifications/beta_requirements/nosql/NOSQL_LANGUAGE_SPEC_TRACKER.md`
- `scratchbird/specifications/beta_requirements/nosql/NOSQL_SCHEMA_SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/NOSQL_STORAGE_STRUCTURES_REPORT.md`
- `scratchbird/specifications/beta_requirements/nosql/README.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/README.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/arangodb_aql/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/cassandra_cql/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/couchbase_n1ql/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/couchdb_mango/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/cypher/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/elasticsearch_dsl/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/flux/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/gremlin/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/hbase_shell/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/influxql/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/lucene_query_syntax/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/milvus_query/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/mongodb_mql/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/promql/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/redis_resp/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/sparql/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/nosql/languages/weaviate_graphql/SPECIFICATION.md`
- `scratchbird/specifications/beta_requirements/optional/AUDIT_TEMPORAL_HISTORY_ARCHIVE.md`
- `scratchbird/specifications/beta_requirements/optional/README.md`
- `scratchbird/specifications/beta_requirements/optional/STORAGE_ENCODING_OPTIMIZATIONS.md`
- `scratchbird/specifications/beta_requirements/optional/TABLESPACE_SHRINK_COMPACTION.md`
- `scratchbird/specifications/beta_requirements/orms-frameworks/README.md`
- `scratchbird/specifications/beta_requirements/replication/BETA_REPLICATION_ARCHITECTURE_FINDINGS.md`
- `scratchbird/specifications/beta_requirements/replication/README.md`
- `scratchbird/specifications/beta_requirements/replication/REPLICATION_AND_SHADOW_PROTOCOLS.md`
- `scratchbird/specifications/beta_requirements/replication/WAL_IMPLEMENTATION.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/00_BETA_REPLICATION_INDEX.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/00_REPLICATION_INDEX.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/01_CORE_ARCHITECTURE.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/01_UUIDV8_HLC_ARCHITECTURE.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/02_LEADERLESS_QUORUM_REPLICATION.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/03_SCHEMA_DRIVEN_COLOCATION.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/04_TIME_PARTITIONED_MERKLE_FOREST.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/05_MGA_INTEGRATION.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/06_IMPLEMENTATION_PHASES.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/07_TESTING_STRATEGY.md`
- `scratchbird/specifications/beta_requirements/replication/uuidv7-optimized/08_MIGRATION_OPERATIONS.md`
- `scratchbird/specifications/beta_requirements/tools/README.md`
- `scratchbird/specifications/catalog/CATALOG_CORRECTION_PLAN.md`
- `scratchbird/specifications/catalog/COMPONENT_MODEL_AND_RESPONSIBILITIES.md`
- `scratchbird/specifications/catalog/README.md`
- `scratchbird/specifications/catalog/SCHEMA_PATH_RESOLUTION.md`
- `scratchbird/specifications/catalog/SCHEMA_PATH_SECURITY_DEFAULTS.md`
- `scratchbird/specifications/catalog/SYSTEM_CATALOG_STRUCTURE.md`
- `scratchbird/specifications/compression/COMPRESSION_FRAMEWORK.md`
- `scratchbird/specifications/compression/README.md`
- `scratchbird/specifications/core/CACHE_AND_BUFFER_ARCHITECTURE.md`
- `scratchbird/specifications/core/CORE_IMPLEMENTATION_SPECS_SUMMARY.md`
- `scratchbird/specifications/core/ENGINE_CORE_UNIFIED_SPEC.md`
- `scratchbird/specifications/core/GIT_METADATA_INTEGRATION_SPECIFICATION.md`
- `scratchbird/specifications/core/IMPLEMENTATION_RECOMMENDATIONS.md`
- `scratchbird/specifications/core/INTERNAL_FUNCTIONS.md`
- `scratchbird/specifications/core/LIVE_MIGRATION_PASSTHROUGH_SPECIFICATION.md`
- `scratchbird/specifications/core/README.md`
- `scratchbird/specifications/core/THREAD_SAFETY.md`
- `scratchbird/specifications/core/Y_VALVE_ARCHITECTURE.md`
- `scratchbird/specifications/core/design_limits.md`
- `scratchbird/specifications/ddl/02_DDL_STATEMENTS_OVERVIEW.md`
- `scratchbird/specifications/ddl/09_DDL_FOREIGN_DATA.md`
- `scratchbird/specifications/ddl/CASCADE_DROP_SPECIFICATION.md`
- `scratchbird/specifications/ddl/DDL_DATABASES.md`
- `scratchbird/specifications/ddl/DDL_EVENTS.md`
- `scratchbird/specifications/ddl/DDL_EXCEPTIONS.md`
- `scratchbird/specifications/ddl/DDL_FUNCTIONS.md`
- `scratchbird/specifications/ddl/DDL_INDEXES.md`
- `scratchbird/specifications/ddl/DDL_PACKAGES.md`
- `scratchbird/specifications/ddl/DDL_PROCEDURES.md`
- `scratchbird/specifications/ddl/DDL_ROLES_AND_GROUPS.md`
- `scratchbird/specifications/ddl/DDL_ROW_LEVEL_SECURITY.md`
- `scratchbird/specifications/ddl/DDL_SCHEMAS.md`
- `scratchbird/specifications/ddl/DDL_SEQUENCES.md`
- `scratchbird/specifications/ddl/DDL_TABLES.md`
- `scratchbird/specifications/ddl/DDL_TABLE_PARTITIONING.md`
- `scratchbird/specifications/ddl/DDL_TEMPORAL_TABLES.md`
- `scratchbird/specifications/ddl/DDL_TRIGGERS.md`
- `scratchbird/specifications/ddl/DDL_TYPES.md`
- `scratchbird/specifications/ddl/DDL_USER_DEFINED_RESOURCES.md`
- `scratchbird/specifications/ddl/DDL_VIEWS.md`
- `scratchbird/specifications/ddl/EXTRACT_AND_ALTER_ELEMENT.md`
- `scratchbird/specifications/ddl/README.md`
- `scratchbird/specifications/deployment/INSTALLATION_AND_BUILD_SPECIFICATION.md`
- `scratchbird/specifications/deployment/INSTALLER_FEATURES_AND_CONFIG_GENERATOR.md`
- `scratchbird/specifications/deployment/README.md`
- `scratchbird/specifications/deployment/SYSTEMD_SERVICE_SPECIFICATION.md`
- `scratchbird/specifications/deployment/WINDOWS_CROSS_COMPILE_SPECIFICATION.md`
- `scratchbird/specifications/dml/04_DML_STATEMENTS_OVERVIEW.md`
- `scratchbird/specifications/dml/DML_COPY.md`
- `scratchbird/specifications/dml/DML_DELETE.md`
- `scratchbird/specifications/dml/DML_INSERT.md`
- `scratchbird/specifications/dml/DML_MERGE.md`
- `scratchbird/specifications/dml/DML_SELECT.md`
- `scratchbird/specifications/dml/DML_UPDATE.md`
- `scratchbird/specifications/dml/DML_XML_JSON_TABLES.md`
- `scratchbird/specifications/dml/README.md`
- `scratchbird/specifications/future/C_API_IMPLEMENTATION_GUIDE.md`
- `scratchbird/specifications/future/C_API_SPECIFICATION.md`
- `scratchbird/specifications/future/ERROR_HANDLING.md`
- `scratchbird/specifications/future/README.md`
- `scratchbird/specifications/indexes/AdaptiveRadixTreeIndex.md`
- `scratchbird/specifications/indexes/AdvancedIndexes.md`
- `scratchbird/specifications/indexes/BloomFilterIndex.md`
- `scratchbird/specifications/indexes/COLUMNSTORE_SPEC.md`
- `scratchbird/specifications/indexes/CountMinSketchIndex.md`
- `scratchbird/specifications/indexes/FSTIndex.md`
- `scratchbird/specifications/indexes/GeohashS2Index.md`
- `scratchbird/specifications/indexes/HyperLogLogIndex.md`
- `scratchbird/specifications/indexes/INDEX_ARCHITECTURE.md`
- `scratchbird/specifications/indexes/INDEX_GC_PROTOCOL.md`
- `scratchbird/specifications/indexes/INDEX_IMPLEMENTATION_GUIDE.md`
- `scratchbird/specifications/indexes/INDEX_IMPLEMENTATION_SPEC.md`
- `scratchbird/specifications/indexes/IVFIndex.md`
- `scratchbird/specifications/indexes/InvertedIndex.md`
- `scratchbird/specifications/indexes/JSONPathIndex.md`
- `scratchbird/specifications/indexes/LSMTimeSeriesIndex.md`
- `scratchbird/specifications/indexes/LSM_TREE_ARCHITECTURE.md`
- `scratchbird/specifications/indexes/LSM_TREE_SPEC.md`
- `scratchbird/specifications/indexes/LearnedIndex.md`
- `scratchbird/specifications/indexes/QuadtreeOctreeIndex.md`
- `scratchbird/specifications/indexes/README.md`
- `scratchbird/specifications/indexes/SuffixIndex.md`
- `scratchbird/specifications/indexes/ZOrderIndex.md`
- `scratchbird/specifications/indexes/ZoneMapsIndex.md`
- `scratchbird/specifications/network/CONTROL_PLANE_PROTOCOL_SPEC.md`
- `scratchbird/specifications/network/DIALECT_AUTH_MAPPING_SPEC.md`
- `scratchbird/specifications/network/ENGINE_PARSER_IPC_CONTRACT.md`
- `scratchbird/specifications/network/NETWORK_LAYER_SPEC.md`
- `scratchbird/specifications/network/NETWORK_LISTENER_AND_PARSER_POOL_SPEC.md`
- `scratchbird/specifications/network/PARSER_AGENT_SPEC.md`
- `scratchbird/specifications/network/README.md`
- `scratchbird/specifications/network/WIRE_PROTOCOL_SPECIFICATIONS.md`
- `scratchbird/specifications/network/Y_VALVE_DESIGN_PRINCIPLES.md`
- `scratchbird/specifications/network/ipc_message_table.json`
- `scratchbird/specifications/network/ipc_message_table.yaml`
- `scratchbird/specifications/operations/LISTENER_POOL_METRICS.md`
- `scratchbird/specifications/operations/MONITORING_DIALECT_MAPPINGS.md`
- `scratchbird/specifications/operations/MONITORING_SQL_VIEWS.md`
- `scratchbird/specifications/operations/OID_MAPPING_STRATEGY.md`
- `scratchbird/specifications/operations/PROMETHEUS_METRICS_REFERENCE.md`
- `scratchbird/specifications/operations/README.md`
- `scratchbird/specifications/parser/01_SQL_DIALECT_OVERVIEW.md`
- `scratchbird/specifications/parser/05_PSQL_PROCEDURAL_LANGUAGE.md`
- `scratchbird/specifications/parser/08_PARSER_AND_DEVELOPER_EXPERIENCE.md`
- `scratchbird/specifications/parser/EMULATED_DATABASE_PARSER_SPECIFICATION.md`
- `scratchbird/specifications/parser/MYSQL_PARSER_SPECIFICATION.md`
- `scratchbird/specifications/parser/POSTGRESQL_PARSER_IMPLEMENTATION.md`
- `scratchbird/specifications/parser/POSTGRESQL_PARSER_SPECIFICATION.md`
- `scratchbird/specifications/parser/PSQL_CURSOR_HANDLES.md`
- `scratchbird/specifications/parser/README.md`
- `scratchbird/specifications/parser/SCRATCHBIRD_SQL_COMPLETE_BNF.md`
- `scratchbird/specifications/parser/SCRATCHBIRD_SQL_CORE_LANGUAGE.md`
- `scratchbird/specifications/parser/SCRATCHBIRD_UNIFIED_NOSQL_EXTENSIONS.md`
- `scratchbird/specifications/parser/ScratchBird Master Grammar Specification v2.0.md`
- `scratchbird/specifications/parser/ScratchBird SQL Language Specification - Master Document.md`
- `scratchbird/specifications/query/PARALLEL_EXECUTION_ARCHITECTURE.md`
- `scratchbird/specifications/query/QUERY_OPTIMIZER_SPEC.md`
- `scratchbird/specifications/query/README.md`
- `scratchbird/specifications/reference/README.md`
- `scratchbird/specifications/reference/UUIDv7 Replication System Design.md`
- `scratchbird/specifications/reference/firebird/2_newtransactionfeatures.pdf`
- `scratchbird/specifications/reference/firebird/FirebirdReferenceDocument.md`
- `scratchbird/specifications/reference/firebird/README.md`
- `scratchbird/specifications/reference/firebird/firebird-50-language-reference.pdf`
- `scratchbird/specifications/reference/firebird/firebird-isql.pdf`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/00_Preface_and_ToC.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/01_About_Firebird_5.0.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/02_SQL_Language_Structure.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/03_Data_Types_and_Subtypes.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/04_Common_Language_Elements.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/05_DDL_Statements.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/06_DML_Statements.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/07_PSQL_Statements.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/08_Built_in_Scalar_Functions.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/09_Aggregate_Functions.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/10_Window_Functions.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/11_System_Packages.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/12_Context_Variables.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/13_Transaction_Control.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/14_Security.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/15_Management_Statements.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_A_Supplementary_Info.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_B_Exception_Codes.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_C_Reserved_Words.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_D_System_Tables.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_E_Monitoring_Tables.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_F_Security_Tables.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_G_Plugin_Tables.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_H_Charsets_and_Collations.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_I_License.md`
- `scratchbird/specifications/reference/firebird/firebird_docs_split/App_J_Document_History.md`
- `scratchbird/specifications/remote_database_udr/01-CORE_TYPES.md`
- `scratchbird/specifications/remote_database_udr/02-CONNECTION_POOL.md`
- `scratchbird/specifications/remote_database_udr/03-POSTGRESQL_ADAPTER.md`
- `scratchbird/specifications/remote_database_udr/04-MYSQL_ADAPTER.md`
- `scratchbird/specifications/remote_database_udr/05-MSSQL_FIREBIRD_ADAPTERS.md`
- `scratchbird/specifications/remote_database_udr/06-QUERY_EXECUTION.md`
- `scratchbird/specifications/remote_database_udr/07-SCHEMA_INTROSPECTION.md`
- `scratchbird/specifications/remote_database_udr/08-SQL_SYNTAX.md`
- `scratchbird/specifications/remote_database_udr/09-MIGRATION_WORKFLOWS.md`
- `scratchbird/specifications/remote_database_udr/README.md`
- `scratchbird/specifications/sblr/Appendix_A_SBLR_BYTECODE.md`
- `scratchbird/specifications/sblr/FIREBIRD_BLR_FIXTURES.md`
- `scratchbird/specifications/sblr/FIREBIRD_BLR_TO_SBLR_MAPPING.md`
- `scratchbird/specifications/sblr/FIREBIRD_TRANSACTION_MODEL_SPEC.md`
- `scratchbird/specifications/sblr/README.md`
- `scratchbird/specifications/sblr/SBLR_DOMAIN_PAYLOADS.md`
- `scratchbird/specifications/sblr/SBLR_EXECUTION_PERFORMANCE_ALPHA.md`
- `scratchbird/specifications/sblr/SBLR_EXECUTION_PERFORMANCE_BETA.md`
- `scratchbird/specifications/sblr/SBLR_EXECUTION_PERFORMANCE_RESEARCH.md`
- `scratchbird/specifications/sblr/SBLR_OPCODE_REGISTRY.md`
- `scratchbird/specifications/scheduler/ALPHA_SCHEDULER_SPECIFICATION.md`
- `scratchbird/specifications/scheduler/README.md`
- `scratchbird/specifications/scheduler/SCHEDULER_JOB_RUNNER_CANONICAL_SPEC.md`
- `scratchbird/specifications/split_firebird_docs.py`
- `scratchbird/specifications/storage/EXTENDED_PAGE_SIZES.md`
- `scratchbird/specifications/storage/HEAP_TOAST_INTEGRATION.md`
- `scratchbird/specifications/storage/MGA_IMPLEMENTATION.md`
- `scratchbird/specifications/storage/ON_DISK_FORMAT.md`
- `scratchbird/specifications/storage/README.md`
- `scratchbird/specifications/storage/STORAGE_ENGINE_BUFFER_POOL.md`
- `scratchbird/specifications/storage/STORAGE_ENGINE_MAIN.md`
- `scratchbird/specifications/storage/STORAGE_ENGINE_PAGE_MANAGEMENT.md`
- `scratchbird/specifications/storage/TABLESPACE_ONLINE_MIGRATION.md`
- `scratchbird/specifications/storage/TABLESPACE_SPECIFICATION.md`
- `scratchbird/specifications/storage/TOAST_LOB_STORAGE.md`
- `scratchbird/specifications/testing/ALPHA3_TEST_PLAN.md`
- `scratchbird/specifications/testing/README.md`
- `scratchbird/specifications/tools/README.md`
- `scratchbird/specifications/tools/SB_BACKUP_CLI_SPECIFICATION.md`
- `scratchbird/specifications/tools/SB_ISQL_CLI_SPECIFICATION.md`
- `scratchbird/specifications/tools/SB_SECURITY_CLI_SPECIFICATION.md`
- `scratchbird/specifications/tools/SB_TOOLING_NETWORK_SPEC.md`
- `scratchbird/specifications/tools/SB_VERIFY_CLI_SPECIFICATION.md`
- `scratchbird/specifications/transaction/07_TRANSACTION_AND_SESSION_CONTROL.md`
- `scratchbird/specifications/transaction/FIREBIRD_GC_SWEEP_GLOSSARY.md`
- `scratchbird/specifications/transaction/README.md`
- `scratchbird/specifications/transaction/TRANSACTION_DISTRIBUTED.md`
- `scratchbird/specifications/transaction/TRANSACTION_LOCK_MANAGER.md`
- `scratchbird/specifications/transaction/TRANSACTION_MAIN.md`
- `scratchbird/specifications/transaction/TRANSACTION_MGA_CORE.md`
- `scratchbird/specifications/triggers/README.md`
- `scratchbird/specifications/triggers/TRIGGER_CONTEXT_VARIABLES.md`
- `scratchbird/specifications/types/03_TYPES_AND_DOMAINS.md`
- `scratchbird/specifications/types/COLLATION_TAILORING_LOADER_SPEC.md`
- `scratchbird/specifications/types/DATA_TYPE_PERSISTENCE_AND_CASTS.md`
- `scratchbird/specifications/types/DDL_DOMAINS_COMPREHENSIVE.md`
- `scratchbird/specifications/types/I18N_CANONICAL_LISTS.md`
- `scratchbird/specifications/types/MULTI_GEOMETRY_TYPES_SPEC.md`
- `scratchbird/specifications/types/POSTGRESQL_ARRAY_TYPE_SPEC.md`
- `scratchbird/specifications/types/README.md`
- `scratchbird/specifications/types/TIMEZONE_SYSTEM_CATALOG.md`
- `scratchbird/specifications/types/UUID_IDENTITY_COLUMNS.md`
- `scratchbird/specifications/types/character_sets_and_collations.md`
- `scratchbird/specifications/udr/10-UDR-System-Specification.md`
- `scratchbird/specifications/udr/README.md`
- `scratchbird/specifications/udr/UDR_PSQL_EXTENSION_LIBRARY.md`
- `scratchbird/specifications/udr_connectors/README.md`
- `scratchbird/specifications/udr_connectors/UDR_CONNECTOR_BASELINE.md`
- `scratchbird/specifications/udr_connectors/firebird_udr/SPECIFICATION.md`
- `scratchbird/specifications/udr_connectors/jdbc_udr/SPECIFICATION.md`
- `scratchbird/specifications/udr_connectors/local_files_udr/SPECIFICATION.md`
- `scratchbird/specifications/udr_connectors/local_scripts_udr/SPECIFICATION.md`
- `scratchbird/specifications/udr_connectors/mssql_udr/SPECIFICATION.md`
- `scratchbird/specifications/udr_connectors/mysql_udr/SPECIFICATION.md`
- `scratchbird/specifications/udr_connectors/odbc_udr/SPECIFICATION.md`
- `scratchbird/specifications/udr_connectors/postgresql_udr/SPECIFICATION.md`
- `scratchbird/specifications/udr_connectors/scratchbird_udr/SPECIFICATION.md`
- `scratchbird/specifications/wire_protocols/FIREBIRD_EMULATION_BEHAVIOR.md`
- `scratchbird/specifications/wire_protocols/MYSQL_EMULATION_BEHAVIOR.md`
- `scratchbird/specifications/wire_protocols/POSTGRESQL_EMULATION_BEHAVIOR.md`
- `scratchbird/specifications/wire_protocols/README.md`
- `scratchbird/specifications/wire_protocols/firebird_wire_protocol.md`
- `scratchbird/specifications/wire_protocols/mysql_wire_protocol.md`
- `scratchbird/specifications/wire_protocols/postgresql_wire_protocol.md`
- `scratchbird/specifications/wire_protocols/scratchbird_native_wire_protocol.md`
- `scratchbird/specifications/wire_protocols/tds_wire_protocol.md`

### storage/

- `storage/EXTENDED_PAGE_SIZES.md`
- `storage/HEAP_TOAST_INTEGRATION.md`
- `storage/MGA_IMPLEMENTATION.md`
- `storage/ON_DISK_FORMAT.md`
- `storage/README.md`
- `storage/STORAGE_ENGINE_BUFFER_POOL.md`
- `storage/STORAGE_ENGINE_MAIN.md`
- `storage/STORAGE_ENGINE_PAGE_MANAGEMENT.md`
- `storage/TABLESPACE_ONLINE_MIGRATION.md`
- `storage/TABLESPACE_SPECIFICATION.md`
- `storage/TOAST_LOB_STORAGE.md`

### testing/

- `testing/ALPHA3_TEST_PLAN.md`

### tools/

- `tools/README.md`
- `tools/SB_BACKUP_CLI_SPECIFICATION.md`
- `tools/SB_ISQL_CLI_SPECIFICATION.md`
- `tools/SB_SECURITY_CLI_SPECIFICATION.md`
- `tools/SB_TOOLING_NETWORK_SPEC.md`
- `tools/SB_VERIFY_CLI_SPECIFICATION.md`

### transaction/

- `transaction/07_TRANSACTION_AND_SESSION_CONTROL.md`
- `transaction/FIREBIRD_GC_SWEEP_GLOSSARY.md`
- `transaction/README.md`
- `transaction/TRANSACTION_DISTRIBUTED.md`
- `transaction/TRANSACTION_LOCK_MANAGER.md`
- `transaction/TRANSACTION_MAIN.md`
- `transaction/TRANSACTION_MGA_CORE.md`

### triggers/

- `triggers/README.md`
- `triggers/TRIGGER_CONTEXT_VARIABLES.md`

### types/

- `types/03_TYPES_AND_DOMAINS.md`
- `types/COLLATION_TAILORING_LOADER_SPEC.md`
- `types/DATA_TYPE_PERSISTENCE_AND_CASTS.md`
- `types/DDL_DOMAINS_COMPREHENSIVE.md`
- `types/I18N_CANONICAL_LISTS.md`
- `types/MULTI_GEOMETRY_TYPES_SPEC.md`
- `types/POSTGRESQL_ARRAY_TYPE_SPEC.md`
- `types/README.md`
- `types/TIMEZONE_SYSTEM_CATALOG.md`
- `types/UUID_IDENTITY_COLUMNS.md`
- `types/character_sets_and_collations.md`

### udr/

- `udr/10-UDR-System-Specification.md`
- `udr/README.md`
- `udr/UDR_PSQL_EXTENSION_LIBRARY.md`

### udr_connectors/

- `udr_connectors/README.md`
- `udr_connectors/UDR_CONNECTOR_BASELINE.md`
- `udr_connectors/firebird_udr/SPECIFICATION.md`
- `udr_connectors/jdbc_udr/SPECIFICATION.md`
- `udr_connectors/local_files_udr/SPECIFICATION.md`
- `udr_connectors/local_scripts_udr/SPECIFICATION.md`
- `udr_connectors/mssql_udr/SPECIFICATION.md`
- `udr_connectors/mysql_udr/SPECIFICATION.md`
- `udr_connectors/odbc_udr/SPECIFICATION.md`
- `udr_connectors/postgresql_udr/SPECIFICATION.md`
- `udr_connectors/scratchbird_udr/SPECIFICATION.md`

### wire_protocols/

- `wire_protocols/FIREBIRD_EMULATION_BEHAVIOR.md`
- `wire_protocols/MYSQL_EMULATION_BEHAVIOR.md`
- `wire_protocols/POSTGRESQL_EMULATION_BEHAVIOR.md`
- `wire_protocols/README.md`
- `wire_protocols/firebird_wire_protocol.md`
- `wire_protocols/mysql_wire_protocol.md`
- `wire_protocols/postgresql_wire_protocol.md`
- `wire_protocols/scratchbird_native_wire_protocol.md`
- `wire_protocols/tds_wire_protocol.md`

## ScratchRobin Authored / Divergent

### Cluster%20Specification%20Work/

- `Cluster%20Specification%20Work/SBCLUSTER-11-SHARD-MIGRATION-AND-REBALANCING.md`

### core/

- `core/AUDIT_LOGGING.md`
- `core/CONNECTION_PROFILE_EDITOR.md`
- `core/CREDENTIAL_STORAGE_POLICY.md`
- `core/ERROR_HANDLING.md`
- `core/OBJECT_STATE_MACHINE.md`
- `core/PROJECT_MIGRATION_POLICY.md`
- `core/PROJECT_ON_DISK_LAYOUT.md`
- `core/PROJECT_PERSISTENCE_FORMAT.md`
- `core/README.md`
- `core/SESSION_PROJECT_INTERACTION.md`
- `core/SESSION_STATE.md`
- `core/TRANSACTION_MANAGEMENT.md`

### deployment/

- `deployment/DEPLOYMENT_PLAN_FORMAT.md`
- `deployment/MIGRATION_GENERATION.md`
- `deployment/README.md`
- `deployment/ROLLBACK_POLICY.md`

### diagramming/

- `diagramming/AUTO_LAYOUT.md`
- `diagramming/DIAGRAM_ROUNDTRIP_RULES.md`
- `diagramming/DIAGRAM_SERIALIZATION_FORMAT.md`
- `diagramming/ERD_MODELING_AND_ENGINEERING.md`
- `diagramming/ERD_NOTATION_DICTIONARIES.md`
- `diagramming/ERD_TOOLING_RESEARCH.md`
- `diagramming/MIND_MAP_SPECIFICATION.md`
- `diagramming/README.md`
- `diagramming/SILVERSTON_DIAGRAM_SPECIFICATION.md`
- `diagramming/UNDO_REDO.md`

### dialect/

- `dialect/README.md`
- `dialect/scratchbird/README.md`
- `dialect/scratchbird/alpha/README.md`
- `dialect/scratchbird/alpha/alpha_1_05_design_synthesis.md`
- `dialect/scratchbird/alpha/alpha_1_05_outstanding_decisions.md`
- `dialect/scratchbird/alpha/alpha_1_05_sql_parser_design_decisions.md`
- `dialect/scratchbird/beta/BETA_SQL2023_IMPLEMENTATION_SPECIFICATION.md`
- `dialect/scratchbird/beta/README.md`
- `dialect/scratchbird/catalog/COMPONENT_MODEL_AND_RESPONSIBILITIES.md`
- `dialect/scratchbird/catalog/README.md`
- `dialect/scratchbird/catalog/SCHEMA_PATH_RESOLUTION.md`
- `dialect/scratchbird/catalog/SYSTEM_CATALOG_STRUCTURE.md`
- `dialect/scratchbird/parser/01_SQL_DIALECT_OVERVIEW.md`
- `dialect/scratchbird/parser/05_PSQL_PROCEDURAL_LANGUAGE.md`
- `dialect/scratchbird/parser/08_PARSER_AND_DEVELOPER_EXPERIENCE.md`
- `dialect/scratchbird/parser/EMULATED_DATABASE_PARSER_SPECIFICATION.md`
- `dialect/scratchbird/parser/MYSQL_PARSER_SPECIFICATION.md`
- `dialect/scratchbird/parser/POSTGRESQL_PARSER_IMPLEMENTATION.md`
- `dialect/scratchbird/parser/POSTGRESQL_PARSER_SPECIFICATION.md`
- `dialect/scratchbird/parser/PSQL_CURSOR_HANDLES.md`
- `dialect/scratchbird/parser/README.md`
- `dialect/scratchbird/parser/SCRATCHBIRD_SQL_COMPLETE_BNF.md`
- `dialect/scratchbird/parser/SCRATCHBIRD_SQL_CORE_LANGUAGE.md`
- `dialect/scratchbird/parser/SCRATCHBIRD_UNIFIED_NOSQL_EXTENSIONS.md`
- `dialect/scratchbird/parser/ScratchBird Master Grammar Specification v2.0.md`
- `dialect/scratchbird/parser/ScratchBird SQL Language Specification - Master Document.md`

### git/

- `git/GIT_CONFLICT_RESOLUTION.md`
- `git/GIT_INTEGRATION_UI.md`
- `git/README.md`

### integrations/

- `integrations/AI_INTEGRATION_SCOPE.md`
- `integrations/README.md`

### managers/

- `managers/BACKUP_RESTORE_UI.md`
- `managers/CLUSTER_MANAGER_UI.md`
- `managers/DATABASE_ADMINISTRATION_SPEC.md`
- `managers/DOMAIN_MANAGER_UI.md`
- `managers/ETL_MANAGER_UI.md`
- `managers/INDEX_DESIGNER_UI.md`
- `managers/JOB_SCHEDULER_UI.md`
- `managers/PROCEDURE_MANAGER_UI.md`
- `managers/README.md`
- `managers/REPLICATION_MANAGER_UI.md`
- `managers/SCHEMA_MANAGER_UI.md`
- `managers/SEQUENCE_MANAGER_UI.md`
- `managers/TABLE_DESIGNER_UI.md`
- `managers/TRIGGER_MANAGER_UI.md`
- `managers/VIEW_MANAGER_UI.md`

### root/

- `COPY_REPORT.md`
- `INDEX.md`
- `MIRROR_NOTICE.md`
- `README.md`
- `SPEC_MAP.md`
- `SPEC_MAP.mmd`
- `SPEC_MAP.png`
- `SPEC_MAP.svg`
- `SPEC_MAP_LEGEND.md`

### sql/

- `sql/DDL_GENERATION_RULES.md`
- `sql/README.md`
- `sql/SCRATCHROBIN_SQL_SURFACE.md`
- `sql/managers/DOMAINS_SQL_SURFACE.md`
- `sql/managers/INDEXES_SQL_SURFACE.md`
- `sql/managers/PROCEDURES_FUNCTIONS_SQL_SURFACE.md`
- `sql/managers/README.md`
- `sql/managers/SCHEMAS_SQL_SURFACE.md`
- `sql/managers/SEQUENCES_SQL_SURFACE.md`
- `sql/managers/TABLES_SQL_SURFACE.md`
- `sql/managers/TRIGGERS_SQL_SURFACE.md`
- `sql/managers/VIEWS_SQL_SURFACE.md`

### testing/

- `testing/PERFORMANCE_BENCHMARKS.md`
- `testing/PERFORMANCE_TARGETS.md`
- `testing/README.md`
- `testing/TEST_FIXTURE_STRATEGY.md`
- `testing/TEST_GENERATION_RULES.md`

### ui/

- `ui/ACCESSIBILITY_AND_INPUT.md`
- `ui/CONTEXT_SENSITIVE_HELP.md`
- `ui/CROSS_PLATFORM_UI_CONSTRAINTS.md`
- `ui/KEYBOARD_SHORTCUTS.md`
- `ui/PREFERENCES.md`
- `ui/README.md`
- `ui/UI_ICON_ASSETS.md`
- `ui/UI_WINDOW_MODEL.md`

## Redirect Stubs

### api/

- `api/CLIENT_LIBRARY_API_SPECIFICATION.md`

### beta_requirements/

- `beta_requirements/replication/00_BETA_REPLICATION_INDEX.md`

### ddl/

- `ddl/DDL_DEFERRABLE_CONSTRAINTS.md`

### dml/

- `dml/DML_FULLTEXT_SEARCH.md`
- `dml/DML_MATCH_RECOGNIZE.md`
- `dml/DML_SELECT_GROUPING.md`

### root/

- `Appendix_A_SBLR_BYTECODE.md`
- `DATA_TYPE_PERSISTENCE_AND_CASTS.md`
- `DraftQueryOptimizationSpecification.md`
- `EMULATED_DATABASE_PARSER_SPECIFICATION.md`
- `FIREBIRD_BLR_FIXTURES.md`
- `FIREBIRD_BLR_TO_SBLR_MAPPING.md`
- `FIREBIRD_TRANSACTION_MODEL_SPEC.md`
- `INDEX_ARCHITECTURE.md`
- `MGA_IMPLEMENTATION.md`
- `SBLR_DOMAIN_PAYLOADS.md`
- `SBLR_OPCODE_REGISTRY.md`
- `SCHEMA_ARCHITECTURE.md`
- `SCHEMA_PATH_RESOLUTION.md`
- `TRANSACTION_MGA_CORE.md`

### wire_protocols/

- `wire_protocols/*.md`

