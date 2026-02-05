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
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "core/simple_json.h"

#include "git_client.h"

namespace scratchrobin {

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
    // In production, use proper filesystem operations
    
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

// File I/O stubs - would be fully implemented
bool Project::SaveProjectFile() { return true; }
bool Project::SaveObjectFiles() { return true; }
bool Project::LoadProjectFile() { return true; }
bool Project::LoadObjectFiles() { return true; }

// Git sync implementations using GitClient
bool Project::SyncToDatabase() {
    // TODO: Implement proper Git sync
    // This requires updating to use actual Project class members
    return true;
}

bool Project::SyncFromDatabase() {
    // TODO: Implement proper Git sync
    // This requires updating to use actual Project class members  
    return true;
}

bool Project::ResolveConflict(const UUID& id, const std::string& resolution) {
    (void)id;
    (void)resolution;
    // TODO: Implement proper conflict resolution
    return true;
}

bool Project::ExtractFromDatabase(const DatabaseConnection& conn,
                                   const std::vector<std::string>& object_patterns) {
    (void)conn;
    (void)object_patterns;
    // TODO: Implement proper database extraction
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
