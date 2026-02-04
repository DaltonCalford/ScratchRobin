/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_AI_ASSISTANT_H
#define SCRATCHROBIN_AI_ASSISTANT_H

#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin {

// Forward declarations
struct IndexInfo;
struct QueryInfo;
struct MigrationInfo;

// Table info for AI context
struct TableInfo {
    std::string name;
    std::string schema;
    std::vector<std::pair<std::string, std::string>> columns;  // name, type
    std::vector<std::string> primary_keys;
    std::vector<std::string> indexes;
};

// ============================================================================
// AI Provider Types
// ============================================================================
enum class AiProviderType {
    OPENAI,
    ANTHROPIC,
    OLLAMA,
    LOCAL_MODEL,
    SCRATCHBIRD_AI  // Native ScratchBird AI service
};

std::string AiProviderTypeToString(AiProviderType type);

// ============================================================================
// AI Provider Configuration
// ============================================================================
struct AiProviderConfig {
    AiProviderType type = AiProviderType::OPENAI;
    std::string api_key;
    std::string api_endpoint;  // For custom endpoints or local models
    std::string model_name;
    int max_tokens = 4096;
    float temperature = 0.3f;  // Lower for more deterministic results
    int timeout_seconds = 60;
    
    // Rate limiting
    int requests_per_minute = 60;
    int tokens_per_minute = 100000;
    
    // Feature toggles
    bool enable_schema_design = true;
    bool enable_query_optimization = true;
    bool enable_code_generation = true;
    bool enable_documentation = true;
    bool enable_migration_assistance = true;
};

// ============================================================================
// AI Assistant Request/Response
// ============================================================================
struct AiRequest {
    std::string id;
    std::string prompt;
    std::vector<std::pair<std::string, std::string>> context;
    std::string expected_format;  // "json", "sql", "markdown", "natural"
    int max_tokens = 4096;
    float temperature = 0.3f;
    std::time_t timestamp;
};

struct AiResponse {
    std::string request_id;
    bool success;
    std::string content;
    std::string error_message;
    int tokens_used;
    std::time_t response_time;
    double latency_ms;
};

// ============================================================================
// Schema Design Suggestions
// ============================================================================
struct SchemaSuggestion {
    std::string id;
    std::string title;
    std::string description;
    
    // Table suggestions
    struct TableSuggestion {
        std::string table_name;
        std::string action;  // "CREATE", "ALTER", "DROP"
        std::vector<std::pair<std::string, std::string>> columns;
        std::vector<std::string> primary_key;
        std::vector<std::vector<std::string>> foreign_keys;
        std::string reason;
    };
    std::vector<TableSuggestion> tables;
    
    // Index suggestions
    struct IndexSuggestion {
        std::string table_name;
        std::string index_name;
        std::vector<std::string> columns;
        bool is_unique;
        std::string reason;
        double estimated_improvement;
    };
    std::vector<IndexSuggestion> indexes;
    
    // Normalization suggestions
    struct NormalizationSuggestion {
        std::string issue_type;  // "1NF", "2NF", "3NF", "BCNF"
        std::string table_name;
        std::string current_state;
        std::string suggested_state;
        std::string sql_to_fix;
    };
    std::vector<NormalizationSuggestion> normalization;
    
    // Confidence
    double confidence_score = 0.0;
    std::vector<std::string> prerequisites;
};

// ============================================================================
// Query Optimization Suggestions
// ============================================================================
struct QueryOptimization {
    std::string id;
    std::string original_query;
    std::string optimized_query;
    std::string explanation;
    
    // Performance estimates
    struct PerformanceEstimate {
        double original_cost = 0.0;
        double optimized_cost = 0.0;
        double improvement_percent = 0.0;
        double estimated_time_original_ms = 0.0;
        double estimated_time_optimized_ms = 0.0;
    };
    PerformanceEstimate performance;
    
    // Specific optimizations applied
    std::vector<std::string> optimizations_applied;
    
    // Index recommendations
    std::vector<std::string> recommended_indexes;
    
    // Warnings
    std::vector<std::string> warnings;
    
    // Alternative queries
    std::vector<std::pair<std::string, std::string>> alternatives;
};

// ============================================================================
// Migration Assistance
// ============================================================================
struct MigrationAssistance {
    std::string id;
    std::string source_database;
    std::string target_database;
    
    // Schema migration
    struct SchemaMapping {
        std::string source_table;
        std::string target_table;
        std::vector<std::pair<std::string, std::string>> column_mappings;
        std::string ddl_script;
    };
    std::vector<SchemaMapping> schema_mappings;
    
    // Data type conversions
    std::vector<std::pair<std::string, std::string>> type_conversions;
    
    // Compatibility issues
    struct CompatibilityIssue {
        std::string severity;  // "ERROR", "WARNING", "INFO"
        std::string category;  // "TYPE", "FUNCTION", "SYNTAX", "FEATURE"
        std::string description;
        std::string workaround;
        bool is_auto_fixable;
    };
    std::vector<CompatibilityIssue> issues;
    
    // Migration steps
    std::vector<std::string> migration_steps;
    
    // Rollback script
    std::string rollback_script;
};

// ============================================================================
// Natural Language to SQL
// ============================================================================
struct NaturalLanguageToSql {
    std::string natural_language;
    std::string generated_sql;
    std::string explanation;
    
    // Parameters detected
    std::vector<std::pair<std::string, std::string>> parameters;
    
    // Tables involved
    std::vector<std::string> tables_referenced;
    
    // Validation
    bool is_valid;
    std::string validation_error;
    
    // Alternatives
    std::vector<std::string> alternative_queries;
};

// ============================================================================
// Code Generation
// ============================================================================
struct CodeGeneration {
    enum class TargetLanguage {
        SQL,
        PYTHON,
        JAVASCRIPT,
        JAVA,
        CSHARP,
        GO,
        RUST,
        PHP,
        RUBY
    };
    
    TargetLanguage language;
    std::string description;
    std::string generated_code;
    
    // Dependencies
    std::vector<std::string> dependencies;
    
    // Usage example
    std::string usage_example;
    
    // Tests
    std::string test_code;
};

// ============================================================================
// Documentation Generation
// ============================================================================
struct DocumentationGeneration {
    enum class DocType {
        SCHEMA_DOCUMENTATION,
        API_DOCUMENTATION,
        QUERY_EXPLANATION,
        ERD_DESCRIPTION,
        CHANGELOG
    };
    
    DocType type;
    std::string title;
    std::string content;
    
    // Sections
    struct DocSection {
        std::string heading;
        std::string content;
        int level;
    };
    std::vector<DocSection> sections;
    
    // Diagram data (Mermaid, PlantUML, etc.)
    std::vector<std::pair<std::string, std::string>> diagrams;
};

// ============================================================================
// AI Provider Interface
// ============================================================================
class AiProvider {
public:
    virtual ~AiProvider() = default;
    
    // Lifecycle
    virtual bool Initialize(const AiProviderConfig& config) = 0;
    virtual bool IsAvailable() const = 0;
    
    // Core request
    virtual AiResponse SendRequest(const AiRequest& request) = 0;
    
    // Streaming support
    using StreamCallback = std::function<void(const std::string& chunk, bool is_done)>;
    virtual bool SendStreamingRequest(const AiRequest& request, 
                                       StreamCallback callback) = 0;
    
    // Specific features
    virtual std::optional<SchemaSuggestion> DesignSchema(
        const std::string& description,
        const std::vector<std::string>& existing_tables) = 0;
    
    virtual std::optional<QueryOptimization> OptimizeQuery(
        const std::string& query,
        const std::vector<TableInfo>& tables) = 0;
    
    virtual std::optional<MigrationAssistance> AssistMigration(
        const std::string& source_schema,
        const std::string& target_type) = 0;
    
    virtual std::optional<NaturalLanguageToSql> ConvertToSql(
        const std::string& natural_language,
        const std::vector<std::string>& available_tables) = 0;
    
    virtual std::optional<CodeGeneration> GenerateCode(
        const std::string& description,
        CodeGeneration::TargetLanguage language) = 0;
    
    virtual std::optional<DocumentationGeneration> GenerateDocumentation(
        const std::vector<TableInfo>& tables,
        DocumentationGeneration::DocType type) = 0;
};

// ============================================================================
// AI Assistant Manager
// ============================================================================
class AiAssistantManager {
public:
    static AiAssistantManager& Instance();
    
    // Provider management
    void RegisterProvider(const std::string& name,
                          std::function<std::unique_ptr<AiProvider>()> factory);
    bool SetActiveProvider(const std::string& name);
    AiProvider* GetActiveProvider();
    std::vector<std::string> GetAvailableProviders() const;
    
    // Configuration
    void SetConfig(const AiProviderConfig& config);
    AiProviderConfig GetConfig() const;
    
    // High-level API
    std::optional<SchemaSuggestion> DesignSchema(const std::string& description);
    std::optional<QueryOptimization> OptimizeQuery(const std::string& query);
    std::optional<MigrationAssistance> AssistMigration(const std::string& source_schema,
                                                        const std::string& target_type);
    std::optional<NaturalLanguageToSql> ConvertToSql(const std::string& natural_language);
    std::optional<CodeGeneration> GenerateCode(const std::string& description,
                                                CodeGeneration::TargetLanguage language);
    std::optional<DocumentationGeneration> GenerateDocumentation(
        DocumentationGeneration::DocType type);
    
    // Context management
    void SetContextDatabase(const std::string& database);
    void SetContextTables(const std::vector<std::string>& tables);
    void ClearContext();
    
    // Feedback
    void RecordFeedback(const std::string& suggestion_id, bool was_helpful,
                        const std::string& feedback);
    
private:
    AiAssistantManager() = default;
    ~AiAssistantManager() = default;
    
    std::map<std::string, std::function<std::unique_ptr<AiProvider>()>> providers_;
    std::unique_ptr<AiProvider> active_provider_;
    AiProviderConfig config_;
    
    // Context
    std::string context_database_;
    std::vector<std::string> context_tables_;
};

// ============================================================================
// Chat Interface
// ============================================================================
struct ChatMessage {
    std::string id;
    std::string role;  // "user", "assistant", "system"
    std::string content;
    std::time_t timestamp;
    std::vector<std::string> attachments;  // File paths, query results, etc.
};

class AiChatSession {
public:
    explicit AiChatSession(const std::string& session_id);
    
    void AddMessage(const ChatMessage& message);
    std::vector<ChatMessage> GetMessages() const;
    std::vector<ChatMessage> GetRecentMessages(int count) const;
    
    void ClearHistory();
    void ExportHistory(const std::string& file_path);
    void ImportHistory(const std::string& file_path);
    
    // Get full conversation for context
    std::string GetConversationContext() const;
    
private:
    std::string session_id_;
    std::vector<ChatMessage> messages_;
    std::time_t created_at_;
};

// ============================================================================
// Prompt Templates
// ============================================================================
class PromptTemplates {
public:
    static std::string SchemaDesignPrompt(const std::string& description,
                                           const std::vector<std::string>& existing_tables);
    
    static std::string QueryOptimizationPrompt(const std::string& query,
                                                const std::vector<TableInfo>& tables);
    
    static std::string MigrationPrompt(const std::string& source_schema,
                                        const std::string& target_type);
    
    static std::string NaturalLanguageToSqlPrompt(const std::string& natural_language,
                                                   const std::vector<std::string>& tables);
    
    static std::string CodeGenerationPrompt(const std::string& description,
                                             CodeGeneration::TargetLanguage language);
    
    static std::string DocumentationPrompt(const std::vector<TableInfo>& tables,
                                            DocumentationGeneration::DocType type);
};

// ============================================================================
// Response Parser
// ============================================================================
class ResponseParser {
public:
    static std::optional<SchemaSuggestion> ParseSchemaSuggestion(
        const std::string& response);
    
    static std::optional<QueryOptimization> ParseQueryOptimization(
        const std::string& response);
    
    static std::optional<MigrationAssistance> ParseMigrationAssistance(
        const std::string& response);
    
    static std::optional<NaturalLanguageToSql> ParseNaturalLanguageToSql(
        const std::string& response);
    
    static std::optional<CodeGeneration> ParseCodeGeneration(
        const std::string& response,
        CodeGeneration::TargetLanguage language);
    
    static std::optional<DocumentationGeneration> ParseDocumentation(
        const std::string& response,
        DocumentationGeneration::DocType type);
};

// ============================================================================
// Usage Analytics
// ============================================================================
struct UsageMetrics {
    int total_requests = 0;
    int successful_requests = 0;
    int failed_requests = 0;
    int64_t total_tokens_used = 0;
    double average_latency_ms = 0.0;
    std::map<std::string, int> requests_by_feature;
    std::map<std::string, int> requests_by_model;
};

class UsageAnalytics {
public:
    static UsageAnalytics& Instance();
    
    void RecordRequest(const std::string& feature, 
                       const std::string& model,
                       bool success,
                       int tokens_used,
                       double latency_ms);
    
    UsageMetrics GetMetrics() const;
    UsageMetrics GetMetricsForPeriod(std::time_t start, std::time_t end) const;
    void ResetMetrics();
    
    void ExportMetrics(const std::string& file_path);
    
private:
    UsageAnalytics() = default;
    
    struct RequestRecord {
        std::time_t timestamp;
        std::string feature;
        std::string model;
        bool success;
        int tokens_used;
        double latency_ms;
    };
    
    std::vector<RequestRecord> records_;
    mutable std::mutex mutex_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_AI_ASSISTANT_H
