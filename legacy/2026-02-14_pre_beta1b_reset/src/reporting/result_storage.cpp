// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "result_storage.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace scratchrobin {
namespace reporting {

// ResultStorage implementation

ResultStorage::ResultStorage(const ResultStorageConfig& config) : config_(config) {}

ResultStorage::~ResultStorage() {
    Shutdown();
}

bool ResultStorage::Initialize() {
    if (initialized_) return true;
    
    backend_ = CreateBackend(config_.storage_config.storage_type);
    if (!backend_) return false;
    
    initialized_ = backend_->Initialize(config_);
    return initialized_;
}

bool ResultStorage::Shutdown() {
    if (!initialized_) return true;
    
    if (backend_) {
        backend_->Shutdown();
        backend_.reset();
    }
    
    initialized_ = false;
    return true;
}

StoredResultHandle ResultStorage::StoreResult(const QueryResult& result,
                                               const std::string& question_id,
                                               const std::string& execution_id,
                                               const std::map<std::string, std::string>& parameters,
                                               const std::vector<std::string>& tags) {
    if (!initialized_) return StoredResultHandle{};
    
    // Generate result ID
    std::stringstream ss;
    ss << question_id << "_" << execution_id << "_" << std::time(nullptr);
    std::string result_id = ss.str();
    
    // Create metadata
    StoredResultMetadata metadata;
    metadata.result_id = result_id;
    metadata.question_id = question_id;
    metadata.execution_id = execution_id;
    metadata.executed_at = std::time(nullptr);
    metadata.expires_at = metadata.executed_at + (config_.storage_config.retention_days * 86400);
    metadata.row_count = static_cast<int>(result.rows.size());
    metadata.column_count = static_cast<int>(result.columns.size());
    metadata.compressed = config_.storage_config.compress_results;
    metadata.encrypted = config_.storage_config.encrypt_results;
    
    // Calculate size
    size_t total_size = 0;
    for (const auto& col : result.columns) {
        total_size += col.name.size();
    }
    for (const auto& row : result.rows) {
        for (const auto& cell : row) {
            total_size += cell.text.size();
        }
    }
    metadata.size_bytes = total_size;
    
    // Hash parameters
    if (!parameters.empty()) {
        std::stringstream param_ss;
        for (const auto& [k, v] : parameters) {
            param_ss << k << "=" << v << "&";
        }
        metadata.parameters_hash = param_ss.str();
    }
    
    // Add tags
    for (const auto& tag : tags) {
        metadata.tags[tag] = tag;
    }
    
    // Store via backend
    return backend_->Store(result, metadata);
}

std::optional<QueryResult> ResultStorage::RetrieveResult(const std::string& result_id) {
    if (!initialized_) return std::nullopt;
    return backend_->Retrieve(result_id);
}

std::optional<QueryResult> ResultStorage::RetrieveResult(const StoredResultHandle& handle) {
    if (!handle.valid) return std::nullopt;
    return RetrieveResult(handle.result_id);
}

std::optional<StoredResultMetadata> ResultStorage::GetMetadata(const std::string& result_id) {
    if (!initialized_) return std::nullopt;
    return backend_->GetMetadata(result_id);
}

std::vector<StoredResultMetadata> ResultStorage::QueryMetadata(const HistoricalResultQuery& query) {
    if (!initialized_) return {};
    return backend_->QueryMetadata(query);
}

bool ResultStorage::DeleteResult(const std::string& result_id) {
    if (!initialized_) return false;
    return backend_->Delete(result_id);
}

bool ResultStorage::DeleteResultsForQuestion(const std::string& question_id) {
    HistoricalResultQuery query;
    query.question_id = question_id;
    auto results = QueryMetadata(query);
    
    bool all_deleted = true;
    for (const auto& metadata : results) {
        if (!DeleteResult(metadata.result_id)) {
            all_deleted = false;
        }
    }
    return all_deleted;
}

int ResultStorage::DeleteExpiredResults() {
    HistoricalResultQuery query;
    query.to_date = std::time(nullptr);
    auto results = QueryMetadata(query);
    
    int deleted = 0;
    for (const auto& metadata : results) {
        if (metadata.expires_at <= std::time(nullptr)) {
            if (DeleteResult(metadata.result_id)) {
                deleted++;
            }
        }
    }
    return deleted;
}

int ResultStorage::DeleteResultsBefore(std::time_t before_date) {
    HistoricalResultQuery query;
    query.to_date = before_date;
    auto results = QueryMetadata(query);
    
    int deleted = 0;
    for (const auto& metadata : results) {
        if (DeleteResult(metadata.result_id)) {
            deleted++;
        }
    }
    return deleted;
}

ResultStorage::StorageStats ResultStorage::GetStats() const {
    if (!initialized_) return StorageStats{};
    return backend_->GetStats();
}

void ResultStorage::EnforceRetentionPolicy() {
    DeleteExpiredResults();
}

void ResultStorage::SetRetentionDays(uint32_t days) {
    config_.storage_config.retention_days = days;
}

std::unique_ptr<ResultStorage::StorageBackend> ResultStorage::CreateBackend(
    const std::string& type) {
    if (type == "embedded") {
        return std::make_unique<EmbeddedStorageBackend>();
    } else if (type == "external") {
        return std::make_unique<ExternalStorageBackend>();
    }
    // Default to embedded
    return std::make_unique<EmbeddedStorageBackend>();
}

// EmbeddedStorageBackend implementation

bool EmbeddedStorageBackend::Initialize(const ResultStorageConfig& config) {
    config_ = config;
    
    // Determine database path
    if (config_.storage_config.database_path.empty()) {
        db_path_ = config_.project_root_path + "/.scratchrobin/reporting_results.db";
    } else {
        db_path_ = config_.project_root_path + "/" + config_.storage_config.database_path;
    }
    
    // Create directory if needed
    std::filesystem::path db_dir = std::filesystem::path(db_path_).parent_path();
    if (!db_dir.empty() && !std::filesystem::exists(db_dir)) {
        std::filesystem::create_directories(db_dir);
    }
    
    // Initialize SQLite (simplified - real implementation would use sqlite3_open)
    // For now, create a marker file
    std::ofstream marker(db_path_ + ".initialized");
    marker << "initialized";
    marker.close();
    
    return CreateSchema();
}

bool EmbeddedStorageBackend::Shutdown() {
    db_handle_ = nullptr;
    return true;
}

bool EmbeddedStorageBackend::CreateSchema() {
    // In real implementation, create tables:
    // - result_metadata: id, question_id, execution_id, executed_at, expires_at, etc.
    // - result_data: result_id, row_data (JSON or binary)
    // - result_columns: result_id, column_name, column_type, column_index
    return true;
}

StoredResultHandle EmbeddedStorageBackend::Store(const QueryResult& result,
                                                  const StoredResultMetadata& metadata) {
    StoredResultHandle handle;
    handle.result_id = metadata.result_id;
    handle.storage_location = GenerateTableName(metadata.result_id);
    
    if (!StoreMetadata(metadata)) {
        return StoredResultHandle{};
    }
    
    if (!StoreData(metadata.result_id, result)) {
        return StoredResultHandle{};
    }
    
    handle.valid = true;
    return handle;
}

std::optional<QueryResult> EmbeddedStorageBackend::Retrieve(const std::string& result_id) {
    // In real implementation:
    // 1. Look up metadata
    // 2. Retrieve row data
    // 3. Reconstruct QueryResult
    return std::nullopt;
}

std::optional<StoredResultMetadata> EmbeddedStorageBackend::GetMetadata(
    const std::string& result_id) {
    // Query metadata table
    return std::nullopt;
}

std::vector<StoredResultMetadata> EmbeddedStorageBackend::QueryMetadata(
    const HistoricalResultQuery& query) {
    std::vector<StoredResultMetadata> results;
    // Query with filters
    return results;
}

bool EmbeddedStorageBackend::Delete(const std::string& result_id) {
    // Delete from metadata and data tables
    return true;
}

ResultStorage::StorageStats EmbeddedStorageBackend::GetStats() const {
    ResultStorage::StorageStats stats;
    // Query metadata table for counts
    return stats;
}

bool EmbeddedStorageBackend::StoreMetadata(const StoredResultMetadata& metadata) {
    // Insert into metadata table
    return true;
}

bool EmbeddedStorageBackend::StoreData(const std::string& result_id, 
                                        const QueryResult& result) {
    // Insert into data table
    return true;
}

std::string EmbeddedStorageBackend::GenerateTableName(const std::string& result_id) {
    // Sanitize result_id for use as table name
    std::string sanitized;
    for (char c : result_id) {
        if (std::isalnum(c) || c == '_') {
            sanitized += c;
        } else {
            sanitized += '_';
        }
    }
    return config_.storage_config.table_prefix + "data_" + sanitized;
}

// ExternalStorageBackend implementation

bool ExternalStorageBackend::Initialize(const ResultStorageConfig& config) {
    config_ = config;
    schema_name_ = config_.storage_config.schema_name;
    table_prefix_ = config_.storage_config.table_prefix;
    return true;
}

bool ExternalStorageBackend::Shutdown() {
    return true;
}

StoredResultHandle ExternalStorageBackend::Store(const QueryResult& result,
                                                  const StoredResultMetadata& metadata) {
    // Store in external database
    StoredResultHandle handle;
    handle.result_id = metadata.result_id;
    handle.storage_location = schema_name_ + "." + table_prefix_ + metadata.result_id;
    handle.valid = true;
    return handle;
}

std::optional<QueryResult> ExternalStorageBackend::Retrieve(const std::string& result_id) {
    // Retrieve from external database
    return std::nullopt;
}

std::optional<StoredResultMetadata> ExternalStorageBackend::GetMetadata(
    const std::string& result_id) {
    return std::nullopt;
}

std::vector<StoredResultMetadata> ExternalStorageBackend::QueryMetadata(
    const HistoricalResultQuery& query) {
    return {};
}

bool ExternalStorageBackend::Delete(const std::string& result_id) {
    return true;
}

ResultStorage::StorageStats ExternalStorageBackend::GetStats() const {
    return {};
}

// ResultStorageManager implementation

ResultStorageManager& ResultStorageManager::Instance() {
    static ResultStorageManager instance;
    return instance;
}

void ResultStorageManager::Initialize(Project* project) {
    project_ = project;
    if (!project_) return;
    
    ResultStorageConfig config;
    config.storage_config = project_->config.reporting_storage;
    config.project_root_path = project_->project_root_path;
    
    storage_ = std::make_unique<ResultStorage>(config);
    storage_->Initialize();
}

void ResultStorageManager::Shutdown() {
    if (storage_) {
        storage_->Shutdown();
        storage_.reset();
    }
    project_ = nullptr;
}

bool ResultStorageManager::IsEnabled() const {
    return storage_ && storage_->IsInitialized() && 
           project_ && project_->config.reporting_storage.enabled;
}

void ResultStorageManager::UpdateConfig(const ProjectConfig::ReportingStorage& config) {
    if (project_) {
        project_->config.reporting_storage = config;
        // Reinitialize with new config
        Shutdown();
        Initialize(project_);
    }
}

ProjectConfig::ReportingStorage ResultStorageManager::GetConfig() const {
    if (project_) {
        return project_->config.reporting_storage;
    }
    return {};
}

// LongDurationReportBuilder implementation

LongDurationReportBuilder::LongDurationReportBuilder(ResultStorage* storage)
    : storage_(storage) {}

std::optional<LongDurationReportBuilder::LongDurationReport> 
LongDurationReportBuilder::BuildReport(const ReportDefinition& definition) {
    if (!storage_) return std::nullopt;
    
    LongDurationReport report;
    report.definition = definition;
    
    // Collect relevant results
    auto results = CollectRelevantResults(definition);
    if (results.empty()) return std::nullopt;
    
    // Build time series
    // Group results by time granularity and aggregate
    std::map<std::time_t, std::vector<StoredResultMetadata>> grouped;
    for (const auto& result : results) {
        std::time_t bucket = result.executed_at;
        // Round to granularity
        if (definition.time_granularity == "daily") {
            bucket = bucket - (bucket % 86400);
        } else if (definition.time_granularity == "hourly") {
            bucket = bucket - (bucket % 3600);
        } else if (definition.time_granularity == "weekly") {
            bucket = bucket - (bucket % 604800);
        }
        grouped[bucket].push_back(result);
    }
    
    // Aggregate each group
    double total = 0;
    double prev_value = 0;
    for (const auto& [timestamp, group] : grouped) {
        auto point = AggregateResults(group, definition);
        if (prev_value != 0) {
            point.change_from_previous = point.value - prev_value;
        }
        prev_value = point.value;
        total += point.value;
        report.series.push_back(point);
    }
    
    // Calculate statistics
    if (!report.series.empty()) {
        report.total = total;
        report.average = total / report.series.size();
        
        // Find peak
        auto peak_it = std::max_element(report.series.begin(), report.series.end(),
            [](const auto& a, const auto& b) { return a.value < b.value; });
        if (peak_it != report.series.end()) {
            report.peak_time = peak_it->timestamp;
            report.peak_value = peak_it->value;
        }
        
        // Calculate trend slope (simplified)
        if (report.series.size() >= 2) {
            double first_val = report.series.front().value;
            double last_val = report.series.back().value;
            report.trend_slope = (last_val - first_val) / report.series.size();
        }
    }
    
    return report;
}

std::vector<StoredResultMetadata> LongDurationReportBuilder::CollectRelevantResults(
    const ReportDefinition& definition) {
    HistoricalResultQuery query;
    query.question_id = definition.question_id;
    query.from_date = definition.start_date;
    query.to_date = definition.end_date;
    
    return storage_->QueryMetadata(query);
}

LongDurationReportBuilder::TimeSeriesPoint LongDurationReportBuilder::AggregateResults(
    const std::vector<StoredResultMetadata>& results,
    const ReportDefinition& definition) {
    TimeSeriesPoint point;
    if (results.empty()) return point;
    
    point.timestamp = results.front().executed_at;
    point.sample_count = static_cast<int>(results.size());
    
    // In real implementation, would retrieve actual data and aggregate
    // For now, use placeholder
    point.value = 0;
    point.min_value = 0;
    point.max_value = 0;
    
    return point;
}

bool LongDurationReportBuilder::ExportToCsv(const LongDurationReport& report,
                                            const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    // Write header
    file << "Timestamp,Value,SampleCount,MinValue,MaxValue,ChangeFromPrevious\n";
    
    // Write data
    for (const auto& point : report.series) {
        file << point.timestamp << ","
             << point.value << ","
             << point.sample_count << ","
             << point.min_value << ","
             << point.max_value << ",";
        if (point.change_from_previous) {
            file << *point.change_from_previous;
        }
        file << "\n";
    }
    
    return true;
}

bool LongDurationReportBuilder::ExportToChart(const LongDurationReport& report,
                                               const std::string& path,
                                               const std::string& format) {
    // In real implementation, generate chart image
    return true;
}

} // namespace reporting
} // namespace scratchrobin
