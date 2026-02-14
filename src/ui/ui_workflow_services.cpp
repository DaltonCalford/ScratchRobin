#include "ui/ui_workflow_services.h"

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

std::vector<beta1b::SchemaCompareOperation> UiWorkflowService::BuildSchemaCompareSet(
    const std::vector<beta1b::SchemaCompareOperation>& operations) const {
    return beta1b::StableSortOps(operations);
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

PlanRenderResult UiWorkflowService::RenderPlan(const std::vector<beta1b::PlanNode>& nodes) const {
    const auto ordered = beta1b::OrderPlanNodes(nodes);
    PlanRenderResult result;
    result.root_count = ordered.count(-1) > 0U ? ordered.at(-1).size() : 0U;
    result.node_count = nodes.size();
    return result;
}

beta1b::BuilderApplyResult UiWorkflowService::ApplyVisualBuilder(bool has_unsupported_construct,
                                                                 bool strict_builder,
                                                                 const std::string& emitted_sql,
                                                                 bool canonical_equivalent) const {
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

void UiWorkflowService::ExecuteSecurityPolicyAction(bool has_permission,
                                                    const std::string& permission_key,
                                                    const std::function<void()>& action) const {
    beta1b::ApplySecurityPolicyAction(has_permission, permission_key, action);
}

}  // namespace scratchrobin::ui
