#include "project_serialization.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

namespace scratchrobin {

namespace {

constexpr uint32_t kMagic = 0x4A505253; // "SRPJ" little-endian
constexpr uint16_t kVersionMajor = 1;
constexpr uint16_t kVersionMinor = 0;
constexpr uint8_t kEndianness = 1; // little
constexpr uint16_t kHeaderSize = 44;
constexpr uint16_t kTocEntrySize = 40;

struct Chunk {
    char id[4];
    std::vector<uint8_t> data;
    uint16_t flags = 0;
    uint64_t uncompressed_length = 0;
};

void WriteU8(std::vector<uint8_t>& out, uint8_t v) { out.push_back(v); }
void WriteU16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}
void WriteU32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}
void WriteU64(std::vector<uint8_t>& out, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
    }
}

uint8_t ReadU8(const uint8_t*& p, const uint8_t* end, bool* ok) {
    if (p + 1 > end) { *ok = false; return 0; }
    uint8_t v = *p; p += 1; return v;
}
uint16_t ReadU16(const uint8_t*& p, const uint8_t* end, bool* ok) {
    if (p + 2 > end) { *ok = false; return 0; }
    uint16_t v = static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8); p += 2; return v;
}
uint32_t ReadU32(const uint8_t*& p, const uint8_t* end, bool* ok) {
    if (p + 4 > end) { *ok = false; return 0; }
    uint32_t v = static_cast<uint32_t>(p[0]) |
                 (static_cast<uint32_t>(p[1]) << 8) |
                 (static_cast<uint32_t>(p[2]) << 16) |
                 (static_cast<uint32_t>(p[3]) << 24);
    p += 4; return v;
}
uint64_t ReadU64(const uint8_t*& p, const uint8_t* end, bool* ok) {
    if (p + 8 > end) { *ok = false; return 0; }
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= (static_cast<uint64_t>(p[i]) << (i * 8));
    }
    p += 8; return v;
}

void WriteBytes(std::vector<uint8_t>& out, const uint8_t* data, size_t len) {
    out.insert(out.end(), data, data + len);
}

void WriteUUID(std::vector<uint8_t>& out, const UUID& uuid) {
    WriteBytes(out, reinterpret_cast<const uint8_t*>(uuid.data), 16);
}

bool ReadUUID(const uint8_t*& p, const uint8_t* end, UUID* out_uuid) {
    if (p + 16 > end) return false;
    std::memcpy(out_uuid->data, p, 16);
    p += 16;
    return true;
}

void WriteUVarint(std::vector<uint8_t>& out, uint64_t v) {
    while (v >= 0x80) {
        out.push_back(static_cast<uint8_t>(v | 0x80));
        v >>= 7;
    }
    out.push_back(static_cast<uint8_t>(v));
}

uint64_t ReadUVarint(const uint8_t*& p, const uint8_t* end, bool* ok) {
    uint64_t result = 0;
    int shift = 0;
    while (p < end && shift <= 63) {
        uint8_t byte = *p++;
        result |= static_cast<uint64_t>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) return result;
        shift += 7;
    }
    *ok = false;
    return 0;
}

void WriteString(std::vector<uint8_t>& out, const std::string& s) {
    WriteUVarint(out, static_cast<uint64_t>(s.size()));
    if (!s.empty()) {
        WriteBytes(out, reinterpret_cast<const uint8_t*>(s.data()), s.size());
    }
}

std::string ReadString(const uint8_t*& p, const uint8_t* end, bool* ok) {
    uint64_t len = ReadUVarint(p, end, ok);
    if (!*ok) return "";
    if (p + len > end) { *ok = false; return ""; }
    std::string s(reinterpret_cast<const char*>(p), static_cast<size_t>(len));
    p += len;
    return s;
}

// CRC32 implementation (polynomial 0xEDB88320)
uint32_t CRC32(const uint8_t* data, size_t len) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        init = true;
    }
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

void WriteBool(std::vector<uint8_t>& out, bool v) {
    WriteU8(out, v ? 1 : 0);
}

bool ReadBool(const uint8_t*& p, const uint8_t* end, bool* ok) {
    uint8_t v = ReadU8(p, end, ok);
    return v != 0;
}

uint8_t ObjectStateToByte(ObjectState state) {
    switch (state) {
        case ObjectState::EXTRACTED: return 0;
        case ObjectState::NEW: return 1;
        case ObjectState::MODIFIED: return 2;
        case ObjectState::DELETED: return 3;
        case ObjectState::PENDING: return 4;
        case ObjectState::APPROVED: return 5;
        case ObjectState::REJECTED: return 6;
        case ObjectState::IMPLEMENTED: return 7;
        case ObjectState::CONFLICTED: return 8;
        default: return 0;
    }
}

ObjectState ByteToObjectState(uint8_t v) {
    switch (v) {
        case 0: return ObjectState::EXTRACTED;
        case 1: return ObjectState::NEW;
        case 2: return ObjectState::MODIFIED;
        case 3: return ObjectState::DELETED;
        case 4: return ObjectState::PENDING;
        case 5: return ObjectState::APPROVED;
        case 6: return ObjectState::REJECTED;
        case 7: return ObjectState::IMPLEMENTED;
        case 8: return ObjectState::CONFLICTED;
        default: return ObjectState::NEW;
    }
}

void WriteMetadataNode(std::vector<uint8_t>& out, const MetadataNode& node) {
    std::vector<uint8_t> buf;
    WriteUVarint(buf, static_cast<uint64_t>(node.id));
    WriteU8(buf, static_cast<uint8_t>(node.type));
    WriteString(buf, node.label);
    WriteString(buf, node.kind);
    WriteString(buf, node.catalog);
    WriteString(buf, node.path);
    WriteString(buf, node.ddl);
    WriteUVarint(buf, static_cast<uint64_t>(node.dependencies.size()));
    for (const auto& dep : node.dependencies) {
        WriteString(buf, dep);
    }
    WriteUVarint(buf, static_cast<uint64_t>(node.children.size()));
    for (const auto& child : node.children) {
        WriteMetadataNode(buf, child);
    }
    WriteString(buf, node.name);
    WriteString(buf, node.schema);
    WriteUVarint(buf, static_cast<uint64_t>(node.parent_id));
    WriteU64(buf, static_cast<uint64_t>(node.row_count));

    WriteUVarint(out, static_cast<uint64_t>(buf.size()));
    WriteBytes(out, buf.data(), buf.size());
}

bool ReadMetadataNode(const uint8_t*& p, const uint8_t* end, MetadataNode* node) {
    bool ok = true;
    uint64_t len = ReadUVarint(p, end, &ok);
    if (!ok || p + len > end) return false;
    const uint8_t* limit = p + len;

    node->id = static_cast<int>(ReadUVarint(p, limit, &ok));
    node->type = static_cast<MetadataType>(ReadU8(p, limit, &ok));
    node->label = ReadString(p, limit, &ok);
    node->kind = ReadString(p, limit, &ok);
    node->catalog = ReadString(p, limit, &ok);
    node->path = ReadString(p, limit, &ok);
    node->ddl = ReadString(p, limit, &ok);

    uint64_t dep_count = ReadUVarint(p, limit, &ok);
    node->dependencies.clear();
    for (uint64_t i = 0; i < dep_count && ok; ++i) {
        node->dependencies.push_back(ReadString(p, limit, &ok));
    }

    uint64_t child_count = ReadUVarint(p, limit, &ok);
    node->children.clear();
    for (uint64_t i = 0; i < child_count && ok; ++i) {
        MetadataNode child;
        if (!ReadMetadataNode(p, limit, &child)) { ok = false; break; }
        node->children.push_back(child);
    }

    node->name = ReadString(p, limit, &ok);
    node->schema = ReadString(p, limit, &ok);
    node->parent_id = static_cast<int>(ReadUVarint(p, limit, &ok));
    node->row_count = static_cast<int64_t>(ReadU64(p, limit, &ok));

    if (!ok) return false;
    p = limit;
    return true;
}

void WriteProjectObject(std::vector<uint8_t>& out, const ProjectObject& obj) {
    std::vector<uint8_t> buf;
    WriteUUID(buf, obj.id);
    WriteString(buf, obj.kind);
    WriteString(buf, obj.name);
    WriteString(buf, obj.path);
    WriteString(buf, obj.schema_name);

    WriteU8(buf, ObjectStateToByte(obj.design_state.state));
    WriteString(buf, obj.design_state.changed_by);
    WriteU64(buf, static_cast<uint64_t>(obj.design_state.changed_at));
    WriteString(buf, obj.design_state.reason);
    WriteString(buf, obj.design_state.review_comment);

    WriteBool(buf, obj.has_source);
    if (obj.has_source) {
        WriteMetadataNode(buf, obj.source_snapshot);
    }
    WriteMetadataNode(buf, obj.current_design);

    WriteUVarint(buf, static_cast<uint64_t>(obj.comments.size()));
    for (const auto& c : obj.comments) {
        WriteString(buf, c.author);
        WriteU64(buf, static_cast<uint64_t>(c.timestamp));
        WriteString(buf, c.text);
        WriteBool(buf, c.resolved);
    }

    WriteUVarint(buf, static_cast<uint64_t>(obj.change_history.size()));
    for (const auto& ch : obj.change_history) {
        WriteString(buf, ch.field);
        WriteString(buf, ch.old_value);
        WriteString(buf, ch.new_value);
        WriteU64(buf, static_cast<uint64_t>(ch.timestamp));
        WriteString(buf, ch.author);
    }

    WriteString(buf, obj.design_file_path);

    WriteUVarint(out, static_cast<uint64_t>(buf.size()));
    WriteBytes(out, buf.data(), buf.size());
}

bool ReadProjectObject(const uint8_t*& p, const uint8_t* end, ProjectObject* obj) {
    bool ok = true;
    uint64_t len = ReadUVarint(p, end, &ok);
    if (!ok || p + len > end) return false;
    const uint8_t* limit = p + len;

    if (!ReadUUID(p, limit, &obj->id)) return false;
    obj->kind = ReadString(p, limit, &ok);
    obj->name = ReadString(p, limit, &ok);
    obj->path = ReadString(p, limit, &ok);
    obj->schema_name = ReadString(p, limit, &ok);

    obj->design_state.state = ByteToObjectState(ReadU8(p, limit, &ok));
    obj->design_state.changed_by = ReadString(p, limit, &ok);
    obj->design_state.changed_at = static_cast<std::time_t>(ReadU64(p, limit, &ok));
    obj->design_state.reason = ReadString(p, limit, &ok);
    obj->design_state.review_comment = ReadString(p, limit, &ok);

    obj->has_source = ReadBool(p, limit, &ok);
    if (obj->has_source) {
        if (!ReadMetadataNode(p, limit, &obj->source_snapshot)) return false;
    }
    if (!ReadMetadataNode(p, limit, &obj->current_design)) return false;

    uint64_t comment_count = ReadUVarint(p, limit, &ok);
    obj->comments.clear();
    for (uint64_t i = 0; i < comment_count && ok; ++i) {
        ProjectObject::Comment c;
        c.author = ReadString(p, limit, &ok);
        c.timestamp = static_cast<std::time_t>(ReadU64(p, limit, &ok));
        c.text = ReadString(p, limit, &ok);
        c.resolved = ReadBool(p, limit, &ok);
        obj->comments.push_back(c);
    }

    uint64_t change_count = ReadUVarint(p, limit, &ok);
    obj->change_history.clear();
    for (uint64_t i = 0; i < change_count && ok; ++i) {
        ProjectObject::ChangeRecord ch;
        ch.field = ReadString(p, limit, &ok);
        ch.old_value = ReadString(p, limit, &ok);
        ch.new_value = ReadString(p, limit, &ok);
        ch.timestamp = static_cast<std::time_t>(ReadU64(p, limit, &ok));
        ch.author = ReadString(p, limit, &ok);
        obj->change_history.push_back(ch);
    }

    obj->design_file_path = ReadString(p, limit, &ok);

    if (!ok) return false;
    p = limit;
    return true;
}

void WriteStringArray(std::vector<uint8_t>& out, const std::vector<std::string>& arr) {
    WriteUVarint(out, static_cast<uint64_t>(arr.size()));
    for (const auto& s : arr) {
        WriteString(out, s);
    }
}

std::vector<std::string> ReadStringArray(const uint8_t*& p, const uint8_t* end, bool* ok) {
    uint64_t count = ReadUVarint(p, end, ok);
    std::vector<std::string> arr;
    for (uint64_t i = 0; i < count && *ok; ++i) {
        arr.push_back(ReadString(p, end, ok));
    }
    return arr;
}

void WriteProjectConfig(std::vector<uint8_t>& out, const Project& project) {
    const ProjectConfig& cfg = project.config;
    WriteUUID(out, project.id);
    WriteString(out, cfg.name);
    WriteString(out, cfg.description);
    WriteString(out, cfg.version);
    WriteString(out, cfg.database_type);
    WriteU64(out, 0); // created_at
    WriteU64(out, 0); // updated_at

    WriteString(out, cfg.designs_path);
    WriteString(out, cfg.diagrams_path);
    WriteString(out, cfg.whiteboards_path);
    WriteString(out, cfg.mindmaps_path);
    WriteString(out, cfg.docs_path);
    WriteString(out, cfg.tests_path);
    WriteString(out, cfg.deployments_path);
    WriteString(out, cfg.reports_path);

    WriteUVarint(out, static_cast<uint64_t>(cfg.connections.size()));
    for (const auto& conn : cfg.connections) {
        UUID empty{};
        WriteUUID(out, empty);
        WriteString(out, conn.name);
        WriteString(out, conn.backend_type);
        WriteString(out, conn.connection_string);
        WriteString(out, conn.credential_ref);
        WriteBool(out, conn.is_source);
        WriteBool(out, conn.is_target);
        WriteString(out, conn.git_branch);
        WriteBool(out, conn.requires_approval);
        WriteBool(out, conn.is_git_enabled);
        WriteString(out, conn.git_repo_url);
    }

    WriteBool(out, cfg.git.enabled);
    WriteString(out, cfg.git.repo_url);
    WriteString(out, cfg.git.default_branch);
    WriteString(out, cfg.git.workflow);
    WriteString(out, cfg.git.sync_mode);
    WriteStringArray(out, cfg.git.auto_sync_branches);
    WriteStringArray(out, cfg.git.protected_branches);
    WriteBool(out, cfg.git.require_conventional_commits);
    WriteBool(out, cfg.git.auto_sync_messages);

    // Governance (empty for now, defaults)
    WriteStringArray(out, cfg.governance.owners);
    WriteStringArray(out, cfg.governance.stewards);

    WriteUVarint(out, static_cast<uint64_t>(cfg.governance.environments.size()));
    for (const auto& env : cfg.governance.environments) {
        WriteString(out, env.id);
        WriteString(out, env.name);
        WriteBool(out, env.approval_required);
        WriteUVarint(out, static_cast<uint64_t>(env.min_reviewers));
        WriteStringArray(out, env.allowed_roles);
    }

    WriteStringArray(out, cfg.governance.compliance_tags);

    WriteUVarint(out, static_cast<uint64_t>(cfg.governance.review_policy.min_reviewers));
    WriteStringArray(out, cfg.governance.review_policy.required_roles);
    WriteUVarint(out, static_cast<uint64_t>(cfg.governance.review_policy.approval_window_hours));

    WriteBool(out, cfg.governance.ai_policy.enabled);
    WriteBool(out, cfg.governance.ai_policy.requires_review);
    WriteStringArray(out, cfg.governance.ai_policy.allowed_scopes);
    WriteStringArray(out, cfg.governance.ai_policy.prohibited_scopes);

    WriteString(out, cfg.governance.audit_policy.log_level);
    WriteUVarint(out, static_cast<uint64_t>(cfg.governance.audit_policy.retain_days));
    WriteString(out, cfg.governance.audit_policy.export_target);
}

bool ReadProjectConfig(const uint8_t*& p, const uint8_t* end, Project* project) {
    bool ok = true;
    if (!ReadUUID(p, end, &project->id)) return false;
    ProjectConfig& cfg = project->config;

    cfg.name = ReadString(p, end, &ok);
    cfg.description = ReadString(p, end, &ok);
    cfg.version = ReadString(p, end, &ok);
    cfg.database_type = ReadString(p, end, &ok);
    (void)ReadU64(p, end, &ok); // created_at
    (void)ReadU64(p, end, &ok); // updated_at

    cfg.designs_path = ReadString(p, end, &ok);
    cfg.diagrams_path = ReadString(p, end, &ok);
    cfg.whiteboards_path = ReadString(p, end, &ok);
    cfg.mindmaps_path = ReadString(p, end, &ok);
    cfg.docs_path = ReadString(p, end, &ok);
    cfg.tests_path = ReadString(p, end, &ok);
    cfg.deployments_path = ReadString(p, end, &ok);
    cfg.reports_path = ReadString(p, end, &ok);

    uint64_t conn_count = ReadUVarint(p, end, &ok);
    cfg.connections.clear();
    for (uint64_t i = 0; i < conn_count && ok; ++i) {
        DatabaseConnection conn;
        UUID tmp{};
        if (!ReadUUID(p, end, &tmp)) { ok = false; break; }
        conn.name = ReadString(p, end, &ok);
        conn.backend_type = ReadString(p, end, &ok);
        conn.connection_string = ReadString(p, end, &ok);
        conn.credential_ref = ReadString(p, end, &ok);
        conn.is_source = ReadBool(p, end, &ok);
        conn.is_target = ReadBool(p, end, &ok);
        conn.git_branch = ReadString(p, end, &ok);
        conn.requires_approval = ReadBool(p, end, &ok);
        conn.is_git_enabled = ReadBool(p, end, &ok);
        conn.git_repo_url = ReadString(p, end, &ok);
        cfg.connections.push_back(conn);
    }

    cfg.git.enabled = ReadBool(p, end, &ok);
    cfg.git.repo_url = ReadString(p, end, &ok);
    cfg.git.default_branch = ReadString(p, end, &ok);
    cfg.git.workflow = ReadString(p, end, &ok);
    cfg.git.sync_mode = ReadString(p, end, &ok);
    cfg.git.auto_sync_branches = ReadStringArray(p, end, &ok);
    cfg.git.protected_branches = ReadStringArray(p, end, &ok);
    cfg.git.require_conventional_commits = ReadBool(p, end, &ok);
    cfg.git.auto_sync_messages = ReadBool(p, end, &ok);

    cfg.governance.owners = ReadStringArray(p, end, &ok);
    cfg.governance.stewards = ReadStringArray(p, end, &ok);

    uint64_t env_count = ReadUVarint(p, end, &ok);
    cfg.governance.environments.clear();
    for (uint64_t i = 0; i < env_count && ok; ++i) {
        ProjectConfig::GovernanceEnvironment env;
        env.id = ReadString(p, end, &ok);
        env.name = ReadString(p, end, &ok);
        env.approval_required = ReadBool(p, end, &ok);
        env.min_reviewers = static_cast<uint32_t>(ReadUVarint(p, end, &ok));
        env.allowed_roles = ReadStringArray(p, end, &ok);
        cfg.governance.environments.push_back(env);
    }

    cfg.governance.compliance_tags = ReadStringArray(p, end, &ok);
    cfg.governance.review_policy.min_reviewers = static_cast<uint32_t>(ReadUVarint(p, end, &ok));
    cfg.governance.review_policy.required_roles = ReadStringArray(p, end, &ok);
    cfg.governance.review_policy.approval_window_hours = static_cast<uint32_t>(ReadUVarint(p, end, &ok));

    cfg.governance.ai_policy.enabled = ReadBool(p, end, &ok);
    cfg.governance.ai_policy.requires_review = ReadBool(p, end, &ok);
    cfg.governance.ai_policy.allowed_scopes = ReadStringArray(p, end, &ok);
    cfg.governance.ai_policy.prohibited_scopes = ReadStringArray(p, end, &ok);

    cfg.governance.audit_policy.log_level = ReadString(p, end, &ok);
    cfg.governance.audit_policy.retain_days = static_cast<uint32_t>(ReadUVarint(p, end, &ok));
    cfg.governance.audit_policy.export_target = ReadString(p, end, &ok);

    return ok;
}

void WriteGitSyncState(std::vector<uint8_t>& out, const GitSyncState& state) {
    WriteU64(out, static_cast<uint64_t>(state.last_sync));

    WriteString(out, state.project_repo.head_commit);
    WriteString(out, state.project_repo.branch);
    WriteStringArray(out, state.project_repo.dirty_files);

    WriteString(out, state.database_repo.head_commit);
    WriteString(out, state.database_repo.branch);
    WriteStringArray(out, state.database_repo.dirty_files);

    WriteUVarint(out, static_cast<uint64_t>(state.mappings.size()));
    for (const auto& m : state.mappings) {
        WriteString(out, m.project_file);
        WriteString(out, m.db_object);
        WriteString(out, m.last_sync_commit);
        WriteString(out, m.sync_status);
    }

    WriteStringArray(out, state.pending.project_to_db);
    WriteStringArray(out, state.pending.db_to_project);
    WriteStringArray(out, state.pending.conflicts);
}

bool ReadGitSyncState(const uint8_t*& p, const uint8_t* end, GitSyncState* state) {
    bool ok = true;
    state->last_sync = static_cast<std::time_t>(ReadU64(p, end, &ok));

    state->project_repo.head_commit = ReadString(p, end, &ok);
    state->project_repo.branch = ReadString(p, end, &ok);
    state->project_repo.dirty_files = ReadStringArray(p, end, &ok);

    state->database_repo.head_commit = ReadString(p, end, &ok);
    state->database_repo.branch = ReadString(p, end, &ok);
    state->database_repo.dirty_files = ReadStringArray(p, end, &ok);

    uint64_t map_count = ReadUVarint(p, end, &ok);
    state->mappings.clear();
    for (uint64_t i = 0; i < map_count && ok; ++i) {
        GitSyncState::ObjectMapping m;
        m.project_file = ReadString(p, end, &ok);
        m.db_object = ReadString(p, end, &ok);
        m.last_sync_commit = ReadString(p, end, &ok);
        m.sync_status = ReadString(p, end, &ok);
        state->mappings.push_back(m);
    }

    state->pending.project_to_db = ReadStringArray(p, end, &ok);
    state->pending.db_to_project = ReadStringArray(p, end, &ok);
    state->pending.conflicts = ReadStringArray(p, end, &ok);

    return ok;
}

void WriteReportingAssets(std::vector<uint8_t>& out,
                          const std::vector<Project::ReportingAsset>& assets) {
    WriteUVarint(out, static_cast<uint64_t>(assets.size()));
    for (const auto& asset : assets) {
        WriteUUID(out, asset.id);
        WriteString(out, asset.object_type);
        WriteUVarint(out, static_cast<uint64_t>(asset.json_payload.size()));
        if (!asset.json_payload.empty()) {
            WriteBytes(out,
                       reinterpret_cast<const uint8_t*>(asset.json_payload.data()),
                       asset.json_payload.size());
        }
    }
}

bool ReadReportingAssets(const uint8_t*& p, const uint8_t* end,
                         std::vector<Project::ReportingAsset>* assets) {
    bool ok = true;
    uint64_t count = ReadUVarint(p, end, &ok);
    if (!ok) return false;
    assets->clear();
    for (uint64_t i = 0; i < count && ok; ++i) {
        Project::ReportingAsset asset;
        if (!ReadUUID(p, end, &asset.id)) return false;
        asset.object_type = ReadString(p, end, &ok);
        uint64_t payload_len = ReadUVarint(p, end, &ok);
        if (!ok || p + payload_len > end) return false;
        asset.json_payload.assign(reinterpret_cast<const char*>(p),
                                  static_cast<size_t>(payload_len));
        p += payload_len;
        assets->push_back(asset);
    }
    return ok;
}

void WriteReportingSchedules(std::vector<uint8_t>& out,
                             const std::vector<Project::ReportingSchedule>& schedules) {
    WriteUVarint(out, static_cast<uint64_t>(schedules.size()));
    for (const auto& schedule : schedules) {
        WriteUUID(out, schedule.id);
        WriteString(out, schedule.action);
        WriteString(out, schedule.target_id);
        WriteString(out, schedule.schedule_spec);
        WriteU32(out, static_cast<uint32_t>(schedule.interval_seconds));
        WriteU64(out, static_cast<uint64_t>(schedule.created_at));
        WriteU64(out, static_cast<uint64_t>(schedule.next_run));
        WriteU64(out, static_cast<uint64_t>(schedule.last_run));
        WriteU8(out, schedule.enabled ? 1 : 0);
    }
}

bool ReadReportingSchedules(const uint8_t*& p, const uint8_t* end,
                            std::vector<Project::ReportingSchedule>* schedules) {
    bool ok = true;
    uint64_t count = ReadUVarint(p, end, &ok);
    if (!ok) return false;
    schedules->clear();
    for (uint64_t i = 0; i < count && ok; ++i) {
        Project::ReportingSchedule schedule;
        if (!ReadUUID(p, end, &schedule.id)) return false;
        schedule.action = ReadString(p, end, &ok);
        schedule.target_id = ReadString(p, end, &ok);
        schedule.schedule_spec = ReadString(p, end, &ok);
        schedule.interval_seconds = static_cast<int>(ReadU32(p, end, &ok));
        schedule.created_at = static_cast<std::time_t>(ReadU64(p, end, &ok));
        schedule.next_run = static_cast<std::time_t>(ReadU64(p, end, &ok));
        schedule.last_run = static_cast<std::time_t>(ReadU64(p, end, &ok));
        schedule.enabled = ReadU8(p, end, &ok) != 0;
        if (!ok) return false;
        schedules->push_back(schedule);
    }
    return ok;
}

void WriteDataViews(std::vector<uint8_t>& out,
                    const std::vector<Project::DataViewSnapshot>& views) {
    WriteUVarint(out, static_cast<uint64_t>(views.size()));
    for (const auto& view : views) {
        WriteUUID(out, view.id);
        WriteUUID(out, view.diagram_id);
        WriteUVarint(out, static_cast<uint64_t>(view.json_payload.size()));
        if (!view.json_payload.empty()) {
            WriteBytes(out,
                       reinterpret_cast<const uint8_t*>(view.json_payload.data()),
                       view.json_payload.size());
        }
    }
}

bool ReadDataViews(const uint8_t*& p, const uint8_t* end,
                   std::vector<Project::DataViewSnapshot>* views) {
    bool ok = true;
    uint64_t count = ReadUVarint(p, end, &ok);
    if (!ok) return false;
    views->clear();
    for (uint64_t i = 0; i < count && ok; ++i) {
        Project::DataViewSnapshot view;
        if (!ReadUUID(p, end, &view.id)) return false;
        if (!ReadUUID(p, end, &view.diagram_id)) return false;
        uint64_t payload_len = ReadUVarint(p, end, &ok);
        if (!ok || p + payload_len > end) return false;
        view.json_payload.assign(reinterpret_cast<const char*>(p),
                                 static_cast<size_t>(payload_len));
        p += payload_len;
        views->push_back(view);
    }
    return ok;
}

} // namespace

bool ProjectSerializer::SaveToFile(const Project& project, const std::string& path, std::string* error) {
    std::vector<Chunk> chunks;

    Chunk proj;
    std::memcpy(proj.id, "PROJ", 4);
    WriteProjectConfig(proj.data, project);
    chunks.push_back(std::move(proj));

    Chunk objs;
    std::memcpy(objs.id, "OBJS", 4);
    WriteUVarint(objs.data, static_cast<uint64_t>(project.objects_by_id.size()));
    for (const auto& pair : project.objects_by_id) {
        WriteProjectObject(objs.data, *pair.second);
    }
    chunks.push_back(std::move(objs));

    if (project.sync_state.last_sync != 0 || !project.sync_state.mappings.empty()) {
        Chunk gits;
        std::memcpy(gits.id, "GITS", 4);
        WriteGitSyncState(gits.data, project.sync_state);
        chunks.push_back(std::move(gits));
    }

    if (!project.reporting_assets.empty()) {
        Chunk rptg;
        std::memcpy(rptg.id, "RPTG", 4);
        WriteReportingAssets(rptg.data, project.reporting_assets);
        chunks.push_back(std::move(rptg));
    }

    if (!project.reporting_schedules.empty()) {
        Chunk rpts;
        std::memcpy(rpts.id, "RPTS", 4);
        WriteReportingSchedules(rpts.data, project.reporting_schedules);
        chunks.push_back(std::move(rpts));
    }

    if (!project.data_views.empty()) {
        Chunk dvws;
        std::memcpy(dvws.id, "DVWS", 4);
        WriteDataViews(dvws.data, project.data_views);
        chunks.push_back(std::move(dvws));
    }

    // Build file layout
    std::vector<uint8_t> file;
    file.reserve(1024);

    // Header placeholder
    for (int i = 0; i < kHeaderSize; ++i) file.push_back(0);

    std::vector<uint64_t> offsets;
    offsets.reserve(chunks.size());

    for (const auto& c : chunks) {
        offsets.push_back(static_cast<uint64_t>(file.size()));
        WriteBytes(file, c.data.data(), c.data.size());
    }

    uint64_t toc_offset = static_cast<uint64_t>(file.size());

    // TOC entries
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& c = chunks[i];
        file.insert(file.end(), c.id, c.id + 4);
        WriteU64(file, offsets[i]);
        WriteU64(file, static_cast<uint64_t>(c.data.size()));
        WriteU64(file, c.uncompressed_length);
        uint32_t crc = CRC32(c.data.data(), c.data.size());
        WriteU32(file, crc);
        WriteU16(file, c.flags);
        file.insert(file.end(), 6, 0);
    }

    uint64_t file_length = static_cast<uint64_t>(file.size());

    // Write header
    std::vector<uint8_t> header;
    header.reserve(kHeaderSize);
    WriteU32(header, kMagic);
    WriteU16(header, kVersionMajor);
    WriteU16(header, kVersionMinor);
    WriteU8(header, kEndianness);
    WriteU8(header, 0); // flags
    WriteU16(header, kHeaderSize);
    WriteU16(header, kTocEntrySize);
    WriteU16(header, 0); // reserved
    WriteU64(header, file_length);
    WriteU32(header, 0); // header crc placeholder
    WriteU64(header, toc_offset);
    WriteU32(header, static_cast<uint32_t>(chunks.size()));
    WriteU32(header, 0); // reserved2

    std::vector<uint8_t> header_crc_calc = header;
    header_crc_calc[24] = 0;
    header_crc_calc[25] = 0;
    header_crc_calc[26] = 0;
    header_crc_calc[27] = 0;
    uint32_t header_crc = CRC32(header_crc_calc.data(), header_crc_calc.size());
    header[24] = static_cast<uint8_t>(header_crc & 0xFF);
    header[25] = static_cast<uint8_t>((header_crc >> 8) & 0xFF);
    header[26] = static_cast<uint8_t>((header_crc >> 16) & 0xFF);
    header[27] = static_cast<uint8_t>((header_crc >> 24) & 0xFF);

    std::copy(header.begin(), header.end(), file.begin());

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        if (error) *error = "Failed to open project file for write";
        return false;
    }
    out.write(reinterpret_cast<const char*>(file.data()), static_cast<std::streamsize>(file.size()));
    if (!out) {
        if (error) *error = "Failed to write project file";
        return false;
    }
    return true;
}

bool ProjectSerializer::LoadFromFile(Project* project, const std::string& path, std::string* error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (error) *error = "Failed to open project file";
        return false;
    }

    std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (file.size() < kHeaderSize) {
        if (error) *error = "Project file too small";
        return false;
    }

    const uint8_t* p = file.data();
    const uint8_t* end = file.data() + file.size();
    bool ok = true;

    uint32_t magic = ReadU32(p, end, &ok);
    uint16_t ver_major = ReadU16(p, end, &ok);
    uint16_t ver_minor = ReadU16(p, end, &ok);
    uint8_t endian = ReadU8(p, end, &ok);
    uint8_t flags = ReadU8(p, end, &ok);
    (void)flags;
    uint16_t header_size = ReadU16(p, end, &ok);
    uint16_t toc_entry_size = ReadU16(p, end, &ok);
    (void)ReadU16(p, end, &ok); // reserved
    uint64_t file_length = ReadU64(p, end, &ok);
    uint32_t header_crc = ReadU32(p, end, &ok);
    uint64_t toc_offset = ReadU64(p, end, &ok);
    uint32_t toc_count = ReadU32(p, end, &ok);
    (void)ReadU32(p, end, &ok); // reserved2

    if (!ok || magic != kMagic || endian != kEndianness) {
        if (error) *error = "Invalid project file header";
        return false;
    }
    if (ver_major != kVersionMajor) {
        if (error) *error = "Unsupported project version";
        return false;
    }
    if (header_size != kHeaderSize || toc_entry_size != kTocEntrySize) {
        if (error) *error = "Unsupported header format";
        return false;
    }
    if (file_length != file.size()) {
        // tolerate mismatch
    }

    std::vector<uint8_t> header_calc(file.begin(), file.begin() + kHeaderSize);
    header_calc[24] = 0;
    header_calc[25] = 0;
    header_calc[26] = 0;
    header_calc[27] = 0;
    uint32_t computed_crc = CRC32(header_calc.data(), header_calc.size());
    if (computed_crc != header_crc) {
        // If CRC mismatch, still attempt load
    }

    if (toc_offset + (static_cast<uint64_t>(toc_count) * kTocEntrySize) > file.size()) {
        if (error) *error = "Invalid TOC offset";
        return false;
    }

    struct TocEntry {
        char id[4];
        uint64_t offset;
        uint64_t length;
        uint64_t uncompressed_length;
        uint32_t crc;
        uint16_t flags;
    };

    std::vector<TocEntry> entries;
    entries.reserve(toc_count);

    const uint8_t* t = file.data() + toc_offset;
    for (uint32_t i = 0; i < toc_count; ++i) {
        TocEntry e{};
        std::memcpy(e.id, t, 4); t += 4;
        e.offset = ReadU64(t, end, &ok);
        e.length = ReadU64(t, end, &ok);
        e.uncompressed_length = ReadU64(t, end, &ok);
        e.crc = ReadU32(t, end, &ok);
        e.flags = ReadU16(t, end, &ok);
        t += 6; // reserved
        if (!ok) return false;
        entries.push_back(e);
    }

    for (const auto& e : entries) {
        if (e.offset + e.length > file.size()) continue;
        const uint8_t* cdata = file.data() + e.offset;
        uint32_t crc = CRC32(cdata, static_cast<size_t>(e.length));
        if (crc != e.crc) {
            // Skip corrupted chunk
            continue;
        }

        const uint8_t* cp = cdata;
        const uint8_t* cend = cdata + e.length;

        if (std::memcmp(e.id, "PROJ", 4) == 0) {
            if (!ReadProjectConfig(cp, cend, project)) {
                if (error) *error = "Failed to parse PROJ";
                return false;
            }
        } else if (std::memcmp(e.id, "OBJS", 4) == 0) {
            bool ok2 = true;
            uint64_t count = ReadUVarint(cp, cend, &ok2);
            if (!ok2) return false;
            project->objects_by_id.clear();
            project->objects_by_path.clear();
            for (uint64_t i = 0; i < count; ++i) {
                auto obj = std::make_shared<ProjectObject>();
                if (!ReadProjectObject(cp, cend, obj.get())) break;
                project->objects_by_id[obj->id] = obj;
                if (!obj->path.empty()) {
                    project->objects_by_path[obj->path] = obj;
                }
            }
        } else if (std::memcmp(e.id, "GITS", 4) == 0) {
            ReadGitSyncState(cp, cend, &project->sync_state);
        } else if (std::memcmp(e.id, "RPTG", 4) == 0) {
            ReadReportingAssets(cp, cend, &project->reporting_assets);
        } else if (std::memcmp(e.id, "RPTS", 4) == 0) {
            ReadReportingSchedules(cp, cend, &project->reporting_schedules);
        } else if (std::memcmp(e.id, "DVWS", 4) == 0) {
            ReadDataViews(cp, cend, &project->data_views);
        }
    }

    return true;
}

} // namespace scratchrobin
