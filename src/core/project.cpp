/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "project.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "core/simple_json.h"
#include "project_serialization.h"

#include "git_client.h"

namespace scratchrobin {

namespace {

std::vector<std::string> SplitPath(const std::string& path, char delim) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == delim) {
            if (!current.empty()) parts.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) parts.push_back(current);
    return parts;
}

std::string EnsureTrailingSlash(const std::string& path) {
    if (path.empty()) return path;
    if (path.back() == '/' || path.back() == '\\') return path;
    return path + "/";
}

std::shared_ptr<ProjectObject> FindObjectByDesignPath(Project* project,
                                                      const std::string& design_path) {
    for (const auto& pair : project->objects_by_id) {
        const auto& obj = pair.second;
        if (obj && obj->design_file_path == design_path) {
            return obj;
        }
    }
    return nullptr;
}

void UpdateSyncStateFromGit(Project* project,
                            ProjectGitManager& git,
                            const std::string& designs_path) {
    auto status = git.GetRepositoryStatus();
    project->sync_state.last_sync = std::time(nullptr);
    project->sync_state.project_repo.branch = status.currentBranch;
    project->sync_state.project_repo.head_commit = status.currentCommit.value_or("");
    project->sync_state.project_repo.dirty_files.clear();

    auto changed = git.GetChangedFilesInPath(EnsureTrailingSlash(designs_path));
    for (const auto& file : changed) {
        project->sync_state.project_repo.dirty_files.push_back(file.path);
    }

    project->sync_state.pending.project_to_db.clear();
    for (const auto& file : changed) {
        project->sync_state.pending.project_to_db.push_back(file.path);
    }
}

bool MatchesValue(const std::string& value, const std::string& pattern) {
    if (pattern.empty()) return false;
    return value.find(pattern) != std::string::npos;
}

bool MatchesPatterns(const MetadataNode& node, const std::vector<std::string>& patterns) {
    if (patterns.empty()) return true;

    std::string schema;
    std::string derived_path = DeriveObjectPath(node, &schema);
    const std::string name = node.label.empty() ? node.name : node.label;
    const std::string kind = node.kind.empty() ? node.name : node.kind;
    const std::string catalog = node.catalog;

    for (const auto& p : patterns) {
        if (p.empty()) continue;

        if (p.rfind("kind:", 0) == 0) {
            if (kind == p.substr(5)) return true;
            continue;
        }
        if (p.rfind("type:", 0) == 0) {
            if (kind == p.substr(5)) return true;
            continue;
        }
        if (p.rfind("schema:", 0) == 0) {
            if (schema == p.substr(7)) return true;
            continue;
        }
        if (p.rfind("catalog:", 0) == 0) {
            if (catalog == p.substr(8)) return true;
            continue;
        }
        if (p.rfind("table:", 0) == 0) {
            if (kind == "table" && name == p.substr(6)) return true;
            continue;
        }
        if (p.rfind("view:", 0) == 0) {
            if (kind == "view" && name == p.substr(5)) return true;
            continue;
        }
        if (p.rfind("procedure:", 0) == 0) {
            if (kind == "procedure" && name == p.substr(10)) return true;
            continue;
        }
        if (p.rfind("trigger:", 0) == 0) {
            if (kind == "trigger" && name == p.substr(8)) return true;
            continue;
        }

        if (MatchesValue(node.path, p) ||
            MatchesValue(name, p) ||
            MatchesValue(node.name, p) ||
            MatchesValue(derived_path, p) ||
            MatchesValue(schema, p)) {
            return true;
        }
    }
    return false;
}

bool IsFixtureConnection(const DatabaseConnection& conn, std::string* fixture_path) {
    const std::string& cs = conn.connection_string;
    const std::string prefix1 = "fixture:";
    const std::string prefix2 = "file:";
    if (cs.rfind(prefix1, 0) == 0) {
        *fixture_path = cs.substr(prefix1.size());
        return true;
    }
    if (cs.rfind(prefix2, 0) == 0) {
        *fixture_path = cs.substr(prefix2.size());
        return true;
    }
    return false;
}

std::string DeriveObjectPath(const MetadataNode& node, std::string* out_schema) {
    if (out_schema) out_schema->clear();
    if (!node.path.empty()) {
        auto parts = SplitPath(node.path, '.');
        if (parts.size() >= 2 && out_schema) {
            *out_schema = parts[1];
        }
        if (node.kind == "schema" && parts.size() >= 2) {
            return parts[1];
        }
        if (node.kind == "column" && parts.size() >= 4) {
            return parts[1] + "." + parts[2] + "." + parts[3];
        }
        if (parts.size() >= 3) {
            return parts[1] + "." + parts[2];
        }
    }
    return node.label.empty() ? node.name : node.label;
}

void AddObjectFromNode(Project* project, const MetadataNode& node) {
    const std::string kind = node.kind.empty() ? node.name : node.kind;
    if (kind.empty()) return;
    if (kind == "folder" || kind == "note") return;
    if (!IsSupportedKind(kind)) return;

    std::string schema;
    std::string path = DeriveObjectPath(node, &schema);
    std::string name = node.label.empty() ? node.name : node.label;
    if (name.empty() && !path.empty()) {
        auto parts = SplitPath(path, '.');
        if (!parts.empty()) name = parts.back();
    }

    auto obj = std::make_shared<ProjectObject>(kind, name);
    obj->schema_name = schema;
    obj->path = path;
    obj->design_file_path = project->config.designs_path + "/" + obj->path + "." + kind + ".json";

    obj->has_source = true;
    obj->source_snapshot = node;
    obj->current_design = node;
    obj->design_state.state = ObjectState::EXTRACTED;
    obj->design_state.changed_by = "system";
    obj->design_state.changed_at = std::time(nullptr);

    project->objects_by_id[obj->id] = obj;
    if (!obj->path.empty()) {
        project->objects_by_path[obj->path] = obj;
    }
}

bool IsSupportedKind(const std::string& kind) {
    static const std::vector<std::string> allowed = {
        "schema", "table", "view", "procedure", "function", "trigger",
        "index", "column", "constraint", "sequence", "domain", "type"
    };
    return std::find(allowed.begin(), allowed.end(), kind) != allowed.end();
}

} // namespace

// ============================================================================
// UUID Implementation
// ============================================================================

UUID UUID::Generate() {
    UUID uuid;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (int i = 0; i < 16; ++i) {
        uuid.data[i] = static_cast<char>(dis(gen));
    }
    
    // Set version (4) and variant bits
    uuid.data[6] = (uuid.data[6] & 0x0F) | 0x40;
    uuid.data[8] = (uuid.data[8] & 0x3F) | 0x80;
    
    return uuid;
}

std::string UUID::ToString() const {
    std::ostringstream oss;
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            oss << '-';
        }
        oss << std::hex << std::setw(2) << std::setfill('0')
            << (static_cast<unsigned char>(data[i]) & 0xFF);
    }
    return oss.str();
}

UUID UUID::FromString(const std::string& str) {
    UUID uuid;
    std::string hex = str;
    hex.erase(std::remove(hex.begin(), hex.end(), '-'), hex.end());
    
    if (hex.length() != 32) {
        return UUID();  // Invalid
    }
    
    for (int i = 0; i < 16; ++i) {
        std::string byte_str = hex.substr(i * 2, 2);
        uuid.data[i] = static_cast<char>(std::stoi(byte_str, nullptr, 16));
    }
    
    return uuid;
}

bool UUID::IsValid() const {
    // Simple check - not all zeros
    for (int i = 0; i < 16; ++i) {
        if (data[i] != 0) return true;
    }
    return false;
}

bool UUID::operator==(const UUID& other) const {
    return std::memcmp(data, other.data, 16) == 0;
}

bool UUID::operator<(const UUID& other) const {
    return std::memcmp(data, other.data, 16) < 0;
}

// ============================================================================
// Object State
// ============================================================================

std::string ObjectStateToString(ObjectState state) {
    switch (state) {
        case ObjectState::EXTRACTED: return "extracted";
        case ObjectState::NEW: return "new";
        case ObjectState::MODIFIED: return "modified";
        case ObjectState::DELETED: return "deleted";
        case ObjectState::PENDING: return "pending";
        case ObjectState::APPROVED: return "approved";
        case ObjectState::REJECTED: return "rejected";
        case ObjectState::IMPLEMENTED: return "implemented";
        case ObjectState::CONFLICTED: return "conflicted";
        default: return "unknown";
    }
}

ObjectState ObjectStateFromString(const std::string& str) {
    if (str == "extracted") return ObjectState::EXTRACTED;
    if (str == "new") return ObjectState::NEW;
    if (str == "modified") return ObjectState::MODIFIED;
    if (str == "deleted") return ObjectState::DELETED;
    if (str == "pending") return ObjectState::PENDING;
    if (str == "approved") return ObjectState::APPROVED;
    if (str == "rejected") return ObjectState::REJECTED;
    if (str == "implemented") return ObjectState::IMPLEMENTED;
    if (str == "conflicted") return ObjectState::CONFLICTED;
    return ObjectState::NEW;
}

int GetObjectStateIconIndex(ObjectState state) {
    // Map design states to icon indices
    switch (state) {
        case ObjectState::EXTRACTED: return 10;  // Gray - table icon
        case ObjectState::NEW: return 84;        // Purple - newitem
        case ObjectState::MODIFIED: return 82;   // Orange - modified
        case ObjectState::DELETED: return 83;    // Red - deleted
        case ObjectState::PENDING: return 81;    // Yellow - pending
        case ObjectState::APPROVED: return 80;   // Green - implemented
        case ObjectState::REJECTED: return 3;    // Red - error
        case ObjectState::IMPLEMENTED: return 80; // Blue - implemented
        case ObjectState::CONFLICTED: return 14; // Warning
        default: return 120;  // Default
    }
}

// ============================================================================
// Design State
// ============================================================================

bool DesignState::CanTransitionTo(ObjectState new_state) const {
    auto allowed = GetAllowedTransitions();
    return std::find(allowed.begin(), allowed.end(), new_state) != allowed.end();
}

std::vector<ObjectState> DesignState::GetAllowedTransitions() const {
    switch (state) {
        case ObjectState::EXTRACTED:
            return {ObjectState::MODIFIED, ObjectState::DELETED};
        case ObjectState::NEW:
            return {ObjectState::PENDING, ObjectState::DELETED};
        case ObjectState::MODIFIED:
            return {ObjectState::PENDING, ObjectState::EXTRACTED};  // Revert
        case ObjectState::DELETED:
            return {ObjectState::EXTRACTED, ObjectState::NEW};  // Restore
        case ObjectState::PENDING:
            return {ObjectState::APPROVED, ObjectState::REJECTED, ObjectState::MODIFIED};
        case ObjectState::APPROVED:
            return {ObjectState::IMPLEMENTED, ObjectState::PENDING};  // Back to pending
        case ObjectState::REJECTED:
            return {ObjectState::MODIFIED, ObjectState::PENDING};
        case ObjectState::IMPLEMENTED:
            return {ObjectState::MODIFIED};  // New changes
        case ObjectState::CONFLICTED:
            return {ObjectState::MODIFIED, ObjectState::EXTRACTED};
        default:
            return {};
    }
}

// ============================================================================
// Project Object
// ============================================================================

ProjectObject::ProjectObject() : id(UUID::Generate()) {}

ProjectObject::ProjectObject(const std::string& kind_, const std::string& name_)
    : id(UUID::Generate()), kind(kind_), name(name_) {
    design_state.state = ObjectState::NEW;
    design_state.changed_at = std::time(nullptr);
}

void ProjectObject::SetState(ObjectState new_state, const std::string& reason, 
                             const std::string& user) {
    if (!design_state.CanTransitionTo(new_state)) {
        return;  // Invalid transition
    }
    
    auto old_state = design_state.state;
    design_state.state = new_state;
    design_state.reason = reason;
    design_state.changed_by = user;
    design_state.changed_at = std::time(nullptr);
    
    RecordChange("state", ObjectStateToString(old_state), 
                 ObjectStateToString(new_state), user);
}

void ProjectObject::RecordChange(const std::string& field, const std::string& old_val,
                                 const std::string& new_val, const std::string& user) {
    ChangeRecord change;
    change.field = field;
    change.old_value = old_val;
    change.new_value = new_val;
    change.timestamp = std::time(nullptr);
    change.author = user;
    change_history.push_back(change);
}

void ProjectObject::ToJson(std::ostream& out) const {
    out << "{\n";
    out << "  \"id\": \"" << id.ToString() << "\",\n";
    out << "  \"kind\": \"" << kind << "\",\n";
    out << "  \"name\": \"" << name << "\",\n";
    out << "  \"path\": \"" << path << "\",\n";
    out << "  \"schema_name\": \"" << schema_name << "\",\n";
    out << "  \"state\": \"" << ObjectStateToString(design_state.state) << "\",\n";
    out << "  \"state_changed_by\": \"" << design_state.changed_by << "\",\n";
    out << "  \"state_changed_at\": " << design_state.changed_at << ",\n";
    out << "  \"has_source\": " << (has_source ? "true" : "false") << "\n";
    out << "}";
}

std::unique_ptr<ProjectObject> ProjectObject::FromJson(const std::string& json) {
    // Simple parsing - in production use proper JSON library
    auto obj = std::make_unique<ProjectObject>();
    
    JsonParser parser(json);
    JsonValue root;
    std::string error;
    if (parser.Parse(&root, &error)) {
        if (auto id = FindMember(root, "id")) {
            obj->id = UUID::FromString(id->string_value);
        }
        if (auto kind = FindMember(root, "kind")) {
            obj->kind = kind->string_value;
        }
        if (auto name = FindMember(root, "name")) {
            obj->name = name->string_value;
        }
        if (auto path = FindMember(root, "path")) {
            obj->path = path->string_value;
        }
        if (auto schema = FindMember(root, "schema_name")) {
            obj->schema_name = schema->string_value;
        }
        if (auto state = FindMember(root, "state")) {
            obj->design_state.state = ObjectStateFromString(state->string_value);
        }
    }
    
    return obj;
}

std::string ProjectObject::GenerateDDL() const {
    // Generate DDL based on kind and current_design
    std::ostringstream ddl;
    
    if (kind == "table") {
        ddl << "CREATE TABLE " << schema_name << "." << name << " (\n";
        // Add column definitions from current_design
        // This is simplified - real implementation would be more complex
        ddl << "  -- columns would be defined here\n";
        ddl << ");";
    } else if (kind == "index") {
        ddl << "CREATE INDEX idx_" << name << " ON " << schema_name << "." << name << "();";
    }
    // ... other kinds
    
    return ddl.str();
}

std::string ProjectObject::GenerateRollbackDDL() const {
    std::ostringstream ddl;
    
    if (kind == "table") {
        ddl << "DROP TABLE IF EXISTS " << schema_name << "." << name << " CASCADE;";
    } else if (kind == "index") {
        ddl << "DROP INDEX IF EXISTS idx_" << name << ";";
    }
    
    return ddl.str();
}

// ============================================================================
// Project Implementation
// ============================================================================

Project::Project() : id(UUID::Generate()) {}

Project::~Project() = default;

bool Project::CreateNew(const std::string& path, const ProjectConfig& cfg) {
    project_root_path = path;
    config = cfg;
    
    // Create project directory structure
    std::filesystem::create_directories(project_root_path);
    std::filesystem::create_directories(project_root_path + "/" + config.designs_path);
    std::filesystem::create_directories(project_root_path + "/" + config.diagrams_path);
    std::filesystem::create_directories(project_root_path + "/" + config.whiteboards_path);
    std::filesystem::create_directories(project_root_path + "/" + config.mindmaps_path);
    std::filesystem::create_directories(project_root_path + "/" + config.docs_path);
    std::filesystem::create_directories(project_root_path + "/" + config.tests_path);
    std::filesystem::create_directories(project_root_path + "/" + config.deployments_path);
    std::filesystem::create_directories(project_root_path + "/" + config.reports_path);
    
    is_open_ = true;
    is_modified_ = true;
    
    return Save();
}

bool Project::Open(const std::string& path) {
    project_root_path = path;
    
    if (!LoadProjectFile()) {
        return false;
    }
    
    if (!LoadObjectFiles()) {
        return false;
    }
    
    is_open_ = true;
    is_modified_ = false;
    
    return true;
}

bool Project::Save() {
    if (!SaveProjectFile()) {
        return false;
    }
    
    if (!SaveObjectFiles()) {
        return false;
    }
    
    is_modified_ = false;
    return true;
}

void Project::Close() {
    if (is_modified_) {
        Save();
    }
    
    objects_by_id.clear();
    objects_by_path.clear();
    is_open_ = false;
}

std::shared_ptr<ProjectObject> Project::CreateObject(const std::string& kind,
                                                      const std::string& name,
                                                      const std::string& schema) {
    auto obj = std::make_shared<ProjectObject>(kind, name);
    obj->schema_name = schema;
    obj->path = schema.empty() ? name : schema + "." + name;
    obj->design_file_path = config.designs_path + "/" + obj->path + "." + kind + ".json";
    
    objects_by_id[obj->id] = obj;
    objects_by_path[obj->path] = obj;
    
    is_modified_ = true;
    NotifyObjectChanged(obj->id, "created");
    
    return obj;
}

std::shared_ptr<ProjectObject> Project::GetObject(const UUID& id) {
    auto it = objects_by_id.find(id);
    if (it != objects_by_id.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<ProjectObject> Project::GetObjectByPath(const std::string& path) {
    auto it = objects_by_path.find(path);
    if (it != objects_by_path.end()) {
        return it->second;
    }
    return nullptr;
}

bool Project::DeleteObject(const UUID& id) {
    auto obj = GetObject(id);
    if (!obj) {
        return false;
    }
    
    // Soft delete - mark as deleted
    obj->SetState(ObjectState::DELETED, "User deleted object", "user");
    
    is_modified_ = true;
    NotifyObjectChanged(id, "deleted");
    
    return true;
}

std::vector<std::shared_ptr<ProjectObject>> Project::GetObjectsByState(ObjectState state) {
    std::vector<std::shared_ptr<ProjectObject>> result;
    for (const auto& [id, obj] : objects_by_id) {
        if (obj->GetState() == state) {
            result.push_back(obj);
        }
    }
    return result;
}

std::vector<std::shared_ptr<ProjectObject>> Project::GetObjectsByKind(const std::string& kind) {
    std::vector<std::shared_ptr<ProjectObject>> result;
    for (const auto& [id, obj] : objects_by_id) {
        if (obj->kind == kind) {
            result.push_back(obj);
        }
    }
    return result;
}

bool Project::ModifyObject(const UUID& id, const MetadataNode& new_design) {
    auto obj = GetObject(id);
    if (!obj) {
        return false;
    }
    
    // Store old design if this is first modification
    if (obj->GetState() == ObjectState::EXTRACTED && obj->has_source) {
        obj->source_snapshot = obj->current_design;
    }
    
    obj->current_design = new_design;
    obj->SetState(ObjectState::MODIFIED, "Design modified", "user");
    
    is_modified_ = true;
    NotifyObjectChanged(id, "modified");
    
    return true;
}

Project::Stats Project::GetStats() const {
    Stats stats;
    stats.total_objects = static_cast<int>(objects_by_id.size());
    
    for (const auto& [id, obj] : objects_by_id) {
        switch (obj->GetState()) {
            case ObjectState::EXTRACTED: stats.extracted++; break;
            case ObjectState::NEW: stats.new_objects++; break;
            case ObjectState::MODIFIED: stats.modified++; break;
            case ObjectState::DELETED: stats.deleted++; break;
            case ObjectState::PENDING: stats.pending++; break;
            case ObjectState::APPROVED: stats.approved++; break;
            case ObjectState::IMPLEMENTED: stats.implemented++; break;
            default: break;
        }
    }
    
    return stats;
}

void Project::NotifyObjectChanged(const UUID& id, const std::string& action) {
    for (const auto& callback : observers_) {
        callback(id, action);
    }
}

void Project::AddObserver(ObjectChangedCallback callback) {
    observers_.push_back(callback);
}

void Project::RemoveObserver(ObjectChangedCallback callback) {
    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
            [&callback](const auto& cb) { return cb.target_type() == callback.target_type(); }),
        observers_.end());
}

void Project::SetStatusCallback(StatusCallback callback) {
    status_callback_ = std::move(callback);
}

void Project::ClearStatusCallback() {
    status_callback_ = nullptr;
}

std::vector<Project::StatusEvent> Project::GetStatusEvents() const {
    return status_events_;
}

void Project::EmitStatus(const std::string& message, bool is_error) {
    StatusEvent evt;
    evt.timestamp = std::time(nullptr);
    evt.message = message;
    evt.is_error = is_error;
    status_events_.push_back(evt);
    if (status_events_.size() > 200) {
        status_events_.erase(status_events_.begin(),
                             status_events_.begin() + (status_events_.size() - 200));
    }
    if (status_callback_) {
        status_callback_(evt);
    }
}

// File I/O
bool Project::SaveProjectFile() {
    if (project_file_path.empty()) {
        project_file_path = project_root_path + "/project.srproj";
    }
    std::string error;
    return ProjectSerializer::SaveToFile(*this, project_file_path, &error);
}

bool Project::SaveObjectFiles() {
    std::filesystem::create_directories(project_root_path + "/" + config.designs_path);
    for (const auto& pair : objects_by_id) {
        const auto& obj = pair.second;
        if (obj->design_file_path.empty()) continue;
        std::string full_path = project_root_path + "/" + obj->design_file_path;
        std::filesystem::create_directories(std::filesystem::path(full_path).parent_path());
        std::ofstream out(full_path, std::ios::binary);
        if (!out) continue;
        obj->ToJson(out);
    }
    return true;
}

bool Project::LoadProjectFile() {
    if (project_file_path.empty()) {
        project_file_path = project_root_path + "/project.srproj";
    }
    std::string error;
    return ProjectSerializer::LoadFromFile(this, project_file_path, &error);
}

bool Project::LoadObjectFiles() {
    // Optional: object files are exports; project file is authoritative
    return true;
}

// Git sync implementations using GitClient
bool Project::SyncToDatabase() {
    if (!config.git.enabled) {
        EmitStatus("Git sync disabled for project", true);
        return false;
    }

    auto& git = ProjectGitManager::Instance();
    if (!git.OpenProjectRepository(project_root_path)) {
        if (!git.InitializeProjectRepository(project_root_path,
                                             config.git.repo_url.empty()
                                                 ? std::nullopt
                                                 : std::optional<std::string>(config.git.repo_url))) {
            EmitStatus("Failed to initialize Git repository", true);
            return false;
        }
    }

    if (!SaveProjectFile()) return false;
    SaveObjectFiles();

    if (!git.SyncDesignToRepository(config.designs_path)) {
        EmitStatus("Failed to sync project changes to repository", true);
        return false;
    }

    UpdateSyncStateFromGit(this, git, config.designs_path);

    auto conflicts = git.GetConflictedFilesInPath(EnsureTrailingSlash(config.designs_path));
    sync_state.pending.conflicts = conflicts;
    for (const auto& conflict : conflicts) {
        auto obj = FindObjectByDesignPath(this, conflict);
        if (obj) {
            obj->SetState(ObjectState::CONFLICTED, "Git sync conflict", "git_sync");
        }
    }

    is_modified_ = true;
    EmitStatus("Sync to repository completed", false);
    return true;
}

bool Project::SyncFromDatabase() {
    if (!config.git.enabled) {
        EmitStatus("Git sync disabled for project", true);
        return false;
    }

    auto& git = ProjectGitManager::Instance();
    if (!git.OpenProjectRepository(project_root_path)) {
        EmitStatus("Git repository not open", true);
        return false;
    }

    if (!git.SyncRepositoryToDesign(config.designs_path)) {
        EmitStatus("Failed to pull repository changes", true);
        return false;
    }

    UpdateSyncStateFromGit(this, git, config.designs_path);

    auto conflicts = git.GetConflictedFilesInPath(EnsureTrailingSlash(config.designs_path));
    sync_state.pending.conflicts = conflicts;
    for (const auto& conflict : conflicts) {
        auto obj = FindObjectByDesignPath(this, conflict);
        if (obj) {
            obj->SetState(ObjectState::CONFLICTED, "Git sync conflict", "git_sync");
        }
    }

    is_modified_ = true;
    EmitStatus("Sync from repository completed", false);
    return true;
}

bool Project::ResolveConflict(const UUID& id, const std::string& resolution) {
    auto obj = GetObject(id);
    if (!obj || obj->design_file_path.empty()) {
        EmitStatus("Conflict resolution failed: missing design file", true);
        return false;
    }

    if (!config.git.enabled) {
        EmitStatus("Conflict resolution failed: Git sync disabled", true);
        return false;
    }

    auto& git = ProjectGitManager::Instance();
    if (!git.OpenProjectRepository(project_root_path)) {
        EmitStatus("Conflict resolution failed: Git repository not open", true);
        return false;
    }

    std::string lower = resolution;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    bool keep_project = (lower == "ours" || lower == "project" || lower == "keep_project");
    bool keep_database = (lower == "theirs" || lower == "database" || lower == "keep_database");

    if (!git.ResolveObjectConflict(obj->design_file_path, resolution)) {
        EmitStatus("Conflict resolution failed in Git", true);
        return false;
    }

    if (keep_project) {
        std::string full_path = project_root_path + "/" + obj->design_file_path;
        std::filesystem::create_directories(std::filesystem::path(full_path).parent_path());
        std::ofstream out(full_path, std::ios::binary);
        if (!out) return false;
        obj->ToJson(out);
    } else if (keep_database) {
        std::string full_path = project_root_path + "/" + obj->design_file_path;
        std::ifstream in(full_path, std::ios::binary);
        if (!in) return false;
        std::stringstream buffer;
        buffer << in.rdbuf();
        auto incoming = ProjectObject::FromJson(buffer.str());
        if (!incoming) return false;
        obj->kind = incoming->kind;
        obj->name = incoming->name;
        obj->path = incoming->path;
        obj->schema_name = incoming->schema_name;
    }

    obj->SetState(ObjectState::MODIFIED, "Resolved conflict", "git_sync");
    sync_state.pending.conflicts.erase(
        std::remove(sync_state.pending.conflicts.begin(),
                    sync_state.pending.conflicts.end(),
                    obj->design_file_path),
        sync_state.pending.conflicts.end());
    is_modified_ = true;
    EmitStatus("Conflict resolved for " + obj->name, false);
    return true;
}

bool Project::ExtractFromDatabase(const DatabaseConnection& conn,
                                   const std::vector<std::string>& object_patterns) {
    MetadataModel model;
    std::string fixture_path;
    std::string error;

    if (IsFixtureConnection(conn, &fixture_path)) {
        if (!model.LoadFromFixture(fixture_path, &error)) {
            EmitStatus("Failed to load fixture: " + error, true);
            return false;
        }
    } else {
        model.LoadStub();
    }

    const auto& snapshot = model.GetSnapshot();
    for (const auto& node : snapshot.nodes) {
        if (!MatchesPatterns(node, object_patterns)) {
            continue;
        }
        AddObjectFromNode(this, node);

        if ((node.kind == "table" || node.kind == "view") && !node.children.empty()) {
            for (const auto& child : node.children) {
                AddObjectFromNode(this, child);
            }
        } else {
            for (const auto& child : node.children) {
                if (MatchesPatterns(child, object_patterns)) {
                    AddObjectFromNode(this, child);
                }
            }
        }
    }

    is_modified_ = true;
    EmitStatus("Extraction completed", false);
    return true;
}

bool Project::ApproveObject(const UUID& id, const std::string& approver) {
    auto obj = GetObject(id);
    if (!obj) return false;
    
    obj->SetState(ObjectState::APPROVED, "Approved for deployment", approver);
    is_modified_ = true;
    NotifyObjectChanged(id, "approved");
    return true;
}

bool Project::RejectObject(const UUID& id, const std::string& reason) {
    auto obj = GetObject(id);
    if (!obj) return false;
    
    obj->SetState(ObjectState::REJECTED, reason, "reviewer");
    obj->design_state.review_comment = reason;
    is_modified_ = true;
    NotifyObjectChanged(id, "rejected");
    return true;
}

// ============================================================================
// Project Manager
// ============================================================================

ProjectManager& ProjectManager::Instance() {
    static ProjectManager instance;
    return instance;
}

std::shared_ptr<Project> ProjectManager::CreateProject(const std::string& path,
                                                        const ProjectConfig& config) {
    auto project = std::make_shared<Project>();
    if (project->CreateNew(path, config)) {
        open_projects_[project->id] = project;
        SetCurrentProject(project->id);
        AddRecentProject(path);
        return project;
    }
    return nullptr;
}

std::shared_ptr<Project> ProjectManager::OpenProject(const std::string& path) {
    auto project = std::make_shared<Project>();
    if (project->Open(path)) {
        open_projects_[project->id] = project;
        SetCurrentProject(project->id);
        AddRecentProject(path);
        return project;
    }
    return nullptr;
}

std::shared_ptr<Project> ProjectManager::GetCurrentProject() {
    return GetProject(current_project_id_);
}

std::shared_ptr<Project> ProjectManager::GetProject(const UUID& id) {
    auto it = open_projects_.find(id);
    if (it != open_projects_.end()) {
        return it->second;
    }
    return nullptr;
}

void ProjectManager::SetCurrentProject(const UUID& id) {
    current_project_id_ = id;
}

std::vector<std::shared_ptr<Project>> ProjectManager::GetOpenProjects() {
    std::vector<std::shared_ptr<Project>> result;
    for (const auto& [id, project] : open_projects_) {
        result.push_back(project);
    }
    return result;
}

void ProjectManager::AddRecentProject(const std::string& path) {
    // Remove if exists
    recent_projects_.erase(
        std::remove(recent_projects_.begin(), recent_projects_.end(), path),
        recent_projects_.end());
    
    // Add to front
    recent_projects_.insert(recent_projects_.begin(), path);
    
    // Keep only last 10
    if (recent_projects_.size() > 10) {
        recent_projects_.resize(10);
    }
}

std::vector<std::string> ProjectManager::GetRecentProjects() {
    return recent_projects_;
}

} // namespace scratchrobin
