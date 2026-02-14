// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "report_scheduler.h"

#include <algorithm>
#include <sstream>

namespace scratchrobin {
namespace reporting {

// EmailChannel implementation
bool EmailChannel::Send(const std::string& recipient,
                        const std::string& subject,
                        const std::string& body,
                        const std::optional<std::string>& attachment) {
    // In real implementation, send email via SMTP
    return true;
}

bool EmailChannel::ValidateConfig(const std::map<std::string, std::string>& config) {
    return config.count("smtp_host") && config.count("from_address");
}

// WebhookChannel implementation
bool WebhookChannel::Send(const std::string& url,
                          const std::string& subject,
                          const std::string& body,
                          const std::optional<std::string>& attachment) {
    // In real implementation, POST to webhook URL
    return true;
}

bool WebhookChannel::ValidateConfig(const std::map<std::string, std::string>& config) {
    return config.count("url");
}

// ReportScheduler implementation

ReportScheduler::ReportScheduler(QueryExecutor* executor,
                                  ReportRepository* repository)
    : executor_(executor),
      repository_(repository) {}

ReportScheduler::~ReportScheduler() {
    Stop();
}

void ReportScheduler::Start() {
    if (running_) return;
    running_ = true;
    scheduler_thread_ = std::thread(&ReportScheduler::SchedulerLoop, this);
}

void ReportScheduler::Stop() {
    if (!running_) return;
    running_ = false;
    cv_.notify_all();
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
}

std::string ReportScheduler::ScheduleAlert(const Alert& alert) {
    ScheduledTask task;
    task.id = GenerateTaskId();
    task.name = alert.name;
    task.target_type = "alert";
    task.target_id = alert.id;
    task.schedule = ScheduleParser::Parse(alert.schedule);
    task.next_run = CalculateNextRun(task.schedule, std::time(nullptr)).scheduled_time;
    task.enabled = alert.enabled;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_[task.id] = task;
    }
    
    cv_.notify_all();
    return task.id;
}

std::string ReportScheduler::ScheduleSubscription(const Subscription& subscription) {
    ScheduledTask task;
    task.id = GenerateTaskId();
    task.name = subscription.name;
    task.target_type = "subscription";
    task.target_id = subscription.id;
    task.schedule = ScheduleParser::Parse(subscription.schedule);
    task.next_run = CalculateNextRun(task.schedule, std::time(nullptr)).scheduled_time;
    task.enabled = subscription.enabled;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_[task.id] = task;
    }
    
    cv_.notify_all();
    return task.id;
}

std::string ReportScheduler::ScheduleRefresh(const Question& question,
                                              const ScheduleSpec& schedule) {
    ScheduledTask task;
    task.id = GenerateTaskId();
    task.name = "Refresh: " + question.name;
    task.target_type = "refresh";
    task.target_id = question.id;
    task.schedule = schedule;
    task.next_run = CalculateNextRun(schedule, std::time(nullptr)).scheduled_time;
    task.enabled = true;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_[task.id] = task;
    }
    
    cv_.notify_all();
    return task.id;
}

bool ReportScheduler::Unschedule(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.erase(task_id) > 0;
}

bool ReportScheduler::EnableTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) return false;
    it->second.enabled = true;
    return true;
}

bool ReportScheduler::DisableTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) return false;
    it->second.enabled = false;
    return true;
}

std::optional<ScheduledTask> ReportScheduler::GetTask(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) return it->second;
    return std::nullopt;
}

std::vector<ScheduledTask> ReportScheduler::GetAllTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ScheduledTask> result;
    for (const auto& [id, task] : tasks_) {
        result.push_back(task);
    }
    return result;
}

std::vector<ScheduledTask> ReportScheduler::GetTasksForTarget(const std::string& target_type,
                                                               const std::string& target_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ScheduledTask> result;
    for (const auto& [id, task] : tasks_) {
        if (task.target_type == target_type && task.target_id == target_id) {
            result.push_back(task);
        }
    }
    return result;
}

TaskResult ReportScheduler::RunTaskNow(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        TaskResult result;
        result.task_id = task_id;
        result.success = false;
        result.error_message = "Task not found";
        return result;
    }
    
    ExecuteTask(it->second);
    
    TaskResult result;
    result.task_id = task_id;
    result.success = true;
    result.executed_at = std::time(nullptr);
    return result;
}

std::vector<TaskResult> ReportScheduler::RunAllDueTasks() {
    std::vector<TaskResult> results;
    auto now = std::time(nullptr);
    
    std::vector<std::string> due_tasks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, task] : tasks_) {
            if (task.enabled && task.next_run <= now) {
                due_tasks.push_back(id);
            }
        }
    }
    
    for (const auto& task_id : due_tasks) {
        results.push_back(RunTaskNow(task_id));
    }
    
    return results;
}

ScheduleSpec ReportScheduler::ParseSchedule(const std::string& spec) {
    return ScheduleParser::Parse(spec);
}

NextRun ReportScheduler::CalculateNextRun(const ScheduleSpec& schedule, Timestamp from) {
    NextRun result;
    result.scheduled_time = from + 3600; // Default: 1 hour from now
    result.is_valid = true;
    
    // Simplified calculation
    switch (schedule.type) {
        case ScheduleType::Hourly:
            result.scheduled_time = from + 3600;
            break;
        case ScheduleType::Daily:
            result.scheduled_time = from + 86400;
            break;
        case ScheduleType::Weekly:
            result.scheduled_time = from + 604800;
            break;
        case ScheduleType::Monthly:
            result.scheduled_time = from + 2592000; // ~30 days
            break;
        default:
            break;
    }
    
    return result;
}

bool ReportScheduler::IsDue(const ScheduleSpec& schedule, Timestamp last_run, Timestamp now) {
    NextRun next = CalculateNextRun(schedule, last_run);
    return next.scheduled_time <= now;
}

void ReportScheduler::SetTaskCallback(TaskCallback callback) {
    task_callback_ = callback;
}

void ReportScheduler::SetCompletionCallback(CompletionCallback callback) {
    completion_callback_ = callback;
}

void ReportScheduler::RegisterChannel(std::unique_ptr<NotificationChannel> channel) {
    channels_[channel->GetType()] = std::move(channel);
}

std::vector<std::string> ReportScheduler::GetAvailableChannels() const {
    std::vector<std::string> result;
    for (const auto& [type, channel] : channels_) {
        result.push_back(type);
    }
    return result;
}

bool ReportScheduler::SendNotification(const std::string& channel_type,
                                        const std::vector<std::string>& recipients,
                                        const std::string& subject,
                                        const std::string& body,
                                        const std::optional<std::string>& attachment) {
    auto it = channels_.find(channel_type);
    if (it == channels_.end()) return false;
    
    bool all_sent = true;
    for (const auto& recipient : recipients) {
        if (!it->second->Send(recipient, subject, body, attachment)) {
            all_sent = false;
        }
    }
    return all_sent;
}

ReportScheduler::Stats ReportScheduler::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void ReportScheduler::ResetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = Stats{};
}

// Private methods

void ReportScheduler::SchedulerLoop() {
    while (running_) {
        auto now = std::time(nullptr);
        
        // Check for due tasks
        std::vector<std::string> due_tasks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& [id, task] : tasks_) {
                if (task.enabled && task.next_run <= now) {
                    due_tasks.push_back(id);
                }
            }
        }
        
        // Execute due tasks
        for (const auto& task_id : due_tasks) {
            if (!running_) break;
            
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = tasks_.find(task_id);
            if (it != tasks_.end()) {
                ExecuteTask(it->second);
            }
        }
        
        // Wait for next check (every minute)
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(60), [this] { return !running_; });
    }
}

void ReportScheduler::ExecuteTask(ScheduledTask& task) {
    if (task_callback_) {
        task_callback_(task);
    }
    
    if (task.target_type == "alert") {
        auto alert = repository_->GetAlert(task.target_id);
        if (alert) {
            ExecuteAlertTask(task, *alert);
        }
    } else if (task.target_type == "subscription") {
        auto sub = repository_->GetSubscription(task.target_id);
        if (sub) {
            ExecuteSubscriptionTask(task, *sub);
        }
    } else if (task.target_type == "refresh") {
        auto question = repository_->GetQuestion(task.target_id);
        if (question) {
            ExecuteRefreshTask(task, *question);
        }
    }
    
    task.last_run = std::time(nullptr);
    UpdateNextRun(task);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.tasks_executed++;
    }
}

void ReportScheduler::ExecuteAlertTask(const ScheduledTask& task, const Alert& alert) {
    auto question = repository_->GetQuestion(alert.question_id);
    if (!question) return;
    
    // Execute question
    ExecutionContext ctx;
    ExecutionResult result = executor_->ExecuteQuestion(*question, ctx);
    
    if (!result.success) return;
    
    // Extract value (simplified - would parse result)
    double current_value = 0;
    
    // Check condition
    bool triggered = false;
    if (alert.condition.operator_ == ">") {
        triggered = current_value > alert.condition.threshold;
    } else if (alert.condition.operator_ == "<") {
        triggered = current_value < alert.condition.threshold;
    }
    
    if (triggered && (!alert.only_on_change || 
        !alert.last_triggered || 
        current_value != alert.last_value.value_or(0))) {
        SendAlertNotification(alert, current_value, true);
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.alerts_triggered++;
    }
}

void ReportScheduler::ExecuteSubscriptionTask(const ScheduledTask& task, 
                                               const Subscription& subscription) {
    ExecutionContext ctx;
    ExecutionResult result;
    
    if (subscription.target_type == "dashboard") {
        auto dashboard = repository_->GetDashboard(subscription.target_id);
        if (dashboard) {
            auto results = executor_->ExecuteDashboard(*dashboard, ctx);
            result = results.empty() ? ExecutionResult{} : results[0];
        }
    } else {
        auto question = repository_->GetQuestion(subscription.target_id);
        if (question) {
            result = executor_->ExecuteQuestion(*question, ctx);
        }
    }
    
    if (result.success) {
        SendSubscriptionNotification(subscription, result);
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.subscriptions_sent++;
    }
}

void ReportScheduler::ExecuteRefreshTask(const ScheduledTask& task, const Question& question) {
    ExecutionContext ctx;
    executor_->ExecuteQuestion(question, ctx);
}

void ReportScheduler::SendAlertNotification(const Alert& alert,
                                             double current_value,
                                             bool triggered) {
    std::string subject = "Alert: " + alert.name;
    std::string body = "Alert triggered. Current value: " + std::to_string(current_value);
    
    for (const auto& channel : alert.channels) {
        SendNotification(channel, {"owner"}, subject, body);
    }
}

void ReportScheduler::SendSubscriptionNotification(const Subscription& subscription,
                                                    const ExecutionResult& result) {
    std::string subject = subscription.name;
    std::string body = "Subscription results attached.";
    
    std::optional<std::string> attachment;
    if (subscription.include_csv) {
        // Convert result to CSV
        attachment = "results.csv";
    }
    
    for (const auto& channel : subscription.channels) {
        SendNotification(channel, {"owner"}, subject, body, attachment);
    }
}

std::string ReportScheduler::GenerateTaskId() const {
    static int counter = 0;
    return "task_" + std::to_string(++counter);
}

void ReportScheduler::UpdateNextRun(ScheduledTask& task) {
    NextRun next = CalculateNextRun(task.schedule, task.last_run);
    task.next_run = next.scheduled_time;
}

// ScheduleParser implementation

ScheduleSpec ScheduleParser::Parse(const std::string& input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "hourly") {
        return Hourly();
    } else if (lower == "daily") {
        return Daily();
    } else if (lower == "weekly") {
        return Weekly();
    } else if (lower == "monthly") {
        return Monthly();
    }
    
    // Default to hourly
    return Hourly();
}

std::string ScheduleParser::ToString(const ScheduleSpec& spec) {
    switch (spec.type) {
        case ScheduleType::Hourly: return "hourly";
        case ScheduleType::Daily: return "daily";
        case ScheduleType::Weekly: return "weekly";
        case ScheduleType::Monthly: return "monthly";
        case ScheduleType::Cron: return "cron:" + (spec.cron_expr ? *spec.cron_expr : "");
        default: return "hourly";
    }
}

bool ScheduleParser::IsValid(const std::string& input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower == "hourly" || lower == "daily" || lower == "weekly" || 
           lower == "monthly" || lower.substr(0, 5) == "cron:";
}

ScheduleSpec ScheduleParser::Hourly() {
    ScheduleSpec spec;
    spec.type = ScheduleType::Hourly;
    return spec;
}

ScheduleSpec ScheduleParser::Daily(int hour, int minute) {
    ScheduleSpec spec;
    spec.type = ScheduleType::Daily;
    spec.hour = hour;
    spec.minute = minute;
    return spec;
}

ScheduleSpec ScheduleParser::Weekly(int day_of_week, int hour, int minute) {
    ScheduleSpec spec;
    spec.type = ScheduleType::Weekly;
    spec.day_of_week = day_of_week;
    spec.hour = hour;
    spec.minute = minute;
    return spec;
}

ScheduleSpec ScheduleParser::Monthly(int day_of_month, int hour, int minute) {
    ScheduleSpec spec;
    spec.type = ScheduleType::Monthly;
    spec.day_of_month = day_of_month;
    spec.hour = hour;
    spec.minute = minute;
    return spec;
}

} // namespace reporting
} // namespace scratchrobin
