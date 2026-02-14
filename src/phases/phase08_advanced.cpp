#include "phases/phase08_advanced.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase08AdvancedModule() {
    return {
        .phase_id = "08",
        .title = "Advanced Features and Integrations",
        .spec_section = "08_Advanced_Features_and_Integrations",
        .description = "Implements AI/issue-tracker/CDC/masking, collaboration/review, extension runtime, and preview gating.",
        .dependencies = {"00", "01", "02", "03", "04", "05", "07"}
    };
}

}  // namespace scratchrobin::phases
