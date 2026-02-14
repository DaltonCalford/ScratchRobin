// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "report_types.h"
#include "query_executor.h"
#include "report_repository.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace scratchrobin {
namespace reporting {

/**
 * @brief Schedule types
 */
enum class ScheduleType {
    Hourly,
    Daily,
    Weekly,
    Monthly,
    Cron  // Custom cron expression
};

/**
 * @brief Parsed schedule specification
 */
struct ScheduleSpec {
    ScheduleType type;
    std::optional<std::string> cron_expr;  // For Cron type
    std::optional<int> hour;               // For Daily/Weekly
    std::optional<int> minute;             // For Daily/Weekly
    std::optional<int> day_of_week;        // For Weekly (0=Sunday)
    std::optional<int> day_of_month;       // For Monthly
};

/**
 * @brief Next run calculation
 */
struct NextRun {
    Timestamp scheduled_time;
    bool is_valid;
};

/**
 * @brief Scheduled task
 */
struct ScheduledTask {
    std::string id;
    std::string name;
    std::string target_type;  // "alert", "subscription", "refresh"
    std::string target_id;
    ScheduleSpec schedule;
    Timestamp next_run;
    int retry_count = 0;
    int max_retries = 1;
    bool enabled = true;
    Timestamp last_run;
    std::optional<std::string> last_error;
    int consecutive_failures = 0;
    int max_consecutive_failures = 3;
};

/**
 * @brief Task execution result
 */
struct TaskResult {
    std::string task_id;
    bool success;
    std::optional<std::string> error_message;
    std::chrono::milliseconds execution_time{0};
    Timestamp executed_at;
};

/**
 * @brief Notification channel interface
 */
class NotificationChannel {
public:
    virtual ~NotificationChannel() = default;
    virtual std::string GetType() const = 0;
    virtual bool Send(const std::string& recipient, 
                      const std::string& subject,
                      const std::string& body,
                      const std::optional<std::string>& attachment = std::nullopt) = 0;
    virtual bool ValidateConfig(const std::map<std::string, std::string>& config) = 0;
};

/**
 * @brief Email notification channel
 */
class EmailChannel : public NotificationChannel {
public:
    std::string GetType() const override { return "email"; }
    bool Send(const std::string& recipient,
              const std::string& subject,
              const std::string& body,
              const std::optional<std::string>& attachment = std::nullopt) override;
    bool ValidateConfig(const std::map<std::string, std::string>& config) override;
};

/**
 * @brief Webhook notification channel
 */
class WebhookChannel : public NotificationChannel {
public:
    std::string GetType() const override { return "webhook"; }
    bool Send(const std::string& url,
              const std::string& subject,
              const std::string& body,
              const std::optional<std::string>& attachment = std::nullopt) override;
    bool ValidateConfig(const std::map<std::string, std::string>& config) override;
};

/**
 * @brief Report scheduler for alerts and subscriptions
 */
class ReportScheduler {
public:
    using TaskCallback = std::function<void(const ScheduledTask&)>;
    using CompletionCallback = std::function<void(const TaskResult&)>;
    
    ReportScheduler(QueryExecutor* executor,
                    ReportRepository* repository);
    ~ReportScheduler();
    
    // Lifecycle
    void Start();
    void Stop();
    bool IsRunning() const { return running_; }
    
    // Schedule management
    std::string ScheduleAlert(const Alert& alert);
    std::string ScheduleSubscription(const Subscription& subscription);
    std::string ScheduleRefresh(const Question& question, const ScheduleSpec& schedule);
    
    bool Unschedule(const std::string& task_id);
    bool EnableTask(const std::string& task_id);
    bool DisableTask(const std::string& task_id);
    
    std::optional<ScheduledTask> GetTask(const std::string& task_id) const;
    std::vector<ScheduledTask> GetAllTasks() const;
    std::vector<ScheduledTask> GetTasksForTarget(const std::string& target_type,
                                                  const std::string& target_id) const;
    
    // Manual execution
    TaskResult RunTaskNow(const std::string& task_id);
    std::vector<TaskResult> RunAllDueTasks();
    
    // Schedule parsing
    static ScheduleSpec ParseSchedule(const std::string& spec);
    static NextRun CalculateNextRun(const ScheduleSpec& schedule, Timestamp from);
    static bool IsDue(const ScheduleSpec& schedule, Timestamp last_run, Timestamp now);
    
    // Set callbacks
    void SetTaskCallback(TaskCallback callback);
    void SetCompletionCallback(CompletionCallback callback);
    
    // Notification channels
    void RegisterChannel(std::unique_ptr<NotificationChannel> channel);
    std::vector<std::string> GetAvailableChannels() const;
    bool SendNotification(const std::string& channel_type,
                          const std::vector<std::string>& recipients,
                          const std::string& subject,
                          const std::string& body,
                          const std::optional<std::string>& attachment = std::nullopt);
    
    // Statistics
    struct Stats {
        int tasks_scheduled = 0;
        int tasks_executed = 0;
        int tasks_failed = 0;
        int alerts_triggered = 0;
        int subscriptions_sent = 0;
    };
    Stats GetStats() const;
    void ResetStats();
    
private:
    QueryExecutor* executor_;
    ReportRepository* repository_;
    
    std::atomic<bool> running_{false};
    std::thread scheduler_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    std::map<std::string, ScheduledTask> tasks_;
    std::map<std::string, std::unique_ptr<NotificationChannel>> channels_;
    
    TaskCallback task_callback_;
    CompletionCallback completion_callback_;
    
    Stats stats_;
    mutable std::mutex stats_mutex_;
    
    void SchedulerLoop();
    void ExecuteTask(ScheduledTask& task);
    void ExecuteAlertTask(const ScheduledTask& task, const Alert& alert);
    void ExecuteSubscriptionTask(const ScheduledTask& task, const Subscription& subscription);
    void ExecuteRefreshTask(const ScheduledTask& task, const Question& question);
    
    void SendAlertNotification(const Alert& alert, 
                               double current_value,
                               bool triggered);
    void SendSubscriptionNotification(const Subscription& subscription,
                                      const ExecutionResult& result);
    
    std::string GenerateTaskId() const;
    void UpdateNextRun(ScheduledTask& task);
};

/**
 * @brief Schedule parser utilities
 */
class ScheduleParser {
public:
    static ScheduleSpec Parse(const std::string& input);
    static std::string ToString(const ScheduleSpec& spec);
    static bool IsValid(const std::string& input);
    
    // Predefined schedules
    static ScheduleSpec Hourly();
    static ScheduleSpec Daily(int hour = 8, int minute = 0);
    static ScheduleSpec Weekly(int day_of_week = 1, int hour = 8, int minute = 0);
    static ScheduleSpec Monthly(int day_of_month = 1, int hour = 8, int minute = 0);
};

} // namespace reporting
} // namespace scratchrobin
