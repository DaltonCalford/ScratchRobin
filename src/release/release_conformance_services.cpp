#include "release/release_conformance_services.h"

#include <fstream>

#include "core/reject.h"

namespace scratchrobin::release {

namespace {

std::vector<std::string> SplitBlockerCsvLine(const std::string& line) {
    std::vector<std::string> cols;
    cols.reserve(9);

    std::size_t start = 0;
    for (int i = 0; i < 8; ++i) {
        const std::size_t comma = line.find(',', start);
        if (comma == std::string::npos) {
            return {};
        }
        cols.push_back(line.substr(start, comma - start));
        start = comma + 1;
    }
    cols.push_back(line.substr(start));
    if (cols.size() != 9U) {
        return {};
    }
    return cols;
}

bool IsUnresolved(const beta1b::BlockerRow& row) {
    return row.status == "open" || row.status == "mitigated";
}

}  // namespace

std::vector<beta1b::BlockerRow> ReleaseConformanceService::LoadBlockerRegister(const std::string& csv_path) const {
    std::ifstream in(csv_path, std::ios::binary);
    if (!in) {
        throw MakeReject("SRB1-R-5407", "unable to read blocker register", "governance", "load_blocker_register", false,
                         csv_path);
    }

    std::string header;
    if (!std::getline(in, header)) {
        throw MakeReject("SRB1-R-5407", "invalid blocker register header", "governance", "load_blocker_register");
    }
    const std::string expected =
        "blocker_id,severity,status,source_type,source_id,opened_at,updated_at,owner,summary";
    if (header != expected) {
        throw MakeReject("SRB1-R-5407", "invalid blocker register header", "governance", "load_blocker_register");
    }

    std::vector<beta1b::BlockerRow> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const auto cols = SplitBlockerCsvLine(line);
        if (cols.size() != 9U) {
            throw MakeReject("SRB1-R-5407", "invalid blocker row format", "governance", "load_blocker_register", false,
                             line);
        }
        beta1b::BlockerRow row;
        row.blocker_id = cols[0];
        row.severity = cols[1];
        row.status = cols[2];
        row.source_type = cols[3];
        row.source_id = cols[4];
        row.opened_at = cols[5];
        row.updated_at = cols[6];
        row.owner = cols[7];
        row.summary = cols[8];
        rows.push_back(std::move(row));
    }
    return rows;
}

void ReleaseConformanceService::ValidateBlockerRegister(const std::vector<beta1b::BlockerRow>& rows) const {
    beta1b::ValidateBlockerRows(rows);
}

GateDecision ReleaseConformanceService::EvaluatePhaseAcceptance(const std::vector<beta1b::BlockerRow>& rows) const {
    ValidateBlockerRegister(rows);
    GateDecision out;
    for (const auto& row : rows) {
        if (row.severity == "P0" && IsUnresolved(row)) {
            out.blocking_blocker_ids.push_back(row.blocker_id);
        }
    }
    out.pass = out.blocking_blocker_ids.empty();
    out.reason = out.pass ? "pass" : "unresolved_p0_blockers";
    return out;
}

GateDecision ReleaseConformanceService::EvaluateRcEntry(const std::vector<beta1b::BlockerRow>& rows) const {
    ValidateBlockerRegister(rows);
    GateDecision out;
    for (const auto& row : rows) {
        if ((row.severity == "P0" || row.severity == "P1") && IsUnresolved(row)) {
            out.blocking_blocker_ids.push_back(row.blocker_id);
        }
    }
    out.pass = out.blocking_blocker_ids.empty();
    out.reason = out.pass ? "pass" : "unresolved_p0_p1_blockers";
    return out;
}

void ReleaseConformanceService::ValidateAlphaMirrorPresence(
    const std::string& mirror_root,
    const std::vector<beta1b::AlphaMirrorEntry>& entries) const {
    beta1b::ValidateAlphaMirrorPresence(mirror_root, entries);
}

void ReleaseConformanceService::ValidateAlphaMirrorHashes(
    const std::string& mirror_root,
    const std::vector<beta1b::AlphaMirrorEntry>& entries) const {
    beta1b::ValidateAlphaMirrorHashes(mirror_root, entries);
}

void ReleaseConformanceService::ValidateSilverstonContinuity(
    const std::set<std::string>& present_artifacts,
    const std::set<std::string>& required_artifacts) const {
    beta1b::ValidateSilverstonContinuity(present_artifacts, required_artifacts);
}

void ReleaseConformanceService::ValidateAlphaInventoryMapping(
    const std::set<std::string>& required_element_ids,
    const std::map<std::string, std::string>& file_to_element_id) const {
    beta1b::ValidateAlphaInventoryMapping(required_element_ids, file_to_element_id);
}

void ReleaseConformanceService::ValidateAlphaExtractionGate(bool extraction_passed,
                                                            bool continuity_passed,
                                                            bool deep_contract_passed) const {
    beta1b::ValidateAlphaExtractionGate(extraction_passed, continuity_passed, deep_contract_passed);
}

}  // namespace scratchrobin::release
