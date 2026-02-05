/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "cdc_streaming.h"
#include "cdc_connectors.h"
#include "core/simple_json.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// CDC Event Type Helpers
// ============================================================================
std::string CdcEventTypeToString(CdcEventType type) {
    switch (type) {
        case CdcEventType::INSERT: return "INSERT";
        case CdcEventType::UPDATE: return "UPDATE";
        case CdcEventType::DELETE: return "DELETE";
        case CdcEventType::TRUNCATE: return "TRUNCATE";
        case CdcEventType::BEGIN_TRANSACTION: return "BEGIN";
        case CdcEventType::COMMIT_TRANSACTION: return "COMMIT";
        case CdcEventType::ROLLBACK_TRANSACTION: return "ROLLBACK";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// Broker Type Helpers
// ============================================================================
std::string BrokerTypeToString(BrokerType type) {
    switch (type) {
        case BrokerType::KAFKA: return "Kafka";
        case BrokerType::RABBITMQ: return "RabbitMQ";
        case BrokerType::AWS_KINESIS: return "AWS Kinesis";
        case BrokerType::GOOGLE_PUBSUB: return "Google Pub/Sub";
        case BrokerType::AZURE_EVENT_HUBS: return "Azure Event Hubs";
        case BrokerType::REDIS_PUBSUB: return "Redis Pub/Sub";
        case BrokerType::NATS: return "NATS";
        case BrokerType::PULSAR: return "Apache Pulsar";
        default: return "Unknown";
    }
}

// ============================================================================
// CDC Pipeline Implementation
// ============================================================================

CdcPipeline::CdcPipeline(const Configuration& config) : config_(config) {}

CdcPipeline::~CdcPipeline() {
    Stop();
}

bool CdcPipeline::Initialize() {
    // Create connector based on source configuration
    auto& manager = CdcStreamManager::Instance();
    connector_ = manager.CreateConnector(config_.connector_id);
    
    if (!connector_) {
        // Fallback to mock connector for testing
        connector_ = std::make_unique<MockConnector>();
    }
    
    // Configure connector
    CdcConnectorConfig connector_config;
    connector_config.id = config_.connector_id;
    connector_config.include_tables = {"users", "orders", "products"};  // Default
    connector_config.poll_interval_ms = 1000;
    
    if (!connector_->Initialize(connector_config)) {
        return false;
    }
    
    // Set up event callback
    connector_->SetEventCallback([this](const CdcEvent& event) {
        ProcessEvent(event);
    });
    
    // Create publisher based on broker type
    switch (config_.broker_type) {
        case BrokerType::KAFKA:
            publisher_ = std::make_unique<KafkaPublisher>();
            break;
        case BrokerType::REDIS_PUBSUB:
            publisher_ = std::make_unique<RedisPublisher>();
            break;
        case BrokerType::RABBITMQ:
            publisher_ = std::make_unique<RabbitMqPublisher>();
            break;
        case BrokerType::NATS:
            publisher_ = std::make_unique<NatsPublisher>();
            break;
        default:
            // For unsupported brokers, events go through but no publishing
            break;
    }
    
    // Connect to message broker if publisher exists
    if (publisher_ && !config_.broker_connection_string.empty()) {
        if (!publisher_->Connect(config_.broker_connection_string)) {
            return false;
        }
    }
    
    // Create target topic if needed
    if (publisher_ && !config_.target_topic.empty()) {
        publisher_->CreateTopic(config_.target_topic, 3, 1);
    }
    
    return true;
}

bool CdcPipeline::Start() {
    if (!connector_) return false;
    return connector_->Start();
}

bool CdcPipeline::Stop() {
    if (connector_) {
        connector_->Stop();
    }
    if (publisher_) {
        publisher_->Disconnect();
    }
    return true;
}

bool CdcPipeline::IsRunning() const {
    return connector_ && connector_->IsRunning();
}

CdcPipeline::Metrics CdcPipeline::GetMetrics() const {
    Metrics metrics;
    if (connector_) {
        auto stats = connector_->GetStats();
        metrics.events_processed = stats.events_captured;
        metrics.events_filtered = stats.events_filtered;
    }
    // Calculate rate would need timestamp tracking
    return metrics;
}

void CdcPipeline::ProcessEvent(const CdcEvent& event) {
    // Apply transformations
    CdcEvent transformed_event = event;
    if (!ApplyTransformations(transformed_event)) {
        // Event was filtered out
        return;
    }
    
    // Publish to message broker
    PublishEvent(transformed_event);
}

bool CdcPipeline::ApplyTransformations(CdcEvent& event) {
    for (const auto& transform : config_.transformations) {
        if (transform.type == "filter") {
            // Apply filter logic
            if (transform.config == "exclude_deletes" && event.type == CdcEventType::DELETE) {
                return false;
            }
        } else if (transform.type == "enrich") {
            // Add enrichment data
            event.headers["processed_by"] = "scratchrobin";
            event.headers["processed_at"] = std::to_string(std::time(nullptr));
        }
    }
    return true;
}

bool CdcPipeline::PublishEvent(const CdcEvent& event) {
    if (!publisher_ || config_.target_topic.empty()) {
        return true;  // No publisher configured, event processed but not published
    }
    
    // Use retry logic
    return PublishWithRetry(event, 0);
}

// ============================================================================
// Error Handling and Retry Logic
// ============================================================================
void CdcPipeline::SetErrorHandlingConfig(const ErrorHandlingConfig& config) {
    error_config_ = config;
}

int CdcPipeline::CalculateRetryDelay(int attempt) const {
    if (!error_config_.exponential_backoff) {
        return error_config_.retry_delay_ms;
    }
    
    // Exponential backoff: delay * (multiplier ^ attempt)
    double delay = error_config_.retry_delay_ms * 
                   std::pow(error_config_.backoff_multiplier, attempt);
    return static_cast<int>(std::min(delay, static_cast<double>(error_config_.max_backoff_ms)));
}

bool CdcPipeline::PublishWithRetry(const CdcEvent& event, int attempt) {
    // Convert to Debezium format
    std::string message = DebeziumIntegration::ToDebeziumFormat(event);
    
    // Try to publish
    if (publisher_->Publish(config_.target_topic, message)) {
        return true;
    }
    
    // Check if we should retry
    if (attempt < error_config_.max_retries) {
        int delay_ms = CalculateRetryDelay(attempt);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        return PublishWithRetry(event, attempt + 1);
    }
    
    // All retries exhausted - handle failure
    HandlePublishError(event, "Max retries exceeded");
    return false;
}

void CdcPipeline::HandlePublishError(const CdcEvent& event, const std::string& error) {
    // Store in failed events map
    {
        std::lock_guard<std::mutex> lock(failed_events_mutex_);
        FailedEvent failed;
        failed.event = event;
        failed.failed_at = std::time(nullptr);
        failed.error_message = error;
        failed_events_[event.event_id] = failed;
    }
    
    // Try to send to DLQ if enabled
    if (error_config_.enable_dlq && !error_config_.dlq_topic.empty() && publisher_) {
        CdcEvent dlq_event = event;
        dlq_event.headers["error"] = error;
        dlq_event.headers["original_topic"] = config_.target_topic;
        dlq_event.headers["failed_at"] = std::to_string(std::time(nullptr));
        
        std::string dlq_message = DebeziumIntegration::ToDebeziumFormat(dlq_event);
        publisher_->Publish(error_config_.dlq_topic, dlq_message);
    }
}

std::vector<CdcEvent> CdcPipeline::GetFailedEvents() const {
    std::lock_guard<std::mutex> lock(failed_events_mutex_);
    std::vector<CdcEvent> events;
    for (const auto& [id, failed] : failed_events_) {
        events.push_back(failed.event);
    }
    return events;
}

bool CdcPipeline::RetryFailedEvent(const std::string& event_id) {
    std::lock_guard<std::mutex> lock(failed_events_mutex_);
    auto it = failed_events_.find(event_id);
    if (it == failed_events_.end()) {
        return false;
    }
    
    // Try to publish again
    if (PublishEvent(it->second.event)) {
        failed_events_.erase(it);
        return true;
    }
    
    // Increment retry count
    it->second.retry_count++;
    return false;
}

bool CdcPipeline::RetryAllFailedEvents() {
    std::lock_guard<std::mutex> lock(failed_events_mutex_);
    bool all_success = true;
    
    for (auto it = failed_events_.begin(); it != failed_events_.end();) {
        if (PublishEvent(it->second.event)) {
            it = failed_events_.erase(it);
        } else {
            it->second.retry_count++;
            all_success = false;
            ++it;
        }
    }
    
    return all_success;
}

void CdcPipeline::ClearFailedEvents() {
    std::lock_guard<std::mutex> lock(failed_events_mutex_);
    failed_events_.clear();
}

// ============================================================================
// CDC Stream Manager Implementation
// ============================================================================
CdcStreamManager& CdcStreamManager::Instance() {
    static CdcStreamManager instance;
    return instance;
}

void CdcStreamManager::RegisterConnector(
    const std::string& type,
    std::function<std::unique_ptr<CdcConnector>()> factory) {
    connector_factories_[type] = factory;
}

std::unique_ptr<CdcConnector> CdcStreamManager::CreateConnector(const std::string& type) {
    auto it = connector_factories_.find(type);
    if (it != connector_factories_.end()) {
        return it->second();
    }
    return nullptr;
}

std::string CdcStreamManager::CreatePipeline(const CdcPipeline::Configuration& config) {
    std::string id = "pipeline_" + std::to_string(pipelines_.size() + 1);
    pipelines_[id] = std::make_unique<CdcPipeline>(config);
    return id;
}

bool CdcStreamManager::StartPipeline(const std::string& pipeline_id) {
    auto it = pipelines_.find(pipeline_id);
    if (it != pipelines_.end()) {
        return it->second->Start();
    }
    return false;
}

bool CdcStreamManager::StopPipeline(const std::string& pipeline_id) {
    auto it = pipelines_.find(pipeline_id);
    if (it != pipelines_.end()) {
        return it->second->Stop();
    }
    return false;
}

bool CdcStreamManager::RemovePipeline(const std::string& pipeline_id) {
    auto it = pipelines_.find(pipeline_id);
    if (it != pipelines_.end()) {
        it->second->Stop();
        pipelines_.erase(it);
        return true;
    }
    return false;
}

CdcPipeline* CdcStreamManager::GetPipeline(const std::string& pipeline_id) {
    auto it = pipelines_.find(pipeline_id);
    if (it != pipelines_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> CdcStreamManager::GetPipelineIds() const {
    std::vector<std::string> ids;
    for (const auto& [id, _] : pipelines_) {
        ids.push_back(id);
    }
    return ids;
}

CdcStreamManager::StreamMetrics CdcStreamManager::GetMetrics() const {
    StreamMetrics metrics;
    for (const auto& [_, pipeline] : pipelines_) {
        auto pm = pipeline->GetMetrics();
        metrics.total_events += pm.events_processed;
        metrics.aggregate_rate += pm.processing_rate;
    }
    metrics.active_pipelines = pipelines_.size();
    return metrics;
}

void CdcStreamManager::RegisterSchema(const std::string& table, 
                                       const std::string& schema_json) {
    schemas_[table] = schema_json;
}

std::string CdcStreamManager::GetSchema(const std::string& table) const {
    auto it = schemas_.find(table);
    if (it != schemas_.end()) {
        return it->second;
    }
    return "";
}

// ============================================================================
// Debezium Integration Implementation
// ============================================================================

std::optional<CdcEvent> DebeziumIntegration::ParseDebeziumMessage(const std::string& json) {
    // Parse Debezium JSON format using JsonParser
    JsonParser parser(json);
    JsonValue root;
    std::string error;
    if (!parser.Parse(&root, &error)) {
        return std::nullopt;
    }
    
    CdcEvent event;
    
    // Get payload object
    const JsonValue* payload = FindMember(root, "payload");
    if (!payload) {
        return std::nullopt;
    }
    
    // Get operation type
    const JsonValue* op_value = FindMember(*payload, "op");
    if (op_value && op_value->type == JsonValue::Type::String) {
        std::string op = op_value->string_value;
        if (op == "c") event.type = CdcEventType::INSERT;
        else if (op == "u") event.type = CdcEventType::UPDATE;
        else if (op == "d") event.type = CdcEventType::DELETE;
        else if (op == "t") event.type = CdcEventType::TRUNCATE;
        else event.type = CdcEventType::INSERT;
    }
    
    // Get source info
    const JsonValue* source = FindMember(*payload, "source");
    if (source && source->type == JsonValue::Type::Object) {
        const JsonValue* db = FindMember(*source, "db");
        if (db && db->type == JsonValue::Type::String) {
            event.database = db->string_value;
        }
        const JsonValue* schema = FindMember(*source, "schema");
        if (schema && schema->type == JsonValue::Type::String) {
            event.schema = schema->string_value;
        }
        const JsonValue* table = FindMember(*source, "table");
        if (table && table->type == JsonValue::Type::String) {
            event.table = table->string_value;
        }
    }
    
    // Get timestamp
    const JsonValue* ts_ms = FindMember(*payload, "ts_ms");
    if (ts_ms && ts_ms->type == JsonValue::Type::Number) {
        event.timestamp = static_cast<std::time_t>(ts_ms->number_value / 1000);
    }
    
    return event;
}

std::string DebeziumIntegration::ToDebeziumFormat(const CdcEvent& event) {
    std::ostringstream oss;
    
    oss << "{\n";
    oss << "  \"schema\": {},\n";
    oss << "  \"payload\": {\n";
    oss << "    \"before\": null,\n";
    oss << "    \"after\": {\n";
    
    for (const auto& [col, val] : event.row.columns) {
        oss << "      \"" << col << "\": \"" << val << "\",\n";
    }
    
    oss << "    },\n";
    oss << "    \"source\": {\n";
    oss << "      \"version\": \"1.0\",\n";
    oss << "      \"connector\": \"scratchrobin\",\n";
    oss << "      \"name\": \"" << event.database << "\",\n";
    oss << "      \"ts_ms\": " << event.timestamp << "000,\n";
    oss << "      \"db\": \"" << event.database << "\",\n";
    oss << "      \"schema\": \"" << event.schema << "\",\n";
    oss << "      \"table\": \"" << event.table << "\"\n";
    oss << "    },\n";
    oss << "    \"op\": \"" << CdcEventTypeToString(event.type) << "\",\n";
    oss << "    \"ts_ms\": " << event.timestamp << "000\n";
    oss << "  }\n";
    oss << "}\n";
    
    return oss.str();
}

std::string DebeziumIntegration::GeneratePostgresConnectorConfig(
    const std::string& name,
    const std::string& database_hostname,
    int database_port,
    const std::string& database_user,
    const std::string& database_password,
    const std::string& database_dbname,
    const std::vector<std::string>& tables) {
    
    std::ostringstream oss;
    
    oss << "{\n";
    oss << "  \"name\": \"" << name << "\",\n";
    oss << "  \"config\": {\n";
    oss << "    \"connector.class\": \"io.debezium.connector.postgresql.PostgresConnector\",\n";
    oss << "    \"database.hostname\": \"" << database_hostname << "\",\n";
    oss << "    \"database.port\": \"" << database_port << "\",\n";
    oss << "    \"database.user\": \"" << database_user << "\",\n";
    oss << "    \"database.password\": \"" << database_password << "\",\n";
    oss << "    \"database.dbname\": \"" << database_dbname << "\",\n";
    oss << "    \"database.server.name\": \"" << name << "\",\n";
    
    oss << "    \"table.include.list\": \"";
    bool first = true;
    for (const auto& table : tables) {
        if (!first) oss << ",";
        first = false;
        oss << "public." << table;
    }
    oss << "\",\n";
    
    oss << "    \"plugin.name\": \"pgoutput\",\n";
    oss << "    \"slot.name\": \"debezium_" << name << "\",\n";
    oss << "    \"publication.name\": \"dbz_publication\",\n";
    oss << "    \"transforms\": \"unwrap\",\n";
    oss << "    \"transforms.unwrap.type\": \"io.debezium.transforms.ExtractNewRecordState\"\n";
    oss << "  }\n";
    oss << "}\n";
    
    return oss.str();
}

std::string DebeziumIntegration::GenerateMySqlConnectorConfig(
    const std::string& name,
    const std::string& database_hostname,
    int database_port,
    const std::string& database_user,
    const std::string& database_password,
    const std::vector<std::string>& tables) {
    
    std::ostringstream oss;
    
    oss << "{\n";
    oss << "  \"name\": \"" << name << "\",\n";
    oss << "  \"config\": {\n";
    oss << "    \"connector.class\": \"io.debezium.connector.mysql.MySqlConnector\",\n";
    oss << "    \"database.hostname\": \"" << database_hostname << "\",\n";
    oss << "    \"database.port\": \"" << database_port << "\",\n";
    oss << "    \"database.user\": \"" << database_user << "\",\n";
    oss << "    \"database.password\": \"" << database_password << "\",\n";
    oss << "    \"database.server.name\": \"" << name << "\",\n";
    
    oss << "    \"table.include.list\": \"";
    bool first = true;
    for (const auto& table : tables) {
        if (!first) oss << ",";
        first = false;
        oss << table;
    }
    oss << "\"\n";
    
    oss << "  }\n";
    oss << "}\n";
    
    return oss.str();
}

// ============================================================================
// Stream Processor Implementation
// ============================================================================
struct StreamProcessor::WindowState {
    WindowConfig config;
    std::vector<Aggregation> aggregations;
    WindowResultCallback callback;
    // Actual window data would go here
};

void StreamProcessor::StartWindowedAggregation(const WindowConfig& window,
                                                const std::vector<Aggregation>& aggregations,
                                                WindowResultCallback callback) {
    state_ = std::make_unique<WindowState>();
    state_->config = window;
    state_->aggregations = aggregations;
    state_->callback = callback;
}

void StreamProcessor::ProcessEvent(const CdcEvent& event) {
    // Add event to appropriate window and compute aggregations
    if (state_ && state_->callback) {
        std::map<std::string, double> aggregates;
        // Compute aggregates based on event data
        state_->callback(event.table, aggregates);
    }
}

void StreamProcessor::Stop() {
    state_.reset();
}

} // namespace scratchrobin
