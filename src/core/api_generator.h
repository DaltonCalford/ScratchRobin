/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_API_GENERATOR_H
#define SCRATCHROBIN_API_GENERATOR_H

#include <ctime>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin {

// ============================================================================
// API Endpoint Types
// ============================================================================
enum class HttpMethod {
    GET,
    POST,
    PUT,
    PATCH,
    DELETE,
    HEAD,
    OPTIONS
};

std::string HttpMethodToString(HttpMethod method);

// ============================================================================
// API Authentication Types
// ============================================================================
enum class AuthType {
    NONE,
    API_KEY,
    JWT,
    OAUTH2,
    BASIC_AUTH,
    CUSTOM
};

// ============================================================================
// API Field Definition
// ============================================================================
struct ApiField {
    std::string name;
    std::string type;  // "string", "integer", "boolean", "number", "array", "object"
    std::string format;  // "date-time", "email", "uuid", etc.
    std::string description;
    bool required = false;
    bool read_only = false;
    
    // Validation
    struct Validation {
        int min_length = 0;
        int max_length = 0;
        std::string pattern;  // regex
        double minimum = 0;
        double maximum = 0;
        bool exclusive_min = false;
        bool exclusive_max = false;
        std::vector<std::string> enum_values;
    };
    Validation validation;
    
    // Database mapping
    std::string db_column;
    std::string db_table;
    
    // Nested object fields (for type="object")
    std::vector<ApiField> nested_fields;
};

// ============================================================================
// API Endpoint Definition
// ============================================================================
struct ApiEndpoint {
    std::string id;
    std::string path;
    HttpMethod method;
    std::string summary;
    std::string description;
    std::string operation_id;
    std::vector<std::string> tags;
    
    // Parameters
    struct Parameter {
        std::string name;
        std::string location;  // "path", "query", "header", "cookie"
        std::string type;
        std::string description;
        bool required = false;
    };
    std::vector<Parameter> parameters;
    
    // Request body
    struct RequestBody {
        std::string description;
        std::string content_type;  // "application/json", "multipart/form-data", etc.
        std::vector<ApiField> fields;
        bool required = false;
    };
    std::optional<RequestBody> request_body;
    
    // Responses
    struct Response {
        int status_code;
        std::string description;
        std::string content_type;
        std::vector<ApiField> fields;
    };
    std::map<int, Response> responses;
    
    // Security
    std::vector<std::string> security_requirements;
    
    // Database mapping
    std::string source_table;
    std::string source_schema;
    std::string custom_query;  // For custom endpoints
    
    // Metadata
    bool deprecated = false;
    std::map<std::string, std::string> extensions;
};

// ============================================================================
// API Configuration
// ============================================================================
struct ApiConfiguration {
    // Basic info
    std::string title;
    std::string version;
    std::string description;
    std::string terms_of_service;
    
    // Contact info
    struct Contact {
        std::string name;
        std::string url;
        std::string email;
    };
    Contact contact;
    
    // License
    struct License {
        std::string name;
        std::string url;
    };
    License license;
    
    // Server configuration
    struct Server {
        std::string url;
        std::string description;
        std::map<std::string, std::string> variables;
    };
    std::vector<Server> servers;
    
    // Security
    AuthType default_auth = AuthType::JWT;
    struct SecurityScheme {
        std::string name;
        AuthType type;
        std::string description;
        // JWT specific
        std::string jwt_issuer;
        std::string jwt_audience;
        // API Key specific
        std::string api_key_header;
        // OAuth2 specific
        std::string oauth_authorization_url;
        std::string oauth_token_url;
        std::vector<std::string> oauth_scopes;
    };
    std::map<std::string, SecurityScheme> security_schemes;
    
    // Rate limiting
    struct RateLimit {
        int requests_per_minute = 100;
        int requests_per_hour = 1000;
        int burst_size = 10;
    };
    RateLimit rate_limit;
    
    // CORS
    struct Cors {
        bool enabled = true;
        std::vector<std::string> allowed_origins;
        std::vector<std::string> allowed_methods;
        std::vector<std::string> allowed_headers;
        bool allow_credentials = false;
        int max_age = 3600;
    };
    Cors cors;
    
    // Pagination
    struct Pagination {
        std::string type = "offset";  // "offset", "cursor", "page"
        int default_page_size = 20;
        int max_page_size = 100;
    };
    Pagination pagination;
};

// ============================================================================
// API Specification (OpenAPI/Swagger)
// ============================================================================
class ApiSpecification {
public:
    ApiConfiguration config;
    std::vector<std::unique_ptr<ApiEndpoint>> endpoints;
    
    // Schema definitions (reusable components)
    std::map<std::string, std::vector<ApiField>> schemas;
    
    // Add endpoint
    void AddEndpoint(std::unique_ptr<ApiEndpoint> endpoint);
    void RemoveEndpoint(const std::string& id);
    ApiEndpoint* FindEndpoint(const std::string& id);
    
    // Schema management
    void AddSchema(const std::string& name, const std::vector<ApiField>& fields);
    
    // Export formats
    std::string ExportAsOpenApiJson() const;
    std::string ExportAsOpenApiYaml() const;
    
    // Import
    static std::unique_ptr<ApiSpecification> ImportFromOpenApi(const std::string& content);
    
    // Save/Load
    void SaveToFile(const std::string& path) const;
    static std::unique_ptr<ApiSpecification> LoadFromFile(const std::string& path);
};

// ============================================================================
// Code Generation Templates
// ============================================================================
enum class CodeLanguage {
    PYTHON,
    JAVASCRIPT,
    TYPESCRIPT,
    JAVA,
    CSHARP,
    GO,
    RUST,
    PHP,
    RUBY
};

std::string CodeLanguageToString(CodeLanguage lang);

// ============================================================================
// API Code Generator
// ============================================================================
class ApiCodeGenerator {
public:
    struct GenerationOptions {
        CodeLanguage language;
        std::string framework;  // "fastapi", "express", "spring", etc.
        bool include_validation = true;
        bool include_auth = true;
        bool include_tests = false;
        bool include_docker = false;
        std::string output_directory;
    };
    
    // Generate server code
    bool GenerateServer(const ApiSpecification& spec,
                        const GenerationOptions& options);
    
    // Generate client SDK
    bool GenerateClient(const ApiSpecification& spec,
                        const GenerationOptions& options);
    
    // Generate specific files
    std::string GenerateController(const ApiEndpoint& endpoint,
                                    CodeLanguage language,
                                    const std::string& framework);
    
    std::string GenerateModel(const std::string& schema_name,
                               const std::vector<ApiField>& fields,
                               CodeLanguage language);
    
    std::string GenerateTests(const ApiEndpoint& endpoint,
                               CodeLanguage language);
    
    std::string GenerateDockerfile(CodeLanguage language);
    
private:
    std::string GeneratePythonFastApi(const ApiSpecification& spec);
    std::string GenerateNodeExpress(const ApiSpecification& spec);
    std::string GenerateJavaSpring(const ApiSpecification& spec);
    std::string GenerateGoGin(const ApiSpecification& spec);
};

// ============================================================================
// Database-to-API Mapper
// ============================================================================
class DatabaseApiMapper {
public:
    struct TableMapping {
        std::string schema;
        std::string table;
        std::string api_resource_name;  // Pluralized, e.g., "users"
        std::string api_singular_name;  // e.g., "user"
        
        // CRUD operations to generate
        bool enable_list = true;
        bool enable_get = true;
        bool enable_create = true;
        bool enable_update = true;
        bool enable_delete = true;
        bool enable_search = true;
        
        // Field mappings
        std::map<std::string, ApiField> field_mappings;
        
        // Relationships
        struct Relationship {
            std::string name;
            std::string type;  // "one-to-one", "one-to-many", "many-to-many"
            std::string target_table;
            std::string foreign_key;
        };
        std::vector<Relationship> relationships;
    };
    
    // Map database table to API endpoints
    std::vector<std::unique_ptr<ApiEndpoint>> MapTable(const TableMapping& mapping);
    
    // Auto-generate mappings from database schema
    std::vector<TableMapping> AutoGenerateMappings(
        const std::string& connection_string,
        const std::vector<std::string>& tables);
    
    // Type mapping
    static std::string MapDbTypeToApiType(const std::string& db_type);
};

// ============================================================================
// API Documentation Generator
// ============================================================================
class ApiDocumentationGenerator {
public:
    // Generate human-readable documentation
    std::string GenerateMarkdown(const ApiSpecification& spec);
    std::string GenerateHtml(const ApiSpecification& spec);
    
    // Generate Postman collection
    std::string GeneratePostmanCollection(const ApiSpecification& spec);
    
    // Generate curl examples
    std::string GenerateCurlExamples(const ApiEndpoint& endpoint);
};

// ============================================================================
// API Generator (Main API)
// ============================================================================
class ApiGenerator {
public:
    static ApiGenerator& Instance();
    
    // Configuration
    void SetConfiguration(const ApiConfiguration& config);
    ApiConfiguration* GetConfiguration();
    
    // Generate from database
    std::unique_ptr<ApiSpecification> GenerateFromDatabase(
        const std::string& connection_string,
        const std::vector<std::string>& tables,
        const DatabaseApiMapper::TableMapping& template_mapping);
    
    // Generate CRUD endpoints for a table
    std::vector<std::unique_ptr<ApiEndpoint>> GenerateCrudEndpoints(
        const std::string& table_name,
        const std::vector<ApiField>& fields);
    
    // Add custom endpoint
    void AddCustomEndpoint(ApiSpecification& spec, 
                           std::unique_ptr<ApiEndpoint> endpoint);
    
    // Export/Generate
    bool GenerateServerCode(const ApiSpecification& spec,
                            const ApiCodeGenerator::GenerationOptions& options);
    
    bool GenerateClientSdk(const ApiSpecification& spec,
                           const ApiCodeGenerator::GenerationOptions& options);
    
    std::string GenerateOpenApiSpec(const ApiSpecification& spec);
    
    // Validation
    bool ValidateSpecification(const ApiSpecification& spec,
                                std::vector<std::string>& errors);
    
private:
    ApiGenerator() = default;
    ~ApiGenerator() = default;
    
    ApiConfiguration config_;
    DatabaseApiMapper mapper_;
    ApiCodeGenerator code_generator_;
    ApiDocumentationGenerator doc_generator_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_API_GENERATOR_H
