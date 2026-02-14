/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "core/documentation_generator.h"
#include "core/project.h"
#include "core/metadata_model.h"

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

TEST(DocumentationGeneratorTest, WritesGeneratedReadme) {
    auto temp_dir = MakeTempDir("scratchrobin_docs_gen");
    Project project;
    ProjectConfig cfg;
    cfg.name = "DocGen";
    cfg.docs_path = "docs";
    ASSERT_TRUE(project.CreateNew(temp_dir.string(), cfg));

    auto templates_dir = temp_dir / "docs" / "templates";
    WriteFile(templates_dir / "mop_template.yaml", "steps: []");

    auto obj = project.CreateObject("table", "customers", "public");
    ASSERT_TRUE(obj != nullptr);
    MetadataNode child;
    child.name = "";
    child.ddl = "";
    obj->current_design.children.push_back(child);
    auto diag = project.CreateObject("diagram", "Sample Diagram", "");
    ASSERT_TRUE(diag != nullptr);
    diag->path = "diagram/erd/Sample Diagram";
    diag->design_file_path = "designs/diagrams/sample.txt";
    auto unknown = project.CreateObject("diagram", "Unknown Diagram", "");
    ASSERT_TRUE(unknown != nullptr);
    unknown->path = "diagram/unknown/Unknown Diagram";
    auto unnamed = project.CreateObject("diagram", "", "");
    ASSERT_TRUE(unnamed != nullptr);
    unnamed->path = "diagram/erd/";
    auto no_path = project.CreateObject("diagram", "No Path Diagram", "");
    ASSERT_TRUE(no_path != nullptr);
    no_path->design_file_path = "designs/diagrams/nopath.diagram.json";
    auto bad_path = project.CreateObject("diagram", "Bad Path Diagram", "");
    ASSERT_TRUE(bad_path != nullptr);
    bad_path->path = "diagram/erd/Bad?Path";
    bad_path->design_file_path = "designs/diagrams/bad.diagram.json";
    auto outside = project.CreateObject("diagram", "Outside Diagram", "");
    ASSERT_TRUE(outside != nullptr);
    outside->path = "diagram/erd/Outside Diagram";
    outside->design_file_path = "docs/diagrams/outside.diagram.json";
    auto dup = project.CreateObject("diagram", "Sample Diagram", "");
    ASSERT_TRUE(dup != nullptr);
    dup->path = "diagram/erd/Sample Diagram 2";
    dup->design_file_path = "designs/diagrams/sample.txt";
    Project::ReportingAsset asset;
    asset.id = UUID::Generate();
    asset.object_type = "question";
    asset.json_payload = R"({"name":"customers","collection_id":"col_1","description":"Daily totals","sql_mode":true})";
    project.UpsertReportingAsset(asset);
    Project::ReportingAsset unnamed_asset;
    unnamed_asset.id = UUID::Generate();
    unnamed_asset.object_type = "question";
    unnamed_asset.json_payload = R"({"collection_id":"col_1"})";
    project.UpsertReportingAsset(unnamed_asset);
    Project::ReportingAsset missing_collection;
    missing_collection.id = UUID::Generate();
    missing_collection.object_type = "question";
    missing_collection.json_payload = R"({"name":"orphan","collection_id":"missing"})";
    project.UpsertReportingAsset(missing_collection);
    Project::ReportingAsset invalid_json;
    invalid_json.id = UUID::Generate();
    invalid_json.object_type = "question";
    invalid_json.json_payload = "{bad";
    project.UpsertReportingAsset(invalid_json);
    Project::ReportingAsset missing_id;
    missing_id.id = UUID::Generate();
    missing_id.object_type = "question";
    missing_id.json_payload = R"({"name":"no_id","collection_id":"col_1"})";
    project.UpsertReportingAsset(missing_id);
    Project::ReportingAsset mismatch_id;
    mismatch_id.id = UUID::Generate();
    mismatch_id.object_type = "question";
    mismatch_id.json_payload = R"({"id":"different","name":"mismatch","collection_id":"col_1"})";
    project.UpsertReportingAsset(mismatch_id);
    Project::ReportingAsset duplicate_name;
    duplicate_name.id = UUID::Generate();
    duplicate_name.object_type = "question";
    duplicate_name.json_payload = R"({"name":"customers","collection_id":"col_1"})";
    project.UpsertReportingAsset(duplicate_name);
    Project::ReportingAsset duplicate_id;
    duplicate_id.id = asset.id;
    duplicate_id.object_type = "question";
    duplicate_id.json_payload = R"({"name":"duplicate_id","collection_id":"col_1"})";
    project.InsertReportingAsset(duplicate_id);
    Project::ReportingAsset empty_name_asset;
    empty_name_asset.id = UUID::Generate();
    empty_name_asset.object_type = "question";
    empty_name_asset.json_payload = R"({"name":"","collection_id":"col_1"})";
    project.UpsertReportingAsset(empty_name_asset);
    Project::ReportingAsset collection;
    collection.id = UUID::Generate();
    collection.object_type = "collection";
    collection.json_payload = R"({"id":"col_1","name":"Core Reports"})";
    project.UpsertReportingAsset(collection);
    Project::ReportingAsset empty_collection;
    empty_collection.id = UUID::Generate();
    empty_collection.object_type = "collection";
    empty_collection.json_payload = R"({"id":"col_empty","name":"Empty"})";
    project.UpsertReportingAsset(empty_collection);
    Project::ReportingAsset dup_collection;
    dup_collection.id = UUID::Generate();
    dup_collection.object_type = "collection";
    dup_collection.json_payload = R"({"id":"col_1","name":"Dup"})";
    project.UpsertReportingAsset(dup_collection);

    auto export_dir = temp_dir / "designs";
    std::filesystem::create_directories(export_dir);
    WriteFile(export_dir / "Sample Diagram.diagram.svg", "<svg></svg>");

    std::string error;
    EXPECT_TRUE(DocumentationGenerator::Generate(project, "", &error));
    EXPECT_TRUE(error.empty());

    auto readme = temp_dir / "docs" / "generated" / "README.md";
    ASSERT_TRUE(std::filesystem::exists(readme));
    std::ifstream in(readme);
    std::string contents((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    EXPECT_NE(contents.find("mop_template"), std::string::npos);
    EXPECT_NE(contents.find("Warnings"), std::string::npos);

    auto stub = temp_dir / "docs" / "generated" / "mop_template.md";
    EXPECT_TRUE(std::filesystem::exists(stub));

    auto dictionary = temp_dir / "docs" / "generated" / "data_dictionary.md";
    auto reporting = temp_dir / "docs" / "generated" / "reporting_summary.md";
    auto diagrams = temp_dir / "docs" / "generated" / "diagrams.md";
    EXPECT_TRUE(std::filesystem::exists(dictionary));
    EXPECT_TRUE(std::filesystem::exists(reporting));
    EXPECT_TRUE(std::filesystem::exists(diagrams));
    std::ifstream rep_in(reporting);
    std::string rep_contents((std::istreambuf_iterator<char>(rep_in)),
                             std::istreambuf_iterator<char>());
    EXPECT_NE(rep_contents.find("[customers]"), std::string::npos);
    EXPECT_NE(rep_contents.find("Daily totals"), std::string::npos);
    EXPECT_NE(rep_contents.find("sql_mode: true"), std::string::npos);
    EXPECT_NE(rep_contents.find("Summary"), std::string::npos);
    EXPECT_NE(rep_contents.find("| Collection | Count |"), std::string::npos);
    EXPECT_NE(rep_contents.find("| ID | Name | Collection | SQL Mode |"), std::string::npos);
    EXPECT_NE(rep_contents.find("- Daily totals"), std::string::npos);
    EXPECT_NE(rep_contents.find("| Type | Count |"), std::string::npos);
    EXPECT_NE(rep_contents.find("| ID | Type/Name |"), std::string::npos);
    EXPECT_NE(rep_contents.find("[Core Reports]"), std::string::npos);
    EXPECT_NE(rep_contents.find("Missing `name`"), std::string::npos);
    EXPECT_NE(rep_contents.find("Missing collection reference"), std::string::npos);
    EXPECT_NE(rep_contents.find("Invalid JSON payloads"), std::string::npos);
    EXPECT_NE(rep_contents.find("Invalid JSON Assets"), std::string::npos);
    EXPECT_NE(rep_contents.find("Missing `id` field"), std::string::npos);
    EXPECT_NE(rep_contents.find("Mismatched `id` values"), std::string::npos);
    EXPECT_NE(rep_contents.find("Duplicate reporting name"), std::string::npos);
    EXPECT_NE(rep_contents.find("Duplicate reporting id"), std::string::npos);
    EXPECT_NE(rep_contents.find("Collections exist but"), std::string::npos);
    EXPECT_NE(rep_contents.find("Collection with no assets"), std::string::npos);
    EXPECT_NE(rep_contents.find("Duplicate collection ids detected"), std::string::npos);
    EXPECT_NE(rep_contents.find("Empty `name`"), std::string::npos);

    std::ifstream dict_in(dictionary);
    std::string dict_contents((std::istreambuf_iterator<char>(dict_in)),
                              std::istreambuf_iterator<char>());
    EXPECT_NE(dict_contents.find("Attributes missing names"), std::string::npos);
    EXPECT_NE(dict_contents.find("Attributes missing types/DDL"), std::string::npos);

    std::ifstream diag_in(diagrams);
    std::string diag_contents((std::istreambuf_iterator<char>(diag_in)),
                              std::istreambuf_iterator<char>());
    EXPECT_NE(diag_contents.find("export: missing"), std::string::npos);
    EXPECT_NE(diag_contents.find("design: missing"), std::string::npos);
    EXPECT_NE(diag_contents.find("type: mismatch"), std::string::npos);
    EXPECT_NE(diag_contents.find("type: unknown"), std::string::npos);
    EXPECT_NE(diag_contents.find("Diagram with empty name"), std::string::npos);
    EXPECT_NE(diag_contents.find("path: missing"), std::string::npos);
    EXPECT_NE(diag_contents.find("design: path"), std::string::npos);
    EXPECT_NE(diag_contents.find("path: slash"), std::string::npos);
    EXPECT_NE(diag_contents.find("path: invalid"), std::string::npos);
    EXPECT_NE(diag_contents.find("Warnings"), std::string::npos);
    EXPECT_NE(diag_contents.find("Duplicate diagram name"), std::string::npos);
    EXPECT_NE(contents.find("Duplicate diagram name"), std::string::npos);
    EXPECT_NE(diag_contents.find("export: orphan"), std::string::npos);
    EXPECT_NE(diag_contents.find("Duplicate diagram design path"), std::string::npos);
}

} // namespace scratchrobin
