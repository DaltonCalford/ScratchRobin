/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_AI_PROVIDERS_H
#define SCRATCHROBIN_AI_PROVIDERS_H

#include "ai_assistant.h"

#include <functional>
#include <string>

namespace scratchrobin {

// ============================================================================
// Base class for HTTP-based AI providers
// ============================================================================
class HttpAiProvider : public AiProvider {
public:
    ~HttpAiProvider() override = default;
    
    bool Initialize(const AiProviderConfig& config) override;
    bool IsAvailable() const override;
    
    AiResponse SendRequest(const AiRequest& request) override;
    bool SendStreamingRequest(const AiRequest& request, StreamCallback callback) override;
    
    // Specific features - base implementations using SendRequest
    std::optional<SchemaSuggestion> DesignSchema(
        const std::string& description,
        const std::vector<std::string>& existing_tables) override;
    
    std::optional<QueryOptimization> OptimizeQuery(
        const std::string& query,
        const std::vector<TableInfo>& tables) override;
    
    std::optional<MigrationAssistance> AssistMigration(
        const std::string& source_schema,
        const std::string& target_type) override;
    
    std::optional<NaturalLanguageToSql> ConvertToSql(
        const std::string& natural_language,
        const std::vector<std::string>& available_tables) override;
    
    std::optional<CodeGeneration> GenerateCode(
        const std::string& description,
        CodeGeneration::TargetLanguage language) override;
    
    std::optional<DocumentationGeneration> GenerateDocumentation(
        const std::vector<TableInfo>& tables,
        DocumentationGeneration::DocType type) override;

protected:
    // Provider-specific implementations
    virtual std::string GetApiEndpoint() const = 0;
    virtual std::string BuildRequestPayload(const AiRequest& request) const = 0;
    virtual std::string ParseResponseContent(const std::string& response) const = 0;
    virtual int ParseTokenUsage(const std::string& response) const = 0;
    virtual std::string GetProviderName() const = 0;
    
    // HTTP utilities
    struct HttpResponse {
        int status_code = 0;
        std::string body;
        std::string error;
    };
    
    HttpResponse HttpPost(const std::string& url,
                          const std::string& body,
                          const std::map<std::string, std::string>& headers,
                          int timeout_seconds);
    
    HttpResponse HttpPostStreaming(const std::string& url,
                                   const std::string& body,
                                   const std::map<std::string, std::string>& headers,
                                   int timeout_seconds,
                                   std::function<void(const std::string&)> chunk_callback);
    
    AiProviderConfig config_;
    bool initialized_ = false;
};

// ============================================================================
// OpenAI Provider
// ============================================================================
class OpenAiProvider : public HttpAiProvider {
public:
    static constexpr const char* NAME = "openai";
    
    bool Initialize(const AiProviderConfig& config) override;

protected:
    std::string GetApiEndpoint() const override;
    std::string BuildRequestPayload(const AiRequest& request) const override;
    std::string ParseResponseContent(const std::string& response) const override;
    int ParseTokenUsage(const std::string& response) const override;
    std::string GetProviderName() const override { return "OpenAI"; }
    
private:
    std::string GetModelName() const;
};

// ============================================================================
// Anthropic (Claude) Provider
// ============================================================================
class AnthropicProvider : public HttpAiProvider {
public:
    static constexpr const char* NAME = "anthropic";
    
    bool Initialize(const AiProviderConfig& config) override;

protected:
    std::string GetApiEndpoint() const override;
    std::string BuildRequestPayload(const AiRequest& request) const override;
    std::string ParseResponseContent(const std::string& response) const override;
    int ParseTokenUsage(const std::string& response) const override;
    std::string GetProviderName() const override { return "Anthropic"; }
    
private:
    std::string GetModelName() const;
};

// ============================================================================
// Ollama (Local) Provider
// ============================================================================
class OllamaProvider : public HttpAiProvider {
public:
    static constexpr const char* NAME = "ollama";
    
    bool Initialize(const AiProviderConfig& config) override;

protected:
    std::string GetApiEndpoint() const override;
    std::string BuildRequestPayload(const AiRequest& request) const override;
    std::string ParseResponseContent(const std::string& response) const override;
    int ParseTokenUsage(const std::string& response) const override;
    std::string GetProviderName() const override { return "Ollama"; }
    
private:
    std::string GetModelName() const;
};

// ============================================================================
// Google Gemini Provider
// ============================================================================
class GeminiProvider : public HttpAiProvider {
public:
    static constexpr const char* NAME = "gemini";
    
    bool Initialize(const AiProviderConfig& config) override;

protected:
    std::string GetApiEndpoint() const override;
    std::string BuildRequestPayload(const AiRequest& request) const override;
    std::string ParseResponseContent(const std::string& response) const override;
    int ParseTokenUsage(const std::string& response) const override;
    std::string GetProviderName() const override { return "Google Gemini"; }
    
private:
    std::string GetModelName() const;
};

// ============================================================================
// Provider Registration Helper
// ============================================================================
class AiProviderRegistrar {
public:
    static void RegisterAllProviders();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_AI_PROVIDERS_H
