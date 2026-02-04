/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DEPLOYMENT_PLAN_H
#define SCRATCHROBIN_DEPLOYMENT_PLAN_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include "migration_generator.h"

namespace scratchrobin {

// Forward declarations
class DatabaseConnection;
class MigrationScript;

// ============================================================================
// Deployment Environment
// ============================================================================
struct DeploymentEnvironment {
    std::string id;
    std::string name;
    std::string description;
    
    // Connection info
    std::string connection_string;
    std::string backend_type;  // firebird, postgres, mysql
    
    // Environment type
    enum class Type { DEVELOPMENT, TESTING, STAGING, PRODUCTION };
    Type type = Type::DEVELOPMENT;
    
    // Deployment settings
    bool require_approval = false;
    bool require_backup = true;
    bool allow_downtime = true;
    int max_parallel_migrations = 1;
    std::chrono::seconds timeout{300};
    
    // Current state
    std::string current_version;
    std::time_t last_deployment;
    bool is_locked = false;
    
    // Notifications
    std::vector<std::string> notify_emails;
    std::string webhook_url;
    
    std::string GetTypeString() const;
    bool IsProduction() const { return type == Type::PRODUCTION; }
};

// ============================================================================
// Deployment Step
// ============================================================================
struct DeploymentStep {
    enum class Type {
        BACKUP,
        PRE_DEPLOY_CHECK,
        MIGRATION,
        POST_DEPLOY_CHECK,
        ROLLBACK,
        NOTIFICATION,
        CUSTOM_SCRIPT
    };
    
    Type type;
    std::string name;
    std::string description;
    int execution_order = 0;
    
    // For migration steps
    std::string migration_id;
    
    // For custom scripts
    std::string script_content;
    
    // Status
    enum class Status { PENDING, RUNNING, SUCCESS, FAILED, SKIPPED };
    Status status = Status::PENDING;
    
    // Results
    std::string output_log;
    std::string error_message;
    std::chrono::milliseconds execution_time{0};
    std::time_t started_at = 0;
    std::time_t completed_at = 0;
    
    // Retry configuration
    int max_retries = 0;
    int retry_count = 0;
    
    bool CanRetry() const { return retry_count < max_retries; }
    bool IsBlocking() const;  // Returns true if failure stops deployment
};

// ============================================================================
// Deployment Plan
// ============================================================================
class DeploymentPlan {
public:
    std::string id;
    std::string name;
    std::string description;
    std::string created_by;
    std::time_t created_at;
    
    // Source and target
    DeploymentEnvironment source_env;
    DeploymentEnvironment target_env;
    
    // Migrations to deploy
    std::vector<std::string> migration_ids;
    
    // Steps
    std::vector<DeploymentStep> steps;
    
    // Status
    enum class Status {
        DRAFT,
        PENDING_APPROVAL,
        APPROVED,
        IN_PROGRESS,
        COMPLETED,
        FAILED,
        ROLLING_BACK,
        ROLLED_BACK
    };
    Status status = Status::DRAFT;
    
    // Scheduling
    std::optional<std::time_t> scheduled_for;
    
    // Approval
    struct Approval {
        std::string approver;
        std::time_t approved_at;
        std::string comment;
    };
    std::optional<Approval> approval;
    
    // Execution results
    std::time_t started_at = 0;
    std::time_t completed_at = 0;
    std::chrono::seconds total_duration{0};
    
    // Rollback info
    bool can_rollback = true;
    std::string rollback_plan_id;
    
    DeploymentPlan();
    
    // Plan generation
    void AddStep(const DeploymentStep& step);
    void AddMigrationSteps(const std::vector<MigrationScript*>& migrations);
    void AddValidationSteps();
    void AddBackupStep();
    
    // Execution
    bool Execute(DatabaseConnection* connection, 
                 std::function<void(const DeploymentStep&)> progress_callback = nullptr);
    bool Rollback(DatabaseConnection* connection,
                  std::function<void(const DeploymentStep&)> progress_callback = nullptr);
    
    // Validation
    bool Validate(std::vector<std::string>& errors) const;
    
    // Analysis
    struct Analysis {
        int total_steps = 0;
        int migration_steps = 0;
        int validation_steps = 0;
        bool requires_downtime = false;
        std::chrono::seconds estimated_duration{0};
        std::vector<std::string> risks;
        std::vector<std::string> prerequisites;
    };
    Analysis Analyze() const;
    
    // Serialization
    void ToJson(std::ostream& out) const;
    static std::unique_ptr<DeploymentPlan> FromJson(const std::string& json);
    void SaveToFile(const std::string& path) const;
    static std::unique_ptr<DeploymentPlan> LoadFromFile(const std::string& path);
    
    // Status helpers
    bool IsDraft() const { return status == Status::DRAFT; }
    bool IsPending() const { return status == Status::PENDING_APPROVAL; }
    bool IsInProgress() const { return status == Status::IN_PROGRESS; }
    bool IsComplete() const { return status == Status::COMPLETED; }
    bool HasFailed() const { return status == Status::FAILED; }
    std::string GetStatusString() const;
};

// ============================================================================
// Deployment Manager
// ============================================================================
class DeploymentManager {
public:
    using ProgressCallback = std::function<void(const std::string& message, int percent)>;
    using LogCallback = std::function<void(const std::string& message)>;
    
    // Environment management
    void AddEnvironment(const DeploymentEnvironment& env);
    void UpdateEnvironment(const std::string& id, const DeploymentEnvironment& env);
    void RemoveEnvironment(const std::string& id);
    std::vector<DeploymentEnvironment> GetEnvironments() const;
    std::optional<DeploymentEnvironment> GetEnvironment(const std::string& id) const;
    
    // Plan management
    std::unique_ptr<DeploymentPlan> CreatePlan(
        const std::string& source_env_id,
        const std::string& target_env_id,
        const std::vector<std::string>& migration_ids);
    
    std::unique_ptr<DeploymentPlan> CreateRollbackPlan(
        const std::string& env_id,
        const std::string& to_version);
    
    std::vector<std::unique_ptr<DeploymentPlan>> GetPendingPlans() const;
    std::vector<std::unique_ptr<DeploymentPlan>> GetRecentDeployments(
        const std::string& env_id, int limit = 10);
    
    // Plan approval
    bool ApprovePlan(const std::string& plan_id, const std::string& approver,
                     const std::string& comment);
    bool RejectPlan(const std::string& plan_id, const std::string& reason);
    
    // Execution
    bool ExecutePlan(const std::string& plan_id,
                     DatabaseConnection* connection,
                     ProgressCallback progress = nullptr,
                     LogCallback log = nullptr);
    
    bool ExecutePlanAsync(const std::string& plan_id,
                          DatabaseConnection* connection);
    
    void CancelExecution(const std::string& plan_id);
    
    // Health checks
    struct HealthCheck {
        std::string name;
        bool passed = false;
        std::string message;
        std::chrono::milliseconds duration{0};
    };
    
    std::vector<HealthCheck> RunPreDeployChecks(const DeploymentPlan& plan,
                                                 DatabaseConnection* connection);
    std::vector<HealthCheck> RunPostDeployChecks(const DeploymentPlan& plan,
                                                  DatabaseConnection* connection);
    
    // Comparison
    struct ComparisonResult {
        std::string source_version;
        std::string target_version;
        std::vector<std::string> pending_migrations;
        bool is_up_to_date = false;
    };
    
    ComparisonResult CompareEnvironments(const std::string& source_id,
                                          const std::string& target_id);
    
    // Drift detection
    struct DriftReport {
        std::string environment_id;
        std::time_t detected_at;
        std::vector<std::string> unexpected_objects;
        std::vector<std::string> missing_objects;
        std::vector<std::string> modified_objects;
        bool has_drift = false;
    };
    
    DriftReport DetectDrift(const std::string& env_id,
                            DatabaseConnection* connection);
    
    // Notifications
    void SendNotification(const std::string& plan_id, 
                          const std::string& event,
                          const std::string& message);
    
private:
    std::map<std::string, DeploymentEnvironment> environments_;
    std::map<std::string, std::unique_ptr<DeploymentPlan>> plans_;
    
    bool ExecuteStep(DeploymentStep& step, 
                     DatabaseConnection* connection,
                     LogCallback log);
};

// ============================================================================
// Deployment History
// ============================================================================
class DeploymentHistory {
public:
    struct Entry {
        std::string plan_id;
        std::string environment_id;
        std::string migration_id;
        std::string deployed_by;
        std::time_t deployed_at;
        std::string version_before;
        std::string version_after;
        bool success = true;
        std::string error_message;
        std::chrono::seconds duration{0};
    };
    
    static void Record(const Entry& entry);
    static std::vector<Entry> GetHistory(const std::string& env_id, 
                                          int limit = 50);
    static std::optional<Entry> GetLastDeployment(const std::string& env_id);
    static bool CanRollback(const std::string& env_id);
    static std::vector<std::string> GetRollbackTargets(const std::string& env_id);
};

// ============================================================================
// Deployment Schedule
// ============================================================================
class DeploymentSchedule {
public:
    struct ScheduledDeployment {
        std::string id;
        std::string plan_id;
        std::time_t scheduled_time;
        bool recurring = false;
        std::string recurrence_pattern;  // cron-like
        std::string timezone;
    };
    
    static std::string Schedule(const ScheduledDeployment& deployment);
    static bool Cancel(const std::string& schedule_id);
    static std::vector<ScheduledDeployment> GetUpcoming();
    static void ProcessDueDeployments();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DEPLOYMENT_PLAN_H
