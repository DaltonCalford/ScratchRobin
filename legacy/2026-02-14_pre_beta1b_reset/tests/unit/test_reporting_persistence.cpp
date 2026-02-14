/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <ctime>

#include "core/project.h"
#include "core/project_serialization.h"
#include "core/simple_json.h"

using namespace scratchrobin;

namespace {

std::filesystem::path MakeTempDir(const std::string& name) {
    auto dir = std::filesystem::temp_directory_path() / name;
    std::filesystem::create_directories(dir);
    return dir;
}

bool GetJsonBool(const std::string& json, const std::string& key, bool* out) {
    JsonParser parser(json);
    JsonValue root;
    std::string error;
    if (!parser.Parse(&root, &error)) {
        return false;
    }
    const JsonValue* value = FindMember(root, key);
    if (!value || value->type != JsonValue::Type::Bool) {
        return false;
    }
    *out = value->bool_value;
    return true;
}

} // namespace

TEST(ReportingPersistenceTest, UpsertReportingAssets) {
    Project project;
    Project::ReportingAsset asset;
    asset.id = UUID::Generate();
    asset.object_type = "dashboard";
    asset.json_payload = "{\"id\":\"d1\"}";

    auto* first = project.UpsertReportingAsset(asset);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(project.reporting_assets.size(), 1u);

    asset.json_payload = "{\"id\":\"d1\",\"title\":\"Sales\"}";
    auto* second = project.UpsertReportingAsset(asset);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(project.reporting_assets.size(), 1u);
    EXPECT_EQ(project.reporting_assets[0].json_payload, asset.json_payload);
}

TEST(ReportingPersistenceTest, ReportAndDataViewRoundTrip) {
    Project project;
    ProjectConfig cfg;
    cfg.name = "ReportingTest";
    cfg.database_type = "scratchbird";
    project.config = cfg;

    Project::ReportingAsset asset;
    asset.id = UUID::Generate();
    asset.object_type = "question";
    asset.json_payload = "{\"id\":\"q1\",\"name\":\"Orders\"}";
    project.reporting_assets.push_back(asset);

    Project::DataViewSnapshot view;
    view.id = UUID::Generate();
    view.diagram_id = UUID::Generate();
    view.json_payload = "{\"id\":\"dv1\",\"query\":\"select * from orders\",\"stale\":false}";
    project.data_views.push_back(view);

    Project::ReportingSchedule schedule;
    schedule.id = UUID::Generate();
    schedule.action = "report:query";
    schedule.target_id = "report:query";
    schedule.schedule_spec = "hourly";
    schedule.interval_seconds = 3600;
    schedule.enabled = true;
    project.reporting_schedules.push_back(schedule);

    auto temp_dir = MakeTempDir("scratchrobin_reporting_persistence");
    std::string path = (temp_dir / "project.srproj").string();
    std::string error;
    ASSERT_TRUE(ProjectSerializer::SaveToFile(project, path, &error));

    Project loaded;
    ASSERT_TRUE(ProjectSerializer::LoadFromFile(&loaded, path, &error));
    ASSERT_EQ(loaded.reporting_assets.size(), 1u);
    EXPECT_EQ(loaded.reporting_assets[0].json_payload, asset.json_payload);
    ASSERT_EQ(loaded.data_views.size(), 1u);
    EXPECT_EQ(loaded.data_views[0].json_payload, view.json_payload);
    ASSERT_EQ(loaded.reporting_schedules.size(), 1u);
    EXPECT_EQ(loaded.reporting_schedules[0].schedule_spec, "hourly");
    EXPECT_EQ(loaded.reporting_schedules[0].interval_seconds, 3600);
}

TEST(ReportingPersistenceTest, DataViewInvalidationMatchesQuery) {
    Project project;
    Project::DataViewSnapshot view;
    view.id = UUID::Generate();
    view.diagram_id = UUID::Generate();
    view.json_payload = "{\"id\":\"dv1\",\"query\":\"select * from public.orders\",\"stale\":false}";
    project.data_views.push_back(view);

    Project::DataViewSnapshot view2;
    view2.id = UUID::Generate();
    view2.diagram_id = UUID::Generate();
    view2.json_payload = "{\"id\":\"dv2\",\"query\":\"select * from customers\",\"stale\":false}";
    project.data_views.push_back(view2);

    auto obj = project.CreateObject("table", "orders", "public");
    MetadataNode node;
    node.kind = "table";
    node.label = "orders";
    project.ModifyObject(obj->id, node);

    bool stale1 = false;
    bool stale2 = false;
    ASSERT_TRUE(GetJsonBool(project.data_views[0].json_payload, "stale", &stale1));
    ASSERT_TRUE(GetJsonBool(project.data_views[1].json_payload, "stale", &stale2));
    EXPECT_TRUE(stale1);
    EXPECT_FALSE(stale2);
}

TEST(ReportingPersistenceTest, ReportingCacheExpiry) {
    Project project;
    Project::ReportingCacheEntry entry;
    entry.key = "q1";
    entry.payload_json = "{\"rows\":[]}";
    entry.cached_at = std::time(nullptr) - 120;
    entry.ttl_seconds = 60;
    project.StoreReportingCache(entry);

    auto cached = project.GetReportingCache("q1");
    EXPECT_FALSE(cached.has_value());
}
