/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "docker_config.h"

#include <algorithm>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// Docker Container Config
// ============================================================================

DockerContainerConfig::DockerContainerConfig() {
    SetDefaultPorts();
    SetDefaultDirectories();
}

void DockerContainerConfig::SetDefaultPorts() {
    native_service.port = 3050;
    postgres_service.port = 5432;
    mysql_service.port = 3306;
    firebird_service.port = 3051;
    
    native_service.enabled = true;
    postgres_service.enabled = true;
    mysql_service.enabled = true;
    firebird_service.enabled = true;
}

void DockerContainerConfig::SetDefaultDirectories() {
    data_directory = "./scratchbird-data";
    config_directory = "./scratchbird-config";
    log_directory = "./scratchbird-logs";
    backup_directory = "./scratchbird-backups";
    temp_directory = "./scratchbird-temp";
}

void DockerContainerConfig::EnableService(const std::string& service_name) {
    if (service_name == "native") native_service.enabled = true;
    else if (service_name == "postgres") postgres_service.enabled = true;
    else if (service_name == "mysql") mysql_service.enabled = true;
    else if (service_name == "firebird") firebird_service.enabled = true;
}

void DockerContainerConfig::DisableService(const std::string& service_name) {
    if (service_name == "native") native_service.enabled = false;
    else if (service_name == "postgres") postgres_service.enabled = false;
    else if (service_name == "mysql") mysql_service.enabled = false;
    else if (service_name == "firebird") firebird_service.enabled = false;
}

bool DockerContainerConfig::IsServiceEnabled(const std::string& service_name) const {
    if (service_name == "native") return native_service.enabled;
    if (service_name == "postgres") return postgres_service.enabled;
    if (service_name == "mysql") return mysql_service.enabled;
    if (service_name == "firebird") return firebird_service.enabled;
    return false;
}

std::vector<std::string> DockerContainerConfig::GetEnabledServices() const {
    std::vector<std::string> services;
    if (native_service.enabled) services.push_back("native");
    if (postgres_service.enabled) services.push_back("postgres");
    if (mysql_service.enabled) services.push_back("mysql");
    if (firebird_service.enabled) services.push_back("firebird");
    return services;
}

int DockerContainerConfig::GetServicePort(const std::string& service_name) const {
    if (service_name == "native") return native_service.port;
    if (service_name == "postgres") return postgres_service.port;
    if (service_name == "mysql") return mysql_service.port;
    if (service_name == "firebird") return firebird_service.port;
    return 0;
}

void DockerContainerConfig::SetServicePort(const std::string& service_name, int port) {
    if (service_name == "native") native_service.port = port;
    else if (service_name == "postgres") postgres_service.port = port;
    else if (service_name == "mysql") mysql_service.port = port;
    else if (service_name == "firebird") firebird_service.port = port;
}

bool DockerContainerConfig::IsPortAvailable(int port) const {
    // In real implementation, check if port is in use
    (void)port;
    return true;
}

std::vector<int> DockerContainerConfig::GetAllUsedPorts() const {
    std::vector<int> ports;
    if (native_service.enabled) ports.push_back(native_service.port);
    if (postgres_service.enabled) ports.push_back(postgres_service.port);
    if (mysql_service.enabled) ports.push_back(mysql_service.port);
    if (firebird_service.enabled) ports.push_back(firebird_service.port);
    return ports;
}

std::string DockerContainerConfig::GenerateDockerRunCommand() const {
    std::ostringstream cmd;
    cmd << "docker run -d \\\n";
    cmd << "  --name " << container_name << " \\\n";
    cmd << "  --memory=" << memory_limit << " \\\n";
    cmd << "  --restart=" << restart_policy << " \\\n";
    
    // Port mappings
    for (const auto& service : GetEnabledServices()) {
        int port = GetServicePort(service);
        cmd << "  -p " << port << ":" << port << " \\\n";
    }
    
    // Volume mappings
    cmd << "  -v " << data_directory << ":/var/lib/scratchbird/data \\\n";
    cmd << "  -v " << config_directory << ":/etc/scratchbird \\\n";
    cmd << "  -v " << log_directory << ":/var/log/scratchbird \\\n";
    
    // Environment variables
    cmd << "  -e SB_ENABLE_NATIVE=" << (native_service.enabled ? "true" : "false") << " \\\n";
    cmd << "  -e SB_ENABLE_POSTGRES=" << (postgres_service.enabled ? "true" : "false") << " \\\n";
    cmd << "  -e SB_ENABLE_MYSQL=" << (mysql_service.enabled ? "true" : "false") << " \\\n";
    cmd << "  -e SB_ENABLE_FIREBIRD=" << (firebird_service.enabled ? "true" : "false") << " \\\n";
    cmd << "  -e SB_MEMORY_LIMIT=" << memory_limit << " \\\n";
    
    cmd << "  " << image_name << ":" << image_tag;
    
    return cmd.str();
}

std::string DockerContainerConfig::GenerateDockerComposeYAML() const {
    std::ostringstream yaml;
    yaml << "version: '3.8'\n\n";
    yaml << "services:\n";
    yaml << "  " << container_name << ":\n";
    yaml << "    image: " << image_name << ":" << image_tag << "\n";
    yaml << "    container_name: " << container_name << "\n";
    yaml << "    restart: " << restart_policy << "\n";
    yaml << "\n";
    yaml << "    ports:\n";
    for (const auto& service : GetEnabledServices()) {
        int port = GetServicePort(service);
        yaml << "      - \"" << port << ":" << port << "\"\n";
    }
    yaml << "\n";
    yaml << "    environment:\n";
    yaml << "      SB_ENABLE_NATIVE: \"" << (native_service.enabled ? "true" : "false") << "\"\n";
    yaml << "      SB_ENABLE_POSTGRES: \"" << (postgres_service.enabled ? "true" : "false") << "\"\n";
    yaml << "      SB_ENABLE_MYSQL: \"" << (mysql_service.enabled ? "true" : "false") << "\"\n";
    yaml << "      SB_ENABLE_FIREBIRD: \"" << (firebird_service.enabled ? "true" : "false") << "\"\n";
    yaml << "      SB_MEMORY_LIMIT: \"" << memory_limit << "\"\n";
    yaml << "\n";
    yaml << "    volumes:\n";
    yaml << "      - " << data_directory << ":/var/lib/scratchbird/data\n";
    yaml << "      - " << config_directory << ":/etc/scratchbird\n";
    yaml << "      - " << log_directory << ":/var/log/scratchbird\n";
    
    return yaml.str();
}

DockerContainerConfig::ValidationResult DockerContainerConfig::Validate() const {
    ValidationResult result;
    
    // Check container name
    if (container_name.empty()) {
        result.errors.push_back("Container name is required");
    }
    
    // Check at least one service is enabled
    if (GetEnabledServices().empty()) {
        result.errors.push_back("At least one service must be enabled");
    }
    
    // Check for port conflicts
    auto ports = GetAllUsedPorts();
    std::sort(ports.begin(), ports.end());
    for (size_t i = 1; i < ports.size(); ++i) {
        if (ports[i] == ports[i-1]) {
            result.errors.push_back("Duplicate port: " + std::to_string(ports[i]));
        }
    }
    
    // Check directories
    if (data_directory.empty()) {
        result.errors.push_back("Data directory is required");
    }
    
    result.valid = result.errors.empty();
    return result;
}

// ============================================================================
// Docker Container Manager (Stubs)
// ============================================================================

DockerContainerManager::DockerContainerManager() = default;
DockerContainerManager::~DockerContainerManager() = default;

bool DockerContainerManager::IsDockerAvailable() {
    // Would check for docker executable
    return true;
}

std::string DockerContainerManager::GetDockerVersion() {
    return "24.0.0";  // Stub
}

bool DockerContainerManager::StartContainer(const DockerContainerConfig& config) {
    (void)config;
    return true;  // Stub
}

bool DockerContainerManager::StopContainer(const std::string& container_name, int timeout_seconds) {
    (void)container_name;
    (void)timeout_seconds;
    return true;  // Stub
}

bool DockerContainerManager::RestartContainer(const std::string& container_name, int timeout_seconds) {
    (void)container_name;
    (void)timeout_seconds;
    return true;  // Stub
}

DockerContainerStatus DockerContainerManager::GetContainerStatus(const std::string& container_name) {
    DockerContainerStatus status;
    status.container_name = container_name;
    status.status = "running";
    status.health = "healthy";
    status.cpu_percent = 15.5;
    status.memory_percent = 45.2;
    return status;
}

std::vector<DockerContainerStatus> DockerContainerManager::ListAllContainers(bool include_stopped) {
    (void)include_stopped;
    return {};  // Stub
}

std::vector<std::string> DockerContainerManager::GetContainerLogs(const std::string& container_name,
                                                                   int tail_lines,
                                                                   bool follow) {
    (void)container_name;
    (void)tail_lines;
    (void)follow;
    return {"Log line 1", "Log line 2"};  // Stub
}

bool DockerContainerManager::PullImage(const std::string& image_name, const std::string& tag) {
    (void)image_name;
    (void)tag;
    return true;  // Stub
}

// ============================================================================
// Docker Templates
// ============================================================================

DockerContainerConfig DockerTemplates::DevelopmentTemplate() {
    DockerContainerConfig config;
    config.container_name = "scratchbird-dev";
    config.memory_limit = "2G";
    config.log_level = "DEBUG";
    config.restart_policy = "on-failure";
    return config;
}

DockerContainerConfig DockerTemplates::ProductionTemplate() {
    DockerContainerConfig config;
    config.container_name = "scratchbird-prod";
    config.memory_limit = "8G";
    config.log_level = "INFO";
    config.ssl_enabled = true;
    config.require_auth = true;
    config.backup_enabled = true;
    config.restart_policy = "always";
    return config;
}

DockerContainerConfig DockerTemplates::MinimalTemplate() {
    DockerContainerConfig config;
    config.container_name = "scratchbird-minimal";
    config.memory_limit = "1G";
    config.DisableService("postgres");
    config.DisableService("mysql");
    config.DisableService("firebird");
    return config;
}

std::vector<std::pair<std::string, std::string>> DockerTemplates::ListTemplates() {
    return {
        {"development", "Development - All services, 2GB RAM"},
        {"production", "Production - 8GB RAM, SSL, backups"},
        {"minimal", "Minimal - Native only, 1GB RAM"},
        {"postgres-only", "PostgreSQL Only - Just PostgreSQL"},
        {"mysql-only", "MySQL Only - Just MySQL"},
        {"firebird-only", "Firebird Only - Just Firebird"}
    };
}

} // namespace scratchrobin
