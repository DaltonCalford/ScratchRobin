/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include "documentation_generator.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

#include "project.h"
#include "simple_json.h"

namespace scratchrobin {
namespace {

std::vector<std::string> SplitPath(const std::string& path, char delim) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == delim) {
            if (!current.empty()) parts.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) parts.push_back(current);
    return parts;
}

} // namespace

bool DocumentationGenerator::Generate(Project& project,
                                      const std::string& output_dir,
                                      std::string* error) {
    if (project.project_root_path.empty()) {
        if (error) *error = "Project root not set";
        return false;
    }
    std::filesystem::path out_dir = output_dir.empty()
        ? std::filesystem::path(project.project_root_path) /
              project.config.docs_path / "generated"
        : std::filesystem::path(output_dir);
    std::error_code ec;
    std::filesystem::create_directories(out_dir, ec);
    if (ec) {
        if (error) *error = "Failed to create output directory";
        return false;
    }

    std::string template_error;
    auto templates = project.DiscoverTemplates(&template_error);
    if (!template_error.empty()) {
        if (error) *error = template_error;
        return false;
    }

    std::filesystem::path readme_path = out_dir / "README.md";
    std::ofstream out(readme_path);
    if (!out) {
        if (error) *error = "Failed to write generated README";
        return false;
    }

    out << "# Generated Documentation\n\n";
    out << "This folder is generated from project templates.\n\n";
    out << "## Templates Detected\n\n";
    if (templates.empty()) {
        out << "- (none)\n";
    } else {
        for (const auto& t : templates) {
            out << "- `" << t.name << "` (" << t.kind << ") — `" << t.path << "`\n";
        }
    }

    std::vector<std::string> warnings;
    if (!templates.empty()) {
        out << "\n## Generated Files\n\n";
        for (const auto& t : templates) {
            std::filesystem::path target = out_dir / (t.name + ".md");
            std::ofstream doc(target);
            if (!doc) {
                if (error) *error = "Failed to write template output";
                return false;
            }
            doc << "# " << t.name << "\n\n";
            doc << "Template kind: " << t.kind << "\n\n";
            doc << "Source: `" << t.path << "`\n\n";
            doc << "_Generated stub. Replace with rendered output._\n";
            out << "- `" << t.name << ".md`\n";
        }
    } else {
        warnings.push_back("No templates discovered under docs/templates.");
    }

    // Data dictionary summary
    {
        std::filesystem::path dict_path = out_dir / "data_dictionary.md";
        std::ofstream dict(dict_path);
        if (!dict) {
            if (error) *error = "Failed to write data dictionary";
            return false;
        }
        dict << "# Data Dictionary\n\n";
        if (project.objects_by_id.empty()) {
            dict << "_No objects defined._\n";
            warnings.push_back("Data dictionary is empty (no objects).");
        } else {
            std::vector<std::shared_ptr<ProjectObject>> objects;
            objects.reserve(project.objects_by_id.size());
            for (const auto& pair : project.objects_by_id) {
                if (pair.second) objects.push_back(pair.second);
            }
            std::sort(objects.begin(), objects.end(),
                      [](const std::shared_ptr<ProjectObject>& a,
                         const std::shared_ptr<ProjectObject>& b) {
                          if (a->schema_name != b->schema_name) {
                              return a->schema_name < b->schema_name;
                          }
                          if (a->kind != b->kind) {
                              return a->kind < b->kind;
                          }
                          return a->name < b->name;
                      });

            std::string current_schema;
            dict << "## Contents\n\n";
            struct SchemaSummary {
                size_t total = 0;
                std::map<std::string, size_t> kinds;
            };
            std::map<std::string, SchemaSummary> schema_counts;
            for (const auto& obj : objects) {
                if (!obj || obj->kind == "diagram") continue;
                auto& summary = schema_counts[obj->schema_name];
                summary.total++;
                summary.kinds[obj->kind]++;
            }
            for (const auto& pair : schema_counts) {
                dict << "- " << (pair.first.empty() ? "(default)" : pair.first)
                     << ": " << pair.second.total << "\n";
                for (const auto& kind_pair : pair.second.kinds) {
                    dict << "  - " << kind_pair.first << ": " << kind_pair.second << "\n";
                }
            }
            dict << "\n";
            for (const auto& obj : objects) {
                if (!obj || obj->kind == "diagram") continue;
                std::string anchor = obj->name;
                std::transform(anchor.begin(), anchor.end(), anchor.begin(),
                               [](unsigned char c) {
                                   if (std::isspace(c)) return '-';
                                   return static_cast<char>(std::tolower(c));
                               });
                dict << "- [" << obj->name << "](#" << anchor << ")\n";
            }
            dict << "\n";
            size_t missing_attr_name = 0;
            size_t missing_attr_type = 0;
            for (const auto& obj : objects) {
                if (obj->kind == "diagram") {
                    continue;
                }
                if (obj->schema_name != current_schema) {
                    current_schema = obj->schema_name;
                    dict << "## Schema: " << (current_schema.empty() ? "(default)" : current_schema)
                         << "\n\n";
                    size_t schema_count = 0;
                    std::map<std::string, size_t> kind_counts;
                    for (const auto& item : objects) {
                        if (item->kind == "diagram") continue;
                        if (item->schema_name == current_schema) {
                            schema_count++;
                            kind_counts[item->kind]++;
                        }
                    }
                    dict << "- Count: " << schema_count << "\n\n";
                    if (!kind_counts.empty()) {
                        dict << "| Kind | Count |\n";
                        dict << "| --- | --- |\n";
                        for (const auto& kpair : kind_counts) {
                            dict << "| " << kpair.first << " | " << kpair.second << " |\n";
                        }
                        dict << "\n";
                    }
                }
                std::string obj_anchor = obj->name;
                std::transform(obj_anchor.begin(), obj_anchor.end(), obj_anchor.begin(),
                               [](unsigned char c) {
                                   if (std::isspace(c)) return '-';
                                   return static_cast<char>(std::tolower(c));
                               });
                dict << "### " << obj->name << " {#" << obj_anchor << "}\n\n";
                dict << "- Kind: `" << obj->kind << "`\n";
                if (!obj->path.empty()) {
                    dict << "- Path: `" << obj->path << "`\n";
                }
                if (!obj->current_design.children.empty()) {
                    dict << "\nAttributes:\n\n";
                    dict << "| Name | Kind | Type/DDL |\n";
                    dict << "| --- | --- | --- |\n";
                    for (const auto& child : obj->current_design.children) {
                        std::string name = !child.name.empty() ? child.name : child.label;
                        if (name.empty()) name = "attribute";
                        std::string type = child.ddl;
                        if (type.empty()) type = "-";
                        if (child.name.empty() && child.label.empty()) {
                            missing_attr_name++;
                        }
                        if (child.ddl.empty()) {
                            missing_attr_type++;
                        }
                        dict << "| " << name << " | "
                             << (child.kind.empty() ? "-" : child.kind) << " | "
                             << type << " |\n";
                    }
                    dict << "\n";
                }
                if (!obj->current_design.dependencies.empty()) {
                    dict << "Dependencies:\n\n";
                    for (const auto& dep : obj->current_design.dependencies) {
                        dict << "- " << dep << "\n";
                    }
                    dict << "\n";
                }
            }
            if (missing_attr_name > 0 || missing_attr_type > 0) {
                dict << "## Warnings\n\n";
                if (missing_attr_name > 0) {
                    dict << "- Attributes missing names: " << missing_attr_name << "\n";
                    warnings.push_back("Attributes missing names: " +
                                       std::to_string(missing_attr_name));
                }
                if (missing_attr_type > 0) {
                    dict << "- Attributes missing types/DDL: " << missing_attr_type << "\n";
                    warnings.push_back("Attributes missing types/DDL: " +
                                       std::to_string(missing_attr_type));
                }
                dict << "\n";
            }
        }
    }

    // Reporting summary
    {
        std::filesystem::path rep_path = out_dir / "reporting_summary.md";
        std::ofstream rep(rep_path);
        if (!rep) {
            if (error) *error = "Failed to write reporting summary";
            return false;
        }
        rep << "# Reporting Summary\n\n";
        if (project.reporting_assets.empty()) {
            rep << "_No reporting assets defined._\n";
            warnings.push_back("No reporting assets defined.");
        } else {
            struct AssetInfo {
                std::string id;
                std::string name;
                std::string collection_id;
                std::string description;
                std::string sql_mode;
            };
            std::map<std::string, std::vector<AssetInfo>> by_type;
            size_t missing_name = 0;
            size_t empty_name = 0;
            size_t missing_collection = 0;
            size_t parse_errors = 0;
            size_t missing_id = 0;
            size_t mismatch_id = 0;
            std::vector<std::string> parse_error_ids;
            for (const auto& asset : project.reporting_assets) {
                AssetInfo info;
                info.id = asset.id.ToString();
                bool missing_id_field = false;
                bool id_mismatch = false;
                if (!asset.json_payload.empty()) {
                    JsonParser parser(asset.json_payload);
                    JsonValue root;
                    std::string parse_error;
                    if (parser.Parse(&root, &parse_error)) {
                        if (const JsonValue* id_val = FindMember(root, "id")) {
                            if (id_val->type != JsonValue::Type::String ||
                                id_val->string_value.empty()) {
                                missing_id_field = true;
                            }
                            if (id_val->type == JsonValue::Type::String &&
                                !id_val->string_value.empty() &&
                                id_val->string_value != info.id) {
                                id_mismatch = true;
                            }
                        } else {
                            missing_id_field = true;
                        }
                        if (const JsonValue* name_val = FindMember(root, "name")) {
                            if (name_val->type == JsonValue::Type::String) {
                                info.name = name_val->string_value;
                                if (info.name.empty()) {
                                    empty_name++;
                                }
                            }
                        }
                        if (const JsonValue* col_val = FindMember(root, "collection_id")) {
                            if (col_val->type == JsonValue::Type::String) {
                                info.collection_id = col_val->string_value;
                            }
                        }
                        if (const JsonValue* desc_val = FindMember(root, "description")) {
                            if (desc_val->type == JsonValue::Type::String) {
                                info.description = desc_val->string_value;
                            }
                        }
                        if (const JsonValue* sql_val = FindMember(root, "sql_mode")) {
                            if (sql_val->type == JsonValue::Type::Bool) {
                                info.sql_mode = sql_val->bool_value ? "true" : "false";
                            }
                        }
                    } else {
                        parse_errors++;
                        parse_error_ids.push_back(info.id);
                    }
                }
                if (missing_id_field) {
                    missing_id++;
                    warnings.push_back("Reporting asset missing JSON id field: " + info.id);
                }
                if (id_mismatch) {
                    mismatch_id++;
                    warnings.push_back("Reporting asset id mismatch: " + info.id);
                }
                if (info.name.empty()) missing_name++;
                if (info.collection_id.empty() && asset.object_type != "collection") {
                    missing_collection++;
                }
                by_type[asset.object_type].push_back(info);
            }
            std::map<std::string, std::string> collection_lookup;
            std::map<std::string, size_t> collection_id_counts;
            for (const auto& pair : project.reporting_assets) {
                if (pair.object_type != "collection") continue;
                if (pair.json_payload.empty()) continue;
                JsonParser parser(pair.json_payload);
                JsonValue root;
                std::string parse_error;
                if (!parser.Parse(&root, &parse_error)) continue;
                std::string name;
                std::string id;
                if (const JsonValue* id_val = FindMember(root, "id")) {
                    if (id_val->type == JsonValue::Type::String) {
                        id = id_val->string_value;
                    }
                }
                if (const JsonValue* name_val = FindMember(root, "name")) {
                    if (name_val->type == JsonValue::Type::String) {
                        name = name_val->string_value;
                    }
                }
                if (!id.empty()) {
                    if (collection_lookup.find(id) == collection_lookup.end()) {
                        collection_lookup[id] = name.empty() ? id : name;
                    }
                    collection_id_counts[id]++;
                }
            }
            size_t missing_collection_refs = 0;
            for (const auto& pair : by_type) {
                for (const auto& info : pair.second) {
                    if (pair.first == "collection") {
                        continue;
                    }
                    if (!info.collection_id.empty() &&
                        collection_lookup.find(info.collection_id) == collection_lookup.end()) {
                        missing_collection_refs++;
                    }
                }
            }
            size_t ambiguous_collection_refs = 0;
            for (const auto& pair : by_type) {
                for (const auto& info : pair.second) {
                    if (pair.first == "collection") {
                        continue;
                    }
                    if (!info.collection_id.empty() &&
                        collection_id_counts[info.collection_id] > 1) {
                        ambiguous_collection_refs++;
                    }
                }
            }
            bool has_collections = !collection_lookup.empty();
            if (missing_name > 0 || empty_name > 0 || missing_collection > 0 || parse_errors > 0 ||
                missing_id > 0 || mismatch_id > 0) {
                rep << "## Warnings\n\n";
                if (missing_name > 0) {
                    rep << "- Missing `name` for " << missing_name << " reporting assets.\n";
                    warnings.push_back("Reporting assets missing name: " + std::to_string(missing_name));
                }
                if (empty_name > 0) {
                    rep << "- Empty `name` for " << empty_name << " reporting assets.\n";
                    warnings.push_back("Reporting assets with empty name: " +
                                       std::to_string(empty_name));
                }
                if (missing_collection > 0) {
                    rep << "- Missing `collection_id` for " << missing_collection << " assets.\n";
                    warnings.push_back("Reporting assets missing collection_id: " +
                                       std::to_string(missing_collection));
                    if (has_collections) {
                        rep << "- Collections exist but " << missing_collection
                            << " assets omit `collection_id`.\n";
                        warnings.push_back("Reporting assets missing collection_id while collections exist: " +
                                           std::to_string(missing_collection));
                    }
                }
                if (missing_collection_refs > 0) {
                    rep << "- Missing collection reference for " << missing_collection_refs
                        << " assets.\n";
                    warnings.push_back("Reporting assets with missing collection reference: " +
                                       std::to_string(missing_collection_refs));
                }
                if (ambiguous_collection_refs > 0) {
                    rep << "- Ambiguous collection reference for " << ambiguous_collection_refs
                        << " assets.\n";
                    warnings.push_back("Reporting assets with ambiguous collection reference: " +
                                       std::to_string(ambiguous_collection_refs));
                }
                if (parse_errors > 0) {
                    rep << "- Invalid JSON payloads for " << parse_errors << " assets.\n";
                    std::string ids;
                    for (size_t i = 0; i < parse_error_ids.size(); ++i) {
                        ids += parse_error_ids[i];
                        if (i + 1 < parse_error_ids.size()) ids += ", ";
                    }
                    warnings.push_back("Reporting assets with invalid JSON payloads: " +
                                       std::to_string(parse_errors) +
                                       (ids.empty() ? "" : (" (ids: " + ids + ")")));
                }
                if (missing_id > 0) {
                    rep << "- Missing `id` field for " << missing_id << " assets.\n";
                }
                if (mismatch_id > 0) {
                    rep << "- Mismatched `id` values for " << mismatch_id << " assets.\n";
                }
                rep << "\n";
            }
            if (!parse_error_ids.empty()) {
                rep << "### Invalid JSON Assets\n\n";
                for (const auto& id : parse_error_ids) {
                    rep << "- " << id << "\n";
                }
                rep << "\n";
            }
            rep << "## Summary\n\n";
            rep << "| Type | Count |\n";
            rep << "| --- | --- |\n";
            for (const auto& pair : by_type) {
                rep << "| " << pair.first << " | " << pair.second.size() << " |\n";
            }
            rep << "\n";
            std::map<std::string, std::string> object_lookup;
            for (const auto& pair : project.objects_by_id) {
                const auto& obj = pair.second;
                if (!obj || obj->kind == "diagram") continue;
                if (!obj->name.empty()) {
                    std::string anchor = obj->name;
                    std::transform(anchor.begin(), anchor.end(), anchor.begin(),
                                   [](unsigned char c) {
                                       if (std::isspace(c)) return '-';
                                       return static_cast<char>(std::tolower(c));
                                   });
                    object_lookup[obj->name] = anchor;
                }
            }
            size_t duplicate_collection_ids = 0;
            for (const auto& pair : collection_id_counts) {
                if (pair.second > 1) {
                    duplicate_collection_ids++;
                }
            }
            std::vector<std::string> reporting_warnings;

            for (const auto& pair : by_type) {
                rep << "## " << pair.first << "\n\n";
                rep << "- Count: " << pair.second.size() << "\n\n";
                rep << "| ID | Name | Collection | SQL Mode |\n";
                rep << "| --- | --- | --- | --- |\n";
                std::map<std::string, size_t> name_counts;
                std::map<std::string, size_t> id_counts;
                for (const auto& info : pair.second) {
                    std::string name = info.name;
                    auto found = object_lookup.find(name);
                    if (found != object_lookup.end()) {
                        name = "[" + name + "](../generated/data_dictionary.md#" + found->second + ")";
                    }
                    if (!info.name.empty()) {
                        name_counts[info.name]++;
                    }
                    if (!info.id.empty()) {
                        id_counts[info.id]++;
                    }
                    rep << "| " << info.id << " | "
                        << (name.empty() ? "-" : name) << " | "
                        << (info.collection_id.empty()
                            ? "-"
                            : (collection_lookup.count(info.collection_id)
                                  ? ("[" + collection_lookup[info.collection_id] + "](#" +
                                     info.collection_id + ")")
                                  : info.collection_id))
                        << " | "
                        << (info.sql_mode.empty() ? "-" : info.sql_mode) << " |\n";
                    if (!info.description.empty()) {
                        rep << "  - " << info.description << "\n";
                    }
                    if (!info.sql_mode.empty()) {
                        rep << "  - sql_mode: " << info.sql_mode << "\n";
                    }
                }
                for (const auto& nc : name_counts) {
                    if (nc.second > 1) {
                        std::string msg = "Duplicate reporting name in " + pair.first +
                                          ": " + nc.first;
                        warnings.push_back(msg);
                        reporting_warnings.push_back(msg);
                    }
                }
                for (const auto& ic : id_counts) {
                    if (ic.second > 1) {
                        std::string msg = "Duplicate reporting id in " + pair.first +
                                          ": " + ic.first;
                        warnings.push_back(msg);
                        reporting_warnings.push_back(msg);
                    }
                }
                rep << "\n";
            }

            std::map<std::string, std::vector<AssetInfo>> by_collection;
            for (const auto& pair : by_type) {
                for (const auto& info : pair.second) {
                    std::string key = info.collection_id.empty() ? "(none)" : info.collection_id;
                    AssetInfo copy = info;
                    std::string label = pair.first;
                    if (!info.name.empty()) {
                        label += " — " + info.name;
                    }
                    copy.name = label;
                    by_collection[key].push_back(copy);
                }
            }
            rep << "## Collections\n\n";
            rep << "| Collection | Count |\n";
            rep << "| --- | --- |\n";
            for (const auto& pair : by_collection) {
                rep << "| " << pair.first << " | " << pair.second.size() << " |\n";
            }
            rep << "\n";
            for (const auto& pair : by_collection) {
                std::string heading = pair.first;
                if (collection_lookup.count(pair.first)) {
                    heading = collection_lookup[pair.first];
                }
                rep << "### " << heading << " {#" << pair.first << "}\n\n";
                rep << "- Count: " << pair.second.size() << "\n\n";
                rep << "| ID | Type/Name |\n";
                rep << "| --- | --- |\n";
                for (const auto& info : pair.second) {
                    rep << "| " << info.id << " | " << info.name << " |\n";
                }
                rep << "\n";
            }
            for (const auto& col : collection_lookup) {
                if (by_collection.find(col.first) == by_collection.end()) {
                    warnings.push_back("Reporting collection with no assets: " + col.first);
                    rep << "## Warnings\n\n";
                    rep << "- Collection with no assets: " << col.first << "\n\n";
                }
            }
            for (const auto& pair : collection_id_counts) {
                if (pair.second > 1) {
                    warnings.push_back("Duplicate reporting collection id: " + pair.first);
                    rep << "## Warnings\n\n";
                    rep << "- Duplicate collection id: " << pair.first << "\n\n";
                }
            }
            if (duplicate_collection_ids > 0) {
                rep << "## Warnings\n\n";
                rep << "- Duplicate collection ids detected: " << duplicate_collection_ids << "\n\n";
            }
            if (!reporting_warnings.empty()) {
                rep << "## Warnings\n\n";
                for (const auto& warning : reporting_warnings) {
                    rep << "- " << warning << "\n";
                }
                rep << "\n";
            }
        }
    }

    // Diagram extract summary
    {
        std::filesystem::path diag_path = out_dir / "diagrams.md";
        std::ofstream diag(diag_path);
        if (!diag) {
            if (error) *error = "Failed to write diagram summary";
            return false;
        }
        diag << "# Diagram Index\n\n";
        std::vector<std::string> diagram_warnings;
        diag << "## Summary\n\n";
        std::vector<std::string> toc_types;
        bool any = false;
        std::map<std::string, std::vector<std::shared_ptr<ProjectObject>>> diagrams_by_type;
        for (const auto& pair : project.objects_by_id) {
            const auto& obj = pair.second;
            if (!obj || obj->kind != "diagram") continue;
            std::string type = "diagram";
            if (!obj->path.empty()) {
                auto parts = SplitPath(obj->path, '/');
                if (parts.size() >= 2) {
                    type = parts[1];
                }
            }
            diagrams_by_type[type].push_back(obj);
        }
        if (!diagrams_by_type.empty()) {
            size_t total = 0;
            for (const auto& pair : diagrams_by_type) {
                total += pair.second.size();
            }
            diag << "- Total diagrams: " << total << "\n\n";
            diag << "| Type | Count |\n";
            diag << "| --- | --- |\n";
            for (const auto& pair : diagrams_by_type) {
                diag << "| " << pair.first << " | " << pair.second.size() << " |\n";
            }
            diag << "| **Total** | " << total << " |\n";
            diag << "\n";
            diag << "## Contents\n\n";
            for (const auto& pair : diagrams_by_type) {
                std::string anchor = pair.first;
                std::transform(anchor.begin(), anchor.end(), anchor.begin(),
                               [](unsigned char c) { return c == ' ' ? '-' : static_cast<char>(std::tolower(c)); });
                diag << "- [" << pair.first << "](#" << anchor << ") (" << pair.second.size()
                     << ")\n";
            }
            diag << "\n";
        }
        for (auto& pair : diagrams_by_type) {
            any = true;
            diag << "## " << pair.first << "\n\n";
            diag << "- Count: " << pair.second.size() << "\n\n";
            auto& list = pair.second;
            std::map<std::string, size_t> name_counts;
            std::map<std::string, size_t> path_counts;
            for (const auto& obj : list) {
                if (!obj) continue;
                name_counts[obj->name]++;
                if (!obj->design_file_path.empty()) {
                    path_counts[obj->design_file_path]++;
                }
            }
            std::sort(list.begin(), list.end(),
                      [](const std::shared_ptr<ProjectObject>& a,
                         const std::shared_ptr<ProjectObject>& b) {
                          return a->name < b->name;
                      });
            for (const auto& obj : list) {
                if (obj->name.empty()) {
                    std::string warning = "Diagram with empty name in " + pair.first;
                    warnings.push_back(warning);
                    diagram_warnings.push_back(warning);
                    diag << "- (unnamed)";
                } else {
                    diag << "- " << obj->name;
                }
                auto parts = SplitPath(obj->path, '/');
                bool has_expected_prefix = parts.size() >= 2 && parts[0] == "diagram";
                if ((obj->path.empty() || !has_expected_prefix) && !obj->design_file_path.empty()) {
                    std::string warning = "Diagram missing path for " + obj->name;
                    warnings.push_back(warning);
                    diagram_warnings.push_back(warning);
                    diag << " [path: missing]";
                    std::string mismatch = "Diagram type mismatch for " + obj->name;
                    warnings.push_back(mismatch);
                    diagram_warnings.push_back(mismatch);
                    diag << " [type: mismatch]";
                }
                if (!obj->path.empty()) {
                    if (obj->path.back() == '/' || obj->path.back() == '\\') {
                        std::string warning = "Diagram path has trailing slash: " + obj->path;
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                        diag << " [path: slash]";
                    }
                    if (obj->path.find('*') != std::string::npos ||
                        obj->path.find('?') != std::string::npos) {
                        std::string warning = "Diagram path contains invalid characters: " +
                                              obj->path;
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                        diag << " [path: invalid]";
                    }
                    if (parts.size() >= 2 && parts[1] != pair.first) {
                        std::string warning = "Diagram type mismatch for " + obj->name +
                                              " (path: " + parts[1] + ")";
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                        diag << " [type: mismatch]";
                    }
                    if (parts.size() >= 2) {
                        std::string t = parts[1];
                        if (t != "erd" && t != "silverston" && t != "whiteboard" &&
                            t != "mindmap" && t != "dfd" && t != "diagram") {
                            std::string warning = "Unknown diagram type in path for " +
                                                  obj->name + ": " + t;
                            warnings.push_back(warning);
                            diagram_warnings.push_back(warning);
                            diag << " [type: unknown]";
                        }
                    }
                }
                if (!obj->design_file_path.empty()) {
                    diag << " (`" << obj->design_file_path << "`)";
                    if (obj->design_file_path.find("designs/diagrams") != 0) {
                        std::string warning = "Diagram design path outside designs/diagrams: " +
                                              obj->design_file_path;
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                        diag << " [design: path]";
                    }
                    if (!obj->design_file_path.empty() &&
                        (obj->design_file_path.back() == '/' ||
                         obj->design_file_path.back() == '\\')) {
                        std::string warning = "Diagram design path has trailing slash: " +
                                              obj->design_file_path;
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                        diag << " [design: slash]";
                    }
                    if (obj->design_file_path.find('*') != std::string::npos ||
                        obj->design_file_path.find('?') != std::string::npos) {
                        std::string warning = "Diagram design path contains invalid characters: " +
                                              obj->design_file_path;
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                        diag << " [design: invalid]";
                    }
                    if (obj->design_file_path.rfind(".diagram.json") == std::string::npos &&
                        obj->design_file_path.rfind(".sbdgm") == std::string::npos) {
                        std::string warning = "Unexpected diagram file extension for " +
                                              obj->name;
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                        diag << " [design: ext]";
                    }
                    std::filesystem::path design_path = std::filesystem::path(project.project_root_path) /
                                                        obj->design_file_path;
                    bool design_exists = std::filesystem::exists(design_path);
                    if (!std::filesystem::exists(design_path)) {
                        std::string warning = "Missing diagram design file for " + obj->name;
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                        diag << " [design: missing]";
                    }
                    std::filesystem::path svg = design_path;
                    svg.replace_extension(".svg");
                    std::filesystem::path png = design_path;
                    png.replace_extension(".png");
                    std::filesystem::path pdf = design_path;
                    pdf.replace_extension(".pdf");
                    bool found_export = false;
                    bool found_named_export = false;
                    if (std::filesystem::exists(svg)) {
                        diag << " [svg: `" << std::filesystem::relative(svg, project.project_root_path).string()
                             << "`]";
                        found_export = true;
                    }
                    if (std::filesystem::exists(png)) {
                        diag << " [png: `" << std::filesystem::relative(png, project.project_root_path).string()
                             << "`]";
                        found_export = true;
                    }
                    if (std::filesystem::exists(pdf)) {
                        diag << " [pdf: `" << std::filesystem::relative(pdf, project.project_root_path).string()
                             << "`]";
                        found_export = true;
                    }
                    if (!found_export && !obj->name.empty()) {
                        std::filesystem::path named_base =
                            std::filesystem::path(project.project_root_path) / "designs";
                        std::filesystem::path named_svg = named_base / (obj->name + ".diagram.svg");
                        std::filesystem::path named_png = named_base / (obj->name + ".diagram.png");
                        std::filesystem::path named_pdf = named_base / (obj->name + ".diagram.pdf");
                        if (std::filesystem::exists(named_svg) ||
                            std::filesystem::exists(named_png) ||
                            std::filesystem::exists(named_pdf)) {
                            found_export = true;
                            found_named_export = true;
                        }
                    }
                    if (!found_export) {
                        diag << " [export: missing]";
                        std::string warning = "Missing diagram export for " + obj->name;
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                    }
                    if (!design_exists && found_export) {
                        std::string warning = "Diagram exports exist without design file for " + obj->name;
                        warnings.push_back(warning);
                        diagram_warnings.push_back(warning);
                        diag << " [export: orphan]";
                    }
                } else {
                    std::string warning = "Diagram missing design path for " + obj->name;
                    warnings.push_back(warning);
                    diagram_warnings.push_back(warning);
                    diag << " [design: missing]";
                }
                diag << "\n";
            }
            for (const auto& nc : name_counts) {
                if (nc.second > 1) {
                    std::string warning = "Duplicate diagram name in " + pair.first +
                                          ": " + nc.first;
                    warnings.push_back(warning);
                    diagram_warnings.push_back(warning);
                }
            }
            for (const auto& pc : path_counts) {
                if (pc.second > 1) {
                    std::string warning = "Duplicate diagram design path in " + pair.first +
                                          ": " + pc.first;
                    warnings.push_back(warning);
                    diagram_warnings.push_back(warning);
                }
            }
            diag << "\n";
        }
        if (!any) {
            diag << "_No diagrams registered._\n";
            warnings.push_back("No diagrams registered.");
            diagram_warnings.push_back("No diagrams registered.");
        }
        if (!diagram_warnings.empty()) {
            diag << "## Warnings\n\n";
            for (const auto& warning : diagram_warnings) {
                diag << "- " << warning << "\n";
            }
        }
    }

    if (!warnings.empty()) {
        out << "\n## Warnings\n\n";
        for (const auto& warning : warnings) {
            out << "- " << warning << "\n";
        }
    }

    if (error) error->clear();
    return true;
}

} // namespace scratchrobin
