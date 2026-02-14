/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_AUDIT_POLICY_FRAME_H
#define SCRATCHROBIN_AUDIT_POLICY_FRAME_H

#include <string>
#include <vector>

#include <wx/frame.h>

#include "core/connection_manager.h"

class wxButton;
class wxChoice;
class wxGrid;
class wxGridEvent;
class wxBookCtrlEvent;
class wxNotebook;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

class ResultGridTable;
class WindowManager;
struct AppConfig;

class AuditPolicyFrame : public wxFrame {
public:
    AuditPolicyFrame(WindowManager* windowManager,
                     ConnectionManager* connectionManager,
                     const std::vector<ConnectionProfile>* connections,
                     const AppConfig* appConfig);
    ~AuditPolicyFrame() override;
    wxDECLARE_EVENT_TABLE();

private:
    enum TabIndex {
        kTabPolicies = 0,
        kTabRetention = 1
    };

    void BuildMenu();
    void BuildLayout();
    void PopulateConnections();
    void UpdateControls();
    void UpdateStatus(const wxString& status);
    void SetMessage(const std::string& message);
    bool EnsureConnected(const ConnectionProfile& profile);
    const ConnectionProfile* GetSelectedProfile() const;

    void RefreshPolicies();
    void RefreshRetentionPolicies();
    void RefreshActiveTab();
    std::string GetSelectedAuditPolicyId() const;
    std::string GetSelectedRetentionPolicyId() const;
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
    void OnTabChanged(wxBookCtrlEvent& event);
    void OnPolicySelected(wxGridEvent& event);
    void OnRetentionSelected(wxGridEvent& event);
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
    wxNotebook* notebook_ = nullptr;

    wxGrid* audit_grid_ = nullptr;
    ResultGridTable* audit_table_ = nullptr;
    wxTextCtrl* audit_details_ = nullptr;

    wxGrid* retention_grid_ = nullptr;
    ResultGridTable* retention_table_ = nullptr;
    wxTextCtrl* retention_details_ = nullptr;

    wxStaticText* status_label_ = nullptr;
    wxStaticText* message_label_ = nullptr;

    QueryResult audit_result_;
    QueryResult retention_result_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_AUDIT_POLICY_FRAME_H
