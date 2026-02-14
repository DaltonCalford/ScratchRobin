#include "phases/phase06_diagramming.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase06DiagrammingModule() {
    return {
        .phase_id = "06",
        .title = "Diagramming and Visual Modeling",
        .spec_section = "06_Diagramming_and_Visual_Modeling",
        .description = "Implements diagram models, notations, canvas commands, and forward/reverse engineering.",
        .dependencies = {"00", "01", "02", "03", "04", "05"}
    };
}

}  // namespace scratchrobin::phases
