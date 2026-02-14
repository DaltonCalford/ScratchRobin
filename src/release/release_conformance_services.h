#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "core/beta1b_contracts.h"

namespace scratchrobin::release {

struct GateDecision {
    bool pass = false;
    std::string reason;
    std::vector<std::string> blocking_blocker_ids;
};

struct PromotabilityVerdict {
    GateDecision phase_gate;
    GateDecision rc_gate;
    bool promotable = false;
};

class ReleaseConformanceService {
public:
    std::vector<beta1b::BlockerRow> LoadBlockerRegister(const std::string& csv_path) const;
    void ValidateBlockerRegister(const std::vector<beta1b::BlockerRow>& rows) const;

    GateDecision EvaluatePhaseAcceptance(const std::vector<beta1b::BlockerRow>& rows) const;
    GateDecision EvaluateRcEntry(const std::vector<beta1b::BlockerRow>& rows) const;
    PromotabilityVerdict EvaluatePromotability(const std::vector<beta1b::BlockerRow>& rows) const;
    std::string ExportPromotabilityJson(const PromotabilityVerdict& verdict) const;

    void ValidateAlphaMirrorPresence(const std::string& mirror_root,
                                     const std::vector<beta1b::AlphaMirrorEntry>& entries) const;
    void ValidateAlphaMirrorHashes(const std::string& mirror_root,
                                   const std::vector<beta1b::AlphaMirrorEntry>& entries) const;
    void ValidateSilverstonContinuity(const std::set<std::string>& present_artifacts,
                                      const std::set<std::string>& required_artifacts) const;
    void ValidateAlphaInventoryMapping(const std::set<std::string>& required_element_ids,
                                       const std::map<std::string, std::string>& file_to_element_id) const;
    void ValidateAlphaExtractionGate(bool extraction_passed,
                                     bool continuity_passed,
                                     bool deep_contract_passed) const;
};

}  // namespace scratchrobin::release
