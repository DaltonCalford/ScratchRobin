#include "phases/phase00_governance.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase00GovernanceModule() {
    return {
        .phase_id = "00",
        .title = "Governance and Scaffolding",
        .spec_section = "00_Governance_and_Invariants",
        .description = "Core invariants, reject-code policy, and phase execution order bootstrap.",
        .dependencies = {}
    };
}

}  // namespace scratchrobin::phases
