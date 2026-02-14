#include "phases/phase09_packaging.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase09PackagingModule() {
    return {
        .phase_id = "09",
        .title = "Build, Test, Packaging, and Distribution",
        .spec_section = "09_Build_Test_Packaging_and_Distribution",
        .description = "Implements profile builds, package manifests, strict surface-id registry checks, and installer outputs.",
        .dependencies = {"00", "01", "02", "03", "04", "05", "06", "07", "08"}
    };
}

}  // namespace scratchrobin::phases
