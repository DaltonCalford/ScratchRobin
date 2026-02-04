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

#include <algorithm>
#include <chrono>
#include <mutex>

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
        // TODO: Implement drift detection
    };
    
    RegisterTask(task);
}

void SyncScheduler::AddHealthCheckTask(int interval_minutes) {
    SyncTask task;
    task.name = "Health check monitoring";
    task.interval_minutes = interval_minutes;
    task.execute = []() {
        // TODO: Implement health checks
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
    // TODO: Parse Jira webhook event and update linked issues
    (void)event;
    return true;
}

bool GitHubWebhookHandler::HandleEvent(const WebhookEvent& event) {
    // TODO: Parse GitHub webhook event and update linked issues
    (void)event;
    return true;
}

bool GitLabWebhookHandler::HandleEvent(const WebhookEvent& event) {
    // TODO: Parse GitLab webhook event and update linked issues
    (void)event;
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
    port_ = port;
    // TODO: Implement HTTP server for receiving webhooks
    // For now, just mark as running
    running_ = true;
    return true;
}

void WebhookServer::Stop() {
    running_ = false;
}

bool WebhookServer::IsRunning() const {
    return running_;
}

void WebhookServer::SetSecret(const std::string& secret) {
    secret_ = secret;
}

void WebhookServer::ServerLoop() {
    // TODO: Implement HTTP server loop
}

void WebhookServer::HandleRequest(int client_socket) {
    (void)client_socket;
    // TODO: Handle incoming webhook request
}

bool WebhookServer::VerifySignature(const std::string& payload,
                                     const std::string& signature,
                                     const std::string& provider) {
    (void)payload;
    (void)signature;
    (void)provider;
    // TODO: Implement signature verification
    return true;
}

} // namespace scratchrobin
