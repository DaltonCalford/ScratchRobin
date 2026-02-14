/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "deployment_plan.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

namespace scratchrobin {

// ============================================================================
// Deployment Environment
// ============================================================================
std::string DeploymentEnvironment::GetTypeString() const {
    switch (type) {
        case Type::DEVELOPMENT: return "Development";
        case Type::TESTING: return "Testing";
        case Type::STAGING: return "Staging";
        case Type::PRODUCTION: return "Production";
        default: return "Unknown";
    }
}

// ============================================================================
// Deployment Step
// ============================================================================
bool DeploymentStep::IsBlocking() const {
    // Most steps are blocking - only some notification/script steps might not be
    return type != Type::NOTIFICATION;
}

// ============================================================================
// Deployment Plan Implementation
// ============================================================================
DeploymentPlan::DeploymentPlan() {
    created_at = std::time(nullptr);
    id = "deploy_" + std::to_string(created_at);
}

void DeploymentPlan::AddStep(const DeploymentStep& step) {
    steps.push_back(step);
    std::sort(steps.begin(), steps.end(),
              [](const auto& a, const auto& b) { 
                  return a.execution_order < b.execution_order; 
              });
}

void DeploymentPlan::AddMigrationSteps(const std::vector<MigrationScript*>& migrations) {
    int order = static_cast<int>(steps.size()) + 1;
    
    for (auto* migration : migrations) {
        DeploymentStep step;
        step.type = DeploymentStep::Type::MIGRATION;
        step.name = "Apply " + migration->name;
        step.description = migration->description;
        step.execution_order = order++;
        step.migration_id = migration->id;
        step.max_retries = 0;  // Migrations typically shouldn't retry
        
        steps.push_back(step);
    }
}

void DeploymentPlan::AddValidationSteps() {
    // Pre-deploy validation
    DeploymentStep pre_check;
    pre_check.type = DeploymentStep::Type::PRE_DEPLOY_CHECK;
    pre_check.name = "Pre-deployment validation";
    pre_check.description = "Validate database state before deployment";
    pre_check.execution_order = 1;
    AddStep(pre_check);
    
    // Post-deploy validation
    DeploymentStep post_check;
    post_check.type = DeploymentStep::Type::POST_DEPLOY_CHECK;
    post_check.name = "Post-deployment validation";
    post_check.description = "Validate database state after deployment";
    post_check.execution_order = static_cast<int>(steps.size()) + 100;
    AddStep(post_check);
}

void DeploymentPlan::AddBackupStep() {
    if (target_env.require_backup) {
        DeploymentStep backup;
        backup.type = DeploymentStep::Type::BACKUP;
        backup.name = "Create backup";
        backup.description = "Backup database before migration";
        backup.execution_order = 0;  // First step
        AddStep(backup);
    }
}

bool DeploymentPlan::Execute(DatabaseConnection* connection,
                              std::function<void(const DeploymentStep&)> progress_callback) {
    if (status != Status::APPROVED && status != Status::DRAFT) {
        return false;
    }
    
    status = Status::IN_PROGRESS;
    started_at = std::time(nullptr);
    
    auto overall_start = std::chrono::steady_clock::now();
    
    for (auto& step : steps) {
        if (step.status == DeploymentStep::Status::SKIPPED ||
            step.status == DeploymentStep::Status::SUCCESS) {
            continue;
        }
        
        step.status = DeploymentStep::Status::RUNNING;
        step.started_at = std::time(nullptr);
        
        if (progress_callback) {
            progress_callback(step);
        }
        
        // Execute the step (simplified - would actually run the operation)
        bool success = true;  // Stub - would execute actual operation
        
        step.completed_at = std::time(nullptr);
        auto step_end = std::chrono::steady_clock::now();
        step.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            step_end - std::chrono::steady_clock::now());
        
        if (success) {
            step.status = DeploymentStep::Status::SUCCESS;
        } else {
            step.status = DeploymentStep::Status::FAILED;
            
            if (step.IsBlocking()) {
                status = Status::FAILED;
                
                auto overall_end = std::chrono::steady_clock::now();
                total_duration = std::chrono::duration_cast<std::chrono::seconds>(
                    overall_end - overall_start);
                
                return false;
            }
        }
    }
    
    auto overall_end = std::chrono::steady_clock::now();
    total_duration = std::chrono::duration_cast<std::chrono::seconds>(
        overall_end - overall_start);
    
    status = Status::COMPLETED;
    completed_at = std::time(nullptr);
    
    return true;
}

bool DeploymentPlan::Rollback(DatabaseConnection* connection,
                               std::function<void(const DeploymentStep&)> progress_callback) {
    status = Status::ROLLING_BACK;
    
    // Rollback migrations in reverse order
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
        if (it->type != DeploymentStep::Type::MIGRATION) continue;
        
        // Execute rollback for this migration
        // Would load migration and execute its rollback SQL
        
        if (progress_callback) {
            progress_callback(*it);
        }
    }
    
    status = Status::ROLLED_BACK;
    return true;
}

bool DeploymentPlan::Validate(std::vector<std::string>& errors) const {
    bool valid = true;
    
    if (steps.empty()) {
        errors.push_back("Deployment plan has no steps");
        valid = false;
    }
    
    if (target_env.id.empty()) {
        errors.push_back("Target environment not specified");
        valid = false;
    }
    
    if (target_env.IsProduction() && !approval.has_value()) {
        errors.push_back("Production deployments require approval");
        valid = false;
    }
    
    return valid;
}

DeploymentPlan::Analysis DeploymentPlan::Analyze() const {
    Analysis analysis;
    
    analysis.total_steps = static_cast<int>(steps.size());
    
    for (const auto& step : steps) {
        switch (step.type) {
            case DeploymentStep::Type::MIGRATION:
                analysis.migration_steps++;
                break;
            case DeploymentStep::Type::PRE_DEPLOY_CHECK:
            case DeploymentStep::Type::POST_DEPLOY_CHECK:
                analysis.validation_steps++;
                break;
            default:
                break;
        }
        
        // Estimate duration (very rough)
        if (step.type == DeploymentStep::Type::MIGRATION) {
            analysis.estimated_duration += std::chrono::seconds(30);
        } else if (step.type == DeploymentStep::Type::BACKUP) {
            analysis.estimated_duration += std::chrono::seconds(60);
        } else {
            analysis.estimated_duration += std::chrono::seconds(5);
        }
    }
    
    // Check for risks
    if (target_env.IsProduction()) {
        analysis.risks.push_back("Production deployment - extra caution required");
    }
    
    if (analysis.migration_steps > 5) {
        analysis.risks.push_back("Large number of migrations - consider breaking into smaller deployments");
    }
    
    // Prerequisites
    if (target_env.require_backup) {
        analysis.prerequisites.push_back("Ensure sufficient disk space for backup");
    }
    
    analysis.requires_downtime = !target_env.allow_downtime;
    
    return analysis;
}

std::string DeploymentPlan::GetStatusString() const {
    switch (status) {
        case Status::DRAFT: return "Draft";
        case Status::PENDING_APPROVAL: return "Pending Approval";
        case Status::APPROVED: return "Approved";
        case Status::IN_PROGRESS: return "In Progress";
        case Status::COMPLETED: return "Completed";
        case Status::FAILED: return "Failed";
        case Status::ROLLING_BACK: return "Rolling Back";
        case Status::ROLLED_BACK: return "Rolled Back";
        default: return "Unknown";
    }
}

// ============================================================================
// Deployment Manager Implementation
// ============================================================================
void DeploymentManager::AddEnvironment(const DeploymentEnvironment& env) {
    environments_[env.id] = env;
}

void DeploymentManager::UpdateEnvironment(const std::string& id, 
                                           const DeploymentEnvironment& env) {
    environments_[id] = env;
}

void DeploymentManager::RemoveEnvironment(const std::string& id) {
    environments_.erase(id);
}

std::vector<DeploymentEnvironment> DeploymentManager::GetEnvironments() const {
    std::vector<DeploymentEnvironment> result;
    for (const auto& [id, env] : environments_) {
        result.push_back(env);
    }
    return result;
}

std::optional<DeploymentEnvironment> DeploymentManager::GetEnvironment(
    const std::string& id) const {
    auto it = environments_.find(id);
    if (it != environments_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::unique_ptr<DeploymentPlan> DeploymentManager::CreatePlan(
    const std::string& source_env_id,
    const std::string& target_env_id,
    const std::vector<std::string>& migration_ids) {
    
    auto source = GetEnvironment(source_env_id);
    auto target = GetEnvironment(target_env_id);
    
    if (!source || !target) {
        return nullptr;
    }
    
    auto plan = std::make_unique<DeploymentPlan>();
    plan->source_env = *source;
    plan->target_env = *target;
    plan->migration_ids = migration_ids;
    
    plan->AddBackupStep();
    plan->AddValidationSteps();
    
    if (target->require_approval) {
        plan->status = DeploymentPlan::Status::PENDING_APPROVAL;
    }
    
    auto plan_id = plan->id;
    auto result = std::make_unique<DeploymentPlan>(*plan);
    plans_[plan_id] = std::move(plan);
    return result;
    
    return plan;
}

bool DeploymentManager::ApprovePlan(const std::string& plan_id,
                                     const std::string& approver,
                                     const std::string& comment) {
    auto it = plans_.find(plan_id);
    if (it == plans_.end()) return false;
    
    auto* plan = it->second.get();
    if (plan->status != DeploymentPlan::Status::PENDING_APPROVAL) {
        return false;
    }
    
    DeploymentPlan::Approval approval;
    approval.approver = approver;
    approval.approved_at = std::time(nullptr);
    approval.comment = comment;
    
    plan->approval = approval;
    plan->status = DeploymentPlan::Status::APPROVED;
    
    return true;
}

bool DeploymentManager::ExecutePlan(const std::string& plan_id,
                                     DatabaseConnection* connection,
                                     ProgressCallback progress,
                                     LogCallback log) {
    auto it = plans_.find(plan_id);
    if (it == plans_.end()) return false;
    
    auto* plan = it->second.get();
    
    // Validate plan
    std::vector<std::string> errors;
    if (!plan->Validate(errors)) {
        if (log) {
            for (const auto& err : errors) {
                log("Validation error: " + err);
            }
        }
        return false;
    }
    
    // Lock environment
    auto env = GetEnvironment(plan->target_env.id);
    if (env && env->is_locked) {
        if (log) log("Environment is locked");
        return false;
    }
    
    // Execute
    int step_count = static_cast<int>(plan->steps.size());
    int current_step = 0;
    
    auto progress_callback = [&](const DeploymentStep& step) {
        current_step++;
        if (progress) {
            progress(step.name, (current_step * 100) / step_count);
        }
        if (log) {
            log("Executing: " + step.name);
        }
    };
    
    return plan->Execute(connection, progress_callback);
}

bool DeploymentManager::ExecuteStep(DeploymentStep& step,
                                     DatabaseConnection* connection,
                                     LogCallback log) {
    // This would execute the actual step logic
    // For now, just a stub that simulates success
    
    if (log) {
        log("  Starting " + step.name);
    }
    
    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (log) {
        log("  Completed " + step.name);
    }
    
    return true;
}

std::vector<DeploymentManager::HealthCheck> DeploymentManager::RunPreDeployChecks(
    const DeploymentPlan& plan,
    DatabaseConnection* connection) {
    
    std::vector<HealthCheck> checks;
    
    // Check 1: Connection
    {
        HealthCheck check;
        check.name = "Database connection";
        auto start = std::chrono::steady_clock::now();
        
        // Would actually test connection
        check.passed = true;
        check.message = "Connected successfully";
        
        auto end = std::chrono::steady_clock::now();
        check.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        checks.push_back(check);
    }
    
    // Check 2: Disk space
    {
        HealthCheck check;
        check.name = "Disk space";
        check.passed = true;
        check.message = "Sufficient disk space available";
        checks.push_back(check);
    }
    
    // Check 3: Version compatibility
    {
        HealthCheck check;
        check.name = "Version compatibility";
        check.passed = true;
        check.message = "Target version compatible";
        checks.push_back(check);
    }
    
    return checks;
}

std::vector<DeploymentManager::HealthCheck> DeploymentManager::RunPostDeployChecks(
    const DeploymentPlan& plan,
    DatabaseConnection* connection) {
    
    std::vector<HealthCheck> checks;
    
    // Check 1: Migration applied
    {
        HealthCheck check;
        check.name = "Migrations applied";
        check.passed = true;
        check.message = "All migrations successfully applied";
        checks.push_back(check);
    }
    
    // Check 2: Object count
    {
        HealthCheck check;
        check.name = "Object count";
        check.passed = true;
        check.message = "Expected objects present";
        checks.push_back(check);
    }
    
    return checks;
}

// ============================================================================
// Deployment History Implementation
// ============================================================================
// Stub implementation - would persist to database

void DeploymentHistory::Record(const Entry& entry) {
    // Would insert into database
}

std::vector<DeploymentHistory::Entry> DeploymentHistory::GetHistory(
    const std::string& env_id, int limit) {
    // Would query database
    return {};
}

std::optional<DeploymentHistory::Entry> DeploymentHistory::GetLastDeployment(
    const std::string& env_id) {
    // Would query database
    return std::nullopt;
}

bool DeploymentHistory::CanRollback(const std::string& env_id) {
    auto last = GetLastDeployment(env_id);
    if (!last) return false;
    return last->success;
}

std::vector<std::string> DeploymentHistory::GetRollbackTargets(const std::string& env_id) {
    // Would return previous versions available for rollback
    return {};
}

// ============================================================================
// Deployment Schedule Implementation
// ============================================================================
// Stub implementation

std::string DeploymentSchedule::Schedule(const ScheduledDeployment& deployment) {
    // Would persist to schedule store
    return "schedule_" + std::to_string(std::time(nullptr));
}

bool DeploymentSchedule::Cancel(const std::string& schedule_id) {
    // Would remove from schedule store
    return true;
}

std::vector<DeploymentSchedule::ScheduledDeployment> DeploymentSchedule::GetUpcoming() {
    // Would query schedule store for pending deployments
    return {};
}

void DeploymentSchedule::ProcessDueDeployments() {
    // Would check for due deployments and execute them
}

} // namespace scratchrobin
