// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "report_types.h"
#include "../core/query_types.h"
#include "../core/project.h"

#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin {
namespace reporting {

/**
 * @brief Metadata for a stored result set
 */
struct StoredResultMetadata {
    std::string result_id;           // Unique identifier
    std::string question_id;         // Associated question
    std::string execution_id;        // Execution that produced this result
    std::string connection_ref;      // Connection used
    std::time_t executed_at;         // When executed
    std::time_t expires_at;          // When to delete (based on retention)
    int row_count = 0;
    int column_count = 0;
    size_t size_bytes = 0;
    bool compressed = false;
    bool encrypted = false;
    std::string storage_location;    // Table name or file path
    std::map<std::string, std::string> tags;  // For categorization
    std::optional<std::string> parameters_hash;  // To identify parameter sets
};

/**
 * @brief Handle to a stored result set
 */
struct StoredResultHandle {
    std::string result_id;
    std::string storage_location;
    bool valid = false;
};

/**
 * @brief Query for retrieving historical results
 */
struct HistoricalResultQuery {
    std::optional<std::string> question_id;
    std::optional<std::time_t> from_date;
    std::optional<std::time_t> to_date;
    std::optional<std::string> connection_ref;
    std::vector<std::string> tags;
    int limit = 100;
    int offset = 0;
};

/**
 * @brief Configuration for result storage
 */
struct ResultStorageConfig {
    ProjectConfig::ReportingStorage storage_config;
    std::string project_root_path;
};

/**
 * @brief Persistent result storage for reporting
 * 
 * Stores query results for long-term retention and historical analysis.
 * Supports embedded SQLite, external database, or S3-compatible storage.
 */
class ResultStorage {
public:
    explicit ResultStorage(const ResultStorageConfig& config);
    ~ResultStorage();
    
    // Lifecycle
    bool Initialize();
    bool Shutdown();
    bool IsInitialized() const { return initialized_; }
    
    // Store a result set
    StoredResultHandle StoreResult(const QueryResult& result,
                                   const std::string& question_id,
                                   const std::string& execution_id,
                                   const std::map<std::string, std::string>& parameters,
                                   const std::vector<std::string>& tags = {});
    
    // Retrieve a stored result
    std::optional<QueryResult> RetrieveResult(const std::string& result_id);
    std::optional<QueryResult> RetrieveResult(const StoredResultHandle& handle);
    
    // Metadata operations
    std::optional<StoredResultMetadata> GetMetadata(const std::string& result_id);
    std::vector<StoredResultMetadata> QueryMetadata(const HistoricalResultQuery& query);
    
    // Delete operations
    bool DeleteResult(const std::string& result_id);
    bool DeleteResultsForQuestion(const std::string& question_id);
    int DeleteExpiredResults();
    int DeleteResultsBefore(std::time_t before_date);
    
    // Comparison and analysis
    struct ResultComparison {
        std::string baseline_result_id;
        std::string compare_result_id;
        struct Difference {
            std::string column;
            double baseline_value;
            double compare_value;
            double absolute_diff;
            double percent_diff;
        };
        std::vector<Difference> differences;
        bool has_significant_changes = false;
    };
    
    std::optional<ResultComparison> CompareResults(const std::string& result_id_1,
                                                    const std::string& result_id_2,
                                                    const std::vector<std::string>& key_columns);
    
    // Trend analysis
    struct TrendPoint {
        std::time_t timestamp;
        double value;
        std::optional<double> change_from_previous;
    };
    
    std::vector<TrendPoint> AnalyzeTrend(const std::string& question_id,
                                          const std::string& value_column,
                                          const std::optional<std::time_t>& from_date = std::nullopt,
                                          const std::optional<std::time_t>& to_date = std::nullopt);
    
    // Statistics
    struct StorageStats {
        size_t total_results = 0;
        size_t total_size_bytes = 0;
        size_t results_by_question = 0;
        std::time_t oldest_result = 0;
        std::time_t newest_result = 0;
        std::map<std::string, size_t> results_per_question;
    };
    StorageStats GetStats() const;
    StorageStats GetStatsForQuestion(const std::string& question_id) const;
    
    // Maintenance
    bool CompactStorage();
    bool VerifyIntegrity();
    std::optional<std::string> ExportResults(const std::string& question_id,
                                              const std::string& format = "csv");
    
    // Retention management
    void EnforceRetentionPolicy();
    void SetRetentionDays(uint32_t days);
    
    // Storage backend interface (public so derived classes can use it)
    class StorageBackend {
    public:
        virtual ~StorageBackend() = default;
        virtual bool Initialize(const ResultStorageConfig& config) = 0;
        virtual bool Shutdown() = 0;
        virtual StoredResultHandle Store(const QueryResult& result,
                                          const StoredResultMetadata& metadata) = 0;
        virtual std::optional<QueryResult> Retrieve(const std::string& result_id) = 0;
        virtual std::optional<StoredResultMetadata> GetMetadata(const std::string& result_id) = 0;
        virtual std::vector<StoredResultMetadata> QueryMetadata(const HistoricalResultQuery& query) = 0;
        virtual bool Delete(const std::string& result_id) = 0;
        virtual StorageStats GetStats() const = 0;
    };
    
protected:
    ResultStorageConfig config_;
    std::atomic<bool> initialized_{false};
    mutable std::mutex mutex_;
    std::unique_ptr<StorageBackend> backend_;
    
    // Backend factory
    std::unique_ptr<StorageBackend> CreateBackend(const std::string& type);
};

/**
 * @brief Embedded SQLite storage backend
 */
class EmbeddedStorageBackend : public ResultStorage::StorageBackend {
public:
    bool Initialize(const ResultStorageConfig& config) override;
    bool Shutdown() override;
    StoredResultHandle Store(const QueryResult& result,
                             const StoredResultMetadata& metadata) override;
    std::optional<QueryResult> Retrieve(const std::string& result_id) override;
    std::optional<StoredResultMetadata> GetMetadata(const std::string& result_id) override;
    std::vector<StoredResultMetadata> QueryMetadata(const HistoricalResultQuery& query) override;
    bool Delete(const std::string& result_id) override;
    ResultStorage::StorageStats GetStats() const override;
    
private:
    ResultStorageConfig config_;
    std::string db_path_;
    void* db_handle_ = nullptr;  // sqlite3*
    
    bool CreateSchema();
    bool StoreMetadata(const StoredResultMetadata& metadata);
    bool StoreData(const std::string& result_id, const QueryResult& result);
    std::string GenerateTableName(const std::string& result_id);
};

/**
 * @brief External database storage backend
 */
class ExternalStorageBackend : public ResultStorage::StorageBackend {
public:
    bool Initialize(const ResultStorageConfig& config) override;
    bool Shutdown() override;
    StoredResultHandle Store(const QueryResult& result,
                             const StoredResultMetadata& metadata) override;
    std::optional<QueryResult> Retrieve(const std::string& result_id) override;
    std::optional<StoredResultMetadata> GetMetadata(const std::string& result_id) override;
    std::vector<StoredResultMetadata> QueryMetadata(const HistoricalResultQuery& query) override;
    bool Delete(const std::string& result_id) override;
    ResultStorage::StorageStats GetStats() const override;
    
private:
    ResultStorageConfig config_;
    std::string schema_name_;
    std::string table_prefix_;
};

/**
 * @brief Result storage manager singleton
 */
class ResultStorageManager {
public:
    static ResultStorageManager& Instance();
    
    void Initialize(Project* project);
    void Shutdown();
    
    ResultStorage* GetStorage() { return storage_.get(); }
    bool IsEnabled() const;
    
    // Configuration
    void UpdateConfig(const ProjectConfig::ReportingStorage& config);
    ProjectConfig::ReportingStorage GetConfig() const;
    
private:
    ResultStorageManager() = default;
    std::unique_ptr<ResultStorage> storage_;
    Project* project_ = nullptr;
};

/**
 * @brief Long-running report builder
 * 
 * Builds reports that span multiple result sets over time.
 */
class LongDurationReportBuilder {
public:
    explicit LongDurationReportBuilder(ResultStorage* storage);
    
    struct ReportDefinition {
        std::string name;
        std::string question_id;
        std::string aggregation_column;
        std::string aggregation_function;  // SUM, AVG, COUNT, etc.
        std::optional<std::string> group_by_column;
        std::time_t start_date;
        std::time_t end_date;
        std::string time_granularity;  // hourly, daily, weekly, monthly
    };
    
    struct TimeSeriesPoint {
        std::time_t timestamp;
        double value;
        int sample_count;
        double min_value;
        double max_value;
        std::optional<double> change_from_previous;
    };
    
    struct LongDurationReport {
        ReportDefinition definition;
        std::vector<TimeSeriesPoint> series;
        double total;
        double average;
        double trend_slope;
        std::optional<std::time_t> peak_time;
        double peak_value;
    };
    
    std::optional<LongDurationReport> BuildReport(const ReportDefinition& definition);
    
    // Export options
    bool ExportToCsv(const LongDurationReport& report, const std::string& path);
    bool ExportToChart(const LongDurationReport& report, const std::string& path, 
                       const std::string& format = "png");
    
private:
    ResultStorage* storage_;
    
    std::vector<StoredResultMetadata> CollectRelevantResults(const ReportDefinition& definition);
    TimeSeriesPoint AggregateResults(const std::vector<StoredResultMetadata>& results,
                                     const ReportDefinition& definition);
};

} // namespace reporting
} // namespace scratchrobin
