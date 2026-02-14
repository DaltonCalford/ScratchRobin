#include "phases/phase02_runtime.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase02RuntimeModule() {
    return {
        .phase_id = "02",
        .title = "Runtime Architecture",
        .spec_section = "02_Runtime_Architecture",
        .description = "Creates process model, component contracts, and threading/job execution framework.",
        .dependencies = {"00", "01"}
    };
}

}  // namespace scratchrobin::phases
