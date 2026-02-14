/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "monitoring_frame.h"

#include "diagram_frame.h"
#include "domain_manager_frame.h"
#include "icon_bar.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "result_grid_table.h"
#include "schema_manager_frame.h"
#include "sql_editor_frame.h"
#include "table_designer_frame.h"
#include "users_roles_frame.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>

namespace scratchrobin {

namespace {
constexpr int kMenuConnect = wxID_HIGHEST + 20;
constexpr int kMenuDisconnect = wxID_HIGHEST + 21;
constexpr int kMenuRefresh = wxID_HIGHEST + 22;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 23;
constexpr int kViewChoiceId = wxID_HIGHEST + 24;
constexpr int kViewSessions = 0;
constexpr int kViewTransactions = 1;
constexpr int kViewLocks = 2;
constexpr int kViewStatements = 3;
constexpr int kViewLongRunning = 4;
constexpr int kViewPerformance = 5;
constexpr int kViewTableStats = 6;
constexpr int kViewIoStats = 7;
constexpr int kViewBgWriter = 8;

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string NormalizeBackendName(const std::string& raw) {
    std::string value = ToLowerCopy(Trim(raw));
    if (value.empty() || value == "network" || value == "scratchbird") {
        return "native";
    }
    if (value == "postgres" || value == "pg") {
        return "postgresql";
    }
    if (value == "mariadb") {
        return "mysql";
    }
    if (value == "fb") {
        return "firebird";
    }
    return value;
}

std::string ProfileLabel(const ConnectionProfile& profile) {
    if (!profile.name.empty()) {
        return profile.name;
    }
    if (!profile.database.empty()) {
        return profile.database;
    }
    std::string label = profile.host.empty() ? "localhost" : profile.host;
    if (profile.port != 0) {
        label += ":" + std::to_string(profile.port);
    }
    return label;
}

bool BuildMonitoringQuery(const std::string& backend,
                          int view_index,
                          std::string* out_query,
                          std::string* out_message) {
    if (!out_query) {
        return false;
    }
    out_query->clear();

    if (backend == "native") {
        if (view_index == kViewSessions) {
            *out_query =
                "SELECT session_id, user_name, role_name, database_name, protocol, "
                "client_addr, client_port, state, connected_at, last_activity_at, "
                "transaction_id, statement_id, current_query, wait_event, wait_resource "
                "FROM sys.sessions ORDER BY connected_at DESC;";
        } else if (view_index == kViewTransactions) {
            *out_query =
                "SELECT transaction_id, transaction_uuid, session_id, state, isolation_level, "
                "read_only, start_time, duration_ms, current_query, wait_event, wait_resource, "
                "locks_held, pages_modified "
                "FROM sys.transactions ORDER BY start_time DESC;";
        } else if (view_index == kViewLocks) {
            *out_query =
                "SELECT lock_id, lock_type, lock_mode, granted, lock_state, relation_name, "
                "transaction_id, session_id, wait_start "
                "FROM sys.locks ORDER BY lock_id;";
        } else if (view_index == kViewStatements) {
            *out_query =
                "SELECT statement_id, session_id, transaction_id, state, start_time, "
                "elapsed_ms, rows_processed, wait_event, wait_resource, sql_text "
                "FROM sys.statements ORDER BY start_time DESC;";
        } else if (view_index == kViewLongRunning) {
            *out_query =
                "SELECT statement_id, session_id, transaction_id, state, start_time, "
                "elapsed_ms, rows_processed, wait_event, wait_resource, sql_text "
                "FROM sys.statements WHERE elapsed_ms > 5000 "
                "ORDER BY elapsed_ms DESC;";
        } else if (view_index == kViewPerformance) {
            *out_query =
                "SELECT metric, value, unit, scope, database_name, updated_at "
                "FROM sys.performance ORDER BY metric;";
        } else if (view_index == kViewTableStats) {
            *out_query =
                "SELECT schema_name, table_name, seq_scan_count, seq_rows_read, "
                "idx_scan_count, idx_rows_fetch, rows_inserted, rows_updated, rows_deleted, "
                "live_rows_estimate, dead_rows_estimate, last_vacuum_at, last_analyze_at "
                "FROM sys.table_stats ORDER BY schema_name, table_name;";
        } else if (view_index == kViewIoStats) {
            *out_query =
                "SELECT stat_group, stat_id, session_id, transaction_id, statement_id, "
                "page_reads, page_writes, page_fetches, page_marks "
                "FROM sys.io_stats ORDER BY stat_group, stat_id;";
        } else {
            if (out_message) {
                *out_message = "Unsupported monitoring view for ScratchBird";
            }
            return false;
        }
        return true;
    }

    if (backend == "postgresql") {
        if (view_index == kViewSessions) {
            *out_query =
                "SELECT pid, usename, datname, client_addr, state, "
                "backend_start, xact_start, query_start, wait_event_type, wait_event, query "
                "FROM pg_stat_activity ORDER BY pid;";
        } else if (view_index == kViewTransactions) {
            *out_query =
                "SELECT pid, usename, datname, xact_start, "
                "now() - xact_start AS duration, state, wait_event_type, wait_event, query "
                "FROM pg_stat_activity "
                "WHERE xact_start IS NOT NULL "
                "ORDER BY xact_start;";
        } else if (view_index == kViewLocks) {
            *out_query =
                "SELECT l.pid, l.locktype, l.mode, l.granted, n.nspname, c.relname "
                "FROM pg_locks l "
                "LEFT JOIN pg_class c ON l.relation = c.oid "
                "LEFT JOIN pg_namespace n ON c.relnamespace = n.oid "
                "ORDER BY l.pid;";
        } else if (view_index == kViewStatements) {
            *out_query =
                "SELECT pid, usename, datname, state, query_start, query "
                "FROM pg_stat_activity "
                "WHERE state <> 'idle' "
                "ORDER BY query_start DESC;";
        } else if (view_index == kViewLongRunning) {
            *out_query =
                "SELECT pid, usename, datname, now() - query_start AS duration, "
                "state, wait_event_type, wait_event, query "
                "FROM pg_stat_activity "
                "WHERE state <> 'idle' AND query_start IS NOT NULL "
                "ORDER BY duration DESC;";
        } else if (view_index == kViewPerformance) {
            *out_query =
                "SELECT datname, numbackends, xact_commit, xact_rollback, "
                "blks_read, blks_hit, tup_returned, tup_fetched, "
                "tup_inserted, tup_updated, tup_deleted "
                "FROM pg_stat_database ORDER BY datname;";
        } else if (view_index == kViewTableStats) {
            *out_query =
                "SELECT schemaname, relname, seq_scan, seq_tup_read, "
                "idx_scan, idx_tup_fetch, n_tup_ins, n_tup_upd, n_tup_del, "
                "n_live_tup, n_dead_tup, last_vacuum, last_autovacuum, "
                "last_analyze, last_autoanalyze "
                "FROM pg_stat_all_tables "
                "ORDER BY schemaname, relname;";
        } else if (view_index == kViewIoStats) {
            *out_query =
                "SELECT schemaname, relname, heap_blks_read, heap_blks_hit, "
                "idx_blks_read, idx_blks_hit, toast_blks_read, toast_blks_hit "
                "FROM pg_statio_all_tables "
                "ORDER BY schemaname, relname;";
        } else if (view_index == kViewBgWriter) {
            *out_query =
                "SELECT checkpoints_timed, checkpoints_req, buffers_checkpoint, "
                "buffers_clean, buffers_backend, maxwritten_clean, buffers_alloc "
                "FROM pg_stat_bgwriter;";
        } else {
            if (out_message) {
                *out_message = "Unsupported monitoring view for PostgreSQL";
            }
            return false;
        }
        return true;
    }

    if (backend == "mysql") {
        if (view_index == kViewSessions) {
            *out_query =
                "SELECT ID, USER, HOST, DB, COMMAND, TIME, STATE, INFO "
                "FROM information_schema.PROCESSLIST ORDER BY ID;";
        } else if (view_index == kViewTransactions) {
            *out_query =
                "SELECT trx_id, trx_state, trx_started, trx_mysql_thread_id, trx_query "
                "FROM information_schema.innodb_trx ORDER BY trx_started;";
        } else if (view_index == kViewLocks) {
            *out_query =
                "SELECT ENGINE, LOCK_ID, LOCK_TYPE, LOCK_MODE, LOCK_STATUS, LOCK_DATA "
                "FROM performance_schema.data_locks ORDER BY ENGINE, LOCK_ID;";
        } else if (view_index == kViewStatements) {
            *out_query =
                "SELECT THREAD_ID, EVENT_ID, EVENT_NAME, SQL_TEXT, TIMER_WAIT "
                "FROM performance_schema.events_statements_current "
                "ORDER BY TIMER_WAIT DESC;";
        } else if (view_index == kViewLongRunning) {
            *out_query =
                "SELECT ID, USER, HOST, DB, COMMAND, TIME, STATE, INFO "
                "FROM information_schema.PROCESSLIST "
                "WHERE COMMAND <> 'Sleep' "
                "ORDER BY TIME DESC;";
        } else if (view_index == kViewPerformance) {
            *out_query =
                "SELECT VARIABLE_NAME, VARIABLE_VALUE "
                "FROM performance_schema.global_status "
                "WHERE VARIABLE_NAME IN ("
                "'Threads_connected','Threads_running','Connections','Uptime',"
                "'Questions','Queries','Com_select','Com_insert','Com_update','Com_delete'"
                ") ORDER BY VARIABLE_NAME;";
        } else if (view_index == kViewTableStats) {
            *out_query =
                "SELECT OBJECT_SCHEMA, OBJECT_NAME, COUNT_READ, COUNT_WRITE, "
                "COUNT_FETCH, COUNT_INSERT, COUNT_UPDATE, COUNT_DELETE "
                "FROM performance_schema.table_io_waits_summary_by_table "
                "ORDER BY OBJECT_SCHEMA, OBJECT_NAME;";
        } else if (view_index == kViewIoStats) {
            *out_query =
                "SELECT FILE_NAME, EVENT_NAME, COUNT_READ, SUM_TIMER_READ, "
                "COUNT_WRITE, SUM_TIMER_WRITE "
                "FROM performance_schema.file_summary_by_instance "
                "ORDER BY FILE_NAME;";
        } else {
            if (out_message) {
                *out_message = "Unsupported monitoring view for MySQL";
            }
            return false;
        }
        return true;
    }

    if (backend == "firebird") {
        if (view_index == kViewSessions) {
            *out_query =
                "SELECT MON$ATTACHMENT_ID, MON$USER, MON$REMOTE_ADDRESS, "
                "MON$REMOTE_PROTOCOL, MON$REMOTE_PID, MON$STATE "
                "FROM MON$ATTACHMENTS ORDER BY MON$ATTACHMENT_ID;";
        } else if (view_index == kViewTransactions) {
            *out_query =
                "SELECT MON$TRANSACTION_ID, MON$STATE, MON$TIMESTAMP, "
                "MON$ISOLATION_MODE, MON$READ_ONLY, MON$LOCK_TIMEOUT, "
                "MON$ATTACHMENT_ID "
                "FROM MON$TRANSACTIONS ORDER BY MON$TRANSACTION_ID;";
        } else if (view_index == kViewLocks) {
            *out_query =
                "SELECT MON$LOCK_ID, MON$LOCK_TYPE, MON$LOCK_MODE, MON$LOCK_STATE, "
                "MON$OBJECT_NAME FROM MON$LOCKS ORDER BY MON$LOCK_ID;";
        } else if (view_index == kViewStatements) {
            *out_query =
                "SELECT MON$STATEMENT_ID, MON$ATTACHMENT_ID, MON$STATE, "
                "MON$TIMESTAMP, MON$SQL_TEXT "
                "FROM MON$STATEMENTS ORDER BY MON$STATEMENT_ID;";
        } else if (view_index == kViewLongRunning) {
            *out_query =
                "SELECT MON$STATEMENT_ID, MON$ATTACHMENT_ID, "
                "CURRENT_TIMESTAMP - MON$TIMESTAMP AS DURATION, "
                "MON$STATE, MON$SQL_TEXT "
                "FROM MON$STATEMENTS "
                "WHERE MON$STATE <> 0 "
                "ORDER BY DURATION DESC;";
        } else if (view_index == kViewPerformance) {
            *out_query =
                "SELECT MON$PAGE_SIZE, MON$ODS_MAJOR, MON$ODS_MINOR, "
                "MON$ALLOCATED_PAGES, MON$PAGE_BUFFERS, MON$NEXT_TRANSACTION, "
                "MON$OLDEST_TRANSACTION FROM MON$DATABASE;";
        } else if (view_index == kViewTableStats) {
            *out_query =
                "SELECT MON$RELATION_NAME, MON$RECORD_SEQ_READS, MON$RECORD_IDX_READS, "
                "MON$RECORD_INSERTS, MON$RECORD_UPDATES, MON$RECORD_DELETES, "
                "MON$RECORD_BACKOUTS, MON$RECORD_PURGES, MON$RECORD_EXPUNGES "
                "FROM MON$TABLE_STATS ORDER BY MON$RELATION_NAME;";
        } else if (view_index == kViewIoStats) {
            *out_query =
                "SELECT MON$STAT_GROUP, MON$STAT_ID, MON$PAGE_READS, MON$PAGE_WRITES, "
                "MON$PAGE_FETCHES, MON$PAGE_MARKS "
                "FROM MON$IO_STATS ORDER BY MON$STAT_GROUP, MON$STAT_ID;";
        } else {
            if (out_message) {
                *out_message = "Unsupported monitoring view for Firebird";
            }
            return false;
        }
        return true;
    }

    if (out_message) {
        if (backend == "native") {
            *out_message = "ScratchBird monitoring views unavailable.";
        } else {
            *out_message = "Unsupported backend for monitoring: " + backend;
        }
    }
    return false;
}

} // namespace

wxBEGIN_EVENT_TABLE(MonitoringFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, MonitoringFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, MonitoringFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_USERS_ROLES, MonitoringFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, MonitoringFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, MonitoringFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, MonitoringFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, MonitoringFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, MonitoringFrame::OnOpenIndexDesigner)
    EVT_MENU(wxID_REFRESH, MonitoringFrame::OnRefresh)
    EVT_BUTTON(kMenuConnect, MonitoringFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, MonitoringFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, MonitoringFrame::OnRefresh)
    EVT_CHOICE(kConnectionChoiceId, MonitoringFrame::OnConnectionChanged)
    EVT_CHOICE(kViewChoiceId, MonitoringFrame::OnViewChanged)
    EVT_CLOSE(MonitoringFrame::OnClose)
wxEND_EVENT_TABLE()

MonitoringFrame::MonitoringFrame(WindowManager* windowManager,
                                 ConnectionManager* connectionManager,
                                 const std::vector<ConnectionProfile>* connections,
                                 const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Monitoring", wxDefaultPosition, wxSize(1000, 700)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
    BuildMenu();
    if (app_config_ && app_config_->chrome.monitoring.showIconBar) {
        IconBarType type = app_config_->chrome.monitoring.replicateIconBar
                               ? IconBarType::Main
                               : IconBarType::Monitoring;
        BuildIconBar(this, type, 24);
    }
    BuildLayout();
    PopulateConnections();
    UpdateControls();
    UpdateStatus("Ready");

    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
}

void MonitoringFrame::BuildMenu() {
    // Child windows use minimal menu bar (File + Help only)
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void MonitoringFrame::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* topPanel = new wxPanel(this, wxID_ANY);
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Connection:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
    connection_choice_ = new wxChoice(topPanel, kConnectionChoiceId);
    topSizer->Add(connection_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connect_button_ = new wxButton(topPanel, kMenuConnect, "Connect");
    disconnect_button_ = new wxButton(topPanel, kMenuDisconnect, "Disconnect");
    topSizer->Add(connect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    topSizer->Add(disconnect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "View:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    view_choice_ = new wxChoice(topPanel, kViewChoiceId);
    view_choice_->Append("Sessions");
    view_choice_->Append("Transactions");
    view_choice_->Append("Locks");
    view_choice_->Append("Statements");
    view_choice_->Append("Long Running");
    view_choice_->Append("Performance");
    view_choice_->Append("Table Stats");
    view_choice_->Append("I/O Stats");
    view_choice_->Append("BG Writer");
    view_choice_->SetSelection(0);
    topSizer->Add(view_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    refresh_button_ = new wxButton(topPanel, kMenuRefresh, "Refresh");
    topSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    topSizer->AddStretchSpacer(1);
    status_label_ = new wxStaticText(topPanel, wxID_ANY, "Ready");
    topSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    topPanel->SetSizer(topSizer);
    root->Add(topPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    auto* gridPanel = new wxPanel(this, wxID_ANY);
    auto* gridSizer = new wxBoxSizer(wxVERTICAL);
    result_grid_ = new wxGrid(gridPanel, wxID_ANY);
    result_table_ = new ResultGridTable();
    result_grid_->SetTable(result_table_, true);
    result_grid_->EnableEditing(false);
    result_grid_->SetRowLabelSize(64);
    gridSizer->Add(result_grid_, 1, wxEXPAND | wxALL, 8);
    gridPanel->SetSizer(gridSizer);
    root->Add(gridPanel, 1, wxEXPAND);

    auto* messagePanel = new wxPanel(this, wxID_ANY);
    auto* messageSizer = new wxBoxSizer(wxVERTICAL);
    message_log_ = new wxTextCtrl(messagePanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    messageSizer->Add(message_log_, 1, wxEXPAND | wxALL, 8);
    messagePanel->SetSizer(messageSizer);
    root->Add(messagePanel, 0, wxEXPAND);

    SetSizer(root);
}

void MonitoringFrame::PopulateConnections() {
    if (!connection_choice_) {
        return;
    }
    connection_choice_->Clear();
    if (!connections_) {
        return;
    }
    for (const auto& profile : *connections_) {
        connection_choice_->Append(ProfileLabel(profile));
    }
    if (connections_->empty()) {
        return;
    }
    connection_choice_->SetSelection(0);
}

const ConnectionProfile* MonitoringFrame::GetSelectedProfile() const {
    if (!connections_ || !connection_choice_) {
        return nullptr;
    }
    int selection = connection_choice_->GetSelection();
    if (selection < 0 || static_cast<size_t>(selection) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(selection)];
}

void MonitoringFrame::UpdateControls() {
    bool has_connections = connections_ && !connections_->empty();
    bool connected = connection_manager_ && connection_manager_->IsConnected();

    if (connection_choice_) {
        connection_choice_->Enable(has_connections && !connect_running_ && !query_running_);
    }
    if (connect_button_) {
        connect_button_->Enable(has_connections && !connected && !connect_running_ && !query_running_);
    }
    if (disconnect_button_) {
        disconnect_button_->Enable(connected && !connect_running_ && !query_running_);
    }
    if (view_choice_) {
        view_choice_->Enable(!query_running_);
    }
    if (refresh_button_) {
        refresh_button_->Enable(connected && !query_running_);
    }
}

void MonitoringFrame::UpdateStatus(const std::string& message) {
    if (status_label_) {
        status_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void MonitoringFrame::SetMessage(const std::string& message) {
    if (message_log_) {
        message_log_->SetValue(wxString::FromUTF8(message));
    }
}

void MonitoringFrame::OnConnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        UpdateStatus("No connection profile selected");
        return;
    }
    if (connect_running_) {
        return;
    }
    connect_running_ = true;
    UpdateControls();
    UpdateStatus("Connecting...");
    SetMessage(std::string());

    connect_job_ = connection_manager_->ConnectAsync(*profile,
        [this](bool ok, const std::string& error) {
            CallAfter([this, ok, error]() {
                connect_running_ = false;
                if (ok) {
                    UpdateStatus("Connected");
                } else {
                    UpdateStatus("Connect failed");
                    SetMessage(error.empty() ? "Connect failed" : error);
                }
                UpdateControls();
            });
        });
}

void MonitoringFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void MonitoringFrame::OnNewDiagram(wxCommandEvent&) {
    if (window_manager_) {
        if (auto* host = dynamic_cast<DiagramFrame*>(window_manager_->GetDiagramHost())) {
            host->AddDiagramTab();
            host->Raise();
            host->Show(true);
            return;
        }
    }
    auto* diagram = new DiagramFrame(window_manager_, app_config_);
    diagram->Show(true);
}

void MonitoringFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void MonitoringFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void MonitoringFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    domains->Show(true);
}

void MonitoringFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void MonitoringFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void MonitoringFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void MonitoringFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void MonitoringFrame::OnRefresh(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    if (!connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    if (query_running_) {
        return;
    }

    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    std::string query;
    std::string warning;
    if (!BuildMonitoringQuery(backend, view_choice_ ? view_choice_->GetSelection() : 0,
                              &query, &warning)) {
        UpdateStatus("Unsupported");
        SetMessage(warning);
        return;
    }

    query_running_ = true;
    UpdateControls();
    UpdateStatus("Running...");
    SetMessage(std::string());

    query_job_ = connection_manager_->ExecuteQueryAsync(query,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                query_running_ = false;
                if (result_table_) {
                    result_table_->Reset(result.columns, result.rows);
                }
                if (ok) {
                    UpdateStatus("Updated");
                } else {
                    UpdateStatus("Query failed");
                    SetMessage(error.empty() ? "Query failed" : error);
                }
                UpdateControls();
            });
        });
}

void MonitoringFrame::OnConnectionChanged(wxCommandEvent&) {
    UpdateControls();
}

void MonitoringFrame::OnViewChanged(wxCommandEvent&) {
    UpdateControls();
}

void MonitoringFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
