#include "phases/phase05_ui.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase05UiModule() {
    return {
        .phase_id = "05",
        .title = "UI Surface and Workflows",
        .spec_section = "05_UI_Surface_and_Workflows",
        .description = "Implements shell, menus, SQL productivity, compare/migration, plan builder, and spec workspace.",
        .dependencies = {"00", "01", "02", "03", "04"}
    };
}

}  // namespace scratchrobin::phases
