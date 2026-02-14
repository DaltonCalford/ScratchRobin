#pragma once

#include <string>
#include <vector>

namespace scratchrobin::phases {

struct PhaseModule {
    std::string phase_id;
    std::string title;
    std::string spec_section;
    std::string description;
    std::vector<std::string> dependencies;
};

}  // namespace scratchrobin::phases
