#include "project/project_services.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/reject.h"

namespace scratchrobin::project {

namespace {

constexpr std::uint16_t kHeaderSize = 44;
constexpr std::uint16_t kTocEntrySize = 40;

void WriteU16(std::vector<std::uint8_t>* out, std::uint16_t v) {
    out->push_back(static_cast<std::uint8_t>(v & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void WriteU32(std::vector<std::uint8_t>* out, std::uint32_t v) {
    out->push_back(static_cast<std::uint8_t>(v & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

void WriteU64(std::vector<std::uint8_t>* out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        out->push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF));
    }
}

void OverwriteU16(std::vector<std::uint8_t>* out, std::size_t offset, std::uint16_t v) {
    (*out)[offset] = static_cast<std::uint8_t>(v & 0xFF);
    (*out)[offset + 1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
}

void OverwriteU32(std::vector<std::uint8_t>* out, std::size_t offset, std::uint32_t v) {
    (*out)[offset] = static_cast<std::uint8_t>(v & 0xFF);
    (*out)[offset + 1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
    (*out)[offset + 2] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
    (*out)[offset + 3] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
}

void OverwriteU64(std::vector<std::uint8_t>* out, std::size_t offset, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        (*out)[offset + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF);
    }
}

struct ChunkRow {
    std::string chunk_id;
    std::vector<std::uint8_t> payload;
    std::uint64_t offset = 0;
    std::uint32_t ordinal = 0;
};

void WriteAuditBestEffort(const std::string& audit_path, const std::string& event_json_line) {
    std::ofstream out(audit_path, std::ios::binary | std::ios::app);
    if (!out) {
        return;
    }
    out << event_json_line << '\n';
}

std::string BoolJson(bool value) {
    return value ? "true" : "false";
}

}  // namespace

std::vector<std::uint8_t> ProjectBinaryService::BuildBinary(
    const std::vector<std::uint8_t>& proj_payload,
    const std::vector<std::uint8_t>& objs_payload,
    const std::map<std::string, std::vector<std::uint8_t>>& optional_chunks) const {
    if (proj_payload.empty() || objs_payload.empty()) {
        throw MakeReject("SRB1-R-3101", "mandatory payload empty", "project", "build_binary");
    }

    std::vector<ChunkRow> chunks;
    chunks.push_back({"PROJ", proj_payload, 0, 0});
    chunks.push_back({"OBJS", objs_payload, 0, 1});

    std::uint32_t ordinal = 2;
    for (const auto& [chunk_id, payload] : optional_chunks) {
        if (chunk_id.size() != 4U || payload.empty()) {
            throw MakeReject("SRB1-R-3101", "invalid optional chunk contract", "project", "build_binary", false, chunk_id);
        }
        chunks.push_back({chunk_id, payload, 0, ordinal++});
    }

    const std::uint64_t header_size = kHeaderSize;
    const std::uint64_t toc_size = static_cast<std::uint64_t>(chunks.size()) * kTocEntrySize;
    std::uint64_t payload_offset = header_size + toc_size;
    for (auto& row : chunks) {
        row.offset = payload_offset;
        payload_offset += static_cast<std::uint64_t>(row.payload.size());
    }
    const std::uint64_t file_size = payload_offset;

    std::vector<std::uint8_t> bytes;
    bytes.reserve(static_cast<std::size_t>(file_size));
    for (std::size_t i = 0; i < kHeaderSize; ++i) {
        bytes.push_back(0);
    }

    for (const auto& row : chunks) {
        bytes.push_back(static_cast<std::uint8_t>(row.chunk_id[0]));
        bytes.push_back(static_cast<std::uint8_t>(row.chunk_id[1]));
        bytes.push_back(static_cast<std::uint8_t>(row.chunk_id[2]));
        bytes.push_back(static_cast<std::uint8_t>(row.chunk_id[3]));
        WriteU32(&bytes, 0);
        WriteU64(&bytes, row.offset);
        WriteU64(&bytes, static_cast<std::uint64_t>(row.payload.size()));
        WriteU32(&bytes, beta1b::Crc32(row.payload.data(), row.payload.size()));
        WriteU16(&bytes, 1);
        WriteU16(&bytes, 0);
        WriteU32(&bytes, row.ordinal);
        WriteU32(&bytes, 0);
    }

    for (const auto& row : chunks) {
        bytes.insert(bytes.end(), row.payload.begin(), row.payload.end());
    }

    bytes[0] = 'S'; bytes[1] = 'R'; bytes[2] = 'P'; bytes[3] = 'J';
    OverwriteU16(&bytes, 4, 1);
    OverwriteU16(&bytes, 6, 0);
    OverwriteU16(&bytes, 8, kHeaderSize);
    OverwriteU16(&bytes, 10, kTocEntrySize);
    OverwriteU32(&bytes, 12, static_cast<std::uint32_t>(chunks.size()));
    OverwriteU64(&bytes, 16, kHeaderSize);
    OverwriteU64(&bytes, 24, file_size);
    OverwriteU32(&bytes, 32, 0);
    OverwriteU32(&bytes, 36, 0);

    std::array<std::uint8_t, 44> header_copy{};
    std::copy(bytes.begin(), bytes.begin() + 44, header_copy.begin());
    header_copy[40] = header_copy[41] = header_copy[42] = header_copy[43] = 0;
    OverwriteU32(&bytes, 40, beta1b::Crc32(header_copy.data(), header_copy.size()));
    return bytes;
}

ProjectRoundTripResult ProjectBinaryService::RoundTripFile(
    const std::string& path,
    const std::vector<std::uint8_t>& proj_payload,
    const std::vector<std::uint8_t>& objs_payload,
    const std::map<std::string, std::vector<std::uint8_t>>& optional_chunks) const {
    const std::vector<std::uint8_t> bytes = BuildBinary(proj_payload, objs_payload, optional_chunks);

    const std::filesystem::path out_path(path);
    std::error_code ec;
    std::filesystem::create_directories(out_path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw MakeReject("SRB1-R-3101", "failed to open output path", "project", "roundtrip_file", false, path);
    }
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    out.flush();
    if (!out) {
        throw MakeReject("SRB1-R-3101", "failed to write binary", "project", "roundtrip_file", false, path);
    }

    const auto loaded = LoadFile(path);
    ProjectRoundTripResult result;
    result.bytes_written = bytes.size();
    result.toc_entries = loaded.toc.size();
    result.loaded_chunks = loaded.loaded_chunks;
    return result;
}

beta1b::LoadedProjectBinary ProjectBinaryService::LoadFile(const std::string& path) const {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw MakeReject("SRB1-R-3101", "failed to read binary", "project", "load_file", false, path);
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return beta1b::LoadProjectBinary(bytes);
}

void ValidateProjectPayloadWithSchema(const std::string& schema_path, const JsonValue& payload) {
    if (!std::filesystem::exists(schema_path)) {
        throw MakeReject("SRB1-R-3002", "project schema not found", "project", "validate_payload_schema", false, schema_path);
    }
    beta1b::ValidateProjectPayload(payload);
}

void ValidateSpecsetPayloadWithSchema(const std::string& schema_path, const JsonValue& payload) {
    if (!std::filesystem::exists(schema_path)) {
        throw MakeReject("SRB1-R-5402", "specset schema not found", "spec_workspace", "validate_payload_schema", false, schema_path);
    }
    beta1b::ValidateSpecsetPayload(payload);
}

GovernanceDecision EvaluateGovernance(const GovernanceInput& input, const GovernancePolicy& policy) {
    GovernanceDecision decision;
    decision.allowed = false;

    if (policy.allowed_roles.count(input.actor_role) == 0U) {
        decision.reason = "actor role not allowed";
        return decision;
    }
    if (input.approval_count < policy.min_approval_count) {
        decision.reason = "approval count below minimum";
        return decision;
    }
    if (input.ai_action) {
        if (!policy.ai_enabled) {
            decision.reason = "AI actions disabled";
            return decision;
        }
        if (policy.ai_requires_review && input.approval_count < policy.min_approval_count) {
            decision.reason = "AI action requires review";
            return decision;
        }
        if (!input.ai_scope.empty() && !policy.ai_allowed_scopes.empty() &&
            policy.ai_allowed_scopes.count(input.ai_scope) == 0U) {
            decision.reason = "AI scope denied";
            return decision;
        }
    }

    decision.allowed = true;
    decision.reason = "allowed";
    return decision;
}

void ExecuteGovernedOperation(const GovernanceInput& input,
                              const GovernancePolicy& policy,
                              const std::string& audit_path,
                              const std::function<void()>& operation) {
    const GovernanceDecision decision = EvaluateGovernance(input, policy);

    std::ostringstream event;
    event << "{\"timestamp\":\"2026-02-14T00:00:00Z\""
          << ",\"actor\":\"" << input.actor << "\""
          << ",\"action\":\"" << input.action << "\""
          << ",\"target_id\":\"" << input.target_id << "\""
          << ",\"connection_ref\":\"" << input.connection_ref << "\""
          << ",\"success\":" << BoolJson(decision.allowed)
          << ",\"detail\":\"" << decision.reason << "\"}";

    if (input.requires_guaranteed_audit) {
        beta1b::WriteAuditRequired(audit_path, event.str());
    } else {
        WriteAuditBestEffort(audit_path, event.str());
    }

    if (!decision.allowed) {
        throw MakeReject("SRB1-R-3202", "governance policy denied action", "governance", "execute_governed_operation",
                         false, decision.reason);
    }
    operation();
}

SpecSetIndex SpecSetService::BuildIndex(const std::string& manifest_path, const std::string& indexed_at_utc) const {
    if (indexed_at_utc.empty()) {
        throw MakeReject("SRB1-R-5402", "indexed_at_utc required", "spec_workspace", "build_index");
    }
    SpecSetIndex index;
    index.manifest = beta1b::LoadSpecsetManifest(manifest_path);
    if (index.manifest.set_id != "sb_v3" && index.manifest.set_id != "sb_vnext" && index.manifest.set_id != "sb_beta1") {
        throw MakeReject("SRB1-R-5401", "unknown/unsupported ScratchBird specification set id", "spec_workspace", "build_index",
                         false, index.manifest.set_id);
    }
    index.files = beta1b::LoadSpecsetPackage(manifest_path);
    index.indexed_at_utc = indexed_at_utc;
    return index;
}

void SpecSetService::AssertCoverageComplete(
    const SpecSetIndex& index,
    const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links,
    const std::string& coverage_class) const {
    beta1b::AssertSupportComplete(index.files, coverage_links, coverage_class);
}

void SpecSetService::ValidateConformanceBindings(const std::vector<std::string>& binding_case_ids,
                                                 const std::set<std::string>& conformance_case_ids) const {
    beta1b::ValidateBindings(binding_case_ids, conformance_case_ids);
}

std::map<std::string, int> SpecSetService::CoverageSummary(
    const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links) const {
    return beta1b::AggregateSupport(coverage_links);
}

std::string SpecSetService::ExportImplementationWorkPackage(
    const std::string& set_id,
    const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& gaps,
    const std::string& generated_at_utc) const {
    return beta1b::ExportWorkPackage(set_id, gaps, generated_at_utc);
}

}  // namespace scratchrobin::project

