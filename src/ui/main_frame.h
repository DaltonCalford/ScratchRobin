#ifndef SCRATCHROBIN_MAIN_FRAME_H
#define SCRATCHROBIN_MAIN_FRAME_H

#include <wx/frame.h>
#include <wx/notebook.h>
#include <wx/treectrl.h>

#include "core/config.h"
#include "core/metadata_model.h"

class wxTextCtrl;
class wxButton;

namespace scratchrobin {

class WindowManager;

class MainFrame : public wxFrame, public MetadataObserver {
public:
    MainFrame(WindowManager* windowManager,
              MetadataModel* metadataModel,
              ConnectionManager* connectionManager,
              const std::vector<ConnectionProfile>* connections,
              const AppConfig* appConfig);

    void OnMetadataUpdated(const MetadataSnapshot& snapshot) override;

private:
    void BuildMenu();
    void BuildLayout();

    void OnNewSqlEditor(wxCommandEvent& event);
    void OnNewDiagram(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnTreeSelection(wxTreeEvent& event);
    void OnTreeItemMenu(wxTreeEvent& event);
    void OnFilterChanged(wxCommandEvent& event);
    void OnFilterClear(wxCommandEvent& event);
    void OnTreeOpenEditor(wxCommandEvent& event);
    void OnOpenMonitoring(wxCommandEvent& event);
    void OnOpenUsersRoles(wxCommandEvent& event);
    void OnOpenJobScheduler(wxCommandEvent& event);
    void OnOpenDomainManager(wxCommandEvent& event);
    void OnOpenSchemaManager(wxCommandEvent& event);
    void OnOpenTableDesigner(wxCommandEvent& event);
    void OnOpenIndexDesigner(wxCommandEvent& event);
    void OnTreeCopyName(wxCommandEvent& event);
    void OnTreeCopyDdl(wxCommandEvent& event);
    void OnTreeShowDependencies(wxCommandEvent& event);
    void OnTreeRefresh(wxCommandEvent& event);
    void PopulateTree(const MetadataSnapshot& snapshot);
    void UpdateInspector(const MetadataNode* node);
    const MetadataNode* GetSelectedNode() const;
    bool HasMatch(const MetadataNode& node, const std::string& filter) const;
    std::string BuildSeedSql(const MetadataNode* node) const;
    bool CopyToClipboard(const std::string& text) const;

    WindowManager* window_manager_ = nullptr;
    MetadataModel* metadata_model_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;
    wxTreeCtrl* tree_ = nullptr;
    wxTextCtrl* filter_ctrl_ = nullptr;
    wxButton* filter_clear_button_ = nullptr;
    wxNotebook* inspector_notebook_ = nullptr;
    wxTextCtrl* overview_text_ = nullptr;
    wxTextCtrl* ddl_text_ = nullptr;
    wxTextCtrl* deps_text_ = nullptr;
    int overview_page_index_ = 0;
    int ddl_page_index_ = 0;
    int deps_page_index_ = 0;
    const MetadataNode* context_node_ = nullptr;
    std::string filter_text_;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_MAIN_FRAME_H
