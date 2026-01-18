#ifndef SCRATCHROBIN_MAIN_FRAME_H
#define SCRATCHROBIN_MAIN_FRAME_H

#include <wx/frame.h>
#include <wx/treectrl.h>

#include "core/metadata_model.h"

namespace scratchrobin {

class WindowManager;

class MainFrame : public wxFrame, public MetadataObserver {
public:
    MainFrame(WindowManager* windowManager,
              MetadataModel* metadataModel,
              ConnectionManager* connectionManager,
              const std::vector<ConnectionProfile>* connections);

    void OnMetadataUpdated(const MetadataSnapshot& snapshot) override;

private:
    void BuildMenu();
    void BuildLayout();

    void OnNewSqlEditor(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void PopulateTree(const MetadataSnapshot& snapshot);

    WindowManager* window_manager_ = nullptr;
    MetadataModel* metadata_model_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    wxTreeCtrl* tree_ = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_MAIN_FRAME_H
