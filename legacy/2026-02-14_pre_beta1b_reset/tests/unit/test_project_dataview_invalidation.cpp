/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include <gtest/gtest.h>

#include "core/project.h"
#include "core/simple_json.h"

namespace scratchrobin {

namespace {

bool ExtractStaleFlag(const std::string& payload, bool* stale) {
    JsonParser parser(payload);
    JsonValue root;
    std::string error;
    if (!parser.Parse(&root, &error)) {
        return false;
    }
    if (const JsonValue* stale_val = FindMember(root, "stale")) {
        if (stale_val->type == JsonValue::Type::Bool) {
            *stale = stale_val->bool_value;
            return true;
        }
    }
    return false;
}

} // namespace

TEST(ProjectDataViewInvalidationTest, MarksOnlyReferencedViewsStale) {
    Project project;
    Project::DataViewSnapshot view_a;
    view_a.id = UUID::Generate();
    view_a.diagram_id = UUID::Generate();
    view_a.json_payload = R"({"name":"Orders","query":"SELECT * FROM public.orders","stale":false})";
    project.UpsertDataView(view_a);

    Project::DataViewSnapshot view_b;
    view_b.id = UUID::Generate();
    view_b.diagram_id = UUID::Generate();
    view_b.json_payload = R"({"name":"NoRef","query":"SELECT 1","stale":false})";
    project.UpsertDataView(view_b);

    project.InvalidateDataViewsForObject("public", "orders");

    bool stale_a = false;
    bool stale_b = true;
    ASSERT_TRUE(ExtractStaleFlag(project.data_views[0].json_payload, &stale_a));
    ASSERT_TRUE(ExtractStaleFlag(project.data_views[1].json_payload, &stale_b));
    EXPECT_TRUE(stale_a);
    EXPECT_FALSE(stale_b);
}

TEST(ProjectDataViewInvalidationTest, UsesQueryRefsWhenPresent) {
    Project project;
    Project::DataViewSnapshot view;
    view.id = UUID::Generate();
    view.diagram_id = UUID::Generate();
    view.json_payload = R"({"name":"Orders","query":"","query_refs":["public.orders"],"stale":false})";
    project.UpsertDataView(view);

    project.InvalidateDataViewsForObject("public", "orders");

    bool stale = false;
    ASSERT_TRUE(ExtractStaleFlag(project.data_views[0].json_payload, &stale));
    EXPECT_TRUE(stale);
}

TEST(ProjectDataViewInvalidationTest, SkipsInvalidPayloads) {
    Project project;
    Project::DataViewSnapshot view;
    view.id = UUID::Generate();
    view.diagram_id = UUID::Generate();
    view.json_payload = R"({"name":"Broken","query":"SELECT * FROM orders",)";
    project.UpsertDataView(view);

    project.InvalidateDataViewsForObject("public", "orders");

    // Invalid JSON should not be mutated; payload remains unchanged.
    EXPECT_EQ(project.data_views[0].json_payload, view.json_payload);
}

} // namespace scratchrobin
