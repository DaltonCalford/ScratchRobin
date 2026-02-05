/**
 * @file test_project_serialization.cpp
 * @brief Unit tests for project serialization
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "core/project.h"
#include "core/project_serialization.h"

using namespace scratchrobin;

class ProjectSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir_ = std::filesystem::temp_directory_path() / "scratchrobin_project_test";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(temp_dir_);
    }

    std::filesystem::path temp_dir_;
};

TEST_F(ProjectSerializationTest, SaveAndLoadProjectFile) {
    Project project;
    ProjectConfig cfg;
    cfg.name = "Test Project";
    cfg.description = "Serialization test";
    cfg.version = "1.0";
    cfg.database_type = "scratchbird";
    project.config = cfg;

    auto obj = std::make_shared<ProjectObject>("table", "orders");
    obj->schema_name = "public";
    obj->path = "public.orders";
    obj->design_file_path = cfg.designs_path + "/public.orders.table.json";
    obj->design_state.state = ObjectState::EXTRACTED;
    obj->has_source = true;

    MetadataNode node;
    node.kind = "table";
    node.label = "orders";
    node.path = "native.public.orders";
    obj->source_snapshot = node;
    obj->current_design = node;

    project.objects_by_id[obj->id] = obj;
    project.objects_by_path[obj->path] = obj;

    std::string path = (temp_dir_ / "project.srproj").string();
    std::string error;
    ASSERT_TRUE(ProjectSerializer::SaveToFile(project, path, &error));

    Project loaded;
    ASSERT_TRUE(ProjectSerializer::LoadFromFile(&loaded, path, &error));

    EXPECT_EQ(loaded.config.name, "Test Project");
    EXPECT_EQ(loaded.objects_by_id.size(), 1u);
    auto it = loaded.objects_by_id.begin();
    EXPECT_EQ(it->second->name, "orders");
    EXPECT_EQ(it->second->schema_name, "public");
    EXPECT_EQ(it->second->path, "public.orders");
}
