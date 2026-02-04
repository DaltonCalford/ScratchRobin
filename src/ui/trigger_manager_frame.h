/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_TRIGGER_MANAGER_FRAME_H
#define SCRATCHROBIN_TRIGGER_MANAGER_FRAME_H

#include <string>
#include <vector>

#include <wx/frame.h>
#include <wx/grid.h>

#include "core/connection_manager.h"
#include "core/query_types.h"

class wxChoice;
class wxGrid;
class wxStaticText;
class wxTextCtrl;
class wxButton;
class wxNotebook;

namespace scratchrobin {

class ResultGridTable;
class WindowManager;
struct AppConfig;

class TriggerManagerFrame : public wxFrame {
public:
    TriggerManagerFrame(WindowManager* windowManager,
                        ConnectionManager* connectionManager,
                        const std::vector<ConnectionProfile>* connections,
                        const AppConfig* appConfig);

private:
    void BuildMenu();
    void BuildLayout();
    void PopulateConnections();
    const ConnectionProfile* GetSelectedProfile() const;
    bool EnsureConnected(const ConnectionProfile& profile);
    bool IsNativeProfile(const ConnectionProfile& profile) const;
    void UpdateControls();
    void UpdateStatus(const wxString& status);
    void SetMessage(const std::string& message);

    void RefreshTriggers();
    void RefreshTriggerDefinition(const std::string& trigger_name);
    void RefreshTriggerTiming(const std::string& trigger_name);
    void RefreshTriggerDependencies(const std::string& trigger_name);
    void RunCommand(const std::string& sql, const std::string& success_message);

    std::string GetSelectedTriggerName() const;
    std::string GetSelectedTableName() const;
    int FindColumnIndex(const QueryResult& result, const std::vector<std::string>& names) const;
    std::string ExtractValue(const QueryResult& result, int row, const std::vector<std::string>& names) const;
    std::string FormatTimingDetails(const QueryResult& result) const;
    std::string FormatDependencies(const QueryResult& result) const;

    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnTriggerSelected(wxGridEvent& event);
    void OnCreate(wxCommandEvent& event);
    void OnEdit(wxCommandEvent& event);
    void OnDrop(wxCommandEvent& event);
    void OnEnable(wxCommandEvent& event);
    void OnDisable(wxCommandEvent& event);
    void OnNewSqlEditor(wxCommandEvent& event);
    void OnNewDiagram(wxCommandEvent& event);
    void OnOpenMonitoring(wxCommandEvent& event);
    void OnOpenUsersRoles(wxCommandEvent& event);
    void OnOpenJobScheduler(wxCommandEvent& event);
    void OnOpenSchemaManager(wxCommandEvent& event);
    void OnOpenDomainManager(wxCommandEvent& event);
    void OnOpenTableDesigner(wxCommandEvent& event);
    void OnOpenIndexDesigner(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

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
    wxStaticText* status_text_ = nullptr;
    wxTextCtrl* message_text_ = nullptr;
    wxTextCtrl* definition_text_ = nullptr;
    wxTextCtrl* timing_text_ = nullptr;
    wxTextCtrl* dependencies_text_ = nullptr;

    wxGrid* triggers_grid_ = nullptr;
    ResultGridTable* triggers_table_ = nullptr;

    int active_profile_index_ = -1;
    int pending_queries_ = 0;
    QueryResult triggers_result_;
    QueryResult trigger_details_result_;
    QueryResult trigger_deps_result_;
    std::string selected_trigger_;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TRIGGER_MANAGER_FRAME_H
