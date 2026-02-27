#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/reject.h"
#include "core/simple_json.h"

namespace scratchrobin::beta1b {

// -------------------------
// Connection contracts
// -------------------------

enum class ConnectionMode {
    Network,
    Ipc,
    Embedded,
};

struct ConnectionProfile {
    std::string name;
    std::string backend;
    ConnectionMode mode = ConnectionMode::Network;
    std::string host;
    int port = 0;
    std::string ipc_path;
    std::string database;
    std::string username;
    std::string credential_id;
};

struct TransportContract {
    std::string mode;  // direct|ssh_tunnel|ssh_jump_chain
    std::string tls_mode;
    std::uint32_t connect_timeout_ms = 0;
};

struct SshContract {
    std::string target_host;
    int target_port = 0;
    std::string tunnel_user;
    std::string auth_method;  // password|keypair|agent
    std::string credential_id;
};

struct JumpHost {
    std::string host;
    int port = 0;
    std::string user;
    std::string auth_method;
    std::string credential_id;
};

struct IdentityContract {
    std::string mode;  // local_password|oidc|saml|ldap|kerberos|ident|radius|pam
    std::string provider_id;
    std::vector<std::string> provider_scope;
    std::string auth_method_id;  // optional canonical plugin method id (scratchbird.auth.*)
    std::vector<std::string> auth_required_methods;
    std::vector<std::string> auth_forbidden_methods;
    bool auth_require_channel_binding = false;
    std::string workload_identity_token;
    std::string proxy_principal_assertion;
    std::string provider_profile;
};

struct SecretProviderContract {
    std::string mode;  // app_store|keychain|libsecret|vault
    std::string secret_ref;
};

struct EnterpriseConnectionProfile {
    std::string profile_id;
    std::string username;
    bool allow_inline_secret = false;
    std::optional<std::string> inline_secret;
    TransportContract transport;
    std::optional<SshContract> ssh;
    std::vector<JumpHost> jump_hosts;
    IdentityContract identity;
    std::optional<SecretProviderContract> secret_provider;
    bool no_login_direct = false;
    bool proxy_assertion_only = false;
    std::string proxy_assertion_trust_profile;
};

struct SessionFingerprint {
    std::string profile_id;
    std::string transport_mode;
    std::string identity_mode;
    std::string identity_method_id;
    std::string identity_provider_profile;
    bool auth_require_channel_binding = false;
    std::vector<std::string> auth_required_methods;
    std::vector<std::string> auth_forbidden_methods;
    bool no_login_direct = false;
    bool proxy_assertion_only = false;
    std::string backend_route;
};

std::string SelectBackend(const ConnectionProfile& profile);
int ResolvePort(const ConnectionProfile& profile);
std::string ResolveCredential(const ConnectionProfile& profile,
                              const std::map<std::string, std::string>& credential_store,
                              const std::optional<std::string>& inline_secret);
void EnsureCapability(bool supported, const std::string& backend_name, const std::string& capability_key);
void CancelActive(bool has_active_backend);
std::string RunCopyIo(const std::string& sql,
                      const std::string& source_kind,
                      const std::string& sink_kind,
                      bool source_open_ok,
                      bool sink_open_ok);
std::string PrepareExecuteClose(bool backend_supports_prepared,
                                const std::string& sql,
                                const std::vector<std::string>& bind_values);
std::string StatusSnapshot(bool backend_supports_status,
                           std::int64_t running_queries,
                           std::int64_t queued_jobs);

void ValidateTransport(const EnterpriseConnectionProfile& profile);
std::string ResolveSecret(const std::optional<std::string>& runtime_override,
                          const std::function<std::optional<std::string>(const SecretProviderContract&)>& provider_fetch,
                          const std::optional<SecretProviderContract>& provider,
                          const std::function<std::optional<std::string>(const std::string&)>& credential_fetch,
                          const std::optional<std::string>& credential_id,
                          const std::optional<std::string>& inline_secret,
                          bool allow_inline);
std::string RunIdentityHandshake(const IdentityContract& identity,
                                 const std::string& secret,
                                 const std::function<bool(const std::string&, const std::string&)>& federated_acquire,
                                 const std::function<bool(const std::string&, const std::string&)>& directory_bind);
SessionFingerprint ConnectEnterprise(
    const EnterpriseConnectionProfile& profile,
    const std::optional<std::string>& runtime_override,
    const std::function<std::optional<std::string>(const SecretProviderContract&)>& provider_fetch,
    const std::function<std::optional<std::string>(const std::string&)>& credential_fetch,
    const std::function<bool(const std::string&, const std::string&)>& federated_acquire,
    const std::function<bool(const std::string&, const std::string&)>& directory_bind);

// -------------------------
// Project/data contracts
// -------------------------

struct HeaderV1 {
    std::uint32_t magic = 0;
    std::uint16_t major = 0;
    std::uint16_t minor = 0;
    std::uint16_t header_size = 0;
    std::uint16_t toc_entry_size = 0;
    std::uint32_t chunk_count = 0;
    std::uint64_t toc_offset = 0;
    std::uint64_t declared_file_size = 0;
    std::uint32_t flags = 0;
    std::uint32_t reserved0 = 0;
    std::uint32_t header_crc32 = 0;
};

struct TocEntry {
    std::string chunk_id;
    std::uint32_t chunk_flags = 0;
    std::uint64_t data_offset = 0;
    std::uint64_t data_size = 0;
    std::uint32_t data_crc32 = 0;
    std::uint16_t payload_version = 0;
    std::uint16_t reserved0 = 0;
    std::uint32_t chunk_ordinal = 0;
    std::uint32_t reserved1 = 0;
};

struct LoadedProjectBinary {
    HeaderV1 header;
    std::vector<TocEntry> toc;
    std::set<std::string> loaded_chunks;
};

std::uint32_t Crc32(const std::uint8_t* data, std::size_t len);
void ValidateHeader(const HeaderV1& header, std::uint64_t actual_size, const std::uint8_t* raw44);
LoadedProjectBinary LoadProjectBinary(const std::vector<std::uint8_t>& bytes);

void ValidateProjectPayload(const JsonValue& payload);
void ValidateSpecsetPayload(const JsonValue& payload);
void WriteAuditRequired(const std::string& audit_path, const std::string& event_json_line);

// -------------------------
// Governance contracts
// -------------------------

struct BlockerRow {
    std::string blocker_id;
    std::string severity;
    std::string status;
    std::string source_type;
    std::string source_id;
    std::string opened_at;
    std::string updated_at;
    std::string owner;
    std::string summary;
};

void ValidateBlockerRows(const std::vector<BlockerRow>& rows);
void ValidateRejectCodeReferences(const std::set<std::string>& referenced_codes,
                                  const std::set<std::string>& registry_codes);
void EnforceGovernanceGate(bool allowed,
                           const std::function<void()>& apply_action,
                           const std::function<void(const std::string&)>& audit_write);

// -------------------------
// UI workflow contracts
// -------------------------

struct SuggestionCandidate {
    std::string token;
    int context_weight = 0;
};

struct Snippet {
    std::string snippet_id;
    std::string name;
    std::string body;
    std::string scope;  // global|project|connection
    std::string created_at;
    std::string updated_at;
};

struct QueryHistoryRow {
    std::string query_id;
    std::string profile_id;
    std::string started_at_utc;
    std::int64_t duration_ms = 0;
    std::string status;
    std::string error_code;
    std::string sql_hash;
};

struct SchemaCompareOperation {
    std::string operation_id;
    std::string object_class;
    std::string object_path;
    std::string operation_type;  // add|alter|rename|drop
    std::string ddl_statement;
};

struct DataCompareRow {
    std::vector<std::string> key;
    JsonValue payload;
};

struct DataCompareResult {
    std::vector<DataCompareRow> only_left;
    std::vector<DataCompareRow> only_right;
    std::vector<std::pair<DataCompareRow, DataCompareRow>> different;
    std::vector<std::pair<DataCompareRow, DataCompareRow>> equal;
};

struct PlanNode {
    int node_id = 0;
    int parent_id = -1;
    std::string operator_name;
    double estimated_rows = 0;
    double estimated_cost = 0;
    std::string predicate_summary;
};

struct BuilderApplyResult {
    std::string mode;  // editable|read_only
    std::string sql;
};

std::vector<std::string> SortedSuggestions(const std::vector<SuggestionCandidate>& candidates,
                                           const std::string& prefix,
                                           const std::function<int(std::string_view, std::string_view)>& fuzzy_distance);
std::string SnippetInsertExact(const Snippet& snippet);
std::vector<QueryHistoryRow> PruneHistory(const std::vector<QueryHistoryRow>& rows,
                                          const std::string& cutoff_utc,
                                          std::size_t max_rows = 10000);
std::string ExportHistoryCsv(const std::vector<QueryHistoryRow>& rows);

std::vector<SchemaCompareOperation> StableSortOps(const std::vector<SchemaCompareOperation>& ops);
DataCompareResult RunDataCompareKeyed(const std::vector<DataCompareRow>& left,
                                      const std::vector<DataCompareRow>& right);
std::string GenerateMigrationScript(const std::vector<SchemaCompareOperation>& ops,
                                    const std::string& compare_timestamp_utc,
                                    const std::string& left_source,
                                    const std::string& right_source);
std::map<int, std::vector<PlanNode>> OrderPlanNodes(const std::vector<PlanNode>& nodes);
BuilderApplyResult ApplyBuilderGraph(bool has_unsupported_construct,
                                     bool strict_builder,
                                     const std::string& emitted_sql,
                                     bool canonical_equivalent);
std::vector<std::pair<std::string, std::string>> BuildToolsMenu();
std::string CoverageBadge(const std::string& design, const std::string& development, const std::string& management);
void ApplySecurityPolicyAction(bool has_permission, const std::string& permission_key,
                               const std::function<void()>& action);
struct SurfaceVisibilityState {
    bool embedded_visible = false;
    bool detached_visible = false;
};
void ValidateEmbeddedDetachedExclusivity(
    const std::map<std::string, SurfaceVisibilityState>& visibility_by_surface);
SurfaceVisibilityState ApplyDockingRule(bool detached_visible,
                                        bool dock_action_requested,
                                        double overlap_ratio);
void ValidateUiWorkflowState(const std::string& workflow_id,
                             bool capability_ready,
                             bool state_ready);
std::string ResolveIconSlot(const std::string& slot,
                            const std::map<std::string, std::string>& icon_map,
                            const std::string& fallback_icon);
std::string BuildSpecWorkspaceSummary(const std::map<std::string, int>& gap_counts);

// -------------------------
// Diagram contracts
// -------------------------

struct DiagramNode {
    std::string node_id;
    std::string object_type;
    std::string parent_node_id;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::string logical_datatype;
    std::string name;
    std::vector<std::string> attributes;
    std::string notes;
    std::vector<std::string> tags;
    std::vector<std::string> trace_refs;
    std::string icon_slot;
    std::string display_mode;
    bool collapsed = false;
    bool pinned = false;
    bool ghosted = false;
    int stack_count = 1;
};

struct DiagramEdge {
    std::string edge_id;
    std::string from_node_id;
    std::string to_node_id;
    std::string relation_type;
    std::string label;
    std::string edge_type;
    bool directed = true;
    bool identifying = false;
    std::string source_cardinality;
    std::string target_cardinality;
};

struct DiagramDocument {
    std::string diagram_id;
    std::string notation;
    std::string diagram_type;
    int grid_size = 20;
    std::string alignment_policy;
    std::string drop_policy;
    std::string resize_policy;
    std::string display_profile;
    std::vector<DiagramNode> nodes;
    std::vector<DiagramEdge> edges;
};

std::vector<std::string> PaletteTokensForDiagramType(const std::string& diagram_type);
void ValidatePaletteModeExclusivity(bool docked_visible, bool floating_visible);
DiagramNode BuildNodeFromPaletteToken(const std::string& diagram_type,
                                      const std::string& token,
                                      int x,
                                      int y,
                                      int width = 160,
                                      int height = 80);
void ValidateNotation(const std::string& notation);
void ValidateCanvasOperation(const DiagramDocument& doc,
                             const std::string& operation,
                             const std::string& node_id,
                             const std::string& target_parent_id);
std::string SerializeDiagramModel(const DiagramDocument& doc);
DiagramDocument ParseDiagramModel(const std::string& payload_json);
std::vector<std::string> ForwardEngineerDatatypes(const std::vector<std::string>& logical_types,
                                                  const std::map<std::string, std::string>& mapping);
std::string ExportDiagram(const DiagramDocument& doc, const std::string& format, const std::string& profile_id);

// -------------------------
// Reporting contracts
// -------------------------

struct ReportingAsset {
    std::string id;
    std::string asset_type;
    std::string name;
    std::string payload_json;
    std::string collection_id;
    std::string created_at_utc;
    std::string updated_at_utc;
    std::string created_by;
    std::string updated_by;
};

struct ReportingSchedule {
    std::string schedule_spec;
    std::string schedule_dtstart_local;
    std::string timezone;
    std::vector<std::string> schedule_rdates_local;
    std::vector<std::string> schedule_exdates_local;
};

struct ActivityRow {
    std::string timestamp_utc;
    std::string metric_key;
    double value = 0.0;
};

std::vector<ReportingAsset> CanonicalArtifactOrder(const std::vector<ReportingAsset>& rows);
std::string CanonicalizeRRule(const std::map<std::string, std::string>& key_values);
void ValidateAnchorUntil(const ReportingSchedule& schedule);
std::vector<std::string> ExpandRRuleBounded(const ReportingSchedule& schedule,
                                            const std::string& now_utc,
                                            std::size_t max_candidates = 4096);
std::string NextRun(const ReportingSchedule& schedule, const std::string& now_utc);

std::string RunQuestion(bool question_exists,
                        const std::string& normalized_sql,
                        const std::function<std::string(const std::string&)>& exec,
                        const std::function<bool(const std::string&)>& persist_result_fn);
std::string RunDashboardRuntime(const std::string& dashboard_id,
                                const std::vector<std::pair<std::string, std::string>>& widget_statuses,
                                bool cache_hit);
void PersistResult(const std::string& key,
                   const std::string& result_payload,
                   std::map<std::string, std::string>* storage);
std::string ExportReportingRepository(const std::vector<ReportingAsset>& assets);
std::vector<ReportingAsset> ImportReportingRepository(const std::string& payload_json);

std::vector<ActivityRow> RunActivityWindowQuery(const std::vector<ActivityRow>& source,
                                                const std::string& window,
                                                const std::set<std::string>& allowed_metrics);
std::string ExportActivity(const std::vector<ActivityRow>& rows, const std::string& fmt);

// -------------------------
// Advanced contracts
// -------------------------

std::string RunCdcEvent(const std::string& event_payload,
                        int max_attempts,
                        int backoff_ms,
                        const std::function<bool(const std::string&)>& publish,
                        const std::function<void(const std::string&)>& dead_letter);
std::vector<std::map<std::string, std::string>> PreviewMask(
    const std::vector<std::map<std::string, std::string>>& rows,
    const std::map<std::string, std::string>& rules);
void CheckReviewQuorum(int approved_count, int min_reviewers);
void RequireChangeAdvisory(const std::string& action_id, const std::string& advisory_state);
void ValidateExtension(bool signature_ok, bool compatibility_ok);
void EnforceExtensionAllowlist(const std::set<std::string>& requested_capabilities,
                               const std::set<std::string>& allowlist);
std::pair<std::vector<std::string>, int> BuildLineage(const std::vector<std::string>& node_ids,
                                                       const std::vector<std::pair<std::string, std::optional<std::string>>>& edges);
std::map<std::string, std::optional<std::string>> RegisterOptionalSurfaces(const std::string& profile_id);
void ValidateAiProviderConfig(const std::string& provider_id,
                              bool async_enabled,
                              const std::string& endpoint_or_model,
                              const std::optional<std::string>& credential);
void ValidateIssueTrackerConfig(const std::string& provider_id,
                                const std::string& project_or_repo,
                                const std::optional<std::string>& credential);
void ValidateGitSyncState(bool branch_selected,
                          bool remote_reachable,
                          bool conflicts_resolved);

// -------------------------
// Packaging + spec support
// -------------------------

struct PackageValidationResult {
    bool ok = false;
    std::string profile_id;
};

std::string CanonicalBuildHash(const std::string& full_commit_id);
PackageValidationResult ValidateProfileManifest(const JsonValue& manifest,
                                                const std::set<std::string>& surface_registry,
                                                const std::set<std::string>& backend_enum);
void ValidateSurfaceRegistry(const JsonValue& manifest, const std::set<std::string>& surface_registry);
void ValidatePackageArtifacts(const std::set<std::string>& packaged_paths);

struct SpecSetManifest {
    std::string set_id;
    std::string package_root;
    std::string authoritative_inventory_relpath;
    std::string version_stamp;
    std::string package_hash_sha256;
};

struct SpecFileRow {
    std::string set_id;
    std::string relative_path;
    bool is_normative = false;
    std::string content_hash;
    std::uint64_t size_bytes = 0;
};

std::vector<std::string> DiscoverSpecsets(const std::string& spec_root);
SpecSetManifest LoadSpecsetManifest(const std::string& manifest_path);
std::vector<std::string> ParseAuthoritativeInventory(const std::string& inventory_path);
std::vector<SpecFileRow> LoadSpecsetPackage(const std::string& manifest_path);
void AssertSupportComplete(const std::vector<SpecFileRow>& spec_files,
                           const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links,
                           const std::string& coverage_class);
void ValidateBindings(const std::vector<std::string>& binding_case_ids,
                      const std::set<std::string>& conformance_case_ids);
std::map<std::string, int> AggregateSupport(const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links);
std::string ExportWorkPackage(const std::string& set_id,
                              const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& gaps,
                              const std::string& generated_at_utc);

// -------------------------
// Alpha preservation contracts
// -------------------------

struct AlphaMirrorEntry {
    std::string relative_path;
    std::uint64_t expected_size = 0;
    std::string expected_sha256;
};

void ValidateAlphaMirrorPresence(const std::string& mirror_root,
                                 const std::vector<AlphaMirrorEntry>& entries);
void ValidateAlphaMirrorHashes(const std::string& mirror_root,
                               const std::vector<AlphaMirrorEntry>& entries);
void ValidateSilverstonContinuity(const std::set<std::string>& present_artifacts,
                                  const std::set<std::string>& required_artifacts);
void ValidateAlphaInventoryMapping(const std::set<std::string>& required_element_ids,
                                   const std::map<std::string, std::string>& file_to_element_id);
void ValidateAlphaExtractionGate(bool extraction_passed, bool continuity_passed, bool deep_contract_passed);

}  // namespace scratchrobin::beta1b
