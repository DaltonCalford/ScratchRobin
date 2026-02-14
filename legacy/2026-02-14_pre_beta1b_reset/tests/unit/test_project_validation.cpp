/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include <gtest/gtest.h>

#include "core/project.h"

namespace scratchrobin {

TEST(ProjectValidationTest, ReportsMissingNameAndRoot) {
    Project project;
    auto result = project.Validate();
    EXPECT_FALSE(result.valid);
    EXPECT_GE(result.errors.size(), 2u);
}

TEST(ProjectValidationTest, ValidProjectProducesWarnings) {
    Project project;
    ProjectConfig cfg;
    cfg.name = "Validation";
    project.config = cfg;
    project.project_root_path = "/tmp/scratchrobin_validation";
    auto result = project.Validate();
    EXPECT_TRUE(result.valid);
    EXPECT_FALSE(result.warnings.empty());
}

} // namespace scratchrobin
