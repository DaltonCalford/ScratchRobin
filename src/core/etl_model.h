/**
 * @file etl_model.h
 * @brief Data models for ETL (Extract, Transform, Load) tools (Beta placeholder)
 * 
 * This file defines the data structures for ETL job definitions,
 * data mappings, and workflow orchestration.
 * 
 * Status: Beta Placeholder - UI structure only
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <map>
#include <variant>

namespace scratchrobin {

// Forward declarations
struct DataSource;
struct DataTarget;
struct TransformRule;

/**
 * @brief ETL job execution status
 */
enum class EtlJobStatus {
    Draft,          // Not yet saved/validated
    Ready,          // Ready to run
    Running,        // Currently executing
    Paused,         // Execution paused
    Completed,      // Successfully finished
    Failed,         // Execution failed
    Cancelled,      // User cancelled
    Scheduled       // Waiting for scheduled time
};

/**
 * @brief Data source types
 */
enum class SourceType {
    DatabaseTable,
    DatabaseQuery,
    CsvFile,
    JsonFile,
    XmlFile,
    ExcelFile,
    ParquetFile,
    ApiEndpoint,
    MessageQueue,
    Stream
};

/**
 * @brief Data target types
 */
enum class TargetType {
    DatabaseTable,
    DatabaseQuery,  // For INSERT...SELECT style
    CsvFile,
    JsonFile,
    XmlFile,
    ExcelFile,
    ParquetFile,
    ApiEndpoint,
    MessageQueue,
    Stream
};

/**
 * @brief Transformation operation types
 */
enum class TransformType {
    // Column operations
    Map,            // Direct column mapping
    Rename,         // Rename column
    Cast,           // Type conversion
    Default,        // Set default value
    Calculated,     // Computed column
    
    // Row operations
    Filter,         // Row filtering
    Sort,           // Ordering
    Deduplicate,    // Remove duplicates
    
    // Data cleansing
    Trim,           // Whitespace removal
    Normalize,      // Value normalization
    Validate,       // Data validation
    Replace,        // Find and replace
    RegexReplace,   // Regex-based replacement
    NullHandling,   // NULL value handling
    
    // Aggregation
    Aggregate,      // Group by + aggregates
    Pivot,          // Pivot operation
    Unpivot,        // Unpivot operation
    
    // Advanced
    CustomSql,      // Custom SQL transformation
    Script,         // Custom script (Python/JS)
    Lookup,         // Lookup/reference data
    Join,           // Join with another source
    Union,          // Union with another source
    Split           // Split into multiple outputs
};

/**
 * @brief Data source configuration
 */
struct DataSource {
    std::string sourceId;
    std::string name;
    SourceType type = SourceType::DatabaseTable;
    
    // Connection reference (for database sources)
    std::optional<std::string> connectionProfileId;
    
    // Source-specific configuration structs
    struct DatabaseTableConfig { std::string schema; std::string table; };
    struct DatabaseQueryConfig { std::string sql; };
    struct CsvFileConfig { std::string filePath; std::string encoding; char delimiter; };
    struct JsonFileConfig { std::string filePath; };
    struct ApiEndpointConfig { std::string url; std::string method; std::map<std::string, std::string> headers; };
    
    std::variant<
        std::monostate,
        DatabaseTableConfig,      // DatabaseTable
        DatabaseQueryConfig,      // DatabaseQuery
        CsvFileConfig,            // CsvFile
        JsonFileConfig,           // JsonFile, XmlFile, etc.
        ApiEndpointConfig         // ApiEndpoint
    > config;
    
    // Column definitions (for file sources without schema)
    std::vector<std::map<std::string, std::string>> columnDefs;
    
    // Sampling options
    bool sampleOnly = false;
    std::optional<uint64_t> sampleSize;
    
    // Incremental loading
    bool incremental = false;
    std::optional<std::string> watermarkColumn;
    std::optional<std::string> lastWatermarkValue;
};

/**
 * @brief Data target configuration
 */
struct DataTarget {
    std::string targetId;
    std::string name;
    TargetType type = TargetType::DatabaseTable;
    
    // Connection reference (for database targets)
    std::optional<std::string> connectionProfileId;
    
    // Target-specific configuration structs
    struct TargetDatabaseTableConfig { std::string schema; std::string table; bool createIfNotExists; bool truncateBeforeLoad; };
    struct TargetCsvFileConfig { std::string filePath; std::string encoding; char delimiter; bool includeHeader; };
    struct TargetJsonFileConfig { std::string filePath; bool prettyPrint; };
    struct TargetExcelFileConfig { std::string filePath; std::string sheetName; };
    
    std::variant<
        std::monostate,
        TargetDatabaseTableConfig,  // DatabaseTable
        TargetCsvFileConfig,        // CsvFile
        TargetJsonFileConfig,       // JsonFile
        TargetExcelFileConfig       // ExcelFile
    > config;
    
    // Load options
    uint32_t batchSize = 1000;
    bool useTransactions = true;
    uint32_t maxErrors = 0;  // 0 = fail on first error
    
    // Conflict resolution
    std::string onConflict;  // "abort", "ignore", "replace", "update"
    std::optional<std::vector<std::string>> conflictKeyColumns;
};

/**
 * @brief Column mapping/transform rule
 */
struct TransformRule {
    std::string ruleId;
    TransformType type = TransformType::Map;
    uint32_t executionOrder = 0;
    
    // Source column(s)
    std::vector<std::string> sourceColumns;
    
    // Target column (for column-level transforms)
    std::optional<std::string> targetColumn;
    
    // Transform configuration
    std::map<std::string, std::string> parameters;
    
    // Examples:
    // Cast: {"from_type": "string", "to_type": "integer", "format": "YYYY-MM-DD"}
    // Filter: {"condition": "age > 18 AND status = 'active'"}
    // Calculated: {"expression": "price * quantity * (1 - discount)"}
    // Lookup: {"lookup_table": "countries", "key_column": "code", "value_column": "name"}
    
    // Error handling
    std::string onError = "fail";  // "fail", "skip", "null", "default"
    std::optional<std::string> defaultValue;
    
    // Description
    std::string description;
    bool enabled = true;
};

/**
 * @brief Data quality check
 */
struct DataQualityRule {
    std::string ruleId;
    std::string name;
    std::string description;
    
    // Check configuration
    std::string checkType;  // "not_null", "unique", "range", "pattern", "reference", "custom"
    std::vector<std::string> columns;
    std::map<std::string, std::string> parameters;
    
    // Severity
    std::string severity = "error";  // "error", "warning", "info"
    
    // Thresholds
    std::optional<double> thresholdPercent;  // Allow % of failures
    std::optional<uint64_t> thresholdCount;  // Allow N failures
};

/**
 * @brief ETL job definition
 */
struct EtlJob {
    std::string jobId;
    std::string jobName;
    std::string description;
    std::vector<std::string> tags;
    
    // Status
    EtlJobStatus status = EtlJobStatus::Draft;
    std::string createdBy;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point modifiedAt;
    
    // Source and target
    DataSource source;
    std::vector<DataTarget> targets;  // Multiple targets for fan-out
    
    // Transform pipeline
    std::vector<TransformRule> transforms;
    
    // Data quality
    std::vector<DataQualityRule> qualityRules;
    bool enforceQuality = true;
    
    // Scheduling
    bool scheduled = false;
    std::optional<std::string> cronExpression;
    std::optional<std::string> timezone;
    
    // Runtime options
    uint32_t maxParallelism = 1;
    std::optional<uint64_t> maxMemoryMB;
    std::optional<std::chrono::seconds> timeout;
    
    // Notifications
    std::vector<std::string> notifyOnSuccess;
    std::vector<std::string> notifyOnFailure;
    std::vector<std::string> notifyOnCompletion;
};

/**
 * @brief ETL job execution/run
 */
struct EtlJobRun {
    std::string runId;
    std::string jobId;
    
    // Execution info
    EtlJobStatus status = EtlJobStatus::Scheduled;
    std::chrono::system_clock::time_point startedAt;
    std::optional<std::chrono::system_clock::time_point> completedAt;
    
    // Trigger info
    std::string triggeredBy;  // "manual", "scheduled", "api", "parent"
    std::optional<std::string> parentRunId;  // For dependent jobs
    
    // Statistics
    uint64_t rowsRead = 0;
    uint64_t rowsWritten = 0;
    uint64_t rowsRejected = 0;
    uint64_t bytesProcessed = 0;
    
    // Performance
    std::chrono::milliseconds duration{0};
    std::optional<double> throughputRowsPerSec;
    std::optional<double> throughputBytesPerSec;
    
    // Errors and warnings
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // Data quality results
    uint32_t qualityChecksPassed = 0;
    uint32_t qualityChecksFailed = 0;
    std::vector<std::string> qualityFailures;
};

/**
 * @brief ETL workflow (multi-job pipeline)
 */
struct EtlWorkflow {
    std::string workflowId;
    std::string workflowName;
    std::string description;
    
    // Job DAG (Directed Acyclic Graph)
    struct WorkflowStep {
        std::string stepId;
        std::string jobId;
        std::vector<std::string> dependsOn;  // Step IDs that must complete first
        std::string onSuccess;  // Next step on success (empty = end)
        std::string onFailure;  // Next step on failure (empty = fail workflow)
    };
    std::vector<WorkflowStep> steps;
    
    // Execution settings
    bool continueOnFailure = false;  // Continue to independent branches on failure
    uint32_t maxConcurrentJobs = 1;
};

} // namespace scratchrobin
