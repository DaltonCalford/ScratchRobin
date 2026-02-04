/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_SYNC_SCHEDULER_H
#define SCRATCHROBIN_SYNC_SCHEDULER_H

#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace scratchrobin {

// ============================================================================
// Sync Task
// ============================================================================
struct SyncTask {
    std::string id;
    std::string name;
    std::string tracker_name;
    std::function<void()> execute;
    
    // Scheduling
    int interval_minutes = 5;
    std::time_t last_run = 0;
    std::time_t next_run = 0;
    bool enabled = true;
    bool is_running = false;
    
    // Statistics
    int run_count = 0;
    int success_count = 0;
    int failure_count = 0;
    std::time_t last_success = 0;
    std::string last_error;
};

// ============================================================================
// Webhook Event
// ============================================================================
struct WebhookEvent {
    std::string provider;
    std::string event_type;  // "issue.created", "issue.updated", "comment.added"
    std::string issue_id;
    std::string payload;     // Raw JSON payload
    std::time_t received_at = 0;
    std::string signature;   // For verification
};

// ============================================================================
// Webhook Handler Interface
// ============================================================================
class WebhookHandler {
public:
    virtual ~WebhookHandler() = default;
    virtual bool HandleEvent(const WebhookEvent& event) = 0;
    virtual std::string GetProviderName() const = 0;
};

// ============================================================================
// Sync Scheduler
// ============================================================================
class SyncScheduler {
public:
    static SyncScheduler& Instance();
    
    // Task management
    std::string RegisterTask(const SyncTask& task);
    bool UnregisterTask(const std::string& task_id);
    bool EnableTask(const std::string& task_id);
    bool DisableTask(const std::string& task_id);
    
    // Manual execution
    bool RunTaskNow(const std::string& task_id);
    bool RunAllTasks();
    
    // Scheduling control
    void Start();
    void Stop();
    bool IsRunning() const;
    
    // Task queries
    std::vector<SyncTask> GetTasks() const;
    std::optional<SyncTask> GetTask(const std::string& task_id) const;
    
    // Webhook handling
    void RegisterWebhookHandler(std::unique_ptr<WebhookHandler> handler);
    bool ProcessWebhookEvent(const WebhookEvent& event);
    
    // Built-in sync tasks
    void AddIssueSyncTask(const std::string& tracker_name, int interval_minutes = 5);
    void AddDriftDetectionTask(int interval_minutes = 60);
    void AddHealthCheckTask(int interval_minutes = 5);
    
private:
    SyncScheduler() = default;
    ~SyncScheduler();
    
    void SchedulerLoop();
    void ExecuteTask(SyncTask& task);
    std::time_t CalculateNextRun(const SyncTask& task);
    
    std::map<std::string, SyncTask> tasks_;
    std::vector<std::unique_ptr<WebhookHandler>> webhook_handlers_;
    
    std::unique_ptr<std::thread> scheduler_thread_;
    bool running_ = false;
    mutable std::mutex mutex_;
};

// ============================================================================
// Built-in Webhook Handlers
// ============================================================================
class JiraWebhookHandler : public WebhookHandler {
public:
    std::string GetProviderName() const override { return "jira"; }
    bool HandleEvent(const WebhookEvent& event) override;
};

class GitHubWebhookHandler : public WebhookHandler {
public:
    std::string GetProviderName() const override { return "github"; }
    bool HandleEvent(const WebhookEvent& event) override;
};

class GitLabWebhookHandler : public WebhookHandler {
public:
    std::string GetProviderName() const override { return "gitlab"; }
    bool HandleEvent(const WebhookEvent& event) override;
};

// ============================================================================
// Webhook Server (Simple HTTP endpoint for receiving webhooks)
// ============================================================================
class WebhookServer {
public:
    static WebhookServer& Instance();
    
    bool Start(int port = 8080);
    void Stop();
    bool IsRunning() const;
    
    void SetSecret(const std::string& secret);  // For signature verification
    void RegisterHandler(const std::string& path, 
                         std::function<void(const WebhookEvent&)> handler);
    
private:
    WebhookServer() = default;
    ~WebhookServer();
    
    void ServerLoop();
    void HandleRequest(int client_socket);
    void SendResponse(int client_socket, int status_code, 
                      const std::string& status_text,
                      const std::string& body = "");
    bool VerifySignature(const std::string& payload, const std::string& signature,
                         const std::string& provider);
    
    int server_socket_ = -1;
    int port_ = 8080;
    std::string secret_;
    bool running_ = false;
    std::unique_ptr<std::thread> server_thread_;
    mutable std::mutex mutex_;
    std::map<std::string, std::function<void(const WebhookEvent&)>> handlers_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SYNC_SCHEDULER_H
