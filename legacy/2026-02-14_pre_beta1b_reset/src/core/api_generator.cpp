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
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <set>

namespace scratchrobin {

namespace fs = std::filesystem;

// Forward declarations for helper functions
static std::string EscapeJson(const std::string& str);
static std::string ToPascalCase(const std::string& str);
static std::string ToSnakeCase(const std::string& str);

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
    oss << "  \"openapi\": \"3.0.3\",\n";
    oss << "  \"info\": {\n";
    oss << "    \"title\": \"" << EscapeJson(config.title) << "\",\n";
    oss << "    \"version\": \"" << config.version << "\"\n";
    oss << "  },\n";
    
    // Servers
    if (!config.servers.empty()) {
        oss << "  \"servers\": [\n";
        for (size_t i = 0; i < config.servers.size(); ++i) {
            oss << "    {\n";
            oss << "      \"url\": \"" << config.servers[i].url << "\",\n";
            oss << "      \"description\": \"" << EscapeJson(config.servers[i].description) << "\"\n";
            oss << "    }";
            if (i < config.servers.size() - 1) oss << ",";
            oss << "\n";
        }
        oss << "  ],\n";
    }
    
    // Paths
    oss << "  \"paths\": {\n";
    bool first = true;
    std::map<std::string, std::vector<const ApiEndpoint*>> paths;
    for (const auto& ep : endpoints) {
        if (ep) {
            paths[ep->path].push_back(ep.get());
        }
    }
    
    for (const auto& [path, eps] : paths) {
        if (!first) oss << ",\n";
        first = false;
        oss << "    \"" << path << "\": {\n";
        bool firstMethod = true;
        for (const auto* ep : eps) {
            if (!firstMethod) oss << ",\n";
            firstMethod = false;
            std::string method = HttpMethodToString(ep->method);
            std::transform(method.begin(), method.end(), method.begin(), ::tolower);
            oss << "      \"" << method << "\": {\n";
            oss << "        \"operationId\": \"" << ep->operation_id << "\",\n";
            oss << "        \"summary\": \"" << EscapeJson(ep->summary) << "\"";
            if (!ep->tags.empty()) {
                oss << ",\n        \"tags\": [";
                for (size_t i = 0; i < ep->tags.size(); ++i) {
                    oss << "\"" << ep->tags[i] << "\"";
                    if (i < ep->tags.size() - 1) oss << ", ";
                }
                oss << "]";
            }
            if (!ep->parameters.empty()) {
                oss << ",\n        \"parameters\": [";
                for (size_t i = 0; i < ep->parameters.size(); ++i) {
                    if (i > 0) oss << ", ";
                    const auto& param = ep->parameters[i];
                    oss << "\n          {\n";
                    oss << "            \"name\": \"" << param.name << "\",\n";
                    oss << "            \"in\": \"" << param.location << "\",\n";
                    oss << "            \"description\": \"" << EscapeJson(param.description) << "\",\n";
                    oss << "            \"required\": " << (param.required ? "true" : "false") << ",\n";
                    oss << "            \"schema\": {\n";
                    oss << "              \"type\": \"" << param.type << "\"\n";
                    oss << "            }\n";
                    oss << "          }";
                }
                oss << "\n        ]";
            }
            oss << "\n      }";
        }
        oss << "\n    }";
    }
    oss << "\n  }\n";
    oss << "}\n";
    return oss.str();
}

std::string ApiSpecification::ExportAsOpenApiYaml() const {
    std::ostringstream oss;
    oss << "openapi: 3.0.3\n";
    oss << "info:\n";
    oss << "  title: " << config.title << "\n";
    oss << "  version: " << config.version << "\n";
    if (!config.servers.empty()) {
        oss << "servers:\n";
        for (const auto& server : config.servers) {
            oss << "  - url: " << server.url << "\n";
            oss << "    description: " << server.description << "\n";
        }
    }
    oss << "paths:\n";
    for (const auto& ep : endpoints) {
        if (ep) {
            std::string method = HttpMethodToString(ep->method);
            std::transform(method.begin(), method.end(), method.begin(), ::tolower);
            oss << "  " << ep->path << ":\n";
            oss << "    " << method << ":\n";
            oss << "      operationId: " << ep->operation_id << "\n";
            oss << "      summary: " << ep->summary << "\n";
        }
    }
    return oss.str();
}

std::unique_ptr<ApiSpecification> ApiSpecification::ImportFromOpenApi(
    const std::string& /*content*/) {
    return std::make_unique<ApiSpecification>();
}

void ApiSpecification::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (file.is_open()) {
        file << ExportAsOpenApiJson();
        file.close();
    }
}

std::unique_ptr<ApiSpecification> ApiSpecification::LoadFromFile(
    const std::string& /*path*/) {
    return std::make_unique<ApiSpecification>();
}

// ============================================================================
// Helper functions
// ============================================================================
static std::string EscapeJson(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c;
        }
    }
    return oss.str();
}

static std::string ToPascalCase(const std::string& str) {
    std::string result;
    bool uppercase = true;
    for (char c : str) {
        if (c == '_' || c == '-') {
            uppercase = true;
        } else if (uppercase) {
            result += std::toupper(c);
            uppercase = false;
        } else {
            result += c;
        }
    }
    return result;
}

static std::string ToSnakeCase(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (std::isupper(c) && !result.empty()) {
            result += '_';
        }
        result += std::tolower(c);
    }
    return result;
}

// ============================================================================
// ApiCodeGenerator Implementation
// ============================================================================
bool ApiCodeGenerator::GenerateServer(const ApiSpecification& spec,
                                       const GenerationOptions& options) {
    if (options.output_directory.empty()) {
        return false;
    }
    fs::create_directories(options.output_directory);
    
    // Generate based on language/framework
    if (options.language == CodeLanguage::PYTHON && options.framework == "fastapi") {
        // Generate main.py
        std::ofstream mainFile(options.output_directory + "/main.py");
        mainFile << GeneratePythonFastApi(spec);
        mainFile.close();
        
        // Generate requirements.txt
        std::ofstream reqFile(options.output_directory + "/requirements.txt");
        reqFile << "fastapi==0.104.1\nuvicorn[standard]==0.24.0\npydantic==2.5.0\n";
        reqFile.close();
    } else if (options.language == CodeLanguage::JAVASCRIPT && options.framework == "express") {
        std::ofstream mainFile(options.output_directory + "/main.js");
        mainFile << GenerateNodeExpress(spec);
        mainFile.close();
        
        std::ofstream pkgFile(options.output_directory + "/package.json");
        pkgFile << "{\n  \"name\": \"api-server\",\n  \"version\": \"1.0.0\",\n";
        pkgFile << "  \"dependencies\": {\n    \"express\": \"^4.18.2\",\n    \"cors\": \"^2.8.5\"\n  }\n}\n";
        pkgFile.close();
    } else {
        return false;
    }
    
    // Generate models
    for (const auto& [schemaName, fields] : spec.schemas) {
        std::string modelCode = GenerateModel(schemaName, fields, options.language);
        std::string ext = (options.language == CodeLanguage::PYTHON) ? ".py" : ".js";
        std::ofstream modelFile(options.output_directory + "/" + ToSnakeCase(schemaName) + "_model" + ext);
        modelFile << modelCode;
        modelFile.close();
    }
    
    // Generate Dockerfile if requested
    if (options.include_docker) {
        std::string dockerfile = GenerateDockerfile(options.language);
        std::ofstream dockerFile(options.output_directory + "/Dockerfile");
        dockerFile << dockerfile;
        dockerFile.close();
    }
    
    return true;
}

bool ApiCodeGenerator::GenerateClient(const ApiSpecification& spec,
                                       const GenerationOptions& options) {
    if (options.output_directory.empty()) {
        return false;
    }
    fs::create_directories(options.output_directory);
    
    std::string clientCode;
    std::string ext;
    
    if (options.language == CodeLanguage::PYTHON) {
        clientCode = GeneratePythonClient(spec);
        ext = ".py";
    } else if (options.language == CodeLanguage::JAVASCRIPT) {
        clientCode = GenerateJavaScriptClient(spec, false);
        ext = ".js";
    } else if (options.language == CodeLanguage::TYPESCRIPT) {
        clientCode = GenerateJavaScriptClient(spec, true);
        ext = ".ts";
    } else {
        return false;
    }
    
    std::ofstream file(options.output_directory + "/client" + ext);
    file << clientCode;
    file.close();
    
    return true;
}

std::string ApiCodeGenerator::GenerateController(const ApiEndpoint& endpoint,
                                                  CodeLanguage language,
                                                  const std::string& framework) {
    if (language == CodeLanguage::PYTHON && framework == "fastapi") {
        return GeneratePythonController(endpoint);
    }
    return "// Controller not implemented\n";
}

std::string ApiCodeGenerator::GenerateModel(const std::string& schema_name,
                                             const std::vector<ApiField>& fields,
                                             CodeLanguage language) {
    if (language == CodeLanguage::PYTHON) {
        return GeneratePythonModel(schema_name, fields);
    }
    return "# Model not implemented\n";
}

std::string ApiCodeGenerator::GenerateTests(const ApiEndpoint& endpoint,
                                             CodeLanguage language) {
    if (language == CodeLanguage::PYTHON) {
        return GeneratePythonTests(endpoint);
    }
    return "# Tests not implemented\n";
}

std::string ApiCodeGenerator::GenerateDockerfile(CodeLanguage language) {
    if (language == CodeLanguage::PYTHON) {
        return "FROM python:3.11-slim\n\nWORKDIR /app\n\nCOPY requirements.txt .\nRUN pip install --no-cache-dir -r requirements.txt\n\nCOPY . .\n\nEXPOSE 8000\n\nCMD [\"uvicorn\", \"main:app\", \"--host\", \"0.0.0.0\", \"--port\", \"8000\"]\n";
    }
    return "# Dockerfile not available\n";
}

// Python/FastAPI Generation
std::string ApiCodeGenerator::GeneratePythonFastApi(const ApiSpecification& spec) {
    std::ostringstream oss;
    oss << "\"\"\"\n" << spec.config.title << "\nGenerated by ScratchRobin\n\"\"\"\n\n";
    oss << "from fastapi import FastAPI, HTTPException\n";
    oss << "from pydantic import BaseModel\n";
    oss << "from typing import List, Optional\n\n";
    oss << "app = FastAPI(title=\"" << spec.config.title << "\", version=\"" << spec.config.version << "\")\n\n";
    
    // Models
    for (const auto& [name, fields] : spec.schemas) {
        oss << GeneratePythonModel(name, fields) << "\n";
    }
    
    // Routes
    for (const auto& ep : spec.endpoints) {
        if (ep) {
            oss << GeneratePythonController(*ep) << "\n";
        }
    }
    
    return oss.str();
}

std::string ApiCodeGenerator::GeneratePythonModel(const std::string& schema_name,
                                                   const std::vector<ApiField>& fields) {
    std::ostringstream oss;
    std::string className = ToPascalCase(schema_name);
    oss << "class " << className << "(BaseModel):\n";
    for (const auto& field : fields) {
        oss << "    " << field.name << ": ";
        if (field.type == "string") oss << "str";
        else if (field.type == "integer") oss << "int";
        else if (field.type == "boolean") oss << "bool";
        else if (field.type == "number") oss << "float";
        else oss << "str";
        if (!field.required) oss << " = None";
        oss << "\n";
    }
    return oss.str();
}

std::string ApiCodeGenerator::GeneratePythonController(const ApiEndpoint& ep) {
    std::ostringstream oss;
    std::string method = HttpMethodToString(ep.method);
    std::transform(method.begin(), method.end(), method.begin(), ::tolower);
    oss << "@app." << method << "(\"" << ep.path << "\")\n";
    oss << "async def " << ep.operation_id << "():\n";
    oss << "    \"\"\"" << ep.summary << "\"\"\"\n";
    oss << "    return {\"message\": \"" << ep.summary << "\"}\n";
    return oss.str();
}

std::string ApiCodeGenerator::GeneratePythonClient(const ApiSpecification& spec) {
    std::ostringstream oss;
    oss << "\"\"\"API Client for " << spec.config.title << "\"\"\"\n\n";
    oss << "import requests\n\n";
    oss << "class ApiClient:\n";
    oss << "    def __init__(self, base_url, api_key=None):\n";
    oss << "        self.base_url = base_url\n";
    oss << "        self.api_key = api_key\n\n";
    
    for (const auto& ep : spec.endpoints) {
        if (!ep) continue;
        std::string method = HttpMethodToString(ep->method);
        std::transform(method.begin(), method.end(), method.begin(), ::tolower);
        oss << "    def " << ep->operation_id << "(self):\n";
        oss << "        return requests." << method << "(f\"{self.base_url}" << ep->path << "\").json()\n\n";
    }
    return oss.str();
}

std::string ApiCodeGenerator::GeneratePythonTests(const ApiEndpoint& ep) {
    std::ostringstream oss;
    oss << "import pytest\n";
    oss << "from fastapi.testclient import TestClient\n";
    oss << "from main import app\n\n";
    oss << "client = TestClient(app)\n\n";
    oss << "def test_" << ep.operation_id << "():\n";
    std::string method = HttpMethodToString(ep.method);
    std::transform(method.begin(), method.end(), method.begin(), ::tolower);
    oss << "    response = client." << method << "(\"" << ep.path << "\")\n";
    oss << "    assert response.status_code == 200\n";
    return oss.str();
}

// Node/Express Generation
std::string ApiCodeGenerator::GenerateNodeExpress(const ApiSpecification& spec) {
    std::ostringstream oss;
    oss << "const express = require('express');\n";
    oss << "const app = express();\n";
    oss << "app.use(express.json());\n\n";
    
    for (const auto& ep : spec.endpoints) {
        if (ep) {
            std::string method = HttpMethodToString(ep->method);
            std::transform(method.begin(), method.end(), method.begin(), ::tolower);
            oss << "app." << method << "('" << ep->path << "', (req, res) => {\n";
            oss << "    res.json({ message: '" << ep->summary << "' });\n";
            oss << "});\n\n";
        }
    }
    
    oss << "app.listen(3000, () => console.log('Server running on port 3000'));\n";
    return oss.str();
}

// JavaScript/TypeScript Client
std::string ApiCodeGenerator::GenerateJavaScriptClient(const ApiSpecification& spec, bool isTypeScript) {
    std::ostringstream oss;
    if (isTypeScript) {
        oss << "export class ApiClient {\n";
        oss << "    constructor(private baseUrl: string, private apiKey?: string) {}\n\n";
    } else {
        oss << "class ApiClient {\n";
        oss << "    constructor(baseUrl, apiKey) {\n";
        oss << "        this.baseUrl = baseUrl;\n";
        oss << "        this.apiKey = apiKey;\n";
        oss << "    }\n\n";
    }
    
    for (const auto& ep : spec.endpoints) {
        if (!ep) continue;
        std::string method = HttpMethodToString(ep->method);
        if (isTypeScript) {
            oss << "    async " << ep->operation_id << "(): Promise<any> {\n";
        } else {
            oss << "    async " << ep->operation_id << "() {\n";
        }
        oss << "        return fetch(this.baseUrl + '" << ep->path << "', { method: '" << method << "' }).then(r => r.json());\n";
        oss << "    }\n\n";
    }
    
    oss << "}\n";
    if (!isTypeScript) {
        oss << "module.exports = { ApiClient };\n";
    }
    return oss.str();
}


// DatabaseApiMapper Implementation
std::vector<std::unique_ptr<ApiEndpoint>> DatabaseApiMapper::MapTable(
    const TableMapping& mapping) {
    std::vector<std::unique_ptr<ApiEndpoint>> endpoints;
    std::string resource = mapping.api_resource_name.empty() ? mapping.table : mapping.api_resource_name;
    
    if (mapping.enable_list) {
        auto ep = std::make_unique<ApiEndpoint>();
        ep->id = "list_" + resource;
        ep->path = "/" + resource;
        ep->method = HttpMethod::GET;
        ep->summary = "List all " + resource;
        ep->operation_id = "list" + ToPascalCase(resource);
        ep->tags = {resource};
        endpoints.push_back(std::move(ep));
    }
    
    if (mapping.enable_get) {
        auto ep = std::make_unique<ApiEndpoint>();
        ep->id = "get_" + resource;
        ep->path = "/" + resource + "/{id}";
        ep->method = HttpMethod::GET;
        ep->summary = "Get " + resource + " by ID";
        ep->operation_id = "get" + ToPascalCase(resource);
        ep->tags = {resource};
        ApiEndpoint::Parameter idParam;
        idParam.name = "id";
        idParam.location = "path";
        idParam.type = "string";
        idParam.required = true;
        ep->parameters.push_back(idParam);
        endpoints.push_back(std::move(ep));
    }
    
    if (mapping.enable_create) {
        auto ep = std::make_unique<ApiEndpoint>();
        ep->id = "create_" + resource;
        ep->path = "/" + resource;
        ep->method = HttpMethod::POST;
        ep->summary = "Create new " + resource;
        ep->operation_id = "create" + ToPascalCase(resource);
        ep->tags = {resource};
        endpoints.push_back(std::move(ep));
    }
    
    if (mapping.enable_update) {
        auto ep = std::make_unique<ApiEndpoint>();
        ep->id = "update_" + resource;
        ep->path = "/" + resource + "/{id}";
        ep->method = HttpMethod::PUT;
        ep->summary = "Update " + resource;
        ep->operation_id = "update" + ToPascalCase(resource);
        ep->tags = {resource};
        endpoints.push_back(std::move(ep));
    }
    
    if (mapping.enable_delete) {
        auto ep = std::make_unique<ApiEndpoint>();
        ep->id = "delete_" + resource;
        ep->path = "/" + resource + "/{id}";
        ep->method = HttpMethod::DELETE;
        ep->summary = "Delete " + resource;
        ep->operation_id = "delete" + ToPascalCase(resource);
        ep->tags = {resource};
        endpoints.push_back(std::move(ep));
    }
    
    return endpoints;
}

std::vector<DatabaseApiMapper::TableMapping> DatabaseApiMapper::AutoGenerateMappings(
    const std::string& /*connection_string*/,
    const std::vector<std::string>& tables) {
    std::vector<TableMapping> mappings;
    for (const auto& table : tables) {
        TableMapping mapping;
        mapping.table = table;
        mapping.schema = "public";
        mapping.api_resource_name = table;
        mapping.api_singular_name = table;
        if (mapping.api_singular_name.length() > 1 && mapping.api_singular_name.back() == 's') {
            mapping.api_singular_name.pop_back();
        }
        mapping.enable_list = true;
        mapping.enable_get = true;
        mapping.enable_create = true;
        mapping.enable_update = true;
        mapping.enable_delete = true;
        mappings.push_back(mapping);
    }
    return mappings;
}

std::string DatabaseApiMapper::MapDbTypeToApiType(const std::string& db_type) {
    if (db_type == "integer" || db_type == "int" || db_type == "bigint") return "integer";
    if (db_type == "varchar" || db_type == "text" || db_type == "char") return "string";
    if (db_type == "boolean" || db_type == "bool") return "boolean";
    if (db_type == "numeric" || db_type == "decimal" || db_type == "real" || db_type == "float") return "number";
    if (db_type == "timestamp" || db_type == "date" || db_type == "time") return "string";
    return "string";
}

// ApiDocumentationGenerator Implementation
std::string ApiDocumentationGenerator::GenerateMarkdown(const ApiSpecification& spec) {
    std::ostringstream oss;
    oss << "# " << spec.config.title << "\n\n";
    oss << "**Version:** " << spec.config.version << "\n\n";
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
    oss << "<!DOCTYPE html>\n<html>\n<head><title>" << spec.config.title << "</title></head>\n<body>\n";
    oss << "<h1>" << spec.config.title << "</h1>\n";
    oss << "<p>Version: " << spec.config.version << "</p>\n";
    oss << "</body>\n</html>\n";
    return oss.str();
}

std::string ApiDocumentationGenerator::GeneratePostmanCollection(
    const ApiSpecification& spec) {
    std::ostringstream oss;
    oss << "{\"info\":{\"name\":\"" << spec.config.title << "\"},\"item\":[\n";
    bool first = true;
    for (const auto& ep : spec.endpoints) {
        if (!ep) continue;
        if (!first) oss << ",";
        first = false;
        std::string method = HttpMethodToString(ep->method);
        oss << "{\"name\":\"" << ep->summary << "\",\"request\":{\"method\":\"" << method << "\",\"url\":{\"raw\":\"" << ep->path << "\"}}}\n";
    }
    oss << "]}\n";
    return oss.str();
}

std::string ApiDocumentationGenerator::GenerateCurlExamples(const ApiEndpoint& ep) {
    std::ostringstream oss;
    oss << "# " << ep.summary << "\n";
    oss << "curl -X " << HttpMethodToString(ep.method) << " http://localhost:8000" << ep.path << "\n";
    return oss.str();
}

// ApiGenerator Implementation
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
    const std::string& connection_string,
    const std::vector<std::string>& tables,
    const DatabaseApiMapper::TableMapping& template_mapping) {
    auto spec = std::make_unique<ApiSpecification>();
    spec->config = config_;
    
    auto mappings = mapper_.AutoGenerateMappings(connection_string, tables);
    for (auto& mapping : mappings) {
        mapping.enable_list = template_mapping.enable_list;
        mapping.enable_get = template_mapping.enable_get;
        mapping.enable_create = template_mapping.enable_create;
        mapping.enable_update = template_mapping.enable_update;
        mapping.enable_delete = template_mapping.enable_delete;
        
        auto endpoints = mapper_.MapTable(mapping);
        for (auto& ep : endpoints) {
            spec->AddEndpoint(std::move(ep));
        }
        
        std::vector<ApiField> fields;
        ApiField idField;
        idField.name = "id";
        idField.type = "integer";
        idField.required = true;
        fields.push_back(idField);
        spec->AddSchema(mapping.api_singular_name, fields);
    }
    
    return spec;
}

std::vector<std::unique_ptr<ApiEndpoint>> ApiGenerator::GenerateCrudEndpoints(
    const std::string& table_name,
    const std::vector<ApiField>& /*fields*/) {
    DatabaseApiMapper::TableMapping mapping;
    mapping.table = table_name;
    mapping.api_resource_name = table_name;
    mapping.api_singular_name = table_name;
    if (mapping.api_singular_name.length() > 1 && mapping.api_singular_name.back() == 's') {
        mapping.api_singular_name.pop_back();
    }
    return mapper_.MapTable(mapping);
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

bool ApiGenerator::ValidateSpecification(const ApiSpecification& spec,
                                          std::vector<std::string>& errors) {
    errors.clear();
    bool valid = true;
    std::set<std::string> operationIds;
    for (const auto& ep : spec.endpoints) {
        if (ep) {
            if (operationIds.count(ep->operation_id)) {
                errors.push_back("Duplicate: " + ep->operation_id);
                valid = false;
            }
            operationIds.insert(ep->operation_id);
        }
    }
    return valid;
}

} // namespace scratchrobin
