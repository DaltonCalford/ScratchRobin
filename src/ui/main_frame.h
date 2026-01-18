#ifndef SCRATCHROBIN_MAIN_FRAME_H
#define SCRATCHROBIN_MAIN_FRAME_H

#include <wx/frame.h>
#include <wx/notebook.h>
#include <wx/treectrl.h>

#include "core/config.h"
#include "core/metadata_model.h"

class wxTextCtrl;

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
    void OnQuit(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnTreeSelection(wxTreeEvent& event);
    void PopulateTree(const MetadataSnapshot& snapshot);
    void UpdateInspector(const MetadataNode* node);

    WindowManager* window_manager_ = nullptr;
    MetadataModel* metadata_model_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;
    wxTreeCtrl* tree_ = nullptr;
    wxNotebook* inspector_notebook_ = nullptr;
    wxTextCtrl* overview_text_ = nullptr;
    wxTextCtrl* ddl_text_ = nullptr;
    wxTextCtrl* deps_text_ = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_MAIN_FRAME_H
