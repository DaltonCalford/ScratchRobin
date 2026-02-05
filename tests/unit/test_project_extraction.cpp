/**
 * @file test_project_extraction.cpp
 * @brief Unit tests for project extraction using fixtures
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "core/project.h"

using namespace scratchrobin;

TEST(ProjectExtractionTest, ExtractFromFixture) {
    Project project;
    ProjectConfig cfg;
    cfg.name = "Extract Test";
    cfg.database_type = "scratchbird";
    project.config = cfg;

    std::string fixture = std::filesystem::path(SCRATCHROBIN_TEST_SOURCE_DIR)
                              .append("tests/fixtures/metadata_complex.json")
                              .string();

    DatabaseConnection conn;
    conn.connection_string = std::string("fixture:") + fixture;

    ASSERT_TRUE(project.ExtractFromDatabase(conn, {}));
    EXPECT_GT(project.objects_by_id.size(), 0u);

    // Expect schema and table objects present
    bool found_schema = false;
    bool found_table = false;
    bool found_column = false;

    for (const auto& pair : project.objects_by_id) {
        const auto& obj = pair.second;
        if (obj->kind == "schema" && obj->name == "public") {
            found_schema = true;
        }
        if (obj->kind == "table" && obj->name == "orders") {
            found_table = true;
        }
        if (obj->kind == "column" && obj->name == "order_id") {
            found_column = true;
        }
    }

    EXPECT_TRUE(found_schema);
    EXPECT_TRUE(found_table);
    EXPECT_TRUE(found_column);
}
