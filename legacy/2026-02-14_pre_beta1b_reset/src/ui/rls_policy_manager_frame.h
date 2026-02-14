/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_RLS_POLICY_MANAGER_FRAME_H
#define SCRATCHROBIN_RLS_POLICY_MANAGER_FRAME_H

#include <string>
#include <vector>

#include <wx/frame.h>

#include "core/connection_manager.h"

class wxButton;
class wxChoice;
class wxGrid;
class wxGridEvent;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

class ResultGridTable;
class WindowManager;
struct AppConfig;

class RlsPolicyManagerFrame : public wxFrame {
public:
    RlsPolicyManagerFrame(WindowManager* windowManager,
                          ConnectionManager* connectionManager,
                          const std::vector<ConnectionProfile>* connections,
                          const AppConfig* appConfig);
    ~RlsPolicyManagerFrame() override;
    wxDECLARE_EVENT_TABLE();

private:
    void BuildMenu();
    void BuildLayout();
    void PopulateConnections();
    void UpdateControls();
    void UpdateStatus(const wxString& status);
    void SetMessage(const std::string& message);
    bool EnsureConnected(const ConnectionProfile& profile);
    const ConnectionProfile* GetSelectedProfile() const;
    void RefreshPolicies();
    void RefreshPolicyDetails(const std::string& policy_id);
    std::string GetSelectedPolicyId() const;
    std::string GetSelectedPolicyName() const;
    std::string GetSelectedTableName() const;
    int FindColumnIndex(const QueryResult& result,
                        const std::vector<std::string>& names) const;
    std::string ExtractValue(const QueryResult& result,
                             int row,
                             const std::vector<std::string>& names) const;
    std::string FormatDetails(const QueryResult& result) const;
    void RunCommand(const std::string& sql, const std::string& success_message);

    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnCreate(wxCommandEvent& event);
    void OnEdit(wxCommandEvent& event);
    void OnDrop(wxCommandEvent& event);
    void OnEnableTable(wxCommandEvent& event);
    void OnDisableTable(wxCommandEvent& event);
    void OnPolicySelected(wxGridEvent& event);
    void OnClose(wxCloseEvent& event);

    void OnNewSqlEditor(wxCommandEvent& event);
    void OnNewDiagram(wxCommandEvent& event);
    void OnOpenMonitoring(wxCommandEvent& event);
    void OnOpenUsersRoles(wxCommandEvent& event);
    void OnOpenJobScheduler(wxCommandEvent& event);
    void OnOpenSchemaManager(wxCommandEvent& event);
    void OnOpenDomainManager(wxCommandEvent& event);
    void OnOpenTableDesigner(wxCommandEvent& event);
    void OnOpenIndexDesigner(wxCommandEvent& event);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;

    wxChoice* connection_choice_ = nullptr;
    wxButton* connect_button_ = nullptr;
    wxButton* disconnect_button_ = nullptr;
    wxButton* refresh_button_ = nullptr;
    wxButton* create_button_ = nullptr;
    wxButton* edit_button_ = nullptr;
    wxButton* drop_button_ = nullptr;
    wxButton* enable_button_ = nullptr;
    wxButton* disable_button_ = nullptr;
    wxGrid* policy_grid_ = nullptr;
    ResultGridTable* policy_table_ = nullptr;
    wxTextCtrl* details_text_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxStaticText* message_label_ = nullptr;

    QueryResult policies_result_;
    QueryResult policy_details_result_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_RLS_POLICY_MANAGER_FRAME_H
