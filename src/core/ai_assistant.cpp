/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "ai_assistant.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// AI Provider Type Helpers
// ============================================================================
std::string AiProviderTypeToString(AiProviderType type) {
    switch (type) {
        case AiProviderType::OPENAI: return "OpenAI";
        case AiProviderType::ANTHROPIC: return "Anthropic";
        case AiProviderType::OLLAMA: return "Ollama";
        case AiProviderType::LOCAL_MODEL: return "Local Model";
        case AiProviderType::SCRATCHBIRD_AI: return "ScratchBird AI";
        default: return "Unknown";
    }
}

// ============================================================================
// AI Assistant Manager Implementation
// ============================================================================
AiAssistantManager& AiAssistantManager::Instance() {
    static AiAssistantManager instance;
    return instance;
}

void AiAssistantManager::RegisterProvider(
    const std::string& name,
    std::function<std::unique_ptr<AiProvider>()> factory) {
    providers_[name] = factory;
}

bool AiAssistantManager::SetActiveProvider(const std::string& name) {
    auto it = providers_.find(name);
    if (it != providers_.end()) {
        active_provider_ = it->second();
        if (active_provider_) {
            return active_provider_->Initialize(config_);
        }
    }
    return false;
}

AiProvider* AiAssistantManager::GetActiveProvider() {
    return active_provider_.get();
}

std::vector<std::string> AiAssistantManager::GetAvailableProviders() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : providers_) {
        names.push_back(name);
    }
    return names;
}

void AiAssistantManager::SetConfig(const AiProviderConfig& config) {
    config_ = config;
    if (active_provider_) {
        active_provider_->Initialize(config_);
    }
}

AiProviderConfig AiAssistantManager::GetConfig() const {
    return config_;
}

std::optional<SchemaSuggestion> AiAssistantManager::DesignSchema(
    const std::string& description) {
    if (!active_provider_) return std::nullopt;
    return active_provider_->DesignSchema(description, context_tables_);
}

std::optional<QueryOptimization> AiAssistantManager::OptimizeQuery(
    const std::string& query) {
    if (!active_provider_) return std::nullopt;
    return active_provider_->OptimizeQuery(query, {});
}

std::optional<MigrationAssistance> AiAssistantManager::AssistMigration(
    const std::string& source_schema,
    const std::string& target_type) {
    if (!active_provider_) return std::nullopt;
    return active_provider_->AssistMigration(source_schema, target_type);
}

std::optional<NaturalLanguageToSql> AiAssistantManager::ConvertToSql(
    const std::string& natural_language) {
    if (!active_provider_) return std::nullopt;
    return active_provider_->ConvertToSql(natural_language, context_tables_);
}

std::optional<CodeGeneration> AiAssistantManager::GenerateCode(
    const std::string& description,
    CodeGeneration::TargetLanguage language) {
    if (!active_provider_) return std::nullopt;
    return active_provider_->GenerateCode(description, language);
}

std::optional<DocumentationGeneration> AiAssistantManager::GenerateDocumentation(
    DocumentationGeneration::DocType type) {
    if (!active_provider_) return std::nullopt;
    return active_provider_->GenerateDocumentation({}, type);
}

void AiAssistantManager::SetContextDatabase(const std::string& database) {
    context_database_ = database;
}

void AiAssistantManager::SetContextTables(const std::vector<std::string>& tables) {
    context_tables_ = tables;
}

void AiAssistantManager::ClearContext() {
    context_database_.clear();
    context_tables_.clear();
}

void AiAssistantManager::RecordFeedback(const std::string& suggestion_id,
                                         bool was_helpful,
                                         const std::string& feedback) {
    // Store feedback for analytics
    (void)suggestion_id;
    (void)was_helpful;
    (void)feedback;
}

// ============================================================================
// Chat Session Implementation
// ============================================================================
AiChatSession::AiChatSession(const std::string& session_id)
    : session_id_(session_id), created_at_(std::time(nullptr)) {}

void AiChatSession::AddMessage(const ChatMessage& message) {
    messages_.push_back(message);
}

std::vector<ChatMessage> AiChatSession::GetMessages() const {
    return messages_;
}

std::vector<ChatMessage> AiChatSession::GetRecentMessages(int count) const {
    if (count >= static_cast<int>(messages_.size())) {
        return messages_;
    }
    return std::vector<ChatMessage>(
        messages_.end() - count, messages_.end());
}

void AiChatSession::ClearHistory() {
    messages_.clear();
}

void AiChatSession::ExportHistory(const std::string& file_path) {
    (void)file_path;
    // Export to file
}

void AiChatSession::ImportHistory(const std::string& file_path) {
    (void)file_path;
    // Import from file
}

std::string AiChatSession::GetConversationContext() const {
    std::ostringstream oss;
    for (const auto& msg : messages_) {
        oss << msg.role << ": " << msg.content << "\n\n";
    }
    return oss.str();
}

// ============================================================================
// Prompt Templates Implementation
// ============================================================================
std::string PromptTemplates::SchemaDesignPrompt(
    const std::string& description,
    const std::vector<std::string>& existing_tables) {
    
    std::ostringstream oss;
    oss << "You are a database design expert. Design a database schema based on the following requirements:\n\n";
    oss << "Requirements:\n" << description << "\n\n";
    
    if (!existing_tables.empty()) {
        oss << "Existing tables:\n";
        for (const auto& table : existing_tables) {
            oss << "- " << table << "\n";
        }
        oss << "\n";
    }
    
    oss << "Provide your response in JSON format with:\n";
    oss << "- tables: array of table definitions with columns, types, constraints\n";
    oss << "- indexes: recommended indexes for performance\n";
    oss << "- normalization: any normalization issues and fixes\n";
    oss << "- confidence_score: your confidence in the design (0-1)\n";
    
    return oss.str();
}

std::string PromptTemplates::QueryOptimizationPrompt(
    const std::string& query,
    const std::vector<TableInfo>& tables) {
    
    std::ostringstream oss;
    oss << "You are a database query optimization expert. Analyze and optimize the following SQL query:\n\n";
    oss << "Query:\n```sql\n" << query << "\n```\n\n";
    
    if (!tables.empty()) {
        oss << "Table information:\n";
        // Add table info
    }
    
    oss << "Provide:\n";
    oss << "- optimized_query: the rewritten, optimized query\n";
    oss << "- explanation: why the optimization helps\n";
    oss << "- performance: estimated improvement metrics\n";
    oss << "- recommended_indexes: any indexes that should be created\n";
    oss << "- warnings: any potential issues\n";
    
    return oss.str();
}

std::string PromptTemplates::MigrationPrompt(
    const std::string& source_schema,
    const std::string& target_type) {
    
    std::ostringstream oss;
    oss << "You are a database migration expert. Help migrate the following schema to " << target_type << ":\n\n";
    oss << "Source Schema:\n```sql\n" << source_schema << "\n```\n\n";
    
    oss << "Provide:\n";
    oss << "- schema_mappings: how each table maps to the target\n";
    oss << "- type_conversions: data type mappings\n";
    oss << "- issues: compatibility issues with workarounds\n";
    oss << "- migration_steps: ordered steps to perform the migration\n";
    oss << "- rollback_script: how to undo if needed\n";
    
    return oss.str();
}

std::string PromptTemplates::NaturalLanguageToSqlPrompt(
    const std::string& natural_language,
    const std::vector<std::string>& tables) {
    
    std::ostringstream oss;
    oss << "Convert the following natural language request to SQL:\n\n";
    oss << "\"" << natural_language << "\"\n\n";
    
    if (!tables.empty()) {
        oss << "Available tables:\n";
        for (const auto& table : tables) {
            oss << "- " << table << "\n";
        }
        oss << "\n";
    }
    
    oss << "Provide:\n";
    oss << "- generated_sql: the SQL query\n";
    oss << "- explanation: what the query does\n";
    oss << "- parameters: any detected parameters\n";
    oss << "- tables_referenced: tables used in the query\n";
    
    return oss.str();
}

std::string PromptTemplates::CodeGenerationPrompt(
    const std::string& description,
    CodeGeneration::TargetLanguage language) {
    
    const char* lang_names[] = {"SQL", "Python", "JavaScript", "Java", 
                                 "C#", "Go", "Rust", "PHP", "Ruby"};
    
    std::ostringstream oss;
    oss << "Generate " << lang_names[static_cast<int>(language)] << " code for:\n\n";
    oss << description << "\n\n";
    
    oss << "Provide:\n";
    oss << "- generated_code: the complete, working code\n";
    oss << "- dependencies: required libraries/packages\n";
    oss << "- usage_example: how to use the code\n";
    oss << "- test_code: unit tests for the code\n";
    
    return oss.str();
}

std::string PromptTemplates::DocumentationPrompt(
    const std::vector<TableInfo>& tables,
    DocumentationGeneration::DocType type) {
    
    const char* type_names[] = {"Schema", "API", "Query", "ERD", "Changelog"};
    
    std::ostringstream oss;
    oss << "Generate " << type_names[static_cast<int>(type)] << " documentation.\n\n";
    
    if (!tables.empty()) {
        oss << "Tables:\n";
        // Add table info
    }
    
    oss << "Provide well-structured documentation with proper formatting.";
    
    return oss.str();
}

// ============================================================================
// Usage Analytics Implementation
// ============================================================================
UsageAnalytics& UsageAnalytics::Instance() {
    static UsageAnalytics instance;
    return instance;
}

void UsageAnalytics::RecordRequest(const std::string& feature,
                                    const std::string& model,
                                    bool success,
                                    int tokens_used,
                                    double latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.push_back({
        std::time(nullptr),
        feature,
        model,
        success,
        tokens_used,
        latency_ms
    });
}

UsageMetrics UsageAnalytics::GetMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    UsageMetrics metrics;
    metrics.total_requests = static_cast<int>(records_.size());
    
    double total_latency = 0.0;
    for (const auto& record : records_) {
        if (record.success) {
            metrics.successful_requests++;
        } else {
            metrics.failed_requests++;
        }
        metrics.total_tokens_used += record.tokens_used;
        total_latency += record.latency_ms;
        metrics.requests_by_feature[record.feature]++;
        metrics.requests_by_model[record.model]++;
    }
    
    if (metrics.total_requests > 0) {
        metrics.average_latency_ms = total_latency / metrics.total_requests;
    }
    
    return metrics;
}

UsageMetrics UsageAnalytics::GetMetricsForPeriod(std::time_t start,
                                                  std::time_t end) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    UsageMetrics metrics;
    double total_latency = 0.0;
    int count = 0;
    
    for (const auto& record : records_) {
        if (record.timestamp >= start && record.timestamp <= end) {
            count++;
            if (record.success) {
                metrics.successful_requests++;
            } else {
                metrics.failed_requests++;
            }
            metrics.total_tokens_used += record.tokens_used;
            total_latency += record.latency_ms;
            metrics.requests_by_feature[record.feature]++;
            metrics.requests_by_model[record.model]++;
        }
    }
    
    metrics.total_requests = count;
    if (count > 0) {
        metrics.average_latency_ms = total_latency / count;
    }
    
    return metrics;
}

void UsageAnalytics::ResetMetrics() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
}

void UsageAnalytics::ExportMetrics(const std::string& file_path) {
    (void)file_path;
    // Export metrics to file
}

} // namespace scratchrobin
