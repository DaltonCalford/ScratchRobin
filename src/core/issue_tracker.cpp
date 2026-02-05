/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "issue_tracker.h"
#include "issue_tracker_jira.h"
#include "issue_tracker_github.h"
#include "issue_tracker_gitlab.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// Status Helpers
// ============================================================================
std::string IssueStatusToString(IssueStatus status) {
    switch (status) {
        case IssueStatus::OPEN: return "Open";
        case IssueStatus::IN_PROGRESS: return "In Progress";
        case IssueStatus::RESOLVED: return "Resolved";
        case IssueStatus::CLOSED: return "Closed";
        case IssueStatus::REOPENED: return "Reopened";
        case IssueStatus::PENDING_APPROVAL: return "Pending Approval";
        case IssueStatus::BLOCKED: return "Blocked";
        default: return "Unknown";
    }
}

IssueStatus IssueStatusFromString(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "open" || lower == "opened" || lower == "to do") {
        return IssueStatus::OPEN;
    } else if (lower == "in progress" || lower == "in-progress") {
        return IssueStatus::IN_PROGRESS;
    } else if (lower == "resolved" || lower == "done") {
        return IssueStatus::RESOLVED;
    } else if (lower == "closed" || lower == "close") {
        return IssueStatus::CLOSED;
    } else if (lower == "reopened") {
        return IssueStatus::REOPENED;
    } else if (lower == "pending" || lower == "pending approval") {
        return IssueStatus::PENDING_APPROVAL;
    } else if (lower == "blocked" || lower == "impeded") {
        return IssueStatus::BLOCKED;
    }
    
    return IssueStatus::OPEN;
}

std::string IssuePriorityToString(IssuePriority priority) {
    switch (priority) {
        case IssuePriority::CRITICAL: return "Critical";
        case IssuePriority::HIGH: return "High";
        case IssuePriority::MEDIUM: return "Medium";
        case IssuePriority::LOW: return "Low";
        case IssuePriority::TRIVIAL: return "Trivial";
        default: return "Unknown";
    }
}

std::string IssuePriorityToLabel(IssuePriority priority) {
    switch (priority) {
        case IssuePriority::CRITICAL: return "P0";
        case IssuePriority::HIGH: return "P1";
        case IssuePriority::MEDIUM: return "P2";
        case IssuePriority::LOW: return "P3";
        case IssuePriority::TRIVIAL: return "P4";
        default: return "";
    }
}

// ============================================================================
// Object Reference
// ============================================================================
std::string ObjectReference::ToString() const {
    std::ostringstream oss;
    oss << static_cast<int>(type) << ":";
    if (!database.empty()) oss << database << ".";
    if (!schema.empty()) oss << schema << ".";
    oss << name;
    return oss.str();
}

ObjectReference ObjectReference::FromString(const std::string& str) {
    ObjectReference ref;
    // Parse format: type:database.schema.name or type:schema.name or type:name
    size_t type_sep = str.find(':');
    if (type_sep != std::string::npos) {
        ref.type = static_cast<ObjectType>(std::stoi(str.substr(0, type_sep)));
        std::string rest = str.substr(type_sep + 1);
        
        size_t last_dot = rest.rfind('.');
        if (last_dot != std::string::npos) {
            ref.name = rest.substr(last_dot + 1);
            std::string prefix = rest.substr(0, last_dot);
            
            size_t first_dot = prefix.find('.');
            if (first_dot != std::string::npos) {
                ref.database = prefix.substr(0, first_dot);
                ref.schema = prefix.substr(first_dot + 1);
            } else {
                ref.schema = prefix;
            }
        } else {
            ref.name = rest;
        }
        
        ref.qualified_name = rest;
    }
    return ref;
}

// ============================================================================
// Issue Link Manager
// ============================================================================
IssueLinkManager& IssueLinkManager::Instance() {
    static IssueLinkManager instance;
    return instance;
}

void IssueLinkManager::RegisterAdapter(const std::string& name,
                                        std::function<std::unique_ptr<IssueTrackerAdapter>()> factory) {
    adapter_factories_[name] = factory;
}

bool IssueLinkManager::AddTracker(const TrackerConfig& config) {
    auto it = adapter_factories_.find(config.type);
    if (it == adapter_factories_.end()) {
        return false;
    }
    
    auto tracker = it->second();
    if (!tracker->Initialize(config)) {
        return false;
    }
    
    trackers_[config.name] = std::move(tracker);
    return true;
}

bool IssueLinkManager::RemoveTracker(const std::string& name) {
    return trackers_.erase(name) > 0;
}

IssueTrackerAdapter* IssueLinkManager::GetTracker(const std::string& name) {
    auto it = trackers_.find(name);
    if (it != trackers_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> IssueLinkManager::GetTrackerNames() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : trackers_) {
        names.push_back(name);
    }
    return names;
}

bool IssueLinkManager::LinkObject(const ObjectReference& obj, const IssueReference& issue,
                                   LinkType type) {
    IssueLink link;
    link.link_id = GenerateLinkId();
    link.object = obj;
    link.issue = issue;
    link.type = type;
    link.created_at = std::time(nullptr);
    link.is_sync_enabled = true;
    
    links_[link.link_id] = link;
    return true;
}

bool IssueLinkManager::CreateLink(const ObjectReference& obj, const IssueReference& issue) {
    return LinkObject(obj, issue, LinkType::MANUAL);
}

IssueTrackerAdapter* IssueLinkManager::GetAdapter(const std::string& name) {
    return GetTracker(name);
}

bool IssueLinkManager::UnlinkObject(const ObjectReference& obj, const std::string& issue_id) {
    for (auto it = links_.begin(); it != links_.end(); ++it) {
        if (it->second.object.qualified_name == obj.qualified_name &&
            it->second.issue.issue_id == issue_id) {
            links_.erase(it);
            return true;
        }
    }
    return false;
}

bool IssueLinkManager::UnlinkIssue(const std::string& issue_id) {
    for (auto it = links_.begin(); it != links_.end(); ++it) {
        if (it->second.issue.issue_id == issue_id) {
            links_.erase(it);
            return true;
        }
    }
    return false;
}

std::vector<IssueLink> IssueLinkManager::GetLinkedIssues(const ObjectReference& obj) {
    std::vector<IssueLink> result;
    for (const auto& [_, link] : links_) {
        if (link.object.qualified_name == obj.qualified_name) {
            result.push_back(link);
        }
    }
    return result;
}

std::vector<IssueLink> IssueLinkManager::GetLinkedObjects(const std::string& issue_id) {
    std::vector<IssueLink> result;
    for (const auto& [_, link] : links_) {
        if (link.issue.issue_id == issue_id) {
            result.push_back(link);
        }
    }
    return result;
}

std::optional<IssueLink> IssueLinkManager::GetLink(const ObjectReference& obj,
                                                    const std::string& issue_id) {
    for (const auto& [_, link] : links_) {
        if (link.object.qualified_name == obj.qualified_name &&
            link.issue.issue_id == issue_id) {
            return link;
        }
    }
    return std::nullopt;
}

bool IssueLinkManager::SyncLink(const std::string& link_id) {
    auto it = links_.find(link_id);
    if (it == links_.end()) {
        return false;
    }
    
    auto& link = it->second;
    if (!link.is_sync_enabled) {
        return false;
    }
    
    // Get fresh issue data from tracker
    auto* tracker = GetTracker(link.issue.provider);
    if (!tracker) {
        return false;
    }
    
    auto fresh = tracker->GetIssue(link.issue.issue_id);
    if (fresh) {
        link.issue = *fresh;
        link.last_synced_at = std::time(nullptr);
        return true;
    }
    
    return false;
}

bool IssueLinkManager::SyncAllLinks(const std::string& tracker_name) {
    bool all_success = true;
    for (auto& [_, link] : links_) {
        if (link.issue.provider == tracker_name && link.is_sync_enabled) {
            if (!SyncLink(link.link_id)) {
                all_success = false;
            }
        }
    }
    return all_success;
}

IssueReference IssueLinkManager::AutoCreateIssue(const AutoIssueContext& context,
                                                  const std::string& tracker_name) {
    IssueReference issue;
    
    auto* tracker = GetTracker(tracker_name);
    if (!tracker) {
        return issue;
    }
    
    // Build issue from template based on event type
    IssueCreateRequest request;
    request.title = "Auto: " + context.event_type + " - " + context.source_object.name;
    request.description = "Automatically created for " + context.event_type;
    request.linked_object = context.source_object;
    
    // Set priority based on severity
    if (context.severity == "critical") {
        request.priority = IssuePriority::CRITICAL;
    } else if (context.severity == "high") {
        request.priority = IssuePriority::HIGH;
    } else if (context.severity == "medium") {
        request.priority = IssuePriority::MEDIUM;
    } else {
        request.priority = IssuePriority::LOW;
    }
    
    return tracker->CreateIssue(request);
}

void IssueLinkManager::SaveLinks(const std::string& file_path) {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return;
    }
    
    file << "{\n";
    file << "  \"version\": 1,\n";
    file << "  \"links\": [\n";
    
    bool first = true;
    for (const auto& [link_id, link] : links_) {
        if (!first) file << ",\n";
        first = false;
        
        file << "    {\n";
        file << "      \"link_id\": \"" << link.link_id << "\",\n";
        file << "      \"created_at\": " << link.created_at << ",\n";
        file << "      \"provider\": \"" << link.issue.provider << "\",\n";
        file << "      \"issue_id\": \"" << link.issue.issue_id << "\",\n";
        file << "      \"issue_key\": \"" << link.issue.display_key << "\",\n";
        file << "      \"issue_title\": \"" << link.issue.title << "\",\n";
        file << "      \"object_type\": " << static_cast<int>(link.object.type) << ",\n";
        file << "      \"object_schema\": \"" << link.object.schema << "\",\n";
        file << "      \"object_name\": \"" << link.object.name << "\",\n";
        file << "      \"object_database\": \"" << link.object.database << "\"\n";
        file << "    }";
    }
    
    file << "\n  ]\n";
    file << "}\n";
}

void IssueLinkManager::LoadLinks(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return;
    }
    
    // Clear existing links
    links_.clear();
    
    // Simple JSON parsing - in production would use proper JSON parser
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    
    // Find links array
    size_t links_start = content.find("\"links\":");
    if (links_start == std::string::npos) return;
    
    size_t arr_start = content.find("[", links_start);
    if (arr_start == std::string::npos) return;
    
    // Parse each link object (simplified)
    size_t pos = arr_start + 1;
    while ((pos = content.find("{", pos)) != std::string::npos) {
        size_t obj_end = content.find("}", pos);
        if (obj_end == std::string::npos) break;
        
        std::string obj_str = content.substr(pos, obj_end - pos + 1);
        
        IssueLink link;
        
        // Extract fields
        auto extract_string = [&obj_str](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\": \"";
            size_t start = obj_str.find(search);
            if (start == std::string::npos) return "";
            start += search.length();
            size_t end = obj_str.find("\"", start);
            if (end == std::string::npos) return "";
            return obj_str.substr(start, end - start);
        };
        
        link.link_id = extract_string("link_id");
        link.issue.provider = extract_string("provider");
        link.issue.issue_id = extract_string("issue_id");
        link.issue.display_key = extract_string("issue_key");
        link.issue.title = extract_string("issue_title");
        link.object.schema = extract_string("object_schema");
        link.object.name = extract_string("object_name");
        link.object.database = extract_string("object_database");
        
        if (!link.link_id.empty()) {
            links_[link.link_id] = link;
        }
        
        pos = obj_end + 1;
    }
}

std::string IssueLinkManager::GenerateLinkId() {
    static int counter = 0;
    return "link_" + std::to_string(++counter) + "_" + std::to_string(std::time(nullptr));
}

// ============================================================================
// Context Generator
// ============================================================================
IssueContextGenerator::GeneratedContent IssueContextGenerator::GenerateForSchemaChange(
    const SchemaChangeContext& context) {
    GeneratedContent content;
    
    content.title = "Schema Change: " + context.change_type + " " + context.target_object.name;
    
    std::ostringstream oss;
    oss << "## Schema Change Request\n\n";
    oss << "**Object:** " << context.target_object.qualified_name << "\n";
    oss << "**Type:** " << context.change_type << "\n\n";
    
    oss << "### DDL\n```sql\n" << context.ddl_sql << "\n```\n\n";
    
    oss << "### Impact\n" << context.impact_summary << "\n\n";
    
    if (!context.dependent_objects.empty()) {
        oss << "### Dependencies\n";
        for (const auto& dep : context.dependent_objects) {
            oss << "- " << dep.qualified_name << "\n";
        }
        oss << "\n";
    }
    
    if (context.estimated_downtime_minutes > 0) {
        oss << "### Downtime\nEstimated: " << context.estimated_downtime_minutes << " minutes\n\n";
    }
    
    oss << "### Checklist\n";
    oss << "- [ ] Impact reviewed\n";
    oss << "- [ ] Rollback plan prepared\n";
    oss << "- [ ] Stakeholder approval obtained\n";
    
    content.description = oss.str();
    content.suggested_labels = {"schema-change", "database"};
    content.suggested_type = IssueType::RFC;
    
    return content;
}

IssueContextGenerator::GeneratedContent IssueContextGenerator::GenerateForPerformanceRegression(
    const PerformanceContext& context) {
    GeneratedContent content;
    
    content.title = "Performance Regression: " + context.query_fingerprint.substr(0, 50);
    
    std::ostringstream oss;
    oss << "## Performance Regression Detected\n\n";
    oss << "**Query:** `" << context.query_fingerprint << "`\n\n";
    
    oss << "### Before\n";
    oss << "- Execution Time: " << context.before_duration_ms << "ms\n";
    oss << "- Rows: " << context.before_rows << "\n\n";
    
    oss << "### After\n";
    oss << "- Execution Time: " << context.after_duration_ms << "ms\n";
    oss << "- Rows: " << context.after_rows << "\n\n";
    
    double regression_pct = ((context.after_duration_ms - context.before_duration_ms) / 
                             context.before_duration_ms) * 100.0;
    oss << "### Regression\n";
    oss << "- Increase: " << regression_pct << "%\n\n";
    
    if (!context.execution_plan_diff.empty()) {
        oss << "### Plan Changes\n```\n" << context.execution_plan_diff << "\n```\n";
    }
    
    content.description = oss.str();
    content.suggested_labels = {"performance", "regression"};
    content.suggested_priority = IssuePriority::HIGH;
    content.suggested_type = IssueType::BUG;
    
    return content;
}

IssueContextGenerator::GeneratedContent IssueContextGenerator::GenerateForDrift(
    const DriftContext& context) {
    GeneratedContent content;
    
    content.title = "Schema Drift: " + context.environment;
    content.suggested_priority = IssuePriority::CRITICAL;
    
    std::ostringstream oss;
    oss << "## ⚠️ Schema Drift Detected\n\n";
    oss << "**Environment:** " << context.environment << "\n";
    oss << "**Detected:** " << ctime(&context.detected_at) << "\n\n";
    
    oss << "### Differences\n";
    for (const auto& diff : context.differences) {
        oss << "- " << diff << "\n";
    }
    
    content.description = oss.str();
    content.suggested_labels = {"drift", "database", "urgent"};
    content.suggested_type = IssueType::INCIDENT;
    
    return content;
}

IssueContextGenerator::GeneratedContent IssueContextGenerator::GenerateForHealthCheck(
    const HealthCheckContext& context) {
    GeneratedContent content;
    
    content.title = "Health Check Failed: " + context.check_name;
    
    std::ostringstream oss;
    oss << "## Health Check Failure\n\n";
    oss << "**Check:** " << context.check_name << "\n";
    oss << "**Category:** " << context.check_category << "\n";
    oss << "**Reason:** " << context.failure_reason << "\n\n";
    
    if (!context.metrics.empty()) {
        oss << "### Metrics\n";
        for (const auto& [key, value] : context.metrics) {
            oss << "- " << key << ": " << value << "\n";
        }
        oss << "\n";
    }
    
    if (!context.recommended_action.empty()) {
        oss << "### Recommended Action\n" << context.recommended_action << "\n";
    }
    
    content.description = oss.str();
    content.suggested_labels = {"health-check", context.check_category};
    content.suggested_type = IssueType::INCIDENT;
    
    return content;
}

// ============================================================================
// Issue Template Manager
// ============================================================================
IssueTemplateManager& IssueTemplateManager::Instance() {
    static IssueTemplateManager instance;
    return instance;
}

IssueTemplateManager::IssueTemplateManager() {
    // Register default templates
    
    Template schema_change;
    schema_change.id = "schema_change";
    schema_change.name = "Schema Change";
    schema_change.description = "Template for schema modification requests";
    schema_change.title_template = "Schema Change: {{change_type}} {{object_name}}";
    schema_change.body_template = R"(## Schema Change Request

**Object:** {{object_qualified_name}}
**Type:** {{change_type}}

### DDL
```sql
{{ddl_sql}}
```

### Impact Analysis
- Tables Affected: {{table_count}}
- Estimated Downtime: {{downtime_minutes}} minutes

### Checklist
- [ ] Impact reviewed
- [ ] Rollback plan verified
- [ ] Stakeholder approval obtained
)";
    schema_change.default_labels = {"schema-change", "database"};
    schema_change.default_priority = IssuePriority::MEDIUM;
    schema_change.default_type = IssueType::RFC;
    RegisterTemplate(schema_change);
    
    Template performance;
    performance.id = "performance_regression";
    performance.name = "Performance Regression";
    performance.description = "Template for performance issues";
    performance.title_template = "Performance Issue: {{query_summary}}";
    performance.body_template = R"(## Performance Issue

**Query:** {{query_fingerprint}}

### Before
- Execution Time: {{before_time_ms}}ms

### After
- Execution Time: {{after_time_ms}}ms

### Regression: {{regression_pct}}%
)";
    performance.default_labels = {"performance", "regression"};
    performance.default_priority = IssuePriority::HIGH;
    performance.default_type = IssueType::BUG;
    RegisterTemplate(performance);
    
    Template drift;
    drift.id = "drift_detection";
    drift.name = "Schema Drift";
    drift.description = "Template for schema drift alerts";
    drift.title_template = "Schema Drift: {{environment}}";
    drift.body_template = R"(## ⚠️ Schema Drift Detected

**Environment:** {{environment}}

### Differences
{{differences}}

### Resolution
1. Review changes in {{environment}}
2. Apply to source or revert changes
)";
    drift.default_labels = {"drift", "database", "urgent"};
    drift.default_priority = IssuePriority::CRITICAL;
    drift.default_type = IssueType::INCIDENT;
    RegisterTemplate(drift);
}

void IssueTemplateManager::RegisterTemplate(const Template& tmpl) {
    templates_[tmpl.id] = tmpl;
}

std::optional<IssueTemplateManager::Template> IssueTemplateManager::GetTemplate(
    const std::string& id) {
    auto it = templates_.find(id);
    if (it != templates_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<IssueTemplateManager::Template> IssueTemplateManager::GetAllTemplates() {
    std::vector<Template> result;
    for (const auto& [_, tmpl] : templates_) {
        result.push_back(tmpl);
    }
    return result;
}

std::string IssueTemplateManager::RenderTitle(const Template& tmpl,
                                               const std::map<std::string, std::string>& vars) {
    std::string result = tmpl.title_template;
    for (const auto& [key, value] : vars) {
        std::string placeholder = "{{" + key + "}}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
    return result;
}

std::string IssueTemplateManager::RenderBody(const Template& tmpl,
                                              const std::map<std::string, std::string>& vars) {
    std::string result = tmpl.body_template;
    for (const auto& [key, value] : vars) {
        std::string placeholder = "{{" + key + "}}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
    return result;
}

} // namespace scratchrobin
