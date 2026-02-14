/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#ifndef SCRATCHROBIN_DOCUMENTATION_GENERATOR_H
#define SCRATCHROBIN_DOCUMENTATION_GENERATOR_H

#include <string>

namespace scratchrobin {

class Project;

class DocumentationGenerator {
public:
    static bool Generate(Project& project, const std::string& output_dir, std::string* error);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DOCUMENTATION_GENERATOR_H
