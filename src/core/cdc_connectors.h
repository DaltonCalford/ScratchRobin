/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CDC_CONNECTORS_H
#define SCRATCHROBIN_CDC_CONNECTORS_H

#include "cdc_streaming.h"

#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

// Ensure atomic is properly included before namespace
#include <atomic>

namespace scratchrobin {

// ============================================================================
// PostgreSQL WAL Connector (using logical replication)
// ============================================================================
class PostgresWalConnector : public CdcConnector {
public:
    PostgresWalConnector();
    ~PostgresWalConnector() override;

    bool Initialize(const CdcConnectorConfig& config) override;
    bool Start() override;
    bool Stop() override;
    bool IsRunning() const override;

    void SetEventCallback(EventCallback callback) override;

    int64_t GetCurrentOffset() const override;
    bool SeekToOffset(int64_t offset) override;

    std::vector<std::string> GetMonitoredTables() const override;
    bool AddTable(const std::string& table) override;
    bool RemoveTable(const std::string& table) override;

    bool TriggerSnapshot(const std::vector<std::string>& tables) override;

    Stats GetStats() const override;

private:
    void PollThread();
    void ProcessWalRecord(const std::string& record);
    bool SetupReplicationSlot();
    bool DropReplicationSlot();

    CdcConnectorConfig config_;
    EventCallback callback_;
    std::atomic<bool> running_{false};
    std::thread poll_thread_;
    int64_t current_offset_ = 0;
    Stats stats_;
    mutable std::mutex stats_mutex_;
    
    // Database connection handle (opaque pointer to avoid PG headers here)
    void* pg_conn_ = nullptr;
    std::string replication_slot_;
};

// ============================================================================
// MySQL Binlog Connector
// ============================================================================
class MySqlBinlogConnector : public CdcConnector {
public:
    MySqlBinlogConnector();
    ~MySqlBinlogConnector() override;

    bool Initialize(const CdcConnectorConfig& config) override;
    bool Start() override;
    bool Stop() override;
    bool IsRunning() const override;

    void SetEventCallback(EventCallback callback) override;

    int64_t GetCurrentOffset() const override;
    bool SeekToOffset(int64_t offset) override;

    std::vector<std::string> GetMonitoredTables() const override;
    bool AddTable(const std::string& table) override;
    bool RemoveTable(const std::string& table) override;

    bool TriggerSnapshot(const std::vector<std::string>& tables) override;

    Stats GetStats() const override;

private:
    void PollThread();
    
    CdcConnectorConfig config_;
    EventCallback callback_;
    std::atomic<bool> running_{false};
    std::thread poll_thread_;
    int64_t current_offset_ = 0;
    Stats stats_;
    mutable std::mutex stats_mutex_;
};

// ============================================================================
// Polling-based Connector (works with any database)
// ============================================================================
class PollingConnector : public CdcConnector {
public:
    PollingConnector();
    ~PollingConnector() override;

    bool Initialize(const CdcConnectorConfig& config) override;
    bool Start() override;
    bool Stop() override;
    bool IsRunning() const override;

    void SetEventCallback(EventCallback callback) override;

    int64_t GetCurrentOffset() const override;
    bool SeekToOffset(int64_t offset) override;

    std::vector<std::string> GetMonitoredTables() const override;
    bool AddTable(const std::string& table) override;
    bool RemoveTable(const std::string& table) override;

    bool TriggerSnapshot(const std::vector<std::string>& tables) override;

    Stats GetStats() const override;

private:
    void PollThread();
    void PollTable(const std::string& table);
    bool SetupTrackingTables();
    
    struct TableState {
        std::string last_key_value;
        std::time_t last_poll_time = 0;
    };

    CdcConnectorConfig config_;
    EventCallback callback_;
    std::atomic<bool> running_{false};
    std::thread poll_thread_;
    int64_t sequence_counter_ = 0;
    Stats stats_;
    mutable std::mutex stats_mutex_;
    std::map<std::string, TableState> table_states_;
    mutable std::mutex state_mutex_;
};

// ============================================================================
// Mock Connector (for testing)
// ============================================================================
class MockConnector : public CdcConnector {
public:
    MockConnector();
    ~MockConnector() override;

    bool Initialize(const CdcConnectorConfig& config) override;
    bool Start() override;
    bool Stop() override;
    bool IsRunning() const override;

    void SetEventCallback(EventCallback callback) override;

    int64_t GetCurrentOffset() const override;
    bool SeekToOffset(int64_t offset) override;

    std::vector<std::string> GetMonitoredTables() const override;
    bool AddTable(const std::string& table) override;
    bool RemoveTable(const std::string& table) override;

    bool TriggerSnapshot(const std::vector<std::string>& tables) override;

    Stats GetStats() const override;

    // Test helpers
    void InjectEvent(const CdcEvent& event);

private:
    void GenerateThread();

    CdcConnectorConfig config_;
    EventCallback callback_;
    std::atomic<bool> running_{false};
    std::thread generate_thread_;
    int64_t sequence_counter_ = 0;
    Stats stats_;
    mutable std::mutex stats_mutex_;
    std::vector<std::string> tables_;
    mutable std::mutex tables_mutex_;
};

// ============================================================================
// Kafka Publisher
// ============================================================================
class KafkaPublisher : public MessagePublisher {
public:
    KafkaPublisher();
    ~KafkaPublisher() override;

    bool Connect(const std::string& connection_string) override;
    bool Disconnect() override;
    bool IsConnected() const override;

    bool Publish(const std::string& topic, const std::string& message) override;
    bool PublishBatch(const std::string& topic, 
                      const std::vector<std::string>& messages) override;

    bool CreateTopic(const std::string& topic, 
                     int partitions = 1, 
                     int replication = 1) override;
    bool DeleteTopic(const std::string& topic) override;
    std::vector<std::string> ListTopics() override;

    bool BeginTransaction() override;
    bool CommitTransaction() override;
    bool RollbackTransaction() override;

private:
    std::string connection_string_;
    bool connected_ = false;
    // Opaque pointer for kafka producer
    void* producer_ = nullptr;
};

// ============================================================================
// Redis Pub/Sub Publisher
// ============================================================================
class RedisPublisher : public MessagePublisher {
public:
    RedisPublisher();
    ~RedisPublisher() override;

    bool Connect(const std::string& connection_string) override;
    bool Disconnect() override;
    bool IsConnected() const override;

    bool Publish(const std::string& topic, const std::string& message) override;
    bool PublishBatch(const std::string& topic, 
                      const std::vector<std::string>& messages) override;

    bool CreateTopic(const std::string& topic, 
                     int partitions = 1, 
                     int replication = 1) override;
    bool DeleteTopic(const std::string& topic) override;
    std::vector<std::string> ListTopics() override;

    bool BeginTransaction() override;
    bool CommitTransaction() override;
    bool RollbackTransaction() override;

private:
    std::string host_;
    int port_ = 6379;
    bool connected_ = false;
    // Opaque pointer for redis context
    void* redis_context_ = nullptr;
};

// ============================================================================
// RabbitMQ Publisher
// ============================================================================
class RabbitMqPublisher : public MessagePublisher {
public:
    RabbitMqPublisher();
    ~RabbitMqPublisher() override;

    bool Connect(const std::string& connection_string) override;
    bool Disconnect() override;
    bool IsConnected() const override;

    bool Publish(const std::string& topic, const std::string& message) override;
    bool PublishBatch(const std::string& topic, 
                      const std::vector<std::string>& messages) override;

    bool CreateTopic(const std::string& topic, 
                     int partitions = 1, 
                     int replication = 1) override;
    bool DeleteTopic(const std::string& topic) override;
    std::vector<std::string> ListTopics() override;

    bool BeginTransaction() override;
    bool CommitTransaction() override;
    bool RollbackTransaction() override;

private:
    std::string connection_string_;
    bool connected_ = false;
    void* connection_ = nullptr;
    void* channel_ = nullptr;
};

// ============================================================================
// NATS Publisher
// ============================================================================
class NatsPublisher : public MessagePublisher {
public:
    NatsPublisher();
    ~NatsPublisher() override;

    bool Connect(const std::string& connection_string) override;
    bool Disconnect() override;
    bool IsConnected() const override;

    bool Publish(const std::string& topic, const std::string& message) override;
    bool PublishBatch(const std::string& topic, 
                      const std::vector<std::string>& messages) override;

    bool CreateTopic(const std::string& topic, 
                     int partitions = 1, 
                     int replication = 1) override;
    bool DeleteTopic(const std::string& topic) override;
    std::vector<std::string> ListTopics() override;

    bool BeginTransaction() override;
    bool CommitTransaction() override;
    bool RollbackTransaction() override;

private:
    std::string connection_string_;
    bool connected_ = false;
    void* nats_connection_ = nullptr;
};

// ============================================================================
// Kafka Event Consumer
// ============================================================================
class KafkaEventConsumer : public EventConsumer {
public:
    KafkaEventConsumer();
    ~KafkaEventConsumer() override;

    bool Subscribe(const std::vector<std::string>& topics) override;
    bool Unsubscribe() override;

    std::optional<CdcEvent> Poll(int timeout_ms) override;
    std::vector<CdcEvent> PollBatch(int max_messages, int timeout_ms) override;

    bool CommitOffset(const std::string& topic, int partition, int64_t offset) override;

    void StartConsumption(EventHandler handler) override;
    void StopConsumption() override;

private:
    void ConsumeThread();

    std::vector<std::string> topics_;
    std::atomic<bool> consuming_{false};
    std::thread consume_thread_;
    EventHandler handler_;
    void* consumer_ = nullptr;
    std::string group_id_;
};

// ============================================================================
// Redis Event Consumer
// ============================================================================
class RedisEventConsumer : public EventConsumer {
public:
    RedisEventConsumer();
    ~RedisEventConsumer() override;

    bool Subscribe(const std::vector<std::string>& topics) override;
    bool Unsubscribe() override;

    std::optional<CdcEvent> Poll(int timeout_ms) override;
    std::vector<CdcEvent> PollBatch(int max_messages, int timeout_ms) override;

    bool CommitOffset(const std::string& topic, int partition, int64_t offset) override;

    void StartConsumption(EventHandler handler) override;
    void StopConsumption() override;

private:
    void ConsumeThread();

    std::vector<std::string> topics_;
    std::atomic<bool> consuming_{false};
    std::thread consume_thread_;
    EventHandler handler_;
    void* redis_context_ = nullptr;
    std::string host_;
    int port_ = 6379;
    
    // For non-Pub/Sub mode (using lists)
    std::queue<CdcEvent> event_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CDC_CONNECTORS_H
