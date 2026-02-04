/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DOCKER_CONFIG_H
#define SCRATCHROBIN_DOCKER_CONFIG_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {

// ============================================================================
// Docker Service Configuration
// ============================================================================
struct DockerServiceConfig {
    bool enabled = true;
    std::string name;
    int port = 0;
    std::string data_directory;
    std::map<std::string, std::string> custom_settings;
};

// ============================================================================
// Docker Container Configuration
// ============================================================================
class DockerContainerConfig {
public:
    // Container identity
    std::string container_name = "scratchbird-server";
    std::string image_name = "scratchbird:latest";
    std::string image_tag = "latest";
    
    // Network configuration
    std::string bind_ip = "0.0.0.0";
    std::string network_mode = "bridge";  // bridge, host, or custom network
    std::vector<std::string> custom_networks;
    
    // Resource limits
    std::string memory_limit = "2G";
    std::string memory_swap = "2G";
    int cpu_shares = 1024;
    std::string cpu_quota;
    std::string disk_limit = "100G";
    
    // Directory mappings (host:container)
    std::map<std::string, std::string> volume_mappings;
    std::string data_directory = "./scratchbird-data";
    std::string config_directory = "./scratchbird-config";
    std::string log_directory = "./scratchbird-logs";
    std::string backup_directory = "./scratchbird-backups";
    std::string temp_directory = "./scratchbird-temp";
    
    // Services
    DockerServiceConfig native_service;
    DockerServiceConfig postgres_service;
    DockerServiceConfig mysql_service;
    DockerServiceConfig firebird_service;
    
    // Performance settings
    int max_connections = 100;
    std::string shared_buffers = "256MB";
    std::string work_mem = "64MB";
    std::string maintenance_work_mem = "256MB";
    std::string effective_cache_size = "1GB";
    
    // Security settings
    bool ssl_enabled = true;
    std::string ssl_cert_path;
    std::string ssl_key_path;
    bool require_auth = true;
    
    // Logging settings
    std::string log_level = "INFO";
    std::string log_rotation = "daily";
    int log_retention_days = 30;
    
    // Backup settings
    bool backup_enabled = true;
    std::string backup_schedule = "0 2 * * *";  // Daily at 2 AM
    int backup_retention_days = 7;
    
    // Default database
    std::string default_database = "default";
    std::string default_database_path;
    
    // Environment variables (additional)
    std::map<std::string, std::string> environment_variables;
    
    // Restart policy
    std::string restart_policy = "unless-stopped";  // no, on-failure, always, unless-stopped
    int restart_retry_count = 5;
    
    // Constructor
    DockerContainerConfig();
    
    // Service management
    void EnableService(const std::string& service_name);
    void DisableService(const std::string& service_name);
    bool IsServiceEnabled(const std::string& service_name) const;
    std::vector<std::string> GetEnabledServices() const;
    
    // Port management
    int GetServicePort(const std::string& service_name) const;
    void SetServicePort(const std::string& service_name, int port);
    bool IsPortAvailable(int port) const;
    std::vector<int> GetAllUsedPorts() const;
    
    // Docker command generation
    std::string GenerateDockerRunCommand() const;
    std::string GenerateDockerComposeYAML() const;
    std::string GenerateDockerfile() const;
    std::vector<std::string> GenerateEnvironmentFile() const;
    
    // Validation
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    ValidationResult Validate() const;
    
    // Serialization
    void ToJson(std::ostream& out) const;
    static DockerContainerConfig FromJson(const std::string& json);
    void ToYaml(std::ostream& out) const;
    static DockerContainerConfig FromYaml(const std::string& yaml);
    void SaveToFile(const std::string& path) const;
    static DockerContainerConfig LoadFromFile(const std::string& path);
    
private:
    void SetDefaultPorts();
    void SetDefaultDirectories();
};

// ============================================================================
// Docker Container Status
// ============================================================================
struct DockerContainerStatus {
    std::string container_id;
    std::string container_name;
    std::string image;
    std::string status;  // running, stopped, paused, restarting, dead
    std::string health;  // healthy, unhealthy, starting, none
    
    // Resource usage
    double cpu_percent = 0.0;
    double memory_percent = 0.0;
    uint64_t memory_usage_bytes = 0;
    uint64_t memory_limit_bytes = 0;
    uint64_t disk_usage_bytes = 0;
    
    // Network
    std::string ip_address;
    std::map<std::string, int> port_bindings;
    
    // Timestamps
    std::string created_at;
    std::string started_at;
    std::string finished_at;
    
    // Process info
    int process_count = 0;
    
    bool IsRunning() const { return status == "running"; }
    bool IsHealthy() const { return health == "healthy"; }
};

// ============================================================================
// Docker Container Manager
// ============================================================================
class DockerContainerManager {
public:
    using StatusCallback = std::function<void(const DockerContainerStatus&)>;
    using LogCallback = std::function<void(const std::string&)>;
    
    DockerContainerManager();
    ~DockerContainerManager();
    
    // Docker availability check
    static bool IsDockerAvailable();
    static std::string GetDockerVersion();
    
    // Container lifecycle
    bool StartContainer(const DockerContainerConfig& config);
    bool StopContainer(const std::string& container_name, int timeout_seconds = 30);
    bool RestartContainer(const std::string& container_name, int timeout_seconds = 30);
    bool PauseContainer(const std::string& container_name);
    bool UnpauseContainer(const std::string& container_name);
    bool RemoveContainer(const std::string& container_name, bool force = false, bool remove_volumes = false);
    
    // Status monitoring
    DockerContainerStatus GetContainerStatus(const std::string& container_name);
    std::vector<DockerContainerStatus> ListAllContainers(bool include_stopped = false);
    void StartStatusMonitoring(const std::string& container_name, StatusCallback callback, int interval_seconds = 5);
    void StopStatusMonitoring(const std::string& container_name);
    
    // Logs
    std::vector<std::string> GetContainerLogs(const std::string& container_name, 
                                               int tail_lines = 100,
                                               bool follow = false);
    void StreamContainerLogs(const std::string& container_name, LogCallback callback);
    void StopLogStreaming(const std::string& container_name);
    
    // Configuration management
    bool SaveContainerConfig(const DockerContainerConfig& config, const std::string& name);
    DockerContainerConfig LoadContainerConfig(const std::string& name);
    std::vector<std::string> ListSavedConfigs();
    bool DeleteContainerConfig(const std::string& name);
    
    // Image management
    bool PullImage(const std::string& image_name, const std::string& tag = "latest");
    bool BuildImage(const std::string& dockerfile_path, 
                    const std::string& image_name,
                    const std::string& tag = "latest");
    std::vector<std::string> ListLocalImages();
    bool RemoveImage(const std::string& image_name, bool force = false);
    
    // Volume management
    bool CreateVolume(const std::string& volume_name);
    bool RemoveVolume(const std::string& volume_name);
    std::vector<std::string> ListVolumes();
    
    // Network management
    bool CreateNetwork(const std::string& network_name, const std::string& driver = "bridge");
    bool RemoveNetwork(const std::string& network_name);
    std::vector<std::string> ListNetworks();
    
    // Backup and restore
    bool BackupContainerData(const std::string& container_name, 
                              const std::string& backup_path);
    bool RestoreContainerData(const std::string& container_name,
                               const std::string& backup_path);
    
    // Health check
    bool WaitForHealthy(const std::string& container_name, int timeout_seconds = 60);
    
    // Events
    void SetEventCallback(std::function<void(const std::string& event, const std::string& container)> callback);
    
private:
    bool ExecuteDockerCommand(const std::vector<std::string>& args, 
                               std::string* output = nullptr,
                               std::string* error = nullptr);
    bool ExecuteDockerComposeCommand(const std::vector<std::string>& args,
                                      std::string* output = nullptr,
                                      std::string* error = nullptr);
    
    std::map<std::string, StatusCallback> status_callbacks_;
    std::map<std::string, LogCallback> log_callbacks_;
    std::map<std::string, bool> monitoring_active_;
    std::function<void(const std::string&, const std::string&)> event_callback_;
};

// ============================================================================
// Quick Start Templates
// ============================================================================
class DockerTemplates {
public:
    // Pre-configured templates
    static DockerContainerConfig DevelopmentTemplate();
    static DockerContainerConfig ProductionTemplate();
    static DockerContainerConfig MinimalTemplate();
    static DockerContainerConfig FullFeaturedTemplate();
    static DockerContainerConfig PostgresOnlyTemplate();
    static DockerContainerConfig MySqlOnlyTemplate();
    static DockerContainerConfig FirebirdOnlyTemplate();
    
    // Template registry
    static std::vector<std::pair<std::string, std::string>> ListTemplates();
    static DockerContainerConfig LoadTemplate(const std::string& name);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DOCKER_CONFIG_H
