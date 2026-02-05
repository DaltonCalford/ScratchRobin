/**
 * @file test_project_extraction_kinds.cpp
 * @brief Extraction tests for views, procedures, and triggers
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "core/project.h"

using namespace scratchrobin;

TEST(ProjectExtractionKindsTest, ExtractViewsProceduresTriggers) {
    Project project;
    project.config.database_type = "scratchbird";

    std::string fixture = std::filesystem::path(SCRATCHROBIN_TEST_SOURCE_DIR)
                              .append("tests/fixtures/metadata_rich.json")
                              .string();

    DatabaseConnection conn;
    conn.connection_string = std::string("fixture:") + fixture;

    ASSERT_TRUE(project.ExtractFromDatabase(conn, {}));

    bool found_view = false;
    bool found_proc = false;
    bool found_trigger = false;

    for (const auto& pair : project.objects_by_id) {
        const auto& obj = pair.second;
        if (obj->kind == "view" && obj->name == "orders_view") found_view = true;
        if (obj->kind == "procedure" && obj->name == "calc_total") found_proc = true;
        if (obj->kind == "trigger" && obj->name == "orders_insert") found_trigger = true;
    }

    EXPECT_TRUE(found_view);
    EXPECT_TRUE(found_proc);
    EXPECT_TRUE(found_trigger);
}
