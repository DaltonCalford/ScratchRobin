/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ROLE_SWITCH_POLICY_FRAME_H
#define SCRATCHROBIN_ROLE_SWITCH_POLICY_FRAME_H

#include <vector>

#include <wx/frame.h>

#include "core/connection_manager.h"

class wxButton;
class wxChoice;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

class WindowManager;
struct AppConfig;

class RoleSwitchPolicyFrame : public wxFrame {
public:
    RoleSwitchPolicyFrame(WindowManager* windowManager,
                          ConnectionManager* connectionManager,
                          const std::vector<ConnectionProfile>* connections,
                          const AppConfig* appConfig);
    ~RoleSwitchPolicyFrame() override;
    wxDECLARE_EVENT_TABLE();

private:
    void BuildMenu();
    void BuildLayout();
    void PopulateConnections();
    void UpdateStatus(const wxString& status);
    void SetMessage(const std::string& message);
    bool EnsureConnected(const ConnectionProfile& profile);
    const ConnectionProfile* GetSelectedProfile() const;
    void RefreshPolicy();
    std::string BuildQuery() const;

    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;

    wxChoice* connection_choice_ = nullptr;
    wxButton* connect_button_ = nullptr;
    wxButton* disconnect_button_ = nullptr;
    wxButton* refresh_button_ = nullptr;
    wxTextCtrl* output_ctrl_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxStaticText* message_label_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_ROLE_SWITCH_POLICY_FRAME_H
