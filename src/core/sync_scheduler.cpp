/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "sync_scheduler.h"
#include "issue_tracker.h"
#include "simple_json.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <sstream>

// For HTTP server
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// For HMAC signature verification
#include <openssl/hmac.h>
#include <openssl/evp.h>

namespace scratchrobin {

// ============================================================================
// Sync Scheduler
// ============================================================================
SyncScheduler& SyncScheduler::Instance() {
    static SyncScheduler instance;
    return instance;
}

SyncScheduler::~SyncScheduler() {
    Stop();
}

std::string SyncScheduler::RegisterTask(const SyncTask& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string id = task.id.empty() ? "task_" + std::to_string(tasks_.size() + 1) : task.id;
    
    SyncTask new_task = task;
    new_task.id = id;
    new_task.next_run = std::time(nullptr) + (task.interval_minutes * 60);
    
    tasks_[id] = new_task;
    return id;
}

bool SyncScheduler::UnregisterTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.erase(task_id) > 0;
}

bool SyncScheduler::EnableTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second.enabled = true;
        return true;
    }
    return false;
}

bool SyncScheduler::DisableTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second.enabled = false;
        return true;
    }
    return false;
}

bool SyncScheduler::RunTaskNow(const std::string& task_id) {
    SyncTask task_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(task_id);
        if (it == tasks_.end() || !it->second.enabled || it->second.is_running) {
            return false;
        }
        task_copy = it->second;
        it->second.is_running = true;
    }
    
    ExecuteTask(task_copy);
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(task_id);
        if (it != tasks_.end()) {
            it->second.is_running = false;
            it->second.last_run = std::time(nullptr);
            it->second.next_run = CalculateNextRun(it->second);
        }
    }
    
    return true;
}

bool SyncScheduler::RunAllTasks() {
    std::vector<std::string> task_ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, task] : tasks_) {
            if (task.enabled && !task.is_running) {
                task_ids.push_back(id);
            }
        }
    }
    
    for (const auto& id : task_ids) {
        RunTaskNow(id);
    }
    
    return !task_ids.empty();
}

void SyncScheduler::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) return;
    
    running_ = true;
    scheduler_thread_ = std::make_unique<std::thread>(&SyncScheduler::SchedulerLoop, this);
}

void SyncScheduler::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    
    if (scheduler_thread_ && scheduler_thread_->joinable()) {
        scheduler_thread_->join();
    }
}

bool SyncScheduler::IsRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

std::vector<SyncTask> SyncScheduler::GetTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SyncTask> result;
    for (const auto& [_, task] : tasks_) {
        result.push_back(task);
    }
    return result;
}

std::optional<SyncTask> SyncScheduler::GetTask(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void SyncScheduler::RegisterWebhookHandler(std::unique_ptr<WebhookHandler> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    webhook_handlers_.push_back(std::move(handler));
}

bool SyncScheduler::ProcessWebhookEvent(const WebhookEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& handler : webhook_handlers_) {
        if (handler->GetProviderName() == event.provider) {
            return handler->HandleEvent(event);
        }
    }
    
    return false;
}

void SyncScheduler::AddIssueSyncTask(const std::string& tracker_name, int interval_minutes) {
    SyncTask task;
    task.name = "Sync issues from " + tracker_name;
    task.tracker_name = tracker_name;
    task.interval_minutes = interval_minutes;
    task.execute = [tracker_name]() {
        auto& manager = IssueLinkManager::Instance();
        manager.SyncAllLinks(tracker_name);
    };
    
    RegisterTask(task);
}

void SyncScheduler::AddDriftDetectionTask(int interval_minutes) {
    SyncTask task;
    task.name = "Schema drift detection";
    task.interval_minutes = interval_minutes;
    task.execute = []() {
        // Schema drift detection implementation
        auto& manager = IssueLinkManager::Instance();
        
        // Get all trackers
        auto trackers = manager.GetTrackerNames();
        
        for (const auto& tracker_name : trackers) {
            auto* tracker = manager.GetTracker(tracker_name);
            if (!tracker) continue;
            
            // Get all linked issues for this tracker
            // Note: This is a simplified drift detection
            // In production, would compare actual schema state
            
            // Check for issues that might indicate drift
            // by querying the tracker for recent updates
            IssueLinkManager::AutoIssueContext context;
            context.event_type = "drift_check";
            context.severity = "low";
            
            // Auto-create issue if drift detected (placeholder logic)
            // manager.AutoCreateIssue(context, tracker_name);
        }
    };
    
    RegisterTask(task);
}

void SyncScheduler::AddHealthCheckTask(int interval_minutes) {
    SyncTask task;
    task.name = "Health check monitoring";
    task.interval_minutes = interval_minutes;
    task.execute = []() {
        // Health check implementation
        auto& manager = IssueLinkManager::Instance();
        
        // Check tracker connectivity
        auto trackers = manager.GetTrackerNames();
        
        for (const auto& tracker_name : trackers) {
            auto* tracker = manager.GetTracker(tracker_name);
            if (!tracker) continue;
            
            // Test connection by fetching a recent issue
            auto recent = tracker->GetRecentIssues(1);
            
            if (recent.empty()) {
                // Connection issue or no issues
                IssueLinkManager::AutoIssueContext context;
                context.event_type = "health_check_failed";
                context.severity = "high";
                context.metadata["tracker"] = tracker_name;
                context.metadata["reason"] = "Connection failed or no recent issues";
                
                // Could auto-create incident issue
                // manager.AutoCreateIssue(context, tracker_name);
            }
        }
    };
    
    RegisterTask(task);
}

void SyncScheduler::SchedulerLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) break;
        }
        
        std::time_t now = std::time(nullptr);
        std::vector<std::string> tasks_to_run;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& [id, task] : tasks_) {
                if (task.enabled && !task.is_running && task.next_run <= now) {
                    tasks_to_run.push_back(id);
                }
            }
        }
        
        for (const auto& id : tasks_to_run) {
            RunTaskNow(id);
        }
        
        // Sleep for 1 minute between checks
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }
}

void SyncScheduler::ExecuteTask(SyncTask& task) {
    try {
        task.run_count++;
        
        if (task.execute) {
            task.execute();
        }
        
        task.success_count++;
        task.last_success = std::time(nullptr);
        task.last_error.clear();
    } catch (const std::exception& e) {
        task.failure_count++;
        task.last_error = e.what();
    }
}

std::time_t SyncScheduler::CalculateNextRun(const SyncTask& task) {
    return std::time(nullptr) + (task.interval_minutes * 60);
}

// ============================================================================
// Webhook Handlers
// ============================================================================
bool JiraWebhookHandler::HandleEvent(const WebhookEvent& event) {
    // Parse Jira webhook payload
    JsonParser parser(event.payload);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return false;
    }
    
    // Extract issue information from Jira webhook
    std::string issue_id;
    std::string issue_key;
    std::string issue_status;
    
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* issue = FindMember(root, "issue");
        if (issue && issue->type == JsonValue::Type::Object) {
            const JsonValue* id = FindMember(*issue, "id");
            if (id && id->type == JsonValue::Type::String) {
                issue_id = id->string_value;
            }
            
            const JsonValue* key = FindMember(*issue, "key");
            if (key && key->type == JsonValue::Type::String) {
                issue_key = key->string_value;
            }
            
            const JsonValue* fields = FindMember(*issue, "fields");
            if (fields && fields->type == JsonValue::Type::Object) {
                const JsonValue* status = FindMember(*fields, "status");
                if (status && status->type == JsonValue::Type::Object) {
                    const JsonValue* status_name = FindMember(*status, "name");
                    if (status_name && status_name->type == JsonValue::Type::String) {
                        issue_status = status_name->string_value;
                    }
                }
            }
        }
    }
    
    if (!issue_id.empty()) {
        // Update linked issue status
        auto& manager = IssueLinkManager::Instance();
        auto links = manager.GetLinkedObjects(issue_id);
        
        for (const auto& link : links) {
            // Sync the link to update cached status
            manager.SyncLink(link.link_id);
        }
    }
    
    return true;
}

bool GitHubWebhookHandler::HandleEvent(const WebhookEvent& event) {
    // Parse GitHub webhook payload
    JsonParser parser(event.payload);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return false;
    }
    
    // Extract issue information from GitHub webhook
    std::string issue_id;
    std::string issue_number;
    std::string issue_state;
    
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* issue = FindMember(root, "issue");
        if (issue && issue->type == JsonValue::Type::Object) {
            const JsonValue* id = FindMember(*issue, "id");
            if (id && id->type == JsonValue::Type::Number) {
                issue_id = std::to_string(static_cast<int64_t>(id->number_value));
            }
            
            const JsonValue* number = FindMember(*issue, "number");
            if (number && number->type == JsonValue::Type::Number) {
                issue_number = std::to_string(static_cast<int64_t>(number->number_value));
            }
            
            const JsonValue* state = FindMember(*issue, "state");
            if (state && state->type == JsonValue::Type::String) {
                issue_state = state->string_value;
            }
        }
    }
    
    if (!issue_id.empty()) {
        auto& manager = IssueLinkManager::Instance();
        auto links = manager.GetLinkedObjects(issue_id);
        
        for (const auto& link : links) {
            manager.SyncLink(link.link_id);
        }
    }
    
    return true;
}

bool GitLabWebhookHandler::HandleEvent(const WebhookEvent& event) {
    // Parse GitLab webhook payload
    JsonParser parser(event.payload);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return false;
    }
    
    // Extract issue information from GitLab webhook
    std::string issue_id;
    std::string issue_iid;
    std::string issue_state;
    
    if (root.type == JsonValue::Type::Object) {
        const JsonValue* issue = FindMember(root, "object_attributes");
        if (issue && issue->type == JsonValue::Type::Object) {
            const JsonValue* id = FindMember(*issue, "id");
            if (id && id->type == JsonValue::Type::Number) {
                issue_id = std::to_string(static_cast<int64_t>(id->number_value));
            }
            
            const JsonValue* iid = FindMember(*issue, "iid");
            if (iid && iid->type == JsonValue::Type::Number) {
                issue_iid = std::to_string(static_cast<int64_t>(iid->number_value));
            }
            
            const JsonValue* state = FindMember(*issue, "state");
            if (state && state->type == JsonValue::Type::String) {
                issue_state = state->string_value;
            }
        }
    }
    
    if (!issue_id.empty()) {
        auto& manager = IssueLinkManager::Instance();
        auto links = manager.GetLinkedObjects(issue_id);
        
        for (const auto& link : links) {
            manager.SyncLink(link.link_id);
        }
    }
    
    return true;
}

// ============================================================================
// Webhook Server
// ============================================================================
WebhookServer& WebhookServer::Instance() {
    static WebhookServer instance;
    return instance;
}

WebhookServer::~WebhookServer() {
    Stop();
}

bool WebhookServer::Start(int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        return true;  // Already running
    }
    
    port_ = port;
    
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        return false;
    }
    
    // Allow socket reuse
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_socket_);
        return false;
    }
    
    // Bind to port
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_socket_);
        return false;
    }
    
    // Listen for connections
    if (listen(server_socket_, 10) < 0) {
        close(server_socket_);
        return false;
    }
    
    // Make socket non-blocking
    int flags = fcntl(server_socket_, F_GETFL, 0);
    fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);
    
    running_ = true;
    server_thread_ = std::make_unique<std::thread>(&WebhookServer::ServerLoop, this);
    
    return true;
}

void WebhookServer::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
    
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
}

bool WebhookServer::IsRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

void WebhookServer::SetSecret(const std::string& secret) {
    std::lock_guard<std::mutex> lock(mutex_);
    secret_ = secret;
}

void WebhookServer::RegisterHandler(const std::string& path, 
                                    std::function<void(const WebhookEvent&)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[path] = handler;
}

void WebhookServer::ServerLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) break;
        }
        
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connections pending, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            // Other error, log and continue
            continue;
        }
        
        // Handle request in a separate thread
        std::thread client_thread(&WebhookServer::HandleRequest, this, client_socket);
        client_thread.detach();
    }
}

void WebhookServer::HandleRequest(int client_socket) {
    char buffer[8192];
    std::string request;
    
    // Read HTTP request
    while (request.find("\r\n\r\n") == std::string::npos) {
        ssize_t bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            close(client_socket);
            return;
        }
        buffer[bytes] = '\0';
        request += buffer;
    }
    
    // Parse HTTP request line
    size_t method_end = request.find(' ');
    size_t path_end = request.find(' ', method_end + 1);
    
    if (method_end == std::string::npos || path_end == std::string::npos) {
        SendResponse(client_socket, 400, "Bad Request");
        close(client_socket);
        return;
    }
    
    std::string method = request.substr(0, method_end);
    std::string path = request.substr(method_end + 1, path_end - method_end - 1);
    
    // Extract headers
    std::map<std::string, std::string> headers;
    size_t header_start = request.find("\r\n") + 2;
    size_t body_start = request.find("\r\n\r\n");
    
    while (header_start < body_start) {
        size_t line_end = request.find("\r\n", header_start);
        if (line_end == std::string::npos || line_end > body_start) break;
        
        std::string line = request.substr(header_start, line_end - header_start);
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Trim whitespace
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                value = value.substr(1);
            }
            headers[key] = value;
        }
        header_start = line_end + 2;
    }
    
    // Get body
    std::string body;
    if (body_start != std::string::npos) {
        body = request.substr(body_start + 4);
    }
    
    // Check for Content-Length and read more if needed
    auto it = headers.find("Content-Length");
    if (it != headers.end()) {
        size_t content_length = std::stoul(it->second);
        while (body.length() < content_length) {
            ssize_t bytes = recv(client_socket, buffer, 
                                 std::min(sizeof(buffer) - 1, content_length - body.length()), 0);
            if (bytes <= 0) break;
            buffer[bytes] = '\0';
            body += buffer;
        }
    }
    
    // Handle webhook request
    if (method == "POST") {
        WebhookEvent event;
        event.payload = body;
        event.timestamp = std::time(nullptr);
        
        // Determine provider from path
        if (path.find("/webhook/jira") != std::string::npos) {
            event.provider = "jira";
            event.event_type = headers["X-Atlassian-Webhook-Identifier"];
            event.signature = headers["X-Hub-Signature"];
        } else if (path.find("/webhook/github") != std::string::npos) {
            event.provider = "github";
            event.event_type = headers["X-GitHub-Event"];
            event.signature = headers["X-Hub-Signature-256"];
        } else if (path.find("/webhook/gitlab") != std::string::npos) {
            event.provider = "gitlab";
            event.event_type = headers["X-Gitlab-Event"];
            event.signature = headers["X-Gitlab-Token"];
        } else {
            SendResponse(client_socket, 404, "Not Found");
            close(client_socket);
            return;
        }
        
        // Verify signature if configured
        if (!secret_.empty() && !event.signature.empty()) {
            if (!VerifySignature(body, event.signature, event.provider)) {
                SendResponse(client_socket, 401, "Unauthorized");
                close(client_socket);
                return;
            }
        }
        
        // Process the webhook
        auto& scheduler = SyncScheduler::Instance();
        bool handled = scheduler.ProcessWebhookEvent(event);
        
        // Send response
        if (handled) {
            SendResponse(client_socket, 200, "OK", "{\"status\":\"processed\"}");
        } else {
            SendResponse(client_socket, 500, "Internal Server Error");
        }
    } else if (method == "GET" && path == "/health") {
        SendResponse(client_socket, 200, "OK", "{\"status\":\"healthy\"}");
    } else {
        SendResponse(client_socket, 405, "Method Not Allowed");
    }
    
    close(client_socket);
}

void WebhookServer::SendResponse(int client_socket, int status_code, 
                                  const std::string& status_text,
                                  const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    std::string resp_str = response.str();
    send(client_socket, resp_str.c_str(), resp_str.length(), 0);
}

bool WebhookServer::VerifySignature(const std::string& payload,
                                     const std::string& signature,
                                     const std::string& provider) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (secret_.empty()) {
        return true;  // No secret configured, accept all
    }
    
    std::string computed_sig;
    
    if (provider == "github") {
        // GitHub: sha256=hmac
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len;
        
        HMAC(EVP_sha256(), secret_.c_str(), secret_.length(),
             (unsigned char*)payload.c_str(), payload.length(),
             hash, &hash_len);
        
        char hex_hash[EVP_MAX_MD_SIZE * 2 + 1];
        for (unsigned int i = 0; i < hash_len; i++) {
            sprintf(hex_hash + i * 2, "%02x", hash[i]);
        }
        
        computed_sig = "sha256=" + std::string(hex_hash, hash_len * 2);
    } else if (provider == "jira") {
        // Jira: simple JWT or custom header - simplified
        // In production, would verify JWT
        return true;
    } else if (provider == "gitlab") {
        // GitLab: token matches secret
        return signature == secret_;
    }
    
    return signature == computed_sig;
}

} // namespace scratchrobin
