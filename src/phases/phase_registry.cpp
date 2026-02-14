#include "phases/phase_registry.h"

#include "phases/phase00_governance.h"
#include "phases/phase01_scope.h"
#include "phases/phase02_runtime.h"
#include "phases/phase03_project_data.h"
#include "phases/phase04_backend.h"
#include "phases/phase05_ui.h"
#include "phases/phase06_diagramming.h"
#include "phases/phase07_reporting.h"
#include "phases/phase08_advanced.h"
#include "phases/phase09_packaging.h"
#include "phases/phase10_conformance_release.h"

namespace scratchrobin::phases {

std::vector<PhaseModule> BuildPhaseScaffold() {
    return {
        BuildPhase00GovernanceModule(),
        BuildPhase01ScopeModule(),
        BuildPhase02RuntimeModule(),
        BuildPhase03ProjectDataModule(),
        BuildPhase04BackendModule(),
        BuildPhase05UiModule(),
        BuildPhase06DiagrammingModule(),
        BuildPhase07ReportingModule(),
        BuildPhase08AdvancedModule(),
        BuildPhase09PackagingModule(),
        BuildPhase10ConformanceReleaseModule()
    };
}

}  // namespace scratchrobin::phases
