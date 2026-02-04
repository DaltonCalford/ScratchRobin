/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_USERS_ROLES_FRAME_H
#define SCRATCHROBIN_USERS_ROLES_FRAME_H

#include <vector>

#include <wx/bookctrl.h>
#include <wx/frame.h>

#include "core/connection_manager.h"
#include "core/config.h"
#include "core/job_queue.h"

class wxButton;
class wxChoice;
class wxGrid;
class wxNotebook;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

class ResultGridTable;
class WindowManager;

class UsersRolesFrame : public wxFrame {
public:
    UsersRolesFrame(WindowManager* windowManager,
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
    void OnOpenMonitoring(wxCommandEvent& event);
    void OnOpenJobScheduler(wxCommandEvent& event);
    void OnOpenDomainManager(wxCommandEvent& event);
    void OnOpenSchemaManager(wxCommandEvent& event);
    void OnOpenTableDesigner(wxCommandEvent& event);
    void OnOpenIndexDesigner(wxCommandEvent& event);
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnConnectionChanged(wxCommandEvent& event);
    void OnTabChanged(wxBookCtrlEvent& event);
    void OnCreateUser(wxCommandEvent& event);
    void OnAlterUser(wxCommandEvent& event);
    void OnDropUser(wxCommandEvent& event);
    void OnCreateRole(wxCommandEvent& event);
    void OnAlterRole(wxCommandEvent& event);
    void OnDropRole(wxCommandEvent& event);
    void OnGrantRole(wxCommandEvent& event);
    void OnRevokeRole(wxCommandEvent& event);
    void OnGrantPrivileges(wxCommandEvent& event);
    void OnRevokePrivileges(wxCommandEvent& event);
    void OnGrantMembership(wxCommandEvent& event);
    void OnRevokeMembership(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void RefreshActiveTab();
    void OpenSqlTemplate(const std::string& sql);
    std::string SelectedGridValue(wxGrid* grid) const;
    std::string SelectedGridValueAt(wxGrid* grid, int column) const;

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;

    wxChoice* connection_choice_ = nullptr;
    wxButton* connect_button_ = nullptr;
    wxButton* disconnect_button_ = nullptr;
    wxButton* refresh_button_ = nullptr;
    wxNotebook* notebook_ = nullptr;

    wxGrid* users_grid_ = nullptr;
    ResultGridTable* users_table_ = nullptr;
    wxButton* create_user_button_ = nullptr;
    wxButton* drop_user_button_ = nullptr;

    wxGrid* roles_grid_ = nullptr;
    ResultGridTable* roles_table_ = nullptr;
    wxButton* create_role_button_ = nullptr;
    wxButton* drop_role_button_ = nullptr;
    wxButton* grant_role_button_ = nullptr;
    wxButton* revoke_role_button_ = nullptr;

    wxGrid* memberships_grid_ = nullptr;
    ResultGridTable* memberships_table_ = nullptr;
    wxButton* grant_membership_button_ = nullptr;
    wxButton* revoke_membership_button_ = nullptr;

    wxStaticText* status_label_ = nullptr;
    wxTextCtrl* message_log_ = nullptr;

    int users_tab_index_ = 0;
    int roles_tab_index_ = 1;
    int memberships_tab_index_ = 2;

    JobHandle connect_job_;
    JobHandle query_job_;
    bool connect_running_ = false;
    bool query_running_ = false;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_USERS_ROLES_FRAME_H
