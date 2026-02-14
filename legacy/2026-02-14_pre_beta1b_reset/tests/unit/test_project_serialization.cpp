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

    Project::ReportingAsset report;
    report.id = UUID::Generate();
    report.object_type = "question";
    report.json_payload = "{\"id\":\"q1\",\"name\":\"Orders\"}";
    project.reporting_assets.push_back(report);

    Project::DataViewSnapshot view;
    view.id = UUID::Generate();
    view.diagram_id = UUID::Generate();
    view.json_payload = "{\"id\":\"dv1\",\"name\":\"Sample\",\"stale\":false}";
    project.data_views.push_back(view);

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
    ASSERT_EQ(loaded.reporting_assets.size(), 1u);
    EXPECT_EQ(loaded.reporting_assets[0].object_type, "question");
    EXPECT_EQ(loaded.reporting_assets[0].json_payload, report.json_payload);
    ASSERT_EQ(loaded.data_views.size(), 1u);
    EXPECT_EQ(loaded.data_views[0].json_payload, view.json_payload);
}

TEST_F(ProjectSerializationTest, GovernanceConfigRoundTrip) {
    Project project;
    ProjectConfig cfg;
    cfg.name = "Governance Project";
    cfg.database_type = "scratchbird";
    cfg.governance.owners = {"alice"};
    cfg.governance.stewards = {"bob"};
    cfg.governance.compliance_tags = {"soc2", "hipaa"};
    cfg.governance.review_policy.min_reviewers = 2;
    cfg.governance.review_policy.required_roles = {"admin", "security"};
    cfg.governance.review_policy.approval_window_hours = 24;
    cfg.governance.ai_policy.enabled = true;
    cfg.governance.ai_policy.requires_review = false;
    cfg.governance.ai_policy.allowed_scopes = {"docs", "sql"};
    cfg.governance.ai_policy.prohibited_scopes = {"deploy"};
    cfg.governance.audit_policy.log_level = "info";
    cfg.governance.audit_policy.retain_days = 90;
    cfg.governance.audit_policy.export_target = "file";

    ProjectConfig::GovernanceEnvironment env;
    env.id = "prod";
    env.name = "Production";
    env.approval_required = true;
    env.min_reviewers = 2;
    env.allowed_roles = {"admin"};
    cfg.governance.environments.push_back(env);
    project.config = cfg;

    std::string path = (temp_dir_ / "project_governance.srproj").string();
    std::string error;
    ASSERT_TRUE(ProjectSerializer::SaveToFile(project, path, &error));

    Project loaded;
    ASSERT_TRUE(ProjectSerializer::LoadFromFile(&loaded, path, &error));
    EXPECT_EQ(loaded.config.governance.owners.size(), 1u);
    EXPECT_EQ(loaded.config.governance.owners[0], "alice");
    ASSERT_EQ(loaded.config.governance.environments.size(), 1u);
    EXPECT_EQ(loaded.config.governance.environments[0].id, "prod");
    EXPECT_EQ(loaded.config.governance.review_policy.min_reviewers, 2u);
    EXPECT_EQ(loaded.config.governance.ai_policy.enabled, true);
    EXPECT_EQ(loaded.config.governance.audit_policy.retain_days, 90u);
}
