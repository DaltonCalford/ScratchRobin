/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "core/project.h"

namespace scratchrobin {
namespace {

std::filesystem::path MakeTempDir(const std::string& name) {
    auto dir = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

void WriteFile(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << content;
}

} // namespace

TEST(ProjectTemplateDiscoveryTest, DiscoversTemplatesUnderDocs) {
    auto temp_dir = MakeTempDir("scratchrobin_template_discovery");
    Project project;
    ProjectConfig cfg;
    cfg.name = "Template Test";
    cfg.docs_path = "docs";
    ASSERT_TRUE(project.CreateNew(temp_dir.string(), cfg));

    auto templates_dir = temp_dir / "docs" / "templates";
    WriteFile(templates_dir / "mop_template.yaml", "steps: []");
    WriteFile(templates_dir / "rollback.md", "# Rollback");
    WriteFile(templates_dir / "reporting.json", "{}");

    std::string error;
    auto templates = project.DiscoverTemplates(&error);
    EXPECT_TRUE(error.empty());
    ASSERT_GE(templates.size(), 3u);

    bool found_mop = false;
    bool found_rollback = false;
    bool found_report = false;
    for (const auto& t : templates) {
        if (t.name == "mop_template") {
            found_mop = (t.kind == "mop");
        }
        if (t.name == "rollback") {
            found_rollback = (t.kind == "rollback");
        }
        if (t.name == "reporting") {
            found_report = (t.kind == "report");
        }
    }
    EXPECT_TRUE(found_mop);
    EXPECT_TRUE(found_rollback);
    EXPECT_TRUE(found_report);
}

} // namespace scratchrobin
