/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CONNECTION_DATABASE_MANAGER_H
#define SCRATCHROBIN_CONNECTION_DATABASE_MANAGER_H

#include <string>
#include <vector>

#include <wx/frame.h>
#include <wx/grid.h>

#include "core/connection_manager.h"
#include "core/query_types.h"

class wxChoice;
class wxGrid;
class wxListBox;
class wxStaticText;
class wxTextCtrl;
class wxButton;
class wxSplitterWindow;
class wxPanel;

namespace scratchrobin {

class ResultGridTable;
class WindowManager;
struct AppConfig;

/**
 * @brief Unified Connection and Database Manager
 * 
 * Combines connection profile management with database management
 * in a single interface. Left panel shows connections, right panel
 * shows databases when connected.
 */
class ConnectionDatabaseManager : public wxFrame {
public:
    ConnectionDatabaseManager(wxWindow* parent,
                              WindowManager* windowManager,
                              ConnectionManager* connectionManager,
                              std::vector<ConnectionProfile>* connections,
                              const AppConfig* appConfig);

private:
    void BuildMenu();
    void BuildLayout();
    void RefreshConnectionList();
    void RefreshDatabaseList();
    
    // Connection management
    void OnNewConnection(wxCommandEvent& event);
    void OnEditConnection(wxCommandEvent& event);
    void OnDuplicateConnection(wxCommandEvent& event);
    void OnDeleteConnection(wxCommandEvent& event);
    void OnConnectionSelected(wxCommandEvent& event);
    void OnConnectionActivated(wxCommandEvent& event);
    void UpdateConnectionButtonStates();
    
    // Database operations
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnRefreshDatabases(wxCommandEvent& event);
    void OnDatabaseSelected(wxGridEvent& event);
    void OnCreateDatabase(wxCommandEvent& event);
    void OnDropDatabase(wxCommandEvent& event);
    void OnCloneDatabase(wxCommandEvent& event);
    void OnDatabaseProperties(wxCommandEvent& event);
    void UpdateDatabaseButtonStates();
    
    // Helper methods
    const ConnectionProfile* GetSelectedConnection() const;
    std::string GetSelectedDatabaseName() const;
    bool IsNativeProfile(const ConnectionProfile& profile) const;
    bool EnsureConnected(const ConnectionProfile& profile);
    void RunCommand(const std::string& sql, const std::string& success_message);
    void UpdateStatus(const wxString& status);
    void SetMessage(const std::string& message);
    
    // Data formatting
    std::string FormatDetails(const QueryResult& result) const;
    std::string ExtractValue(const QueryResult& result, int row, 
                             const std::vector<std::string>& names) const;
    int FindColumnIndex(const QueryResult& result, 
                        const std::vector<std::string>& names) const;

    void OnClose(wxCloseEvent& event);

    // Dependencies
    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;

    // Connection panel controls
    wxListBox* connection_list_ = nullptr;
    wxButton* conn_new_button_ = nullptr;
    wxButton* conn_edit_button_ = nullptr;
    wxButton* conn_duplicate_button_ = nullptr;
    wxButton* conn_delete_button_ = nullptr;
    wxButton* conn_connect_button_ = nullptr;
    wxButton* conn_disconnect_button_ = nullptr;
    wxStaticText* conn_status_label_ = nullptr;

    // Database panel controls
    wxGrid* databases_grid_ = nullptr;
    ResultGridTable* databases_table_ = nullptr;
    wxButton* db_refresh_button_ = nullptr;
    wxButton* db_create_button_ = nullptr;
    wxButton* db_drop_button_ = nullptr;
    wxButton* db_clone_button_ = nullptr;
    wxButton* db_properties_button_ = nullptr;
    wxTextCtrl* details_text_ = nullptr;
    wxStaticText* status_text_ = nullptr;
    wxTextCtrl* message_text_ = nullptr;

    // State
    int active_connection_index_ = -1;
    int pending_queries_ = 0;
    QueryResult databases_result_;
    std::string selected_database_;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONNECTION_DATABASE_MANAGER_H
