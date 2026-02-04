/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "ai_providers.h"
#include "simple_json.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>

// For HTTP requests - using system curl command for simplicity
// In production, this would use libcurl directly
#include <cstdlib>

namespace scratchrobin {

// ============================================================================
// HTTP AI Provider Base Implementation
// ============================================================================
bool HttpAiProvider::Initialize(const AiProviderConfig& config) {
    config_ = config;
    
    // Validate configuration
    if (config_.api_key.empty()) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

bool HttpAiProvider::IsAvailable() const {
    return initialized_ && !config_.api_key.empty();
}

AiResponse HttpAiProvider::SendRequest(const AiRequest& request) {
    AiResponse response;
    response.request_id = request.id;
    response.response_time = std::time(nullptr);
    
    if (!IsAvailable()) {
        response.success = false;
        response.error_message = "Provider not initialized or API key not set";
        return response;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::string url = GetApiEndpoint();
    std::string payload = BuildRequestPayload(request);
    
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer " + config_.api_key;
    
    HttpResponse http_response = HttpPost(url, payload, headers, 
                                          config_.timeout_seconds);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    response.latency_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();
    
    if (http_response.status_code != 200) {
        response.success = false;
        response.error_message = http_response.error.empty() 
            ? "HTTP error: " + std::to_string(http_response.status_code)
            : http_response.error;
        return response;
    }
    
    response.success = true;
    response.content = ParseResponseContent(http_response.body);
    response.tokens_used = ParseTokenUsage(http_response.body);
    
    return response;
}

bool HttpAiProvider::SendStreamingRequest(const AiRequest& request, 
                                          StreamCallback callback) {
    if (!IsAvailable()) {
        return false;
    }
    
    std::string url = GetApiEndpoint();
    std::string payload = BuildRequestPayload(request);
    
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer " + config_.api_key;
    
    // For now, simulate streaming by calling callback with full response
    HttpResponse http_response = HttpPost(url, payload, headers,
                                          config_.timeout_seconds);
    
    if (http_response.status_code == 200) {
        std::string content = ParseResponseContent(http_response.body);
        // Simulate streaming by sending in chunks
        size_t chunk_size = 20;
        for (size_t i = 0; i < content.length(); i += chunk_size) {
            std::string chunk = content.substr(i, chunk_size);
            callback(chunk, i + chunk_size >= content.length());
        }
        return true;
    }
    
    return false;
}

HttpAiProvider::HttpResponse HttpAiProvider::HttpPost(
    const std::string& url,
    const std::string& body,
    const std::map<std::string, std::string>& headers,
    int timeout_seconds) {
    
    HttpResponse response;
    
    // Build curl command
    std::ostringstream cmd;
    cmd << "curl -s -w \"\\n%{http_code}\" ";
    cmd << "-m " << timeout_seconds << " ";
    cmd << "-X POST ";
    
    for (const auto& [key, value] : headers) {
        cmd << "-H \"" << key << ": " << value << "\" ";
    }
    
    // Escape body for shell
    std::string escaped_body = body;
    size_t pos = 0;
    while ((pos = escaped_body.find('"', pos)) != std::string::npos) {
        escaped_body.insert(pos, "\\");
        pos += 2;
    }
    
    cmd << "-d \"" << escaped_body << "\" ";
    cmd << "\"" << url << "\"";
    
    // Execute curl command
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        response.error = "Failed to execute HTTP request";
        return response;
    }
    
    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    int status = pclose(pipe);
    if (status != 0) {
        response.error = "HTTP request failed";
        return response;
    }
    
    // Parse response - last line is HTTP code
    size_t last_newline = output.rfind('\n');
    if (last_newline != std::string::npos && last_newline + 1 < output.length()) {
        std::string code_str = output.substr(last_newline + 1);
        // Remove any trailing whitespace
        code_str.erase(code_str.find_last_not_of(" \t\n\r") + 1);
        try {
            response.status_code = std::stoi(code_str);
        } catch (...) {
            response.status_code = 0;
        }
        response.body = output.substr(0, last_newline);
    } else {
        response.body = output;
        response.status_code = 200;  // Assume success if no code found
    }
    
    return response;
}

HttpAiProvider::HttpResponse HttpAiProvider::HttpPostStreaming(
    const std::string& url,
    const std::string& body,
    const std::map<std::string, std::string>& headers,
    int timeout_seconds,
    std::function<void(const std::string&)> chunk_callback) {
    
    // For now, just delegate to non-streaming version
    HttpResponse response = HttpPost(url, body, headers, timeout_seconds);
    
    if (response.status_code == 200 && chunk_callback) {
        chunk_callback(response.body);
    }
    
    return response;
}

// Helper to extract content from JSON response using simple_json API
std::string ExtractJsonContent(const std::string& json_response) {
    JsonParser parser(json_response);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return "";
    }
    
    // Try OpenAI format: choices[0].message.content
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* choices = FindMember(root, "choices");
        if (choices && choices->type == JsonValue::Type::Array && !choices->array_value.empty()) {
            const JsonValue& first_choice = choices->array_value[0];
            if (first_choice.type == JsonValue::Type::Object) {
                const JsonValue* message = FindMember(first_choice, "message");
                if (message && message->type == JsonValue::Type::Object) {
                    const JsonValue* content = FindMember(*message, "content");
                    if (content && content->type == JsonValue::Type::String) {
                        return content->string_value;
                    }
                }
                // Try direct text field
                const JsonValue* text = FindMember(first_choice, "text");
                if (text && text->type == JsonValue::Type::String) {
                    return text->string_value;
                }
            }
        }
        
        // Try Anthropic format: content[0].text
        const JsonValue* content_arr = FindMember(root, "content");
        if (content_arr && content_arr->type == JsonValue::Type::Array && !content_arr->array_value.empty()) {
            const JsonValue& first_content = content_arr->array_value[0];
            if (first_content.type == JsonValue::Type::Object) {
                const JsonValue* text = FindMember(first_content, "text");
                if (text && text->type == JsonValue::Type::String) {
                    return text->string_value;
                }
            }
        }
        
        // Try Ollama format: message.content
        const JsonValue* message = FindMember(root, "message");
        if (message && message->type == JsonValue::Type::Object) {
            const JsonValue* content = FindMember(*message, "content");
            if (content && content->type == JsonValue::Type::String) {
                return content->string_value;
            }
        }
        
        // Try direct response field
        const JsonValue* response = FindMember(root, "response");
        if (response && response->type == JsonValue::Type::String) {
            return response->string_value;
        }
    }
    
    return "";
}

int ExtractTokenUsage(const std::string& json_response) {
    JsonParser parser(json_response);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return 0;
    }
    
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* usage = FindMember(root, "usage");
        if (usage && usage->type == JsonValue::Type::Object) {
            const JsonValue* total_tokens = FindMember(*usage, "total_tokens");
            if (total_tokens && total_tokens->type == JsonValue::Type::Number) {
                return static_cast<int>(total_tokens->number_value);
            }
            
            // Try Anthropic format: input_tokens + output_tokens
            const JsonValue* input_tokens = FindMember(*usage, "input_tokens");
            const JsonValue* output_tokens = FindMember(*usage, "output_tokens");
            int total = 0;
            if (input_tokens && input_tokens->type == JsonValue::Type::Number) {
                total += static_cast<int>(input_tokens->number_value);
            }
            if (output_tokens && output_tokens->type == JsonValue::Type::Number) {
                total += static_cast<int>(output_tokens->number_value);
            }
            if (total > 0) return total;
        }
    }
    
    return 0;
}

// ============================================================================
// OpenAI Provider Implementation
// ============================================================================
bool OpenAiProvider::Initialize(const AiProviderConfig& config) {
    if (!HttpAiProvider::Initialize(config)) {
        return false;
    }
    
    // Validate OpenAI-specific settings
    if (config_.api_endpoint.empty()) {
        config_.api_endpoint = "https://api.openai.com/v1/chat/completions";
    }
    
    return true;
}

std::string OpenAiProvider::GetApiEndpoint() const {
    return config_.api_endpoint.empty() 
        ? "https://api.openai.com/v1/chat/completions"
        : config_.api_endpoint;
}

std::string OpenAiProvider::GetModelName() const {
    if (!config_.model_name.empty()) {
        return config_.model_name;
    }
    // Default to GPT-4o for best SQL understanding
    return "gpt-4o";
}

std::string OpenAiProvider::BuildRequestPayload(const AiRequest& request) const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"model\": \"" << GetModelName() << "\",";
    oss << "\"messages\": [";
    oss << "{\"role\": \"system\", \"content\": \"You are a database expert. Provide accurate SQL and schema advice.\"},";
    oss << "{\"role\": \"user\", \"content\": \"";
    
    // Escape the prompt for JSON
    std::string escaped = request.prompt;
    size_t pos = 0;
    while ((pos = escaped.find('\\', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find('\n', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\n");
        pos += 2;
    }
    
    oss << escaped;
    oss << "\"}";
    oss << "],";
    oss << "\"temperature\": " << request.temperature << ",";
    oss << "\"max_tokens\": " << request.max_tokens;
    oss << "}";
    
    return oss.str();
}

std::string OpenAiProvider::ParseResponseContent(const std::string& response) const {
    return ExtractJsonContent(response);
}

int OpenAiProvider::ParseTokenUsage(const std::string& response) const {
    return ExtractTokenUsage(response);
}

// ============================================================================
// Anthropic Provider Implementation
// ============================================================================
bool AnthropicProvider::Initialize(const AiProviderConfig& config) {
    if (!HttpAiProvider::Initialize(config)) {
        return false;
    }
    
    if (config_.api_endpoint.empty()) {
        config_.api_endpoint = "https://api.anthropic.com/v1/messages";
    }
    
    return true;
}

std::string AnthropicProvider::GetApiEndpoint() const {
    return config_.api_endpoint.empty()
        ? "https://api.anthropic.com/v1/messages"
        : config_.api_endpoint;
}

std::string AnthropicProvider::GetModelName() const {
    if (!config_.model_name.empty()) {
        return config_.model_name;
    }
    return "claude-3-5-sonnet-20241022";
}

std::string AnthropicProvider::BuildRequestPayload(const AiRequest& request) const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"model\": \"" << GetModelName() << "\",";
    oss << "\"max_tokens\": " << request.max_tokens << ",";
    oss << "\"temperature\": " << request.temperature << ",";
    oss << "\"messages\": [";
    oss << "{\"role\": \"user\", \"content\": \"";
    
    // Escape the prompt for JSON
    std::string escaped = request.prompt;
    size_t pos = 0;
    while ((pos = escaped.find('\\', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find('\n', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\n");
        pos += 2;
    }
    
    oss << escaped;
    oss << "\"}";
    oss << "]";
    oss << "}";
    
    return oss.str();
}

std::string AnthropicProvider::ParseResponseContent(const std::string& response) const {
    return ExtractJsonContent(response);
}

int AnthropicProvider::ParseTokenUsage(const std::string& response) const {
    return ExtractTokenUsage(response);
}

// ============================================================================
// Ollama Provider Implementation
// ============================================================================
bool OllamaProvider::Initialize(const AiProviderConfig& config) {
    if (!HttpAiProvider::Initialize(config)) {
        return false;
    }
    
    // Ollama typically runs locally, no API key needed
    if (config_.api_endpoint.empty()) {
        config_.api_endpoint = "http://localhost:11434/api/chat";
    }
    
    return true;
}

std::string OllamaProvider::GetApiEndpoint() const {
    return config_.api_endpoint.empty()
        ? "http://localhost:11434/api/chat"
        : config_.api_endpoint;
}

std::string OllamaProvider::GetModelName() const {
    if (!config_.model_name.empty()) {
        return config_.model_name;
    }
    return "codellama";  // Good for code/SQL tasks
}

std::string OllamaProvider::BuildRequestPayload(const AiRequest& request) const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"model\": \"" << GetModelName() << "\",";
    oss << "\"messages\": [";
    oss << "{\"role\": \"system\", \"content\": \"You are a database expert. Provide accurate SQL and schema advice.\"},";
    oss << "{\"role\": \"user\", \"content\": \"";
    
    // Escape the prompt for JSON
    std::string escaped = request.prompt;
    size_t pos = 0;
    while ((pos = escaped.find('\\', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find('\n', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\n");
        pos += 2;
    }
    
    oss << escaped;
    oss << "\"}";
    oss << "],";
    oss << "\"stream\": false";
    oss << "}";
    
    return oss.str();
}

std::string OllamaProvider::ParseResponseContent(const std::string& response) const {
    return ExtractJsonContent(response);
}

int OllamaProvider::ParseTokenUsage(const std::string& response) const {
    return ExtractTokenUsage(response);
}

// ============================================================================
// Gemini Provider Implementation
// ============================================================================
bool GeminiProvider::Initialize(const AiProviderConfig& config) {
    if (!HttpAiProvider::Initialize(config)) {
        return false;
    }
    
    if (config_.api_endpoint.empty()) {
        config_.api_endpoint = "https://generativelanguage.googleapis.com/v1beta/models/";
    }
    
    return true;
}

std::string GeminiProvider::GetApiEndpoint() const {
    std::string base = config_.api_endpoint.empty()
        ? "https://generativelanguage.googleapis.com/v1beta/models/"
        : config_.api_endpoint;
    
    return base + GetModelName() + ":generateContent?key=" + config_.api_key;
}

std::string GeminiProvider::GetModelName() const {
    if (!config_.model_name.empty()) {
        return config_.model_name;
    }
    return "gemini-pro";
}

std::string GeminiProvider::BuildRequestPayload(const AiRequest& request) const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"contents\": [";
    oss << "{\"parts\": [";
    oss << "{\"text\": \"";
    
    // Escape the prompt for JSON
    std::string escaped = request.prompt;
    size_t pos = 0;
    while ((pos = escaped.find('\\', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.insert(pos, "\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = escaped.find('\n', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\n");
        pos += 2;
    }
    
    oss << escaped;
    oss << "\"}";
    oss << "]}";
    oss << "]";
    oss << "}";
    
    return oss.str();
}

std::string GeminiProvider::ParseResponseContent(const std::string& response) const {
    JsonParser parser(response);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return "";
    }
    
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* candidates = FindMember(root, "candidates");
        if (candidates && candidates->type == JsonValue::Type::Array && !candidates->array_value.empty()) {
            const JsonValue& first_candidate = candidates->array_value[0];
            if (first_candidate.type == JsonValue::Type::Object) {
                const JsonValue* content = FindMember(first_candidate, "content");
                if (content && content->type == JsonValue::Type::Object) {
                    const JsonValue* parts = FindMember(*content, "parts");
                    if (parts && parts->type == JsonValue::Type::Array && !parts->array_value.empty()) {
                        const JsonValue& first_part = parts->array_value[0];
                        if (first_part.type == JsonValue::Type::Object) {
                            const JsonValue* text = FindMember(first_part, "text");
                            if (text && text->type == JsonValue::Type::String) {
                                return text->string_value;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return "";
}

int GeminiProvider::ParseTokenUsage(const std::string& response) const {
    JsonParser parser(response);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return 0;
    }
    
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* usage = FindMember(root, "usageMetadata");
        if (usage && usage->type == JsonValue::Type::Object) {
            const JsonValue* total = FindMember(*usage, "totalTokenCount");
            if (total && total->type == JsonValue::Type::Number) {
                return static_cast<int>(total->number_value);
            }
        }
    }
    
    return 0;
}

// ============================================================================
// HttpAiProvider Feature Implementations (Stubs)
// ============================================================================
std::optional<SchemaSuggestion> HttpAiProvider::DesignSchema(
    const std::string& /*description*/,
    const std::vector<std::string>& /*existing_tables*/) {
    // TODO: Implement schema design using AI
    return std::nullopt;
}

std::optional<QueryOptimization> HttpAiProvider::OptimizeQuery(
    const std::string& /*query*/,
    const std::vector<TableInfo>& /*tables*/) {
    // TODO: Implement query optimization using AI
    return std::nullopt;
}

std::optional<MigrationAssistance> HttpAiProvider::AssistMigration(
    const std::string& /*source_schema*/,
    const std::string& /*target_type*/) {
    // TODO: Implement migration assistance using AI
    return std::nullopt;
}

std::optional<NaturalLanguageToSql> HttpAiProvider::ConvertToSql(
    const std::string& /*natural_language*/,
    const std::vector<std::string>& /*available_tables*/) {
    // TODO: Implement natural language to SQL using AI
    return std::nullopt;
}

std::optional<CodeGeneration> HttpAiProvider::GenerateCode(
    const std::string& description,
    CodeGeneration::TargetLanguage /*language*/) {
    // TODO: Implement code generation using AI
    return std::nullopt;
}

std::optional<DocumentationGeneration> HttpAiProvider::GenerateDocumentation(
    const std::vector<TableInfo>& /*tables*/,
    DocumentationGeneration::DocType /*type*/) {
    // TODO: Implement documentation generation using AI
    return std::nullopt;
}

// ============================================================================
// Provider Registration
// ============================================================================
void AiProviderRegistrar::RegisterAllProviders() {
    auto& manager = AiAssistantManager::Instance();
    
    manager.RegisterProvider(OpenAiProvider::NAME, []() {
        return std::make_unique<OpenAiProvider>();
    });
    
    manager.RegisterProvider(AnthropicProvider::NAME, []() {
        return std::make_unique<AnthropicProvider>();
    });
    
    manager.RegisterProvider(OllamaProvider::NAME, []() {
        return std::make_unique<OllamaProvider>();
    });
    
    manager.RegisterProvider(GeminiProvider::NAME, []() {
        return std::make_unique<GeminiProvider>();
    });
}

} // namespace scratchrobin
