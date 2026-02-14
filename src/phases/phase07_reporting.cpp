#include "phases/phase07_reporting.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase07ReportingModule() {
    return {
        .phase_id = "07",
        .title = "Reporting, Governance, and Analytics",
        .spec_section = "07_Reporting_Governance_and_Analytics",
        .description = "Implements reporting runtime, RRULE scheduling, governance gates, and support dashboards.",
        .dependencies = {"00", "01", "02", "03", "04", "05"}
    };
}

}  // namespace scratchrobin::phases
