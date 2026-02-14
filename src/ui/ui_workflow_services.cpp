#include "ui/ui_workflow_services.h"

#include <cctype>
#include <queue>
#include <sstream>

#include "core/reject.h"

namespace scratchrobin::ui {

UiWorkflowService::UiWorkflowService(connection::BackendAdapterService* adapter,
                                     project::SpecSetService* specset_service)
    : adapter_(adapter), specset_service_(specset_service) {
    if (adapter_ == nullptr || specset_service_ == nullptr) {
        throw MakeReject("SRB1-R-5101", "ui workflow dependencies missing", "ui", "constructor");
    }
}

std::vector<std::string> UiWorkflowService::MainMenuTopology() const {
    return {
        "Connections",
        "Objects",
        "Edit",
        "View",
        "Admin",
        "Tools",
        "Window",
        "Help",
        "Layout",
    };
}

std::vector<std::pair<std::string, std::string>> UiWorkflowService::ToolsMenu() const {
    return beta1b::BuildToolsMenu();
}

void UiWorkflowService::EnsureSpecWorkspaceEntrypoint() const {
    const auto tools = ToolsMenu();
    bool found = false;
    for (const auto& item : tools) {
        if (item.first == "Spec Workspace") {
            found = true;
            break;
        }
    }
    if (!found) {
        throw MakeReject("SRB1-R-5101", "spec workspace menu entry missing", "ui", "ensure_spec_workspace_entry");
    }
}

void UiWorkflowService::ValidateSurfaceOpen(const std::string& workflow_id,
                                            bool capability_ready,
                                            bool state_ready) const {
    beta1b::ValidateUiWorkflowState(workflow_id, capability_ready, state_ready);
}

SqlRunResult UiWorkflowService::RunSqlEditorQuery(const std::string& sql,
                                                  bool with_status_snapshot,
                                                  std::int64_t running_queries,
                                                  std::int64_t queued_jobs) {
    ValidateSurfaceOpen("sql_editor", adapter_->IsConnected(), true);
    auto query = adapter_->ExecuteQuery(sql);

    SqlRunResult result;
    result.command_tag = query.command_tag;
    result.rows_affected = query.rows_affected;
    if (with_status_snapshot) {
        result.status_payload = adapter_->FetchStatus(running_queries, queued_jobs);
    }
    return result;
}

std::vector<std::string> UiWorkflowService::SortedSqlSuggestions(
    const std::vector<beta1b::SuggestionCandidate>& candidates,
    const std::string& prefix,
    const std::function<int(std::string_view, std::string_view)>& fuzzy_distance) const {
    return beta1b::SortedSuggestions(candidates, prefix, fuzzy_distance);
}

std::string UiWorkflowService::InsertSnippetExact(const beta1b::Snippet& snippet) const {
    return beta1b::SnippetInsertExact(snippet);
}

void UiWorkflowService::UpsertSnippet(bool has_permission, const beta1b::Snippet& snippet) {
    beta1b::ApplySecurityPolicyAction(has_permission, "snippet.manage", [] {});
    (void)beta1b::SnippetInsertExact(snippet);
    snippets_by_id_[snippet.snippet_id] = snippet;
}

std::vector<beta1b::Snippet> UiWorkflowService::ListSnippets(bool has_permission, const std::string& scope) const {
    beta1b::ApplySecurityPolicyAction(has_permission, "snippet.read", [] {});
    std::vector<beta1b::Snippet> out;
    for (const auto& [_, snippet] : snippets_by_id_) {
        if (scope.empty() || snippet.scope == scope) {
            out.push_back(snippet);
        }
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return std::tie(a.scope, a.name, a.snippet_id) < std::tie(b.scope, b.name, b.snippet_id);
    });
    return out;
}

void UiWorkflowService::RemoveSnippet(bool has_permission, const std::string& snippet_id) {
    beta1b::ApplySecurityPolicyAction(has_permission, "snippet.manage", [] {});
    if (snippets_by_id_.erase(snippet_id) == 0U) {
        throw MakeReject("SRB1-R-5103", "snippet not found", "ui", "remove_snippet", false, snippet_id);
    }
}

HistoryExportResult UiWorkflowService::PruneAndExportHistory(const std::vector<beta1b::QueryHistoryRow>& rows,
                                                             const std::string& cutoff_utc,
                                                             const std::string& format) const {
    auto retained = beta1b::PruneHistory(rows, cutoff_utc);
    HistoryExportResult result;
    result.retained_rows = retained.size();

    if (format == "csv") {
        result.payload = beta1b::ExportHistoryCsv(retained);
        return result;
    }

    if (format == "json") {
        std::ostringstream out;
        out << "[";
        for (size_t i = 0; i < retained.size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            const auto& row = retained[i];
            out << "{\"query_id\":\"" << row.query_id
                << "\",\"profile_id\":\"" << row.profile_id
                << "\",\"started_at_utc\":\"" << row.started_at_utc
                << "\",\"duration_ms\":" << row.duration_ms
                << ",\"status\":\"" << row.status
                << "\",\"error_code\":\"" << row.error_code
                << "\",\"sql_hash\":\"" << row.sql_hash << "\"}";
        }
        out << "]";
        result.payload = out.str();
        return result;
    }

    throw MakeReject("SRB1-R-5104", "history export format unsupported", "ui", "prune_and_export_history", false, format);
}

void UiWorkflowService::AppendHistoryRow(const beta1b::QueryHistoryRow& row) {
    (void)beta1b::PruneHistory({row}, "1970-01-01T00:00:00Z", 1);
    history_rows_.push_back(row);
}

std::vector<beta1b::QueryHistoryRow> UiWorkflowService::QueryHistoryByProfile(const std::string& profile_id) const {
    std::vector<beta1b::QueryHistoryRow> filtered;
    for (const auto& row : history_rows_) {
        if (profile_id.empty() || row.profile_id == profile_id) {
            filtered.push_back(row);
        }
    }
    std::sort(filtered.begin(), filtered.end(), [](const auto& a, const auto& b) {
        return std::tie(a.started_at_utc, a.query_id) < std::tie(b.started_at_utc, b.query_id);
    });
    return filtered;
}

HistoryExportResult UiWorkflowService::PruneAndExportStoredHistory(const std::string& profile_id,
                                                                   const std::string& cutoff_utc,
                                                                   const std::string& format) const {
    return PruneAndExportHistory(QueryHistoryByProfile(profile_id), cutoff_utc, format);
}

std::vector<beta1b::SchemaCompareOperation> UiWorkflowService::BuildSchemaCompareSet(
    const std::vector<beta1b::SchemaCompareOperation>& operations) const {
    return beta1b::StableSortOps(operations);
}

std::vector<beta1b::SchemaCompareOperation> UiWorkflowService::BuildSchemaCompareFromSnapshots(
    const std::vector<SchemaObjectSnapshot>& left,
    const std::vector<SchemaObjectSnapshot>& right) const {
    std::map<std::string, SchemaObjectSnapshot> left_map;
    std::map<std::string, SchemaObjectSnapshot> right_map;
    for (const auto& row : left) {
        if (row.object_path.empty() || row.object_class.empty()) {
            throw MakeReject("SRB1-R-5105", "invalid left schema snapshot row", "ui", "build_schema_compare_from_snapshots");
        }
        left_map[row.object_path] = row;
    }
    for (const auto& row : right) {
        if (row.object_path.empty() || row.object_class.empty()) {
            throw MakeReject("SRB1-R-5105", "invalid right schema snapshot row", "ui", "build_schema_compare_from_snapshots");
        }
        right_map[row.object_path] = row;
    }

    std::set<std::string> all_paths;
    for (const auto& [path, _] : left_map) {
        all_paths.insert(path);
    }
    for (const auto& [path, _] : right_map) {
        all_paths.insert(path);
    }

    auto make_op_id = [](const std::string& path, const std::string& kind) {
        std::string id = kind + ":" + path;
        for (auto& ch : id) {
            if (!std::isalnum(static_cast<unsigned char>(ch))) {
                ch = '_';
            }
        }
        return id;
    };

    std::vector<beta1b::SchemaCompareOperation> ops;
    for (const auto& path : all_paths) {
        auto l = left_map.find(path);
        auto r = right_map.find(path);
        if (l == left_map.end()) {
            const auto& rr = r->second;
            ops.push_back(
                {make_op_id(path, "add"), rr.object_class, rr.object_path, "add", rr.canonical_ddl});
            continue;
        }
        if (r == right_map.end()) {
            const auto& ll = l->second;
            ops.push_back(
                {make_op_id(path, "drop"), ll.object_class, ll.object_path, "drop",
                 "DROP " + ll.object_class + " " + ll.object_path + ";"});
            continue;
        }
        if (l->second.canonical_ddl != r->second.canonical_ddl) {
            ops.push_back(
                {make_op_id(path, "alter"), r->second.object_class, r->second.object_path, "alter", r->second.canonical_ddl});
        }
    }
    return beta1b::StableSortOps(ops);
}

beta1b::DataCompareResult UiWorkflowService::RunDataCompare(
    const std::vector<beta1b::DataCompareRow>& left,
    const std::vector<beta1b::DataCompareRow>& right) const {
    return beta1b::RunDataCompareKeyed(left, right);
}

std::string UiWorkflowService::BuildMigrationScript(
    const std::vector<beta1b::SchemaCompareOperation>& operations,
    const std::string& compare_timestamp_utc,
    const std::string& left_source,
    const std::string& right_source) const {
    return beta1b::GenerateMigrationScript(operations, compare_timestamp_utc, left_source, right_source);
}

std::string UiWorkflowService::ApplyMigrationScript(
    const std::string& script,
    const std::function<bool(const std::string&)>& apply_statement) const {
    if (script.empty()) {
        throw MakeReject("SRB1-R-5106", "empty migration script", "ui", "apply_migration_script");
    }

    std::vector<std::string> statements;
    std::string current;
    std::stringstream ss(script);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.rfind("--", 0) == 0U) {
            continue;
        }
        current += line;
        current.push_back('\n');
        std::size_t semicolon = current.find(';');
        while (semicolon != std::string::npos) {
            std::string statement = current.substr(0, semicolon + 1U);
            current.erase(0, semicolon + 1U);
            if (!statement.empty()) {
                statements.push_back(statement);
            }
            semicolon = current.find(';');
        }
    }
    if (statements.empty()) {
        throw MakeReject("SRB1-R-5106", "no executable migration statements", "ui", "apply_migration_script");
    }

    std::size_t applied = 0U;
    for (const auto& statement : statements) {
        if (!apply_statement(statement)) {
            throw MakeReject("SRB1-R-5106", "migration apply failed", "ui", "apply_migration_script", false, statement);
        }
        ++applied;
    }
    std::ostringstream out;
    out << "{\"applied\":" << applied << ",\"status\":\"ok\"}";
    return out.str();
}

PlanRenderResult UiWorkflowService::RenderPlan(const std::vector<beta1b::PlanNode>& nodes) const {
    const auto ordered = beta1b::OrderPlanNodes(nodes);
    PlanRenderResult result;
    result.root_count = ordered.count(-1) > 0U ? ordered.at(-1).size() : 0U;
    result.node_count = nodes.size();
    return result;
}

std::vector<PlanLayoutRow> UiWorkflowService::RenderPlanLayout(const std::vector<beta1b::PlanNode>& nodes) const {
    const auto ordered = beta1b::OrderPlanNodes(nodes);
    std::vector<PlanLayoutRow> rows;
    std::queue<std::pair<beta1b::PlanNode, int>> queue;
    if (ordered.count(-1) > 0U) {
        for (const auto& root : ordered.at(-1)) {
            queue.push({root, 0});
        }
    } else {
        for (const auto& node : nodes) {
            queue.push({node, 0});
        }
    }

    int ordinal = 0;
    while (!queue.empty()) {
        const auto [node, depth] = queue.front();
        queue.pop();
        rows.push_back({node.node_id, depth, ordinal++, node.estimated_cost, node.operator_name});
        auto children = ordered.find(node.node_id);
        if (children != ordered.end()) {
            for (const auto& child : children->second) {
                queue.push({child, depth + 1});
            }
        }
    }
    return rows;
}

beta1b::BuilderApplyResult UiWorkflowService::ApplyVisualBuilder(bool has_unsupported_construct,
                                                                 bool strict_builder,
                                                                 const std::string& emitted_sql,
                                                                 bool canonical_equivalent) const {
    return beta1b::ApplyBuilderGraph(has_unsupported_construct, strict_builder, emitted_sql, canonical_equivalent);
}

beta1b::BuilderApplyResult UiWorkflowService::ApplyVisualBuilderWithRoundTrip(
    bool has_unsupported_construct,
    bool strict_builder,
    const std::string& emitted_sql,
    const std::function<std::string(const std::string&)>& normalize_sql,
    const std::string& expected_canonical_sql) const {
    if (expected_canonical_sql.empty()) {
        throw MakeReject("SRB1-R-5108", "expected canonical sql required", "ui", "apply_builder_round_trip");
    }
    const std::string normalized = normalize_sql(emitted_sql);
    const bool canonical_equivalent = normalized == expected_canonical_sql;
    return beta1b::ApplyBuilderGraph(has_unsupported_construct, strict_builder, emitted_sql, canonical_equivalent);
}

std::string UiWorkflowService::BuildSpecWorkspaceGapSummary(
    const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links) const {
    const auto summary = specset_service_->CoverageSummary(coverage_links);
    std::map<std::string, int> gaps = {
        {"design", summary.count("design:missing") ? summary.at("design:missing") : 0},
        {"development", summary.count("development:missing") ? summary.at("development:missing") : 0},
        {"management", summary.count("management:missing") ? summary.at("management:missing") : 0},
    };
    return beta1b::BuildSpecWorkspaceSummary(gaps);
}

std::string UiWorkflowService::BuildSpecWorkspaceDashboard(
    const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links) const {
    const auto summary = specset_service_->CoverageSummary(coverage_links);
    const auto read = [&](const std::string& key) -> int {
        auto it = summary.find(key);
        return it == summary.end() ? 0 : it->second;
    };
    const std::string design_badge = beta1b::CoverageBadge(
        read("design:covered") > 0 ? "covered" : "missing",
        read("design:partial") > 0 ? "partial" : "covered",
        read("design:missing") > 0 ? "missing" : "covered");
    const std::string development_badge = beta1b::CoverageBadge(
        read("development:covered") > 0 ? "covered" : "missing",
        read("development:partial") > 0 ? "partial" : "covered",
        read("development:missing") > 0 ? "missing" : "covered");
    const std::string management_badge = beta1b::CoverageBadge(
        read("management:covered") > 0 ? "covered" : "missing",
        read("management:partial") > 0 ? "partial" : "covered",
        read("management:missing") > 0 ? "missing" : "covered");

    std::ostringstream out;
    out << "{"
        << "\"design\":{\"covered\":" << read("design:covered")
        << ",\"partial\":" << read("design:partial")
        << ",\"missing\":" << read("design:missing")
        << ",\"badge\":\"" << design_badge << "\"},"
        << "\"development\":{\"covered\":" << read("development:covered")
        << ",\"partial\":" << read("development:partial")
        << ",\"missing\":" << read("development:missing")
        << ",\"badge\":\"" << development_badge << "\"},"
        << "\"management\":{\"covered\":" << read("management:covered")
        << ",\"partial\":" << read("management:partial")
        << ",\"missing\":" << read("management:missing")
        << ",\"badge\":\"" << management_badge << "\"}"
        << "}";
    return out.str();
}

std::string UiWorkflowService::ExportSpecWorkspaceWorkPackage(
    const std::string& set_id,
    const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& gaps,
    const std::string& generated_at_utc) const {
    return specset_service_->ExportImplementationWorkPackage(set_id, gaps, generated_at_utc);
}

void UiWorkflowService::ExecuteSecurityPolicyAction(bool has_permission,
                                                    const std::string& permission_key,
                                                    const std::function<void()>& action) const {
    beta1b::ApplySecurityPolicyAction(has_permission, permission_key, action);
}

void UiWorkflowService::UpsertSecurityPolicy(bool has_permission,
                                             const std::string& policy_id,
                                             const std::string& policy_json) {
    beta1b::ApplySecurityPolicyAction(has_permission, "security.manage", [] {});
    if (policy_id.empty() || policy_json.empty()) {
        throw MakeReject("SRB1-R-8301", "invalid security policy payload", "ui", "upsert_security_policy");
    }
    security_policies_[policy_id] = policy_json;
}

std::string UiWorkflowService::GetSecurityPolicy(bool has_permission,
                                                 const std::string& policy_id) const {
    beta1b::ApplySecurityPolicyAction(has_permission, "security.read", [] {});
    auto it = security_policies_.find(policy_id);
    if (it == security_policies_.end()) {
        throw MakeReject("SRB1-R-8301", "security policy not found", "ui", "get_security_policy", false, policy_id);
    }
    return it->second;
}

std::vector<std::string> UiWorkflowService::ListSecurityPolicyIds(bool has_permission) const {
    beta1b::ApplySecurityPolicyAction(has_permission, "security.read", [] {});
    std::vector<std::string> out;
    out.reserve(security_policies_.size());
    for (const auto& [id, _] : security_policies_) {
        out.push_back(id);
    }
    return out;
}

void UiWorkflowService::RemoveSecurityPolicy(bool has_permission,
                                             const std::string& policy_id) {
    beta1b::ApplySecurityPolicyAction(has_permission, "security.manage", [] {});
    auto it = security_policies_.find(policy_id);
    if (it == security_policies_.end()) {
        throw MakeReject("SRB1-R-8301", "security policy not found", "ui", "remove_security_policy", false, policy_id);
    }
    security_policies_.erase(it);
}

}  // namespace scratchrobin::ui
