#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "advanced/advanced_services.h"
#include "connection/connection_services.h"
#include "diagram/diagram_services.h"
#include "project/project_services.h"
#include "reporting/reporting_services.h"
#include "runtime/runtime_config.h"
#include "core/beta1b_contracts.h"
#include "runtime/runtime_services.h"
#include "ui/ui_workflow_services.h"

#include <wx/frame.h>
#include <wx/treectrl.h>

class wxChoice;
class wxCheckBox;
class wxBookCtrlEvent;
class wxListCtrl;
class wxMenu;
class wxNotebook;
class wxPanel;
class wxSplitterWindow;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin::gui {

class DiagramCanvasPanel;

class MainFrame final : public wxFrame {
public:
    MainFrame(const runtime::StartupReport& report, const std::filesystem::path& repo_root);

private:
    enum CommandId {
        kCmdConnect = wxID_HIGHEST + 100,
        kCmdDisconnect,
        kCmdRunSql,
        kCmdCancelSql,
        kCmdExportHistoryCsv,
        kCmdSaveObject,
        kCmdGenerateMigration,
        kCmdOpenDiagramLink,
        kCmdExportDiagramSvg,
        kCmdExportDiagramPng,
        kCmdRefreshSpecWorkspace,
        kCmdRefreshMonitoring,
        kCmdOpenSqlEditorFrame,
        kCmdOpenObjectEditorFrame,
        kCmdOpenDiagramFrame,
        kCmdOpenMonitoringFrame,
        kCmdOpenReportingFrame,
        kCmdOpenDataMaskingFrame,
        kCmdOpenCdcConfigFrame,
        kCmdOpenGitIntegrationFrame,
        kCmdOpenSpecWorkspaceFrame,
        kCmdOpenSchemaManagerFrame,
        kCmdOpenTableManagerFrame,
        kCmdOpenIndexManagerFrame,
        kCmdOpenDomainManagerFrame,
        kCmdOpenSequenceManagerFrame,
        kCmdOpenViewManagerFrame,
        kCmdOpenTriggerManagerFrame,
        kCmdOpenProcedureManagerFrame,
        kCmdOpenPackageManagerFrame,
        kCmdOpenUsersManagerFrame,
        kCmdOpenJobsManagerFrame,
        kCmdOpenStorageManagerFrame,
        kCmdOpenBackupManagerFrame,
        kCmdTreeCopyObjectName,
        kCmdTreeCopyDdl,
        kCmdTreeShowDependencies,
        kCmdTreeRefreshNode,
    };

    void BuildMenu();
    void BuildToolbar();
    void BuildLayout();
    void LoadProfiles();
    void SeedUiState();

    void EnsureConnected();
    void RefreshCatalog();
    void RefreshHistory();
    void RefreshMonitoring();
    void RefreshSpecWorkspace();
    void RefreshPlan(const std::string& sql);
    void AppendLogLine(const std::string& line);

    void PopulateHistoryList(wxListCtrl* target);
    void SeedDiagramLinks(wxListCtrl* target);
    void RefreshMonitoringList(wxListCtrl* target);
    void RefreshSpecWorkspaceControls(wxChoice* set_choice,
                                      wxTextCtrl* summary,
                                      wxTextCtrl* dashboard,
                                      wxTextCtrl* work_package);

    void RunSqlIntoSurface(const std::string& sql,
                           wxListCtrl* results,
                           wxTextCtrl* status,
                           wxListCtrl* history,
                           bool focus_plan_tab);
    void CancelSqlIntoStatus(wxTextCtrl* status);
    void ExportHistoryIntoStatus(wxTextCtrl* status,
                                 wxListCtrl* history);

    void SaveObjectFromControls(wxChoice* object_class,
                                wxTextCtrl* object_path,
                                wxTextCtrl* object_ddl);
    void GenerateMigrationIntoControls(wxChoice* object_class,
                                       wxTextCtrl* object_path,
                                       wxTextCtrl* object_ddl);

    void OpenDiagramFromControls(wxListCtrl* links,
                                 wxTextCtrl* output);
    void OpenDiagramByTypeAndName(const std::string& type_name,
                                  const std::string& diagram_name,
                                  wxTextCtrl* output);
    void ExportDiagramToOutput(const std::string& format,
                               wxTextCtrl* output);
    void ExecuteCanvasAction(DiagramCanvasPanel* canvas,
                             wxTextCtrl* output,
                             const std::function<bool(std::string*)>& action);
    void RefreshDiagramPresentation();
    void RefreshDiagramPaletteControls(const std::string& type_name);
    void PopulateDiagramPaletteList(wxListCtrl* list,
                                    const std::string& type_name);
    void ToggleDiagramPaletteDetached(bool detach);
    void BindDiagramPaletteInteractions(wxListCtrl* list);
    void SelectWorkspacePage(int page_index);
    void EnsureWorkspacePageBuilt(int page_index);
    void BindSurfaceDetachDrag(wxWindow* handle, int page_index);
    void DetachSurfaceByDrag(int page_index, const wxPoint& screen_pos);
    void BindDiagramPaletteDetachDrag(wxWindow* handle);
    void EnsureDetachedSurfaceNotEmbedded(int page_index);
    void CloseDetachedSurfaceForPage(int page_index);
    void OnWorkspaceNotebookPageChanged(wxBookCtrlEvent& event);
    void BindDetachedFrameDropDock(wxFrame* frame, int page_index);

    void OpenOrFocusSqlEditorFrame();
    void OpenOrFocusObjectEditorFrame();
    void OpenOrFocusDiagramFrame();
    void OpenOrFocusMonitoringFrame();
    void OpenOrFocusReportingFrame();
    void OpenOrFocusDataMaskingFrame();
    void OpenOrFocusCdcConfigFrame();
    void OpenOrFocusGitIntegrationFrame();
    void OpenOrFocusSpecWorkspaceFrame();
    void OpenOrFocusAdminManager(const std::string& manager_key);
    void OpenAdminManagerByCommand(int command_id);

    std::string AdminManagerKeyForCommand(int command_id) const;
    std::string AdminManagerTitle(const std::string& manager_key) const;
    std::string AdminManagerDescription(const std::string& manager_key) const;
    std::string AdminManagerDefaultPath(const std::string& manager_key) const;
    std::string AdminManagerDefaultTemplate(const std::string& manager_key) const;
    std::string AdminManagerNodeLabelForKey(const std::string& manager_key) const;
    std::string AdminManagerKeyForNodeLabel(const std::string& node_label) const;

    std::string ActiveProfileName() const;
    runtime::ConnectionProfile ActiveProfile() const;
    std::filesystem::path SpecRootPath() const;
    std::filesystem::path ManifestPathForSet(const std::string& set_id) const;

    wxPanel* BuildSqlEditorTab(wxWindow* parent);
    wxPanel* BuildObjectEditorTab(wxWindow* parent);
    wxPanel* BuildDiagramTab(wxWindow* parent);
    wxPanel* BuildPlanTab(wxWindow* parent);
    wxPanel* BuildSpecWorkspaceTab(wxWindow* parent);
    wxPanel* BuildMonitoringTab(wxWindow* parent);

    wxMenu* BuildTreeContextMenu() const;

    void OnAboutMenu(wxCommandEvent& event);
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnRunSql(wxCommandEvent& event);
    void OnCancelSql(wxCommandEvent& event);
    void OnExportHistoryCsv(wxCommandEvent& event);
    void OnSaveObject(wxCommandEvent& event);
    void OnGenerateMigration(wxCommandEvent& event);
    void OnOpenDiagramLink(wxCommandEvent& event);
    void OnExportDiagramSvg(wxCommandEvent& event);
    void OnExportDiagramPng(wxCommandEvent& event);
    void OnRefreshSpecWorkspace(wxCommandEvent& event);
    void OnRefreshMonitoring(wxCommandEvent& event);
    void OnOpenSqlEditorFrame(wxCommandEvent& event);
    void OnOpenObjectEditorFrame(wxCommandEvent& event);
    void OnOpenDiagramFrame(wxCommandEvent& event);
    void OnOpenMonitoringFrame(wxCommandEvent& event);
    void OnOpenReportingFrame(wxCommandEvent& event);
    void OnOpenDataMaskingFrame(wxCommandEvent& event);
    void OnOpenCdcConfigFrame(wxCommandEvent& event);
    void OnOpenGitIntegrationFrame(wxCommandEvent& event);
    void OnOpenSpecWorkspaceFrame(wxCommandEvent& event);
    void OnOpenAdminManager(wxCommandEvent& event);
    void OnTreeContext(wxTreeEvent& event);
    void OnTreeActivate(wxTreeEvent& event);
    void OnTreeCopyObjectName(wxCommandEvent& event);
    void OnTreeCopyDdl(wxCommandEvent& event);
    void OnTreeShowDependencies(wxCommandEvent& event);
    void OnTreeRefreshNode(wxCommandEvent& event);
    void OnExitMenu(wxCommandEvent& event);

    runtime::StartupReport startup_report_;
    std::filesystem::path repo_root_;
    runtime::ConfigStore config_store_;
    std::vector<runtime::ConnectionProfile> profiles_;

    connection::BackendAdapterService adapter_;
    project::SpecSetService specset_service_;
    ui::UiWorkflowService ui_service_;
    diagram::DiagramService diagram_service_;
    reporting::ReportingService reporting_service_;
    advanced::AdvancedService advanced_service_;

    std::int64_t next_query_id_ = 1;
    beta1b::DiagramDocument active_diagram_;
    std::string active_diagram_name_ = "Core Domain ERD";
    static constexpr int kWorkspacePageSql = 0;
    static constexpr int kWorkspacePageObject = 1;
    static constexpr int kWorkspacePageDiagram = 2;
    static constexpr int kWorkspacePagePlan = 3;
    static constexpr int kWorkspacePageSpec = 4;
    static constexpr int kWorkspacePageMonitoring = 5;
    std::array<bool, 6> workspace_page_built_ = {{false, false, false, false, false, false}};

    wxChoice* profile_choice_ = nullptr;
    wxTreeCtrl* tree_ = nullptr;
    wxNotebook* workspace_notebook_ = nullptr;

    wxTextCtrl* sql_editor_ = nullptr;
    wxListCtrl* sql_results_ = nullptr;
    wxListCtrl* sql_history_ = nullptr;
    wxTextCtrl* sql_status_ = nullptr;

    wxChoice* object_class_ = nullptr;
    wxTextCtrl* object_path_ = nullptr;
    wxTextCtrl* object_ddl_ = nullptr;

    wxListCtrl* diagram_links_ = nullptr;
    wxStaticText* diagram_heading_ = nullptr;
    wxChoice* diagram_type_choice_ = nullptr;
    wxTextCtrl* diagram_name_input_ = nullptr;
    wxTextCtrl* diagram_output_ = nullptr;
    DiagramCanvasPanel* diagram_canvas_ = nullptr;
    wxCheckBox* diagram_grid_toggle_ = nullptr;
    wxCheckBox* diagram_snap_toggle_ = nullptr;
    wxSplitterWindow* diagram_splitter_ = nullptr;
    wxPanel* diagram_palette_panel_docked_ = nullptr;
    wxPanel* diagram_canvas_panel_ = nullptr;
    wxListCtrl* diagram_palette_list_docked_ = nullptr;
    wxFrame* diagram_palette_frame_ = nullptr;
    wxListCtrl* diagram_palette_list_floating_ = nullptr;
    std::map<std::string, std::vector<std::string>> diagram_palette_custom_items_;

    wxListCtrl* plan_rows_ = nullptr;

    wxChoice* specset_choice_ = nullptr;
    wxTextCtrl* spec_summary_ = nullptr;
    wxTextCtrl* spec_dashboard_ = nullptr;
    wxTextCtrl* spec_work_package_ = nullptr;

    wxListCtrl* monitoring_rows_ = nullptr;
    wxTextCtrl* log_output_ = nullptr;

    wxFrame* sql_editor_frame_ = nullptr;
    wxTextCtrl* sql_editor_detached_ = nullptr;
    wxListCtrl* sql_results_detached_ = nullptr;
    wxTextCtrl* sql_status_detached_ = nullptr;
    wxListCtrl* sql_history_detached_ = nullptr;

    wxFrame* object_editor_frame_ = nullptr;
    wxChoice* object_class_detached_ = nullptr;
    wxTextCtrl* object_path_detached_ = nullptr;
    wxTextCtrl* object_ddl_detached_ = nullptr;

    wxFrame* diagram_frame_ = nullptr;
    wxListCtrl* diagram_links_detached_ = nullptr;
    wxStaticText* diagram_heading_detached_ = nullptr;
    wxChoice* diagram_type_choice_detached_ = nullptr;
    wxTextCtrl* diagram_name_input_detached_ = nullptr;
    wxTextCtrl* diagram_output_detached_ = nullptr;
    DiagramCanvasPanel* diagram_canvas_detached_ = nullptr;
    wxCheckBox* diagram_grid_toggle_detached_ = nullptr;
    wxCheckBox* diagram_snap_toggle_detached_ = nullptr;
    wxListCtrl* diagram_palette_list_detached_ = nullptr;

    wxFrame* monitoring_frame_ = nullptr;
    wxListCtrl* monitoring_rows_detached_ = nullptr;

    wxFrame* reporting_frame_ = nullptr;
    wxTextCtrl* reporting_sql_detached_ = nullptr;
    wxTextCtrl* reporting_status_detached_ = nullptr;
    wxTextCtrl* reporting_dashboard_output_detached_ = nullptr;
    wxListCtrl* reporting_repository_rows_detached_ = nullptr;

    wxFrame* data_masking_frame_ = nullptr;
    wxFrame* cdc_config_frame_ = nullptr;
    wxFrame* git_integration_frame_ = nullptr;

    wxFrame* spec_workspace_frame_ = nullptr;
    wxChoice* specset_choice_detached_ = nullptr;
    wxTextCtrl* spec_summary_detached_ = nullptr;
    wxTextCtrl* spec_dashboard_detached_ = nullptr;
    wxTextCtrl* spec_work_package_detached_ = nullptr;

    std::map<std::string, wxFrame*> admin_manager_frames_;
};

}  // namespace scratchrobin::gui
