#include "phases/phase01_scope.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase01ScopeModule() {
    return {
        .phase_id = "01",
        .title = "Product Scope and Goals",
        .spec_section = "01_Product_Scope_and_Goals",
        .description = "Implements release scope, non-goals, and gap-closure commitments.",
        .dependencies = {"00"}
    };
}

}  // namespace scratchrobin::phases
