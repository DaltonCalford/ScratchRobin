#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "connection/connection_services.h"
#include "core/beta1b_contracts.h"
#include "project/project_services.h"

namespace scratchrobin::ui {

struct SqlRunResult {
    std::string command_tag;
    std::int64_t rows_affected = 0;
    std::string status_payload;
};

struct HistoryExportResult {
    std::size_t retained_rows = 0;
    std::string payload;
};

struct PlanRenderResult {
    std::size_t root_count = 0;
    std::size_t node_count = 0;
};

class UiWorkflowService {
public:
    explicit UiWorkflowService(connection::BackendAdapterService* adapter,
                               project::SpecSetService* specset_service);

    std::vector<std::string> MainMenuTopology() const;
    std::vector<std::pair<std::string, std::string>> ToolsMenu() const;
    void EnsureSpecWorkspaceEntrypoint() const;
    void ValidateSurfaceOpen(const std::string& workflow_id,
                             bool capability_ready,
                             bool state_ready) const;

    SqlRunResult RunSqlEditorQuery(const std::string& sql,
                                   bool with_status_snapshot,
                                   std::int64_t running_queries,
                                   std::int64_t queued_jobs);

    std::vector<std::string> SortedSqlSuggestions(
        const std::vector<beta1b::SuggestionCandidate>& candidates,
        const std::string& prefix,
        const std::function<int(std::string_view, std::string_view)>& fuzzy_distance) const;

    std::string InsertSnippetExact(const beta1b::Snippet& snippet) const;

    HistoryExportResult PruneAndExportHistory(const std::vector<beta1b::QueryHistoryRow>& rows,
                                              const std::string& cutoff_utc,
                                              const std::string& format) const;

    std::vector<beta1b::SchemaCompareOperation> BuildSchemaCompareSet(
        const std::vector<beta1b::SchemaCompareOperation>& operations) const;

    beta1b::DataCompareResult RunDataCompare(const std::vector<beta1b::DataCompareRow>& left,
                                             const std::vector<beta1b::DataCompareRow>& right) const;

    std::string BuildMigrationScript(const std::vector<beta1b::SchemaCompareOperation>& operations,
                                     const std::string& compare_timestamp_utc,
                                     const std::string& left_source,
                                     const std::string& right_source) const;

    PlanRenderResult RenderPlan(const std::vector<beta1b::PlanNode>& nodes) const;

    beta1b::BuilderApplyResult ApplyVisualBuilder(bool has_unsupported_construct,
                                                  bool strict_builder,
                                                  const std::string& emitted_sql,
                                                  bool canonical_equivalent) const;

    std::string BuildSpecWorkspaceGapSummary(
        const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links) const;

    void ExecuteSecurityPolicyAction(bool has_permission,
                                     const std::string& permission_key,
                                     const std::function<void()>& action) const;

private:
    connection::BackendAdapterService* adapter_ = nullptr;
    project::SpecSetService* specset_service_ = nullptr;
};

}  // namespace scratchrobin::ui

