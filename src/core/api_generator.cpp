/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "api_generator.h"

#include <algorithm>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// HTTP Method Helpers
// ============================================================================
std::string HttpMethodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

std::string CodeLanguageToString(CodeLanguage lang) {
    switch (lang) {
        case CodeLanguage::PYTHON: return "Python";
        case CodeLanguage::JAVASCRIPT: return "JavaScript";
        case CodeLanguage::TYPESCRIPT: return "TypeScript";
        case CodeLanguage::JAVA: return "Java";
        case CodeLanguage::CSHARP: return "C#";
        case CodeLanguage::GO: return "Go";
        case CodeLanguage::RUST: return "Rust";
        case CodeLanguage::PHP: return "PHP";
        case CodeLanguage::RUBY: return "Ruby";
        default: return "Unknown";
    }
}

// ============================================================================
// ApiSpecification Implementation
// ============================================================================
void ApiSpecification::AddEndpoint(std::unique_ptr<ApiEndpoint> endpoint) {
    if (endpoint) {
        endpoints.push_back(std::move(endpoint));
    }
}

void ApiSpecification::RemoveEndpoint(const std::string& id) {
    endpoints.erase(
        std::remove_if(endpoints.begin(), endpoints.end(),
                       [&id](const auto& ep) { return ep && ep->id == id; }),
        endpoints.end());
}

ApiEndpoint* ApiSpecification::FindEndpoint(const std::string& id) {
    for (auto& ep : endpoints) {
        if (ep && ep->id == id) {
            return ep.get();
        }
    }
    return nullptr;
}

void ApiSpecification::AddSchema(const std::string& name, 
                                  const std::vector<ApiField>& fields) {
    schemas[name] = fields;
}

std::string ApiSpecification::ExportAsOpenApiJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"openapi\": \"3.0.0\",\n";
    oss << "  \"info\": {\n";
    oss << "    \"title\": \"" << config.title << "\",\n";
    oss << "    \"version\": \"" << config.version << "\"\n";
    oss << "  },\n";
    oss << "  \"paths\": {}\n";
    oss << "}\n";
    return oss.str();
}

std::string ApiSpecification::ExportAsOpenApiYaml() const {
    std::ostringstream oss;
    oss << "openapi: 3.0.0\n";
    oss << "info:\n";
    oss << "  title: " << config.title << "\n";
    oss << "  version: " << config.version << "\n";
    oss << "paths: {}\n";
    return oss.str();
}

std::unique_ptr<ApiSpecification> ApiSpecification::ImportFromOpenApi(
    const std::string& /*content*/) {
    return std::make_unique<ApiSpecification>();
}

void ApiSpecification::SaveToFile(const std::string& /*path*/) const {
    // Stub
}

std::unique_ptr<ApiSpecification> ApiSpecification::LoadFromFile(
    const std::string& /*path*/) {
    return std::make_unique<ApiSpecification>();
}

// ============================================================================
// ApiCodeGenerator Implementation
// ============================================================================
bool ApiCodeGenerator::GenerateServer(const ApiSpecification& /*spec*/,
                                       const GenerationOptions& /*options*/) {
    return true;  // Stub
}

bool ApiCodeGenerator::GenerateClient(const ApiSpecification& /*spec*/,
                                       const GenerationOptions& /*options*/) {
    return true;  // Stub
}

std::string ApiCodeGenerator::GenerateController(const ApiEndpoint& /*endpoint*/,
                                                  CodeLanguage /*language*/,
                                                  const std::string& /*framework*/) {
    return "// Controller stub\n";
}

std::string ApiCodeGenerator::GenerateModel(const std::string& /*schema_name*/,
                                             const std::vector<ApiField>& /*fields*/,
                                             CodeLanguage /*language*/) {
    return "// Model stub\n";
}

std::string ApiCodeGenerator::GenerateTests(const ApiEndpoint& /*endpoint*/,
                                             CodeLanguage /*language*/) {
    return "// Tests stub\n";
}

std::string ApiCodeGenerator::GenerateDockerfile(CodeLanguage /*language*/) {
    return "# Dockerfile stub\n";
}

std::string ApiCodeGenerator::GeneratePythonFastApi(const ApiSpecification& /*spec*/) {
    return "# FastAPI stub\n";
}

std::string ApiCodeGenerator::GenerateNodeExpress(const ApiSpecification& /*spec*/) {
    return "// Express.js stub\n";
}

std::string ApiCodeGenerator::GenerateJavaSpring(const ApiSpecification& /*spec*/) {
    return "// Spring Boot stub\n";
}

std::string ApiCodeGenerator::GenerateGoGin(const ApiSpecification& /*spec*/) {
    return "// Gin stub\n";
}

// ============================================================================
// DatabaseApiMapper Implementation
// ============================================================================
std::vector<std::unique_ptr<ApiEndpoint>> DatabaseApiMapper::MapTable(
    const TableMapping& /*mapping*/) {
    return {};  // Stub
}

std::vector<DatabaseApiMapper::TableMapping> DatabaseApiMapper::AutoGenerateMappings(
    const std::string& /*connection_string*/,
    const std::vector<std::string>& /*tables*/) {
    return {};  // Stub
}

std::string DatabaseApiMapper::MapDbTypeToApiType(const std::string& db_type) {
    if (db_type == "integer" || db_type == "int" || db_type == "bigint") {
        return "integer";
    }
    if (db_type == "varchar" || db_type == "text" || db_type == "char") {
        return "string";
    }
    if (db_type == "boolean" || db_type == "bool") {
        return "boolean";
    }
    if (db_type == "numeric" || db_type == "decimal" || db_type == "real" || db_type == "float") {
        return "number";
    }
    if (db_type == "timestamp" || db_type == "date" || db_type == "time") {
        return "string";
    }
    return "string";
}

// ============================================================================
// ApiDocumentationGenerator Implementation
// ============================================================================
std::string ApiDocumentationGenerator::GenerateMarkdown(const ApiSpecification& spec) {
    std::ostringstream oss;
    oss << "# " << spec.config.title << "\n\n";
    oss << "Version: " << spec.config.version << "\n\n";
    oss << spec.config.description << "\n\n";
    oss << "## Endpoints\n\n";
    for (const auto& ep : spec.endpoints) {
        if (ep) {
            oss << "### " << HttpMethodToString(ep->method) << " " << ep->path << "\n\n";
            oss << ep->summary << "\n\n";
        }
    }
    return oss.str();
}

std::string ApiDocumentationGenerator::GenerateHtml(const ApiSpecification& spec) {
    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n<html>\n<head>\n";
    oss << "<title>" << spec.config.title << "</title>\n";
    oss << "</head>\n<body>\n";
    oss << "<h1>" << spec.config.title << "</h1>\n";
    oss << "<p>Version: " << spec.config.version << "</p>\n";
    oss << "</body>\n</html>\n";
    return oss.str();
}

std::string ApiDocumentationGenerator::GeneratePostmanCollection(
    const ApiSpecification& /*spec*/) {
    return "{}";
}

std::string ApiDocumentationGenerator::GenerateCurlExamples(const ApiEndpoint& /*endpoint*/) {
    return "# curl example\n";
}

// ============================================================================
// ApiGenerator Implementation
// ============================================================================
ApiGenerator& ApiGenerator::Instance() {
    static ApiGenerator instance;
    return instance;
}

void ApiGenerator::SetConfiguration(const ApiConfiguration& config) {
    config_ = config;
}

ApiConfiguration* ApiGenerator::GetConfiguration() {
    return &config_;
}

std::unique_ptr<ApiSpecification> ApiGenerator::GenerateFromDatabase(
    const std::string& /*connection_string*/,
    const std::vector<std::string>& /*tables*/,
    const DatabaseApiMapper::TableMapping& /*template_mapping*/) {
    auto spec = std::make_unique<ApiSpecification>();
    spec->config = config_;
    return spec;
}

std::vector<std::unique_ptr<ApiEndpoint>> ApiGenerator::GenerateCrudEndpoints(
    const std::string& table_name,
    const std::vector<ApiField>& /*fields*/) {
    std::vector<std::unique_ptr<ApiEndpoint>> endpoints;
    
    // Create LIST endpoint
    auto list = std::make_unique<ApiEndpoint>();
    list->id = "list_" + table_name;
    list->path = "/" + table_name;
    list->method = HttpMethod::GET;
    list->summary = "List all " + table_name;
    list->operation_id = "list" + table_name;
    endpoints.push_back(std::move(list));
    
    // Create GET endpoint
    auto get = std::make_unique<ApiEndpoint>();
    get->id = "get_" + table_name;
    get->path = "/" + table_name + "/{id}";
    get->method = HttpMethod::GET;
    get->summary = "Get a " + table_name + " by ID";
    get->operation_id = "get" + table_name;
    endpoints.push_back(std::move(get));
    
    // Create POST endpoint
    auto create = std::make_unique<ApiEndpoint>();
    create->id = "create_" + table_name;
    create->path = "/" + table_name;
    create->method = HttpMethod::POST;
    create->summary = "Create a new " + table_name;
    create->operation_id = "create" + table_name;
    endpoints.push_back(std::move(create));
    
    // Create PUT endpoint
    auto update = std::make_unique<ApiEndpoint>();
    update->id = "update_" + table_name;
    update->path = "/" + table_name + "/{id}";
    update->method = HttpMethod::PUT;
    update->summary = "Update a " + table_name;
    update->operation_id = "update" + table_name;
    endpoints.push_back(std::move(update));
    
    // Create DELETE endpoint
    auto del = std::make_unique<ApiEndpoint>();
    del->id = "delete_" + table_name;
    del->path = "/" + table_name + "/{id}";
    del->method = HttpMethod::DELETE;
    del->summary = "Delete a " + table_name;
    del->operation_id = "delete" + table_name;
    endpoints.push_back(std::move(del));
    
    return endpoints;
}

void ApiGenerator::AddCustomEndpoint(ApiSpecification& spec,
                                      std::unique_ptr<ApiEndpoint> endpoint) {
    spec.AddEndpoint(std::move(endpoint));
}

bool ApiGenerator::GenerateServerCode(const ApiSpecification& spec,
                                       const ApiCodeGenerator::GenerationOptions& options) {
    return code_generator_.GenerateServer(spec, options);
}

bool ApiGenerator::GenerateClientSdk(const ApiSpecification& spec,
                                      const ApiCodeGenerator::GenerationOptions& options) {
    return code_generator_.GenerateClient(spec, options);
}

std::string ApiGenerator::GenerateOpenApiSpec(const ApiSpecification& spec) {
    return spec.ExportAsOpenApiJson();
}

bool ApiGenerator::ValidateSpecification(const ApiSpecification& /*spec*/,
                                          std::vector<std::string>& errors) {
    errors.clear();
    return true;
}

} // namespace scratchrobin
