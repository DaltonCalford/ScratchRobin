/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_INDEX_DESIGNER_FRAME_H
#define SCRATCHROBIN_INDEX_DESIGNER_FRAME_H

#include <string>
#include <vector>

#include <wx/frame.h>
#include <wx/grid.h>

#include "core/connection_manager.h"
#include "core/query_types.h"

class wxChoice;
class wxGrid;
class wxNotebook;
class wxStaticText;
class wxTextCtrl;
class wxButton;

namespace scratchrobin {

class ResultGridTable;
class WindowManager;
struct AppConfig;

class IndexDesignerFrame : public wxFrame {
public:
    IndexDesignerFrame(WindowManager* windowManager,
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

    void RefreshIndexes();
    void RefreshIndexDetails(const std::string& index_name);
    void RefreshIndexColumns(const std::string& table_name, const std::string& index_name);
    void RunCommand(const std::string& sql, const std::string& success_message);

    std::string GetSelectedIndexName() const;
    std::string GetSelectedIndexTable() const;
    int FindColumnIndex(const QueryResult& result, const std::vector<std::string>& names) const;
    std::string ExtractValue(const QueryResult& result, int row, const std::vector<std::string>& names) const;
    std::string FormatDetails(const QueryResult& result) const;
    QueryResult FilterIndexRows(QueryResult result, const std::string& index_name) const;

    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnIndexSelected(wxGridEvent& event);
    void OnCreate(wxCommandEvent& event);
    void OnEdit(wxCommandEvent& event);
    void OnDrop(wxCommandEvent& event);
    void OnNewSqlEditor(wxCommandEvent& event);
    void OnNewDiagram(wxCommandEvent& event);
    void OnOpenMonitoring(wxCommandEvent& event);
    void OnOpenUsersRoles(wxCommandEvent& event);
    void OnOpenJobScheduler(wxCommandEvent& event);
    void OnOpenDomainManager(wxCommandEvent& event);
    void OnOpenSchemaManager(wxCommandEvent& event);
    void OnOpenTableDesigner(wxCommandEvent& event);
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
    wxStaticText* status_text_ = nullptr;
    wxTextCtrl* message_text_ = nullptr;
    wxTextCtrl* details_text_ = nullptr;

    wxGrid* indexes_grid_ = nullptr;
    wxGrid* columns_grid_ = nullptr;
    ResultGridTable* indexes_table_ = nullptr;
    ResultGridTable* columns_table_ = nullptr;

    int active_profile_index_ = -1;
    int pending_queries_ = 0;
    QueryResult indexes_result_;
    QueryResult index_details_result_;
    QueryResult columns_result_;
    std::string selected_index_;
    std::string selected_table_;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_INDEX_DESIGNER_FRAME_H
