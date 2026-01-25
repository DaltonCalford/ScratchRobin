#ifndef SCRATCHROBIN_MONITORING_FRAME_H
#define SCRATCHROBIN_MONITORING_FRAME_H

#include <string>
#include <vector>

#include <wx/frame.h>

#include "core/connection_manager.h"
#include "core/job_queue.h"
#include "core/config.h"

class wxButton;
class wxChoice;
class wxGrid;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

class ResultGridTable;
class WindowManager;

class MonitoringFrame : public wxFrame {
public:
    MonitoringFrame(WindowManager* windowManager,
                    ConnectionManager* connectionManager,
                    const std::vector<ConnectionProfile>* connections,
                    const AppConfig* appConfig);

private:
    void BuildMenu();
    void BuildLayout();
    void PopulateConnections();
    const ConnectionProfile* GetSelectedProfile() const;
    void UpdateControls();
    void UpdateStatus(const std::string& message);
    void SetMessage(const std::string& message);

    void OnNewSqlEditor(wxCommandEvent& event);
    void OnNewDiagram(wxCommandEvent& event);
    void OnOpenUsersRoles(wxCommandEvent& event);
    void OnOpenJobScheduler(wxCommandEvent& event);
    void OnOpenDomainManager(wxCommandEvent& event);
    void OnOpenSchemaManager(wxCommandEvent& event);
    void OnOpenTableDesigner(wxCommandEvent& event);
    void OnOpenIndexDesigner(wxCommandEvent& event);
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnConnectionChanged(wxCommandEvent& event);
    void OnViewChanged(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;

    wxChoice* connection_choice_ = nullptr;
    wxButton* connect_button_ = nullptr;
    wxButton* disconnect_button_ = nullptr;
    wxChoice* view_choice_ = nullptr;
    wxButton* refresh_button_ = nullptr;
    wxGrid* result_grid_ = nullptr;
    ResultGridTable* result_table_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxTextCtrl* message_log_ = nullptr;

    JobHandle connect_job_;
    JobHandle query_job_;
    bool connect_running_ = false;
    bool query_running_ = false;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_MONITORING_FRAME_H
