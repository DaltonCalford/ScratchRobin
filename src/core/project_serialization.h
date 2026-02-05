#ifndef SCRATCHROBIN_PROJECT_SERIALIZATION_H
#define SCRATCHROBIN_PROJECT_SERIALIZATION_H

#include <cstdint>
#include <string>
#include <vector>

#include "project.h"

namespace scratchrobin {

class ProjectSerializer {
public:
    static bool SaveToFile(const Project& project, const std::string& path, std::string* error);
    static bool LoadFromFile(Project* project, const std::string& path, std::string* error);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PROJECT_SERIALIZATION_H
