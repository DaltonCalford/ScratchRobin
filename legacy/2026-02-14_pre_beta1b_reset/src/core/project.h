/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_PROJECT_H
#define SCRATCHROBIN_PROJECT_H

#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "metadata_model.h"

namespace scratchrobin {

// Forward declarations
class ProjectObject;
class DesignState;
class GitSyncState;

// ============================================================================
// UUID - Simple unique identifier
// ============================================================================
struct UUID {
    char data[16];
    
    static UUID Generate();
    std::string ToString() const;
    static UUID FromString(const std::string& str);
    bool IsValid() const;
    bool operator==(const UUID& other) const;
    bool operator<(const UUID& other) const;
};

// ============================================================================
// Design State - Tracks object lifecycle state
// ============================================================================
enum class ObjectState {
    EXTRACTED,     // Read-only from source database
    NEW,           // Newly created in design
    MODIFIED,      // Modified from source
    DELETED,       // Marked for deletion
    PENDING,       // Awaiting review
    APPROVED,      // Approved for deployment
    REJECTED,      // Changes rejected
    IMPLEMENTED,   // Deployed to target
    CONFLICTED     // Merge conflict
};

std::string ObjectStateToString(ObjectState state);
ObjectState ObjectStateFromString(const std::string& str);
int GetObjectStateIconIndex(ObjectState state);

// ============================================================================
// Design State with metadata
// ============================================================================
struct DesignState {
    ObjectState state = ObjectState::NEW;
    std::string changed_by;
    std::time_t changed_at = 0;
    std::string reason;
    std::string review_comment;
    
    // State transition helpers
    bool CanTransitionTo(ObjectState new_state) const;
    std::vector<ObjectState> GetAllowedTransitions() const;
};

// ============================================================================
// Project Object - Base class for all project items
// ============================================================================
class ProjectObject {
public:
    UUID id;
    std::string kind;           // table, view, design, whiteboard, etc.
    std::string name;
    std::string path;           // Hierarchical path like "schema/table"
    std::string schema_name;
    
    DesignState design_state;
    
    // Source metadata (if extracted from DB)
    MetadataNode source_snapshot;
    bool has_source = false;
    
    // Current design
    MetadataNode current_design;
    
    // Comments and history
    struct Comment {
        std::string author;
        std::time_t timestamp;
        std::string text;
        bool resolved = false;
    };
    std::vector<Comment> comments;
    
    struct ChangeRecord {
        std::string field;
        std::string old_value;
        std::string new_value;
        std::time_t timestamp;
        std::string author;
    };
    std::vector<ChangeRecord> change_history;
    
    // File paths (relative to project root)
    std::string design_file_path;
    
    ProjectObject();
    explicit ProjectObject(const std::string& kind_, const std::string& name_);
    
    // State management
    void SetState(ObjectState new_state, const std::string& reason, const std::string& user);
    ObjectState GetState() const { return design_state.state; }
    
    // Modification tracking
    void RecordChange(const std::string& field, const std::string& old_val, 
                      const std::string& new_val, const std::string& user);
    
    // Serialization
    void ToJson(std::ostream& out) const;
    static std::unique_ptr<ProjectObject> FromJson(const std::string& json);
    
    // DDL generation
    std::string GenerateDDL() const;
    std::string GenerateRollbackDDL() const;
};

// ============================================================================
// Database Connection Configuration
// ============================================================================
struct DatabaseConnection {
    std::string name;
    std::string connection_string;
    std::string backend_type;  // postgresql, mysql, firebird, scratchbird
    bool is_source = false;    // Read-only extraction source
    bool is_target = false;    // Deployment target
    std::string git_branch;    // Associated Git branch
    bool requires_approval = false;
    std::string credential_ref;
    
    // For ScratchBird Git repos
    bool is_git_enabled = false;
    std::string git_repo_url;
};

// ============================================================================
// Git Configuration
// ============================================================================
struct GitConfig {
    bool enabled = false;
    std::string repo_url;
    std::string default_branch = "main";
    std::string workflow = "gitflow";  // gitflow, trunk-based, etc.
    
    // Sync settings
    std::string sync_mode = "bidirectional";  // bidirectional, project-to-db, db-to-project
    std::vector<std::string> auto_sync_branches;
    std::vector<std::string> protected_branches;
    
    // Commit settings
    bool require_conventional_commits = true;
    bool auto_sync_messages = true;
};

// ============================================================================
// Project Configuration
// ============================================================================
struct ProjectConfig {
    std::string name;
    std::string description;
    std::string version;
    std::string database_type;
    
    std::vector<DatabaseConnection> connections;
    GitConfig git;
    
    // Paths within project
    std::string designs_path = "designs";
    std::string diagrams_path = "diagrams";
    std::string whiteboards_path = "whiteboards";
    std::string mindmaps_path = "mindmaps";
    std::string docs_path = "docs";
    std::string reports_path = "reports";
    std::string tests_path = "tests";
    std::string deployments_path = "deployments";
    
    // Reporting storage configuration
    struct ReportingStorage {
        bool enabled = false;
        std::string storage_type = "embedded";  // embedded, external, s3
        std::string database_path;              // For embedded: relative path to result database
        std::string connection_ref;             // For external: connection profile name
        std::string schema_name = "reporting";  // Schema for result tables
        std::string table_prefix = "rpt_";      // Prefix for result tables
        uint32_t retention_days = 90;           // How long to keep results
        uint32_t max_result_size_mb = 100;      // Max size per result set
        bool compress_results = true;           // Compress stored results
        bool encrypt_results = false;           // Encrypt at rest
    };
    ReportingStorage reporting_storage;

    struct GovernanceReviewPolicy {
        uint32_t min_reviewers = 0;
        std::vector<std::string> required_roles;
        uint32_t approval_window_hours = 0;
    };

    struct GovernanceAiPolicy {
        bool enabled = false;
        bool requires_review = true;
        std::vector<std::string> allowed_scopes;
        std::vector<std::string> prohibited_scopes;
    };

    struct GovernanceAuditPolicy {
        std::string log_level;
        uint32_t retain_days = 0;
        std::string export_target;
    };

    struct GovernanceEnvironment {
        std::string id;
        std::string name;
        bool approval_required = false;
        uint32_t min_reviewers = 0;
        std::vector<std::string> allowed_roles;
    };

    struct Governance {
        std::vector<std::string> owners;
        std::vector<std::string> stewards;
        std::vector<GovernanceEnvironment> environments;
        std::vector<std::string> compliance_tags;
        GovernanceReviewPolicy review_policy;
        GovernanceAiPolicy ai_policy;
        GovernanceAuditPolicy audit_policy;
    };

    Governance governance;

    // Serialization
    void ToYaml(std::ostream& out) const;
    static ProjectConfig FromYaml(const std::string& yaml);
};

// ============================================================================
// Cross-Repo Sync State
// ============================================================================
struct GitSyncState {
    std::time_t last_sync = 0;
    
    struct RepoState {
        std::string head_commit;
        std::string branch;
        std::vector<std::string> dirty_files;
    };
    RepoState project_repo;
    RepoState database_repo;
    
    struct ObjectMapping {
        std::string project_file;
        std::string db_object;
        std::string last_sync_commit;
        std::string sync_status;  // in_sync, new_in_project, modified, conflict
    };
    std::vector<ObjectMapping> mappings;
    
    struct PendingSync {
        std::vector<std::string> project_to_db;
        std::vector<std::string> db_to_project;
        std::vector<std::string> conflicts;
    };
    PendingSync pending;
    
    void ToJson(std::ostream& out) const;
    static GitSyncState FromJson(const std::string& json);
};

// ============================================================================
// Main Project Class
// ============================================================================
class Project {
public:
    Project();
    ~Project();
    
    // Project identity
    UUID id;
    ProjectConfig config;
    GitSyncState sync_state;
    
    // File system
    std::string project_root_path;
    std::string project_file_path;
    
    // Object storage
    std::map<UUID, std::shared_ptr<ProjectObject>> objects_by_id;
    std::map<std::string, std::shared_ptr<ProjectObject>> objects_by_path;

    struct ReportingAsset {
        UUID id;
        std::string object_type;
        std::string json_payload;
    };

    struct ReportingSchedule {
        UUID id;
        std::string action;
        std::string target_id;
        std::string schedule_spec;
        int interval_seconds = 0;
        std::time_t created_at = 0;
        std::time_t next_run = 0;
        std::time_t last_run = 0;
        bool enabled = true;
    };

    struct ReportingCacheEntry {
        std::string key;
        std::string payload_json;
        std::time_t cached_at = 0;
        int ttl_seconds = 0;
        int64_t rows_returned = 0;
        std::string source_id;
    };

    struct DataViewSnapshot {
        UUID id;
        UUID diagram_id;
        std::string json_payload;
    };

    struct TemplateAsset {
        std::string name;
        std::string kind;
        std::string path;
    };

    std::vector<ReportingAsset> reporting_assets;
    std::vector<DataViewSnapshot> data_views;
    std::vector<ReportingSchedule> reporting_schedules;
    std::unordered_map<std::string, ReportingCacheEntry> reporting_cache;
    
    // Lifecycle
    bool CreateNew(const std::string& path, const ProjectConfig& cfg);
    bool Open(const std::string& path);
    bool Save();
    bool SaveAs(const std::string& new_path);
    void Close();
    bool IsModified() const { return is_modified_; }
    
    // Object management
    std::shared_ptr<ProjectObject> CreateObject(const std::string& kind, 
                                                 const std::string& name,
                                                 const std::string& schema = "");
    std::shared_ptr<ProjectObject> GetObject(const UUID& id);
    std::shared_ptr<ProjectObject> GetObjectByPath(const std::string& path);
    bool DeleteObject(const UUID& id);
    std::vector<std::shared_ptr<ProjectObject>> GetObjectsByState(ObjectState state);
    std::vector<std::shared_ptr<ProjectObject>> GetObjectsByKind(const std::string& kind);
    
    // Extraction from database
    bool ExtractFromDatabase(const DatabaseConnection& conn, 
                             const std::vector<std::string>& object_patterns);
    
    // Design operations
    bool ModifyObject(const UUID& id, const MetadataNode& new_design);
    bool MarkObjectDeleted(const UUID& id);
    bool RestoreObject(const UUID& id);

    // Diagram registration
    std::shared_ptr<ProjectObject> RegisterDiagramObject(const std::string& diagram_name,
                                                         const std::string& design_path,
                                                         const std::string& diagram_type);

    // Governance enforcement
    struct GovernanceContext {
        std::string action;
        std::string role;
        std::string environment_id;
        uint32_t approvals = 0;
        bool ai_action = false;
        std::string ai_scope;
    };

    struct GovernanceDecision {
        bool allowed = true;
        std::string reason;
    };
    
    // State operations
    bool ApproveObject(const UUID& id, const std::string& approver);
    bool RejectObject(const UUID& id, const std::string& reason);
    bool ApproveObjectWithGovernance(const UUID& id,
                                     const std::string& approver,
                                     const GovernanceContext& context,
                                     std::string* reason);
    bool RejectObjectWithGovernance(const UUID& id,
                                    const std::string& reason,
                                    const GovernanceContext& context,
                                    std::string* out_reason);

    // Status reporting
    struct StatusEvent {
        std::time_t timestamp = 0;
        std::string message;
        bool is_error = false;
    };
    using StatusCallback = std::function<void(const StatusEvent&)>;
    void SetStatusCallback(StatusCallback callback);
    void ClearStatusCallback();
    std::vector<StatusEvent> GetStatusEvents() const;

    GovernanceDecision EvaluateGovernance(const GovernanceContext& context) const;
    bool CanExecuteReportingAction(const GovernanceContext& context, std::string* reason) const;

    // Audit logging
    struct AuditEvent {
        std::time_t timestamp = 0;
        std::string actor;
        std::string action;
        std::string target_id;
        std::string connection_ref;
        bool success = true;
        std::string detail;
        std::map<std::string, std::string> metadata;
    };

    void RecordReportingAudit(const AuditEvent& event);
    bool RefreshDataViewWithGovernance(const UUID& id,
                                       const std::string& captured_at_iso,
                                       const GovernanceContext& context,
                                       std::string* reason);
    bool ScheduleReportingAction(const std::string& action,
                                 const std::string& target_id,
                                 const GovernanceContext& context,
                                 std::string* reason);

    // Reporting + Data Views
    ReportingAsset* UpsertReportingAsset(const ReportingAsset& asset);
    ReportingAsset* InsertReportingAsset(const ReportingAsset& asset);
    ReportingSchedule* AddReportingSchedule(const std::string& action,
                                            const std::string& target_id,
                                            const std::string& schedule_spec);
    size_t ExecuteDueReportingSchedules();
    std::optional<ReportingCacheEntry> GetReportingCache(const std::string& key) const;
    void StoreReportingCache(const ReportingCacheEntry& entry);
    void ClearReportingCache();
    DataViewSnapshot* UpsertDataView(const DataViewSnapshot& view);
    bool MarkDataViewStale(const UUID& id, const std::string& reason);
    bool MarkDataViewRefreshed(const UUID& id, const std::string& captured_at_iso);
    void InvalidateAllDataViews(const std::string& reason);
    void InvalidateDataViewsForObject(const std::string& schema, const std::string& name);

    // Templates + automated documentation
    std::vector<TemplateAsset> DiscoverTemplates(std::string* error) const;

    // Git operations
    bool SyncToDatabase();
    bool SyncFromDatabase();
    bool ResolveConflict(const UUID& id, const std::string& resolution);
    
    // Validation
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    ValidationResult Validate() const;
    
    // Statistics
    struct Stats {
        int total_objects = 0;
        int extracted = 0;
        int new_objects = 0;
        int modified = 0;
        int deleted = 0;
        int pending = 0;
        int approved = 0;
        int implemented = 0;
    };
    Stats GetStats() const;
    
    // Events
    using ObjectChangedCallback = std::function<void(const UUID&, const std::string& action)>;
    void AddObserver(ObjectChangedCallback callback);
    void RemoveObserver(ObjectChangedCallback callback);
    void EmitStatus(const std::string& message, bool is_error = false);
    
private:
    bool is_modified_ = false;
    bool is_open_ = false;
    std::vector<ObjectChangedCallback> observers_;
    StatusCallback status_callback_;
    std::vector<StatusEvent> status_events_;
    
    void NotifyObjectChanged(const UUID& id, const std::string& action);
    bool SaveProjectFile();
    bool SaveObjectFiles();
    bool LoadProjectFile();
    bool LoadObjectFiles();
    std::string GetObjectFilePath(const ProjectObject& obj) const;
};

// ============================================================================
// Project Manager - Singleton for managing open projects
// ============================================================================
class ProjectManager {
public:
    static ProjectManager& Instance();
    
    ProjectManager(const ProjectManager&) = delete;
    ProjectManager& operator=(const ProjectManager&) = delete;
    
    // Project lifecycle
    std::shared_ptr<Project> CreateProject(const std::string& path, 
                                           const ProjectConfig& config);
    std::shared_ptr<Project> OpenProject(const std::string& path);
    bool CloseProject(const UUID& id);
    bool CloseAllProjects();
    
    // Access
    std::shared_ptr<Project> GetCurrentProject();
    std::shared_ptr<Project> GetProject(const UUID& id);
    std::vector<std::shared_ptr<Project>> GetOpenProjects();
    
    void SetCurrentProject(const UUID& id);
    
    // Recent projects
    std::vector<std::string> GetRecentProjects();
    void AddRecentProject(const std::string& path);
    
private:
    ProjectManager() = default;
    
    std::map<UUID, std::shared_ptr<Project>> open_projects_;
    UUID current_project_id_;
    std::vector<std::string> recent_projects_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PROJECT_H
