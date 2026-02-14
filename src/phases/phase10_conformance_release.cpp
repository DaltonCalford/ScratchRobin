#include "phases/phase10_conformance_release.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase10ConformanceReleaseModule() {
    return {
        .phase_id = "10",
        .title = "Execution Tracks, Alpha Preservation, and Release Gates",
        .spec_section = "10_Execution_Tracks_and_Conformance + 11_Alpha_Material_Deep_Preservation",
        .description = "Implements conformance execution, blocker gating, alpha continuity verification, and RC decision hooks.",
        .dependencies = {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09"}
    };
}

}  // namespace scratchrobin::phases
