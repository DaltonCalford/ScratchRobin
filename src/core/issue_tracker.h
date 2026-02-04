/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ISSUE_TRACKER_H
#define SCRATCHROBIN_ISSUE_TRACKER_H

#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin {

// ============================================================================
// Issue Status Enum
// ============================================================================
enum class IssueStatus {
    OPEN,
    IN_PROGRESS,
    RESOLVED,
    CLOSED,
    REOPENED,
    PENDING_APPROVAL,
    BLOCKED
};

std::string IssueStatusToString(IssueStatus status);
IssueStatus IssueStatusFromString(const std::string& str);

// ============================================================================
// Priority Enum
// ============================================================================
enum class IssuePriority {
    CRITICAL,   // P0
    HIGH,       // P1
    MEDIUM,     // P2
    LOW,        // P3
    TRIVIAL     // P4
};

std::string IssuePriorityToString(IssuePriority priority);
std::string IssuePriorityToLabel(IssuePriority priority);

// ============================================================================
// Issue Type Enum
// ============================================================================
enum class IssueType {
    BUG,
    TASK,
    STORY,
    EPIC,
    SUBTASK,
    INCIDENT,
    RFC,        // Request for Change
    ADR         // Architecture Decision Record
};

// ============================================================================
// Issue Reference
// ============================================================================
struct IssueReference {
    std::string provider;        // "jira", "github", "gitlab"
    std::string issue_id;        // Provider-specific ID
    std::string display_key;     // "PROJ-123", "#456"
    std::string url;             // Direct link
    std::string title;
    IssueStatus status = IssueStatus::OPEN;
    IssuePriority priority = IssuePriority::MEDIUM;
    IssueType type = IssueType::TASK;
    std::vector<std::string> labels;
    std::string assignee;
    std::string reporter;
    std::time_t created_at = 0;
    std::time_t updated_at = 0;
    std::time_t resolved_at = 0;
};

// ============================================================================
// Object Reference (Database Object)
// ============================================================================
enum class ObjectType {
    DATABASE,
    SCHEMA,
    TABLE,
    VIEW,
    PROCEDURE,
    FUNCTION,
    TRIGGER,
    INDEX,
    SEQUENCE,
    DOMAIN,
    COLUMN,
    CONSTRAINT,
    MIGRATION,
    DEPLOYMENT,
    UNKNOWN
};

struct ObjectReference {
    ObjectType type = ObjectType::UNKNOWN;
    std::string database;
    std::string schema;
    std::string name;
    std::string qualified_name;  // "database.schema.name"
    std::string version_hash;
    std::time_t modified_at = 0;
    
    std::string ToString() const;
    static ObjectReference FromString(const std::string& str);
};

// ============================================================================
// Link Type
// ============================================================================
enum class LinkType {
    MANUAL,           // User manually created link
    AUTO_CREATED,     // Issue auto-created from event
    AUTO_DETECTED,    // Link detected automatically
    MIGRATION_LINK,   // Linked via migration
    DESIGN_LINK       // Linked via whiteboard/design
};

enum class SyncDirection {
    TO_ISSUE,         // Push changes to issue tracker only
    FROM_ISSUE,       // Pull changes from issue tracker only
    BIDIRECTIONAL     // Sync both ways
};

// ============================================================================
// Issue Link
// ============================================================================
struct IssueLink {
    std::string link_id;
    ObjectReference object;
    IssueReference issue;
    LinkType type = LinkType::MANUAL;
    SyncDirection sync_direction = SyncDirection::BIDIRECTIONAL;
    std::string created_by;
    std::time_t created_at = 0;
    std::time_t last_synced_at = 0;
    bool is_sync_enabled = true;
};

// ============================================================================
// Comment
// ============================================================================
struct IssueComment {
    std::string comment_id;
    std::string issue_id;
    std::string author;
    std::string text;
    std::time_t created_at = 0;
    std::time_t updated_at = 0;
    bool is_internal = false;  // Internal note vs public comment
};

// ============================================================================
// Attachment
// ============================================================================
struct IssueAttachment {
    std::string attachment_id;
    std::string issue_id;
    std::string filename;
    std::string content_type;
    size_t size_bytes = 0;
    std::string url;
    std::string description;
    std::time_t uploaded_at = 0;
};

// ============================================================================
// Tracker Configuration
// ============================================================================
struct TrackerConfig {
    std::string name;
    std::string type;  // "jira", "github", "gitlab"
    bool enabled = true;
    
    // Connection
    struct Auth {
        std::string type;  // "api_token", "oauth", "personal_token"
        std::string email;  // For Jira
        std::string token;
        std::string client_id;     // For OAuth
        std::string client_secret; // For OAuth
    };
    Auth auth;
    
    // Provider-specific settings
    std::string base_url;        // Jira: https://company.atlassian.net
    std::string owner;           // GitHub: owner name
    std::string repo;            // GitHub: repo name
    std::string project_key;     // Jira: project key
    int project_id = 0;          // GitLab: project ID
    
    // Defaults
    std::string default_issue_type = "Task";
    std::vector<std::string> default_labels;
    
    // Sync settings
    struct SyncSettings {
        bool enabled = true;
        int poll_interval_minutes = 5;
        std::string webhook_secret;
        bool sync_status = true;
        bool sync_comments = false;
        bool sync_assignee = true;
    };
    SyncSettings sync;
};

// ============================================================================
// Issue Create/Update Requests
// ============================================================================
struct IssueCreateRequest {
    std::string title;
    std::string description;
    IssueType type = IssueType::TASK;
    IssuePriority priority = IssuePriority::MEDIUM;
    std::vector<std::string> labels;
    std::string assignee;
    std::vector<std::string> attachments;  // File paths
    std::map<std::string, std::string> custom_fields;
    
    // Optional linked object
    std::optional<ObjectReference> linked_object;
};

struct IssueUpdateRequest {
    std::optional<std::string> title;
    std::optional<std::string> description;
    std::optional<IssueStatus> status;
    std::optional<IssuePriority> priority;
    std::optional<std::vector<std::string>> labels;
    std::optional<std::string> assignee;
};

struct SearchQuery {
    std::string text;
    std::vector<IssueStatus> status_filter;
    std::vector<std::string> label_filter;
    std::string assignee_filter;
    std::time_t created_after = 0;
    std::time_t created_before = 0;
    int limit = 50;
    int offset = 0;
};

// ============================================================================
// Issue Tracker Adapter Interface
// ============================================================================
class IssueTrackerAdapter {
public:
    virtual ~IssueTrackerAdapter() = default;
    
    // Lifecycle
    virtual bool Initialize(const TrackerConfig& config) = 0;
    virtual bool TestConnection() = 0;
    virtual std::string GetProviderName() const = 0;
    
    // Issue CRUD
    virtual IssueReference CreateIssue(const IssueCreateRequest& request) = 0;
    virtual bool UpdateIssue(const std::string& issue_id, 
                              const IssueUpdateRequest& request) = 0;
    virtual bool DeleteIssue(const std::string& issue_id) = 0;
    virtual std::optional<IssueReference> GetIssue(const std::string& issue_id) = 0;
    
    // Search/Query
    virtual std::vector<IssueReference> SearchIssues(const SearchQuery& query) = 0;
    virtual std::vector<IssueReference> GetRecentIssues(int count = 10) = 0;
    virtual std::vector<IssueReference> GetIssuesByLabel(const std::string& label) = 0;
    
    // Comments
    virtual IssueComment AddComment(const std::string& issue_id, 
                                     const std::string& text) = 0;
    virtual std::vector<IssueComment> GetComments(const std::string& issue_id) = 0;
    
    // Attachments
    virtual IssueAttachment AttachFile(const std::string& issue_id,
                                        const std::string& file_path,
                                        const std::string& description) = 0;
    
    // Metadata
    virtual std::vector<std::string> GetLabels() = 0;
    virtual std::vector<std::string> GetIssueTypes() = 0;
    virtual std::vector<std::string> GetUsers() = 0;
    
    // Webhook management
    struct WebhookConfig {
        std::string url;
        std::vector<std::string> events;  // "issue.created", "issue.updated"
        std::string secret;
    };
    virtual std::string RegisterWebhook(const WebhookConfig& config) = 0;
    virtual bool UnregisterWebhook(const std::string& webhook_id) = 0;
};

// ============================================================================
// Issue Link Manager
// ============================================================================
class IssueLinkManager {
public:
    static IssueLinkManager& Instance();
    
    // Provider management
    void RegisterAdapter(const std::string& name,
                         std::function<std::unique_ptr<IssueTrackerAdapter>()> factory);
    bool AddTracker(const TrackerConfig& config);
    bool RemoveTracker(const std::string& name);
    IssueTrackerAdapter* GetTracker(const std::string& name);
    std::vector<std::string> GetTrackerNames() const;
    
    // Link management
    bool LinkObject(const ObjectReference& obj, const IssueReference& issue, 
                     LinkType type = LinkType::MANUAL);
    bool UnlinkObject(const ObjectReference& obj, const std::string& issue_id);
    bool UnlinkIssue(const std::string& issue_id);
    
    std::vector<IssueLink> GetLinkedIssues(const ObjectReference& obj);
    std::vector<IssueLink> GetLinkedObjects(const std::string& issue_id);
    std::optional<IssueLink> GetLink(const ObjectReference& obj, 
                                      const std::string& issue_id);
    
    // Sync
    bool SyncLink(const std::string& link_id);
    bool SyncAllLinks(const std::string& tracker_name);
    
    // Auto-creation
    struct AutoIssueContext {
        std::string event_type;       // "schema_change", "drift", "performance"
        std::string severity;         // "low", "medium", "high", "critical"
        std::map<std::string, std::string> metadata;
        ObjectReference source_object;
    };
    IssueReference AutoCreateIssue(const AutoIssueContext& context,
                                    const std::string& tracker_name);
    
    // Storage
    void SaveLinks(const std::string& file_path);
    void LoadLinks(const std::string& file_path);
    
private:
    IssueLinkManager() = default;
    ~IssueLinkManager() = default;
    
    std::map<std::string, std::function<std::unique_ptr<IssueTrackerAdapter>()>> adapter_factories_;
    std::map<std::string, std::unique_ptr<IssueTrackerAdapter>> trackers_;
    std::map<std::string, IssueLink> links_;  // link_id -> link
    
    std::string GenerateLinkId();
};

// ============================================================================
// Context Generator for Auto-Created Issues
// ============================================================================
class IssueContextGenerator {
public:
    struct GeneratedContent {
        std::string title;
        std::string description;
        std::vector<std::string> suggested_labels;
        IssuePriority suggested_priority = IssuePriority::MEDIUM;
        IssueType suggested_type = IssueType::TASK;
    };
    
    // Schema change context
    struct SchemaChangeContext {
        std::string ddl_sql;
        std::string change_type;  // "CREATE", "ALTER", "DROP"
        ObjectReference target_object;
        std::vector<ObjectReference> dependent_objects;
        std::string impact_summary;
        int estimated_downtime_minutes = 0;
    };
    GeneratedContent GenerateForSchemaChange(const SchemaChangeContext& context);
    
    // Performance regression context
    struct PerformanceContext {
        std::string query_fingerprint;
        double before_duration_ms = 0;
        double after_duration_ms = 0;
        int64_t before_rows = 0;
        int64_t after_rows = 0;
        std::string execution_plan_diff;
    };
    GeneratedContent GenerateForPerformanceRegression(const PerformanceContext& context);
    
    // Drift detection context
    struct DriftContext {
        std::string environment;
        std::string expected_schema;
        std::string actual_schema;
        std::vector<std::string> differences;
        std::time_t detected_at = 0;
    };
    GeneratedContent GenerateForDrift(const DriftContext& context);
    
    // Health check failure context
    struct HealthCheckContext {
        std::string check_name;
        std::string check_category;
        std::string failure_reason;
        std::map<std::string, std::string> metrics;
        std::string recommended_action;
    };
    GeneratedContent GenerateForHealthCheck(const HealthCheckContext& context);
};

// ============================================================================
// Issue Templates
// ============================================================================
class IssueTemplateManager {
public:
    static IssueTemplateManager& Instance();
    
    struct Template {
        std::string id;
        std::string name;
        std::string description;
        std::string title_template;
        std::string body_template;  // Markdown with {{variable}} placeholders
        std::vector<std::string> default_labels;
        IssuePriority default_priority = IssuePriority::MEDIUM;
        IssueType default_type = IssueType::TASK;
    };
    
    void RegisterTemplate(const Template& tmpl);
    std::optional<Template> GetTemplate(const std::string& id);
    std::vector<Template> GetAllTemplates();
    
    // Template rendering
    std::string RenderTitle(const Template& tmpl, 
                            const std::map<std::string, std::string>& vars);
    std::string RenderBody(const Template& tmpl,
                           const std::map<std::string, std::string>& vars);
    
private:
    IssueTemplateManager();
    std::map<std::string, Template> templates_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_ISSUE_TRACKER_H
