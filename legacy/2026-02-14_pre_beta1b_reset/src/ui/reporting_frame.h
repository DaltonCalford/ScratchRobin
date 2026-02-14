/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#ifndef SCRATCHROBIN_REPORTING_FRAME_H
#define SCRATCHROBIN_REPORTING_FRAME_H

#include <vector>

#include <wx/frame.h>
#include <wx/timer.h>

#include "core/config.h"

class wxChoice;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

class WindowManager;
class ConnectionManager;
struct ConnectionProfile;

class ReportingFrame : public wxFrame {
public:
    ReportingFrame(WindowManager* windowManager,
                   ConnectionManager* connectionManager,
                   std::vector<ConnectionProfile>* connections,
                   const AppConfig* appConfig);

private:
    void BuildMenu();
    void BuildLayout();
    void PopulateConnections();
    const ConnectionProfile* GetSelectedProfile() const;
    bool EnsureConnected(const ConnectionProfile& profile);
    void SetStatus(const wxString& message);
    void OnScheduleTick(wxTimerEvent& event);

    void OnRunQuery(wxCommandEvent& event);
    void OnSaveQuery(wxCommandEvent& event);
    void OnScheduleQuery(wxCommandEvent& event);
    void OnRefreshPreview(wxCommandEvent& event);
    void OnAddAlert(wxCommandEvent& event);
    void OnScheduleAlert(wxCommandEvent& event);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;
    wxChoice* connection_choice_ = nullptr;
    wxTextCtrl* sql_editor_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxTimer schedule_timer_;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_REPORTING_FRAME_H
