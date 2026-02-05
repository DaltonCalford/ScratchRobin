/**
 * @file test_project_extraction_patterns.cpp
 * @brief Pattern filtering tests for project extraction
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "core/project.h"

using namespace scratchrobin;

static std::string FixturePath(const std::string& name) {
    return std::filesystem::path(SCRATCHROBIN_TEST_SOURCE_DIR)
        .append("tests/fixtures/")
        .append(name)
        .string();
}

TEST(ProjectExtractionPatternsTest, KindFilterOnlyTables) {
    Project project;
    project.config.database_type = "scratchbird";

    DatabaseConnection conn;
    conn.connection_string = std::string("fixture:") + FixturePath("metadata_rich.json");

    ASSERT_TRUE(project.ExtractFromDatabase(conn, {"kind:table"}));

    bool found_table = false;
    bool found_view = false;
    bool found_proc = false;

    for (const auto& pair : project.objects_by_id) {
        const auto& obj = pair.second;
        if (obj->kind == "table") found_table = true;
        if (obj->kind == "view") found_view = true;
        if (obj->kind == "procedure") found_proc = true;
    }

    EXPECT_TRUE(found_table);
    EXPECT_FALSE(found_view);
    EXPECT_FALSE(found_proc);
}

TEST(ProjectExtractionPatternsTest, SchemaFilter) {
    Project project;
    project.config.database_type = "scratchbird";

    DatabaseConnection conn;
    conn.connection_string = std::string("fixture:") + FixturePath("metadata_multicatalog.json");

    ASSERT_TRUE(project.ExtractFromDatabase(conn, {"schema:public"}));

    for (const auto& pair : project.objects_by_id) {
        const auto& obj = pair.second;
        EXPECT_EQ(obj->schema_name, "public");
    }
}

TEST(ProjectExtractionPatternsTest, TableNameFilter) {
    Project project;
    project.config.database_type = "scratchbird";

    DatabaseConnection conn;
    conn.connection_string = std::string("fixture:") + FixturePath("metadata_rich.json");

    ASSERT_TRUE(project.ExtractFromDatabase(conn, {"table:orders"}));

    bool found_orders = false;
    bool found_other = false;

    for (const auto& pair : project.objects_by_id) {
        const auto& obj = pair.second;
        if (obj->kind == "table" && obj->name == "orders") found_orders = true;
        if (obj->kind == "view" || obj->kind == "procedure" || obj->kind == "trigger") found_other = true;
    }

    EXPECT_TRUE(found_orders);
    EXPECT_FALSE(found_other);
}
