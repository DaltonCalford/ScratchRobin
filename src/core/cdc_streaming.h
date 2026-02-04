/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CDC_STREAMING_H
#define SCRATCHROBIN_CDC_STREAMING_H

#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin {

// ============================================================================
// CDC Event Types
// ============================================================================
enum class CdcEventType {
    INSERT,
    UPDATE,
    DELETE,
    TRUNCATE,
    BEGIN_TRANSACTION,
    COMMIT_TRANSACTION,
    ROLLBACK_TRANSACTION
};

std::string CdcEventTypeToString(CdcEventType type);

// ============================================================================
// CDC Event
// ============================================================================
struct CdcEvent {
    // Event metadata
    std::string event_id;
    CdcEventType type;
    std::time_t timestamp;
    int64_t sequence_number;
    
    // Source location
    std::string database;
    std::string schema;
    std::string table;
    
    // Transaction context
    std::string transaction_id;
    int64_t position;  // Log position (LSN, offset, etc.)
    
    // Row data
    struct RowData {
        std::map<std::string, std::string> columns;
        std::map<std::string, std::string> old_columns;  // For UPDATE (before image)
        std::map<std::string, std::string> metadata;     // Type info, nullability, etc.
    };
    RowData row;
    
    // Key columns for identification
    std::vector<std::string> primary_key_columns;
    std::map<std::string, std::string> primary_key_values;
    
    // Source metadata
    std::string source_connector;  // "postgres_wal", "mysql_binlog", "debezium", etc.
    
    // Extensions
    std::map<std::string, std::string> headers;
};

// ============================================================================
// Message Broker Types
// ============================================================================
enum class BrokerType {
    KAFKA,
    RABBITMQ,
    AWS_KINESIS,
    GOOGLE_PUBSUB,
    AZURE_EVENT_HUBS,
    REDIS_PUBSUB,
    NATS,
    PULSAR
};

std::string BrokerTypeToString(BrokerType type);

// ============================================================================
// CDC Connector Configuration
// ============================================================================
struct CdcConnectorConfig {
    std::string id;
    std::string name;
    
    // Source database
    std::string connection_string;
    std::string database_type;  // "postgresql", "mysql", "firebird"
    
    // Tables to monitor
    std::vector<std::string> include_tables;
    std::vector<std::string> exclude_tables;
    std::vector<std::string> include_schemas;
    
    // Column filtering
    std::vector<std::string> include_columns;
    std::vector<std::string> exclude_columns;
    
    // Event filtering
    bool capture_inserts = true;
    bool capture_updates = true;
    bool capture_deletes = true;
    
    // Update filtering
    bool capture_only_changed_columns = false;
    std::vector<std::string> update_filter_columns;  // Only capture if these changed
    
    // Row filtering
    std::string row_filter;  // SQL WHERE clause
    
    // Snapshotting
    bool initial_snapshot = true;
    std::string snapshot_mode;  // "initial", "schema_only", "never"
    
    // Performance
    int batch_size = 100;
    int poll_interval_ms = 1000;
    int max_events_per_poll = 1000;
    
    // Recovery
    std::string offset_storage;  // "file", "database", "kafka"
    std::string offset_file_path;
    
    // Format
    std::string message_format;  // "debezium", "canal", "scratchrobin"
    bool include_schema_changes = true;
    
    // Status
    bool enabled = true;
};

// ============================================================================
// CDC Connector (Abstract Base)
// ============================================================================
class CdcConnector {
public:
    virtual ~CdcConnector() = default;
    
    // Lifecycle
    virtual bool Initialize(const CdcConnectorConfig& config) = 0;
    virtual bool Start() = 0;
    virtual bool Stop() = 0;
    virtual bool IsRunning() const = 0;
    
    // Event handling
    using EventCallback = std::function<void(const CdcEvent& event)>;
    virtual void SetEventCallback(EventCallback callback) = 0;
    
    // Offset management
    virtual int64_t GetCurrentOffset() const = 0;
    virtual bool SeekToOffset(int64_t offset) = 0;
    
    // Schema management
    virtual std::vector<std::string> GetMonitoredTables() const = 0;
    virtual bool AddTable(const std::string& table) = 0;
    virtual bool RemoveTable(const std::string& table) = 0;
    
    // Snapshot
    virtual bool TriggerSnapshot(const std::vector<std::string>& tables) = 0;
    
    // Statistics
    struct Stats {
        int64_t events_captured = 0;
        int64_t events_filtered = 0;
        int64_t bytes_read = 0;
        std::time_t start_time;
        int64_t lag_ms = 0;  // Replication lag
    };
    virtual Stats GetStats() const = 0;
};

// ============================================================================
// Message Publisher
// ============================================================================
class MessagePublisher {
public:
    virtual ~MessagePublisher() = default;
    
    // Lifecycle
    virtual bool Connect(const std::string& connection_string) = 0;
    virtual bool Disconnect() = 0;
    virtual bool IsConnected() const = 0;
    
    // Publishing
    virtual bool Publish(const std::string& topic, const std::string& message) = 0;
    virtual bool PublishBatch(const std::string& topic, 
                               const std::vector<std::string>& messages) = 0;
    
    // Topic management
    virtual bool CreateTopic(const std::string& topic, 
                              int partitions = 1, 
                              int replication = 1) = 0;
    virtual bool DeleteTopic(const std::string& topic) = 0;
    virtual std::vector<std::string> ListTopics() = 0;
    
    // Transactions
    virtual bool BeginTransaction() = 0;
    virtual bool CommitTransaction() = 0;
    virtual bool RollbackTransaction() = 0;
};

// ============================================================================
// CDC Pipeline
// ============================================================================
class CdcPipeline {
public:
    struct Configuration {
        // Source
        std::string connector_id;
        
        // Transformations
        struct Transformation {
            std::string type;  // "filter", "enrich", "transform", "aggregate"
            std::string config;
        };
        std::vector<Transformation> transformations;
        
        // Sink (Publisher)
        BrokerType broker_type;
        std::string broker_connection_string;
        std::string target_topic;
        
        // Dead letter queue
        bool enable_dlq = true;
        std::string dlq_topic;
        int max_retries = 3;
    };
    
    CdcPipeline(const Configuration& config);
    ~CdcPipeline();
    
    // Lifecycle
    bool Initialize();
    bool Start();
    bool Stop();
    bool IsRunning() const;
    
    // Metrics
    struct Metrics {
        int64_t events_processed = 0;
        int64_t events_failed = 0;
        int64_t events_filtered = 0;
        double processing_rate = 0;  // events/second
        double latency_ms = 0;       // end-to-end latency
    };
    Metrics GetMetrics() const;
    
private:
    Configuration config_;
    std::unique_ptr<CdcConnector> connector_;
    std::unique_ptr<MessagePublisher> publisher_;
    
    void ProcessEvent(const CdcEvent& event);
    bool ApplyTransformations(CdcEvent& event);
    bool PublishEvent(const CdcEvent& event);
};

// ============================================================================
// CDC Stream Manager
// ============================================================================
class CdcStreamManager {
public:
    static CdcStreamManager& Instance();
    
    // Connector management
    void RegisterConnector(const std::string& type,
                           std::function<std::unique_ptr<CdcConnector>()> factory);
    std::unique_ptr<CdcConnector> CreateConnector(const std::string& type);
    
    // Pipeline management
    std::string CreatePipeline(const CdcPipeline::Configuration& config);
    bool StartPipeline(const std::string& pipeline_id);
    bool StopPipeline(const std::string& pipeline_id);
    bool RemovePipeline(const std::string& pipeline_id);
    CdcPipeline* GetPipeline(const std::string& pipeline_id);
    std::vector<std::string> GetPipelineIds() const;
    
    // Monitoring
    struct StreamMetrics {
        int64_t total_events = 0;
        int64_t active_pipelines = 0;
        double aggregate_rate = 0;
    };
    StreamMetrics GetMetrics() const;
    
    // Schema registry
    void RegisterSchema(const std::string& table, const std::string& schema_json);
    std::string GetSchema(const std::string& table) const;
    
private:
    CdcStreamManager() = default;
    ~CdcStreamManager() = default;
    
    std::map<std::string, std::function<std::unique_ptr<CdcConnector>()>> connector_factories_;
    std::map<std::string, std::unique_ptr<CdcPipeline>> pipelines_;
    std::map<std::string, std::string> schemas_;
};

// ============================================================================
// Event Consumer (for downstream applications)
// ============================================================================
class EventConsumer {
public:
    virtual ~EventConsumer() = default;
    
    virtual bool Subscribe(const std::vector<std::string>& topics) = 0;
    virtual bool Unsubscribe() = 0;
    
    virtual std::optional<CdcEvent> Poll(int timeout_ms) = 0;
    virtual std::vector<CdcEvent> PollBatch(int max_messages, int timeout_ms) = 0;
    
    virtual bool CommitOffset(const std::string& topic, int partition, int64_t offset) = 0;
    
    using EventHandler = std::function<void(const CdcEvent& event)>;
    virtual void StartConsumption(EventHandler handler) = 0;
    virtual void StopConsumption() = 0;
};

// ============================================================================
// Debezium Integration
// ============================================================================
class DebeziumIntegration {
public:
    // Parse Debezium-formatted messages
    static std::optional<CdcEvent> ParseDebeziumMessage(const std::string& json);
    
    // Create Debezium-formatted messages
    static std::string ToDebeziumFormat(const CdcEvent& event);
    
    // Connector configuration generation
    static std::string GeneratePostgresConnectorConfig(
        const std::string& name,
        const std::string& database_hostname,
        int database_port,
        const std::string& database_user,
        const std::string& database_password,
        const std::string& database_dbname,
        const std::vector<std::string>& tables);
    
    static std::string GenerateMySqlConnectorConfig(
        const std::string& name,
        const std::string& database_hostname,
        int database_port,
        const std::string& database_user,
        const std::string& database_password,
        const std::vector<std::string>& tables);
};

// ============================================================================
// Stream Processing (Simple windowing)
// ============================================================================
class StreamProcessor {
public:
    struct WindowConfig {
        std::string type;  // "tumbling", "sliding", "session"
        int duration_ms = 60000;  // 1 minute default
        int slide_ms = 10000;     // For sliding windows
        int session_gap_ms = 300000;  // 5 minute gap for session windows
    };
    
    struct Aggregation {
        std::string type;  // "count", "sum", "avg", "min", "max", "group_by"
        std::string column;
        std::string output_column;
    };
    
    // Windowed aggregation
    using WindowResultCallback = std::function<void(const std::string& window_key,
                                                     const std::map<std::string, 
                                                     double>& aggregates)>;
    
    void StartWindowedAggregation(const WindowConfig& window,
                                   const std::vector<Aggregation>& aggregations,
                                   WindowResultCallback callback);
    
    void ProcessEvent(const CdcEvent& event);
    void Stop();
    
private:
    struct WindowState;
    std::unique_ptr<WindowState> state_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CDC_STREAMING_H
