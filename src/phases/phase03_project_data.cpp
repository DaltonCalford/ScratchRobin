#include "phases/phase03_project_data.h"

namespace scratchrobin::phases {

PhaseModule BuildPhase03ProjectDataModule() {
    return {
        .phase_id = "03",
        .title = "Project and Data Model",
        .spec_section = "03_Project_and_Data_Model",
        .description = "Implements project model, binary format, governance/audit model, and specset package model.",
        .dependencies = {"00", "01", "02"}
    };
}

}  // namespace scratchrobin::phases
