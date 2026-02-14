/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "sql_editor_frame.h"
#include "core/metadata_model.h"
#include "core/result_exporter.h"
#include "core/statement_splitter.h"
#include "core/value_formatter.h"
#include "copy_dialog.h"
#include "prepared_params_dialog.h"
#include "diagram_frame.h"
#include "domain_manager_frame.h"
#include "icon_bar.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "result_grid_table.h"
#include "schema_manager_frame.h"
#include "table_designer_frame.h"
#include "users_roles_frame.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/filename.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include <ctime>
#include <iomanip>

namespace scratchrobin {

namespace {

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

std::string StripLeadingComments(const std::string& input) {
    size_t pos = 0;
    while (pos < input.size()) {
        while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) {
            ++pos;
        }
        if (pos + 1 < input.size() && input[pos] == '-' && input[pos + 1] == '-') {
            pos += 2;
            while (pos < input.size() && input[pos] != '\n') {
                ++pos;
            }
            continue;
        }
        if (pos + 1 < input.size() && input[pos] == '/' && input[pos + 1] == '*') {
            pos += 2;
            while (pos + 1 < input.size() && !(input[pos] == '*' && input[pos + 1] == '/')) {
                ++pos;
            }
            if (pos + 1 < input.size()) {
                pos += 2;
            }
            continue;
        }
        break;
    }
    return input.substr(pos);
}

std::string FirstToken(const std::string& input) {
    std::string token;
    for (char c : input) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!token.empty()) {
                break;
            }
            continue;
        }
        token.push_back(c);
    }
    return token;
}

} // namespace

SqlEditorFrame::SqlEditorFrame(WindowManager* windowManager,
                               ConnectionManager* connectionManager,
                               const std::vector<ConnectionProfile>* connections,
                               const AppConfig* appConfig,
                               MetadataModel* metadataModel)
    : wxFrame(nullptr, wxID_ANY, "SQL Editor", wxDefaultPosition, wxSize(900, 700)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig),
      metadata_model_(metadataModel) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    if (app_config_ && app_config_->historyMaxItems > 0) {
        history_max_items_ = static_cast<size_t>(app_config_->historyMaxItems);
    }
    if (app_config_ && app_config_->rowLimit > 0) {
        row_limit_ = app_config_->rowLimit;
    }

    WindowChromeConfig chrome;
    if (app_config_) {
        chrome = app_config_->chrome.sqlEditor;
    }
    if (chrome.showMenu) {
        MenuBuildOptions options;
        options.includeConnections = chrome.replicateMenu;
        auto* menu_bar = BuildMenuBar(options, window_manager_, this);
        SetMenuBar(menu_bar);
    }
    if (chrome.showIconBar) {
        IconBarType type = chrome.replicateIconBar ? IconBarType::Main : IconBarType::SqlEditor;
        BuildIconBar(this, type, 24);
    }

    auto* sessionPanel = new wxPanel(this, wxID_ANY);
    auto* sessionSizer = new wxBoxSizer(wxHORIZONTAL);
    sessionSizer->Add(new wxStaticText(sessionPanel, wxID_ANY, "Connection:"), 0,
                      wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
    connection_choice_ = new wxChoice(sessionPanel, wxID_ANY);
    sessionSizer->Add(connection_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connect_button_ = new wxButton(sessionPanel, wxID_ANY, "Connect");
    disconnect_button_ = new wxButton(sessionPanel, wxID_ANY, "Disconnect");
    sessionSizer->Add(connect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    sessionSizer->Add(disconnect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    auto_commit_check_ = new wxCheckBox(sessionPanel, wxID_ANY, "Auto-commit");
    sessionSizer->Add(auto_commit_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    begin_button_ = new wxButton(sessionPanel, wxID_ANY, "Begin");
    commit_button_ = new wxButton(sessionPanel, wxID_ANY, "Commit");
    rollback_button_ = new wxButton(sessionPanel, wxID_ANY, "Rollback");
    sessionSizer->Add(begin_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    sessionSizer->Add(commit_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    sessionSizer->Add(rollback_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    
    // Savepoint controls
    sessionSizer->Add(new wxStaticText(sessionPanel, wxID_ANY, "Savepoint:"), 0,
                      wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    savepoint_choice_ = new wxChoice(sessionPanel, wxID_ANY);
    savepoint_choice_->SetMinSize(wxSize(120, -1));
    sessionSizer->Add(savepoint_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    savepoint_button_ = new wxButton(sessionPanel, wxID_ANY, "Create", wxDefaultPosition, wxSize(60, -1));
    sessionSizer->Add(savepoint_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    
    // Isolation level dropdown
    sessionSizer->Add(new wxStaticText(sessionPanel, wxID_ANY, "Isolation:"), 0,
                      wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    isolation_choice_ = new wxChoice(sessionPanel, wxID_ANY);
    isolation_choice_->Append("Read Committed");
    isolation_choice_->Append("Read Uncommitted");
    isolation_choice_->Append("Repeatable Read");
    isolation_choice_->Append("Serializable");
    isolation_choice_->SetSelection(0);
    sessionSizer->Add(isolation_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    
    // Transaction indicator
    transaction_indicator_ = new wxStaticText(sessionPanel, wxID_ANY, "Auto");
    transaction_indicator_->SetBackgroundColour(wxColour(108, 117, 125));  // Gray
    transaction_indicator_->SetForegroundColour(wxColour(255, 255, 255));  // White
    sessionSizer->Add(transaction_indicator_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    
    sessionSizer->AddStretchSpacer(1);
    sessionPanel->SetSizer(sessionSizer);
    rootSizer->Add(sessionPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    auto* execPanel = new wxPanel(this, wxID_ANY);
    auto* execSizer = new wxBoxSizer(wxVERTICAL);

    auto* execRow1 = new wxBoxSizer(wxHORIZONTAL);
    run_button_ = new wxButton(execPanel, wxID_ANY, "Run");
    execRow1->Add(run_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
    cancel_button_ = new wxButton(execPanel, wxID_ANY, "Cancel");
    execRow1->Add(cancel_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    paging_check_ = new wxCheckBox(execPanel, wxID_ANY, "Paging");
    paging_check_->SetValue(true);
    execRow1->Add(paging_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    stream_check_ = new wxCheckBox(execPanel, wxID_ANY, "Stream");
    stream_check_->SetValue(false);
    execRow1->Add(stream_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    execRow1->Add(new wxStaticText(execPanel, wxID_ANY, "Page size:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    page_size_ctrl_ = new wxSpinCtrl(execPanel, wxID_ANY);
    page_size_ctrl_->SetRange(1, 10000);
    page_size_ctrl_->SetValue(page_size_);
    execRow1->Add(page_size_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    execRow1->Add(new wxStaticText(execPanel, wxID_ANY, "Row limit:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row_limit_ctrl_ = new wxSpinCtrl(execPanel, wxID_ANY);
    row_limit_ctrl_->SetRange(0, 1000000);
    row_limit_ctrl_->SetValue(row_limit_);
    execRow1->Add(row_limit_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    prev_page_button_ = new wxButton(execPanel, wxID_ANY, "Prev");
    next_page_button_ = new wxButton(execPanel, wxID_ANY, "Next");
    page_label_ = new wxStaticText(execPanel, wxID_ANY, "Page 1");
    execRow1->Add(prev_page_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    execRow1->Add(next_page_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    execRow1->Add(page_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    export_csv_button_ = new wxButton(execPanel, wxID_ANY, "Export CSV");
    export_json_button_ = new wxButton(execPanel, wxID_ANY, "Export JSON");
    execRow1->Add(export_csv_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    execRow1->Add(export_json_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    copy_button_ = new wxButton(execPanel, wxID_ANY, "COPY");
    execRow1->Add(copy_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    stream_status_label_ = new wxStaticText(execPanel, wxID_ANY, "Streaming: off");
    execRow1->Add(stream_status_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    progress_label_ = new wxStaticText(execPanel, wxID_ANY, "Progress: n/a");
    execRow1->Add(progress_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    execRow1->AddStretchSpacer(1);
    status_label_ = new wxStaticText(execPanel, wxID_ANY, "Ready");
    execRow1->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    auto* execRow2 = new wxBoxSizer(wxHORIZONTAL);
    execRow2->Add(new wxStaticText(execPanel, wxID_ANY, "Result:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
    result_choice_ = new wxChoice(execPanel, wxID_ANY);
    execRow2->Add(result_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    execRow2->Add(new wxStaticText(execPanel, wxID_ANY, "History:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    history_choice_ = new wxChoice(execPanel, wxID_ANY);
    execRow2->Add(history_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    history_load_button_ = new wxButton(execPanel, wxID_ANY, "Load");
    execRow2->Add(history_load_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    explain_button_ = new wxButton(execPanel, wxID_ANY, "Explain");
    sblr_button_ = new wxButton(execPanel, wxID_ANY, "SBLR");
    execRow2->Add(explain_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    execRow2->Add(sblr_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    execRow2->AddStretchSpacer(1);

    execSizer->Add(execRow1, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);
    execSizer->Add(execRow2, 0, wxEXPAND | wxBOTTOM, 2);

    auto* execRow3 = new wxBoxSizer(wxHORIZONTAL);
    NetworkOptions stream_options = connection_manager_
                                        ? connection_manager_->GetNetworkOptions()
                                        : (app_config_ ? app_config_->network : NetworkOptions{});
    execRow3->Add(new wxStaticText(execPanel, wxID_ANY, "Stream window:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    stream_window_ctrl_ = new wxSpinCtrl(execPanel, wxID_ANY);
    stream_window_ctrl_->SetRange(0, 10485760);
    stream_window_ctrl_->SetValue(static_cast<int>(stream_options.streamWindowBytes));
    execRow3->Add(stream_window_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    execRow3->Add(new wxStaticText(execPanel, wxID_ANY, "Chunk:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    stream_chunk_ctrl_ = new wxSpinCtrl(execPanel, wxID_ANY);
    stream_chunk_ctrl_->SetRange(0, 10485760);
    stream_chunk_ctrl_->SetValue(static_cast<int>(stream_options.streamChunkBytes));
    execRow3->Add(stream_chunk_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    stream_apply_button_ = new wxButton(execPanel, wxID_ANY, "Apply Stream Settings");
    execRow3->Add(stream_apply_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    stream_metrics_label_ = new wxStaticText(execPanel, wxID_ANY, "Last stream: n/a");
    execRow3->Add(stream_metrics_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    execSizer->Add(execRow3, 0, wxEXPAND | wxBOTTOM, 2);

    auto* prepBox = new wxStaticBox(execPanel, wxID_ANY, "Prepared Statements");
    auto* prepSizer = new wxStaticBoxSizer(prepBox, wxHORIZONTAL);
    prepared_status_label_ = new wxStaticText(execPanel, wxID_ANY,
                                              "No prepared statement selected.");
    prepared_edit_button_ = new wxButton(execPanel, wxID_ANY, "Edit Params");
    prepared_prepare_button_ = new wxButton(execPanel, wxID_ANY, "Prepare");
    prepared_execute_button_ = new wxButton(execPanel, wxID_ANY, "Execute");
    prepSizer->Add(prepared_status_label_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    prepSizer->Add(prepared_prepare_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    prepSizer->Add(prepared_edit_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    prepSizer->Add(prepared_execute_button_, 0, wxALIGN_CENTER_VERTICAL);
    execSizer->Add(prepSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

    execPanel->SetSizer(execSizer);
    rootSizer->Add(execPanel, 0, wxEXPAND | wxBOTTOM, 4);

    auto* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          wxSP_LIVE_UPDATE);

    auto* editorPanel = new wxPanel(splitter, wxID_ANY);
    auto* editorSizer = new wxBoxSizer(wxVERTICAL);
    editor_ = new wxTextCtrl(editorPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE | wxTE_RICH2);
    editorSizer->Add(editor_, 1, wxEXPAND | wxALL, 8);
    editorPanel->SetSizer(editorSizer);

    auto* gridPanel = new wxPanel(splitter, wxID_ANY);
    auto* gridSizer = new wxBoxSizer(wxVERTICAL);

    result_notebook_ = new wxNotebook(gridPanel, wxID_ANY);

    auto* resultsPage = new wxPanel(result_notebook_, wxID_ANY);
    auto* resultsSizer = new wxBoxSizer(wxVERTICAL);
    result_grid_ = new wxGrid(resultsPage, wxID_ANY);
    result_table_ = new ResultGridTable();
    result_grid_->SetTable(result_table_, true);
    result_grid_->EnableEditing(false);
    result_grid_->SetRowLabelSize(64);
    resultsSizer->Add(result_grid_, 1, wxEXPAND | wxALL, 8);
    resultsPage->SetSizer(resultsSizer);

    auto* planPage = new wxPanel(result_notebook_, wxID_ANY);
    auto* planSizer = new wxBoxSizer(wxVERTICAL);
    plan_view_ = new wxTextCtrl(planPage, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    planSizer->Add(plan_view_, 1, wxEXPAND | wxALL, 8);
    planPage->SetSizer(planSizer);

    auto* sblrPage = new wxPanel(result_notebook_, wxID_ANY);
    auto* sblrSizer = new wxBoxSizer(wxVERTICAL);
    sblr_view_ = new wxTextCtrl(sblrPage, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    sblrSizer->Add(sblr_view_, 1, wxEXPAND | wxALL, 8);
    sblrPage->SetSizer(sblrSizer);

    auto* messagesPage = new wxPanel(result_notebook_, wxID_ANY);
    auto* messagesSizer = new wxBoxSizer(wxVERTICAL);
    message_log_ = new wxTextCtrl(messagesPage, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    messagesSizer->Add(message_log_, 1, wxEXPAND | wxALL, 8);
    messagesPage->SetSizer(messagesSizer);

    result_notebook_->AddPage(resultsPage, "Results");
    result_notebook_->AddPage(planPage, "Plan");
    result_notebook_->AddPage(sblrPage, "SBLR");
    result_notebook_->AddPage(messagesPage, "Messages");
    results_page_index_ = 0;
    plan_page_index_ = 1;
    sblr_page_index_ = 2;
    messages_page_index_ = 3;

    auto* notificationsPage = new wxPanel(result_notebook_, wxID_ANY);
    auto* notificationsSizer = new wxBoxSizer(wxVERTICAL);
    auto* notifyControls = new wxBoxSizer(wxHORIZONTAL);
    notifyControls->Add(new wxStaticText(notificationsPage, wxID_ANY, "Channel:"), 0,
                        wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    notification_channel_ctrl_ = new wxTextCtrl(notificationsPage, wxID_ANY);
    notifyControls->Add(notification_channel_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    notifyControls->Add(new wxStaticText(notificationsPage, wxID_ANY, "Filter:"), 0,
                        wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    notification_filter_ctrl_ = new wxTextCtrl(notificationsPage, wxID_ANY);
    notifyControls->Add(notification_filter_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    notification_subscribe_button_ = new wxButton(notificationsPage, wxID_ANY, "Subscribe");
    notification_unsubscribe_button_ = new wxButton(notificationsPage, wxID_ANY, "Unsubscribe");
    notification_fetch_button_ = new wxButton(notificationsPage, wxID_ANY, "Fetch");
    notification_clear_button_ = new wxButton(notificationsPage, wxID_ANY, "Clear");
    notifyControls->Add(notification_subscribe_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    notifyControls->Add(notification_unsubscribe_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    notifyControls->Add(notification_fetch_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    notifyControls->Add(notification_clear_button_, 0, wxALIGN_CENTER_VERTICAL);
    notificationsSizer->Add(notifyControls, 0, wxEXPAND | wxALL, 6);

    auto* notifyControls2 = new wxBoxSizer(wxHORIZONTAL);
    notification_poll_check_ = new wxCheckBox(notificationsPage, wxID_ANY, "Auto-poll");
    notification_autoscroll_check_ = new wxCheckBox(notificationsPage, wxID_ANY, "Auto-scroll");
    notification_autoscroll_check_->SetValue(true);
    notification_poll_interval_ctrl_ = new wxSpinCtrl(notificationsPage, wxID_ANY);
    notification_poll_interval_ctrl_->SetRange(250, 60000);
    notification_poll_interval_ctrl_->SetValue(2000);
    notifyControls2->Add(notification_poll_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    notifyControls2->Add(notification_autoscroll_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    notifyControls2->Add(new wxStaticText(notificationsPage, wxID_ANY, "Interval (ms):"), 0,
                         wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    notifyControls2->Add(notification_poll_interval_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    notificationsSizer->Add(notifyControls2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    notifications_log_ = new wxTextCtrl(notificationsPage, wxID_ANY, "", wxDefaultPosition,
                                        wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    notificationsSizer->Add(notifications_log_, 1, wxEXPAND | wxALL, 6);
    notificationsPage->SetSizer(notificationsSizer);
    result_notebook_->AddPage(notificationsPage, "Notifications");

    auto* statusPage = new wxPanel(result_notebook_, wxID_ANY);
    auto* statusSizer = new wxBoxSizer(wxVERTICAL);
    auto* statusControls = new wxBoxSizer(wxHORIZONTAL);
    statusControls->Add(new wxStaticText(statusPage, wxID_ANY, "Type:"), 0,
                        wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    status_type_choice_ = new wxChoice(statusPage, wxID_ANY);
    status_type_choice_->Append("Server Info");
    status_type_choice_->Append("Connection Info");
    status_type_choice_->Append("Database Info");
    status_type_choice_->Append("Statistics");
    status_type_choice_->SetSelection(0);
    statusControls->Add(status_type_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    statusControls->Add(new wxStaticText(statusPage, wxID_ANY, "Category:"), 0,
                        wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    status_category_choice_ = new wxChoice(statusPage, wxID_ANY);
    status_category_choice_->Append("All");
    status_category_choice_->SetSelection(0);
    statusControls->Add(status_category_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    status_diff_check_ = new wxCheckBox(statusPage, wxID_ANY, "Diff");
    statusControls->Add(status_diff_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    status_fetch_button_ = new wxButton(statusPage, wxID_ANY, "Fetch");
    status_clear_button_ = new wxButton(statusPage, wxID_ANY, "Clear");
    status_copy_button_ = new wxButton(statusPage, wxID_ANY, "Copy JSON");
    status_save_button_ = new wxButton(statusPage, wxID_ANY, "Save JSON");
    statusControls->Add(status_fetch_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    statusControls->Add(status_clear_button_, 0, wxALIGN_CENTER_VERTICAL);
    statusControls->Add(status_copy_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
    statusControls->Add(status_save_button_, 0, wxALIGN_CENTER_VERTICAL);
    statusSizer->Add(statusControls, 0, wxEXPAND | wxALL, 6);

    auto* statusControlsDiff = new wxBoxSizer(wxHORIZONTAL);
    statusControlsDiff->Add(new wxStaticText(statusPage, wxID_ANY, "Diff options:"), 0,
                            wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    status_diff_ignore_unchanged_check_ =
        new wxCheckBox(statusPage, wxID_ANY, "Ignore unchanged");
    status_diff_ignore_empty_check_ = new wxCheckBox(statusPage, wxID_ANY, "Ignore empty");
    status_diff_ignore_unchanged_check_->SetValue(true);
    status_diff_ignore_empty_check_->SetValue(true);
    statusControlsDiff->Add(status_diff_ignore_unchanged_check_, 0,
                            wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    statusControlsDiff->Add(status_diff_ignore_empty_check_, 0, wxALIGN_CENTER_VERTICAL);
    statusSizer->Add(statusControlsDiff, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    auto* statusControls2 = new wxBoxSizer(wxHORIZONTAL);
    status_poll_check_ = new wxCheckBox(statusPage, wxID_ANY, "Auto-poll");
    status_poll_interval_ctrl_ = new wxSpinCtrl(statusPage, wxID_ANY);
    status_poll_interval_ctrl_->SetRange(250, 60000);
    status_poll_interval_ctrl_->SetValue(2000);
    statusControls2->Add(status_poll_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    statusControls2->Add(new wxStaticText(statusPage, wxID_ANY, "Interval (ms):"), 0,
                         wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    statusControls2->Add(status_poll_interval_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    statusSizer->Add(statusControls2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    status_message_label_ = new wxStaticText(statusPage, wxID_ANY, "Ready");
    statusSizer->Add(status_message_label_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    auto* statusBody = new wxBoxSizer(wxHORIZONTAL);
    auto* historyBox = new wxStaticBox(statusPage, wxID_ANY, "History");
    auto* historySizer = new wxStaticBoxSizer(historyBox, wxVERTICAL);
    status_history_list_ = new wxListBox(historyBox, wxID_ANY);
    status_history_list_->SetMinSize(wxSize(180, -1));
    historySizer->Add(status_history_list_, 1, wxEXPAND | wxALL, 6);
    statusBody->Add(historySizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    status_cards_panel_ = new wxScrolledWindow(statusPage, wxID_ANY, wxDefaultPosition,
                                               wxDefaultSize, wxVSCROLL);
    status_cards_panel_->SetScrollRate(0, 10);
    status_cards_sizer_ = new wxBoxSizer(wxVERTICAL);
    status_cards_panel_->SetSizer(status_cards_sizer_);
    statusBody->Add(status_cards_panel_, 1, wxEXPAND | wxALL, 6);
    statusSizer->Add(statusBody, 1, wxEXPAND);
    statusPage->SetSizer(statusSizer);
    result_notebook_->AddPage(statusPage, "Status");

    gridSizer->Add(result_notebook_, 1, wxEXPAND | wxALL, 4);
    gridPanel->SetSizer(gridSizer);

    splitter->SplitHorizontally(editorPanel, gridPanel, 350);
    rootSizer->Add(splitter, 1, wxEXPAND);

    SetSizer(rootSizer);

    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }

    Bind(wxEVT_CLOSE_WINDOW, &SqlEditorFrame::OnClose, this);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnExecuteQuery, this, ID_SQL_RUN);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnCancelQuery, this, ID_SQL_CANCEL);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnExportCsv, this, ID_SQL_EXPORT_CSV);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnExportJson, this, ID_SQL_EXPORT_JSON);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnNewSqlEditor, this, ID_MENU_NEW_SQL_EDITOR);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnNewDiagram, this, ID_MENU_NEW_DIAGRAM);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnOpenMonitoring, this, ID_MENU_MONITORING);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnOpenUsersRoles, this, ID_MENU_USERS_ROLES);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnOpenJobScheduler, this, ID_MENU_JOB_SCHEDULER);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnOpenDomainManager, this, ID_MENU_DOMAIN_MANAGER);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnOpenSchemaManager, this, ID_MENU_SCHEMA_MANAGER);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnOpenTableDesigner, this, ID_MENU_TABLE_DESIGNER);
    Bind(wxEVT_MENU, &SqlEditorFrame::OnOpenIndexDesigner, this, ID_MENU_INDEX_DESIGNER);
    run_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExecuteQuery, this);
    cancel_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnCancelQuery, this);
    copy_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnCopy, this);
    connect_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnConnect, this);
    disconnect_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnDisconnect, this);
    connection_choice_->Bind(wxEVT_CHOICE, &SqlEditorFrame::OnConnectionChanged, this);
    auto_commit_check_->Bind(wxEVT_CHECKBOX, &SqlEditorFrame::OnToggleAutoCommit, this);
    begin_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnBegin, this);
    commit_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnCommit, this);
    rollback_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnRollback, this);
    savepoint_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnSavepoint, this);
    savepoint_choice_->Bind(wxEVT_CHOICE, &SqlEditorFrame::OnRollbackToSavepoint, this);
    isolation_choice_->Bind(wxEVT_CHOICE, &SqlEditorFrame::OnIsolationLevelChanged, this);
    prev_page_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnPrevPage, this);
    next_page_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnNextPage, this);
    page_size_ctrl_->Bind(wxEVT_SPINCTRL, &SqlEditorFrame::OnPageSizeChanged, this);
    row_limit_ctrl_->Bind(wxEVT_SPINCTRL, &SqlEditorFrame::OnRowLimitChanged, this);
    paging_check_->Bind(wxEVT_CHECKBOX, &SqlEditorFrame::OnTogglePaging, this);
    stream_check_->Bind(wxEVT_CHECKBOX, &SqlEditorFrame::OnToggleStream, this);
    export_csv_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExportCsv, this);
    export_json_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExportJson, this);
    if (stream_apply_button_) {
        stream_apply_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnApplyStreamSettings, this);
    }
    result_choice_->Bind(wxEVT_CHOICE, &SqlEditorFrame::OnResultSelection, this);
    history_load_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnHistoryLoad, this);
    explain_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExplain, this);
    sblr_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnSblr, this);
    prepared_edit_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnEditPreparedParams, this);
    prepared_prepare_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnPrepareStatement, this);
    prepared_execute_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExecutePrepared, this);
    if (notification_subscribe_button_) {
        notification_subscribe_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnSubscribeNotifications, this);
    }
    if (notification_unsubscribe_button_) {
        notification_unsubscribe_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnUnsubscribeNotifications, this);
    }
    if (notification_fetch_button_) {
        notification_fetch_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnFetchNotification, this);
    }
    if (notification_clear_button_) {
        notification_clear_button_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (notifications_log_) {
                notifications_log_->Clear();
            }
        });
    }
    if (status_fetch_button_) {
        status_fetch_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnFetchStatus, this);
    }
    if (status_clear_button_) {
        status_clear_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnClearStatus, this);
    }
    if (status_copy_button_) {
        status_copy_button_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (!has_status_) {
                SetStatusMessage("No status data to copy");
                return;
            }
            std::string json = BuildStatusJson(last_status_, SelectedStatusCategory(),
                                               status_diff_check_ && status_diff_check_->GetValue());
            if (wxTheClipboard->Open()) {
                wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(json)));
                wxTheClipboard->Close();
                SetStatusMessage("Status JSON copied to clipboard");
            } else {
                SetStatusMessage("Unable to access clipboard");
            }
        });
    }
    if (status_save_button_) {
        status_save_button_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (!has_status_) {
                SetStatusMessage("No status data to save");
                return;
            }
            wxFileDialog dialog(this, "Save Status JSON", "", "status.json",
                                "JSON files (*.json)|*.json|All files (*.*)|*.*",
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (dialog.ShowModal() != wxID_OK) {
                return;
            }
            std::ofstream out(dialog.GetPath().ToStdString());
            if (!out.is_open()) {
                SetStatusMessage("Failed to save status JSON");
                return;
            }
            out << BuildStatusJson(last_status_, SelectedStatusCategory(),
                                   status_diff_check_ && status_diff_check_->GetValue());
            SetStatusMessage("Status JSON saved");
        });
    }
    if (status_type_choice_) {
        status_type_choice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
            PersistStatusPreferences();
        });
    }
    if (status_category_choice_) {
        status_category_choice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
            if (has_status_) {
                BuildStatusCards(last_status_);
            }
            PersistStatusPreferences();
        });
    }
    if (status_diff_check_) {
        status_diff_check_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            UpdateDiffOptionControls();
            if (has_status_) {
                BuildStatusCards(last_status_);
            }
            PersistStatusPreferences();
        });
    }
    if (status_poll_interval_ctrl_) {
        status_poll_interval_ctrl_->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent&) {
            if (status_timer_ && status_poll_check_ && status_poll_check_->GetValue()) {
                int interval = status_poll_interval_ctrl_->GetValue();
                if (interval <= 0) {
                    interval = 2000;
                }
                status_timer_->Start(interval);
            }
            PersistStatusPreferences();
        });
    }
    if (status_diff_ignore_unchanged_check_) {
        status_diff_ignore_unchanged_check_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            if (has_status_) {
                BuildStatusCards(last_status_);
            }
            PersistStatusPreferences();
        });
    }
    if (status_diff_ignore_empty_check_) {
        status_diff_ignore_empty_check_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            if (has_status_) {
                BuildStatusCards(last_status_);
            }
            PersistStatusPreferences();
        });
    }
    if (status_history_list_) {
        status_history_list_->Bind(wxEVT_LISTBOX, &SqlEditorFrame::OnStatusHistorySelection, this);
    }
    if (status_poll_check_) {
        status_poll_check_->Bind(wxEVT_CHECKBOX, &SqlEditorFrame::OnToggleStatusPolling, this);
    }
    status_timer_ = new wxTimer(this);
    Bind(wxEVT_TIMER, &SqlEditorFrame::OnStatusTimer, this, status_timer_->GetId());
    if (notification_poll_check_) {
        notification_poll_check_->Bind(wxEVT_CHECKBOX, &SqlEditorFrame::OnToggleNotificationPolling, this);
    }
    notification_timer_ = new wxTimer(this);
    Bind(wxEVT_TIMER, &SqlEditorFrame::OnNotificationTimer, this, notification_timer_->GetId());

    PopulateConnections();
    UpdateSessionControls();
    UpdatePagingControls();
    UpdateResultControls();
    UpdateHistoryControls();
    UpdateExportControls();
}

void SqlEditorFrame::LoadStatement(const std::string& sql) {
    if (!editor_) {
        return;
    }
    editor_->SetValue(sql);
    editor_->SetInsertionPointEnd();
    editor_->SetFocus();
}

void SqlEditorFrame::OnExecuteQuery(wxCommandEvent&) {
    if (!connection_manager_) {
        wxMessageBox("No connection manager configured.", "Execution Error", wxOK | wxICON_ERROR, this);
        return;
    }

    std::string sql = editor_ ? editor_->GetValue().ToStdString() : "";
    if (!ExecuteStatements(sql)) {
        return;
    }
}

void SqlEditorFrame::OnCancelQuery(wxCommandEvent&) {
    if (!query_running_) {
        return;
    }
    if (connection_manager_) {
        connection_manager_->CancelActive();
        connection_manager_->SetProgressCallback(nullptr);
    }
    active_query_job_.Cancel();
    UpdateStatus("Cancel requested");
    UpdateSessionControls();
}

void SqlEditorFrame::OnEditPreparedParams(wxCommandEvent&) {
    if (!active_prepared_) {
        wxMessageBox("Prepare a statement first.", "Prepared Parameters",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }
    PreparedParamsDialog dialog(this, active_prepared_->parameterCount, prepared_params_);
    if (dialog.ShowModal() == wxID_OK) {
        prepared_params_ = dialog.GetParams();
        UpdateStatus("Prepared params updated");
    }
}

void SqlEditorFrame::OnPrepareStatement(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    std::string sql = editor_ ? editor_->GetStringSelection().ToStdString() : "";
    if (sql.empty() && editor_) {
        sql = editor_->GetValue().ToStdString();
    }
    if (sql.empty()) {
        wxMessageBox("Enter SQL to prepare.", "Prepare Statement",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    PreparedStatementHandlePtr handle;
    if (!connection_manager_->PrepareStatement(sql, &handle)) {
        wxMessageBox(connection_manager_->LastError(), "Prepare Failed",
                     wxOK | wxICON_ERROR, this);
        return;
    }
    active_prepared_ = handle;
    prepared_params_.assign(handle->parameterCount, PreparedParameter{});
    UpdateStatus("Prepared statement ready (" + std::to_string(handle->parameterCount) + " params)");
    UpdateSessionControls();
}

void SqlEditorFrame::OnExecutePrepared(wxCommandEvent&) {
    if (!connection_manager_ || !active_prepared_) {
        wxMessageBox("Prepare a statement first.", "Execute Prepared",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }
    QueryResult result;
    if (!connection_manager_->ExecutePrepared(active_prepared_, prepared_params_, &result)) {
        wxMessageBox(connection_manager_->LastError(), "Execute Failed",
                     wxOK | wxICON_ERROR, this);
        return;
    }
    ResultEntry entry;
    entry.statement = active_prepared_->sql;
    entry.result = result;
    entry.isPaged = false;
    result_sets_.push_back(std::move(entry));
    int result_index = static_cast<int>(result_sets_.size() - 1);
    UpdateResultChoiceSelection(result_index);
    PopulateGrid(result_sets_[static_cast<size_t>(result_index)].result, false);
    UpdateStatus("Prepared statement executed");
    UpdateExportControls();
}

void SqlEditorFrame::OnSubscribeNotifications(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    std::string channel = notification_channel_ctrl_
                              ? notification_channel_ctrl_->GetValue().ToStdString()
                              : std::string();
    std::string filter = notification_filter_ctrl_
                             ? notification_filter_ctrl_->GetValue().ToStdString()
                             : std::string();
    if (channel.empty()) {
        wxMessageBox("Enter a channel name.", "Subscribe",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    if (!connection_manager_->Subscribe(channel, filter)) {
        wxMessageBox(connection_manager_->LastError(), "Subscribe Failed",
                     wxOK | wxICON_ERROR, this);
        return;
    }
    AppendNotificationLine("Subscribed to channel: " + channel);
}

void SqlEditorFrame::OnUnsubscribeNotifications(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    std::string channel = notification_channel_ctrl_
                              ? notification_channel_ctrl_->GetValue().ToStdString()
                              : std::string();
    if (channel.empty()) {
        wxMessageBox("Enter a channel name.", "Unsubscribe",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    if (!connection_manager_->Unsubscribe(channel)) {
        wxMessageBox(connection_manager_->LastError(), "Unsubscribe Failed",
                     wxOK | wxICON_ERROR, this);
        return;
    }
    AppendNotificationLine("Unsubscribed from channel: " + channel);
}

void SqlEditorFrame::OnFetchNotification(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    if (notification_fetch_button_) {
        notification_fetch_button_->Enable(false);
    }
    notification_fetch_pending_ = true;
    connection_manager_->FetchNotificationAsync(
        [this](bool ok, NotificationEvent ev, const std::string& error) {
            CallAfter([this, ok, ev, error]() {
                if (notification_fetch_button_) {
                    notification_fetch_button_->Enable(true);
                }
                notification_fetch_pending_ = false;
                if (!ok) {
                    if (error == "No notification") {
                        UpdateStatus("No notification available");
                        return;
                    }
                    wxMessageBox(error, "Fetch Notification Failed",
                                 wxOK | wxICON_ERROR, this);
                    return;
                }
                if (ShouldDisplayNotification(ev)) {
                    AppendNotificationLine(FormatNotificationPayload(ev));
                }
            });
        });
}

void SqlEditorFrame::OnToggleNotificationPolling(wxCommandEvent&) {
    if (!notification_timer_ || !notification_poll_check_ || !notification_poll_interval_ctrl_) {
        return;
    }
    if (notification_poll_check_->GetValue()) {
        int interval = notification_poll_interval_ctrl_->GetValue();
        if (interval <= 0) {
            interval = 2000;
        }
        notification_poll_interval_ctrl_->SetValue(interval);
        notification_timer_->Start(interval);
        AppendNotificationLine("Notification polling enabled (" + std::to_string(interval) + " ms)");
    } else {
        notification_timer_->Stop();
        AppendNotificationLine("Notification polling disabled");
    }
}

void SqlEditorFrame::OnNotificationTimer(wxTimerEvent&) {
    if (notification_fetch_pending_) {
        return;
    }
    if (!connection_manager_) {
        return;
    }
    if (!notification_poll_check_ || !notification_poll_check_->GetValue()) {
        return;
    }
    notification_fetch_pending_ = true;
    connection_manager_->FetchNotificationAsync(
        [this](bool ok, NotificationEvent ev, const std::string& error) {
            CallAfter([this, ok, ev, error]() {
                notification_fetch_pending_ = false;
                if (!ok) {
                    if (error != "No notification") {
                        AppendNotificationLine("Notification error: " + error);
                    }
                    return;
                }
                if (ShouldDisplayNotification(ev)) {
                    AppendNotificationLine(FormatNotificationPayload(ev));
                }
            });
        });
}

void SqlEditorFrame::OnFetchStatus(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    if (!connection_manager_->IsConnected()) {
        wxMessageBox("Connect to a database before fetching status.", "Status",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    StatusRequestKind kind = StatusRequestKind::ServerInfo;
    if (status_type_choice_) {
        switch (status_type_choice_->GetSelection()) {
            case 1: kind = StatusRequestKind::ConnectionInfo; break;
            case 2: kind = StatusRequestKind::DatabaseInfo; break;
            case 3: kind = StatusRequestKind::Statistics; break;
            default: kind = StatusRequestKind::ServerInfo; break;
        }
    }
    if (status_fetch_button_) {
        status_fetch_button_->Enable(false);
    }
    status_fetch_pending_ = true;
    connection_manager_->FetchStatusAsync(
        kind,
        [this](bool ok, StatusSnapshot snapshot, const std::string& error) {
            CallAfter([this, ok, snapshot, error]() {
                if (status_fetch_button_) {
                    status_fetch_button_->Enable(true);
                }
                status_fetch_pending_ = false;
                if (!ok) {
                    SetStatusMessage("Status error: " + error);
                    return;
                }
                if (has_status_) {
                    previous_status_ = last_status_;
                }
                last_status_ = snapshot;
                has_status_ = true;
                AddStatusHistory(snapshot);
                DisplayStatusSnapshot(snapshot);
                SetStatusMessage("Status updated");
                UpdateResultControls();
            });
        });
}

void SqlEditorFrame::OnClearStatus(wxCommandEvent&) {
    ClearStatusCards();
    has_status_ = false;
    previous_status_ = StatusSnapshot{};
    status_history_.clear();
    RefreshStatusHistory();
    SetStatusMessage("Status cleared");
    UpdateResultControls();
}

void SqlEditorFrame::OnToggleStatusPolling(wxCommandEvent&) {
    if (!status_timer_ || !status_poll_check_ || !status_poll_interval_ctrl_) {
        return;
    }
    if (status_poll_check_->GetValue()) {
        int interval = status_poll_interval_ctrl_->GetValue();
        if (interval <= 0) {
            interval = 2000;
        }
        status_poll_interval_ctrl_->SetValue(interval);
        status_timer_->Start(interval);
        SetStatusMessage("Status auto-poll enabled (" + std::to_string(interval) + " ms)");
    } else {
        status_timer_->Stop();
        SetStatusMessage("Status auto-poll disabled");
    }
    PersistStatusPreferences();
}

void SqlEditorFrame::OnStatusTimer(wxTimerEvent&) {
    if (status_fetch_pending_) {
        return;
    }
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        return;
    }
    auto caps = connection_manager_->Capabilities();
    if (!caps.supportsStatus) {
        return;
    }
    if (!status_poll_check_ || !status_poll_check_->GetValue()) {
        return;
    }
    StatusRequestKind kind = StatusRequestKind::ServerInfo;
    if (status_type_choice_) {
        switch (status_type_choice_->GetSelection()) {
            case 1: kind = StatusRequestKind::ConnectionInfo; break;
            case 2: kind = StatusRequestKind::DatabaseInfo; break;
            case 3: kind = StatusRequestKind::Statistics; break;
            default: kind = StatusRequestKind::ServerInfo; break;
        }
    }
    status_fetch_pending_ = true;
    connection_manager_->FetchStatusAsync(
        kind,
        [this](bool ok, StatusSnapshot snapshot, const std::string& error) {
            CallAfter([this, ok, snapshot, error]() {
                status_fetch_pending_ = false;
                if (!ok) {
                    SetStatusMessage("Status error: " + error);
                    return;
                }
                if (has_status_) {
                    previous_status_ = last_status_;
                }
                last_status_ = snapshot;
                has_status_ = true;
                AddStatusHistory(snapshot);
                DisplayStatusSnapshot(snapshot);
                UpdateResultControls();
            });
        });
}

void SqlEditorFrame::OnStatusHistorySelection(wxCommandEvent&) {
    if (!status_history_list_) {
        return;
    }
    int selection = status_history_list_->GetSelection();
    if (selection < 0) {
        return;
    }
    ShowHistorySnapshot(static_cast<size_t>(selection));
    SetStatusMessage("Status history selected");
}

void SqlEditorFrame::OnCopy(wxCommandEvent&) {
    if (!connection_manager_) {
        wxMessageBox("No connection manager configured.", "COPY Error", wxOK | wxICON_ERROR, this);
        return;
    }
    if (!connection_manager_->IsConnected()) {
        wxMessageBox("Connect to a database before running COPY.", "COPY Error",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    std::string sql = editor_ ? editor_->GetStringSelection().ToStdString() : "";
    if (sql.empty() && editor_) {
        sql = editor_->GetValue().ToStdString();
    }
    CopyDialog dialog(this, connection_manager_, sql);
    dialog.ShowModal();
}

void SqlEditorFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, metadata_model_);
    editor->Show(true);
}

void SqlEditorFrame::OnNewDiagram(wxCommandEvent&) {
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

void SqlEditorFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_, app_config_);
    monitor->Show(true);
}

void SqlEditorFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_, app_config_);
    users->Show(true);
}

void SqlEditorFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_, app_config_);
    scheduler->Show(true);
}

void SqlEditorFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    domains->Show(true);
}

void SqlEditorFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    schemas->Show(true);
}

void SqlEditorFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    tables->Show(true);
}

void SqlEditorFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    indexes->Show(true);
}

void SqlEditorFrame::OnConnect(wxCommandEvent&) {
    const ConnectionProfile* profile = GetSelectedProfile();
    if (!profile) {
        wxMessageBox("Select a connection profile first.", "Connection Error", wxOK | wxICON_WARNING, this);
        return;
    }
    if (!EnsureConnected(*profile)) {
        wxMessageBox(connection_manager_->LastError(), "Connection Error", wxOK | wxICON_ERROR, this);
        return;
    }
    ApplyStatusDefaults(profile, true);
    paging_active_ = false;
    current_statement_.clear();
    UpdateStatus("Connected to " + (profile->name.empty() ? profile->database : profile->name));
    UpdateSessionControls();
    UpdatePagingControls();
}

void SqlEditorFrame::OnDisconnect(wxCommandEvent&) {
    if (connection_manager_) {
        connection_manager_->Disconnect();
        active_profile_index_ = -1;
    }
    ApplyStatusDefaults(nullptr, false);
    ClearStatusCards();
    has_status_ = false;
    previous_status_ = StatusSnapshot{};
    status_history_.clear();
    RefreshStatusHistory();
    paging_active_ = false;
    current_statement_.clear();
    UpdateStatus("Disconnected");
    UpdateSessionControls();
    UpdatePagingControls();
}

void SqlEditorFrame::OnConnectionChanged(wxCommandEvent&) {
    if (connection_manager_ && connection_manager_->IsConnected()) {
        connection_manager_->Disconnect();
    }
    ApplyStatusDefaults(GetSelectedProfile(), false);
    ClearStatusCards();
    has_status_ = false;
    previous_status_ = StatusSnapshot{};
    status_history_.clear();
    RefreshStatusHistory();
    active_profile_index_ = -1;
    paging_active_ = false;
    current_statement_.clear();
    UpdateSessionControls();
    UpdatePagingControls();
}

void SqlEditorFrame::OnBegin(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    
    // Set isolation level before beginning transaction
    if (isolation_choice_) {
        int selection = isolation_choice_->GetSelection();
        std::string isolationLevel;
        switch (selection) {
            case 0: isolationLevel = "READ COMMITTED"; break;
            case 1: isolationLevel = "READ UNCOMMITTED"; break;
            case 2: isolationLevel = "REPEATABLE READ"; break;
            case 3: isolationLevel = "SERIALIZABLE"; break;
            default: isolationLevel = "READ COMMITTED"; break;
        }
        // Note: The actual SET TRANSACTION would be sent to the backend here
        // For now, we just track it for display purposes
    }
    
    if (!connection_manager_->BeginTransaction()) {
        wxMessageBox(connection_manager_->LastError(), "Transaction Error", wxOK | wxICON_ERROR, this);
        transaction_failed_ = true;
        UpdateSessionControls();
        return;
    }
    
    transaction_start_time_ = std::chrono::steady_clock::now();
    transaction_statement_count_ = 0;
    transaction_failed_ = false;
    UpdateStatus("Transaction started");
    UpdateSessionControls();
}

void SqlEditorFrame::OnCommit(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    if (!connection_manager_->Commit()) {
        wxMessageBox(connection_manager_->LastError(), "Transaction Error", wxOK | wxICON_ERROR, this);
        transaction_failed_ = true;
        UpdateSessionControls();
        return;
    }
    
    auto elapsed = std::chrono::steady_clock::now() - transaction_start_time_;
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    
    UpdateStatus(wxString::Format("Committed (%d statements, %ld ms)", 
                                  transaction_statement_count_, elapsedMs));
    transaction_failed_ = false;
    UpdateSessionControls();
}

void SqlEditorFrame::OnRollback(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    if (!connection_manager_->Rollback()) {
        wxMessageBox(connection_manager_->LastError(), "Transaction Error", wxOK | wxICON_ERROR, this);
        return;
    }
    // Clear savepoints on full rollback
    savepoints_.clear();
    if (savepoint_choice_) {
        savepoint_choice_->Clear();
    }
    UpdateStatus("Rolled back");
    transaction_failed_ = false;
    UpdateSessionControls();
}

void SqlEditorFrame::OnSavepoint(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    
    // Generate savepoint name
    std::string sp_name = "sp_" + std::to_string(savepoints_.size() + 1);
    
    // Execute SAVEPOINT command
    std::string sql = "SAVEPOINT " + sp_name + ";";
    connection_manager_->ExecuteQueryAsync(sql, 
        [this, sp_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, sp_name, error]() {
                if (ok) {
                    savepoints_.push_back(sp_name);
                    if (savepoint_choice_) {
                        savepoint_choice_->Append(sp_name);
                        savepoint_choice_->SetSelection(savepoint_choice_->GetCount() - 1);
                    }
                    UpdateStatus("Savepoint " + sp_name + " created");
                } else {
                    wxMessageBox("Failed to create savepoint: " + error, 
                                "Savepoint Error", wxOK | wxICON_ERROR, this);
                }
            });
        });
}

void SqlEditorFrame::OnRollbackToSavepoint(wxCommandEvent&) {
    if (!connection_manager_ || !savepoint_choice_) {
        return;
    }
    
    int selection = savepoint_choice_->GetSelection();
    if (selection == wxNOT_FOUND) {
        return;
    }
    
    std::string sp_name = savepoints_[selection];
    std::string sql = "ROLLBACK TO SAVEPOINT " + sp_name + ";";
    
    connection_manager_->ExecuteQueryAsync(sql,
        [this, sp_name, selection](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, sp_name, error]() {
                if (ok) {
                    UpdateStatus("Rolled back to savepoint " + sp_name);
                } else {
                    wxMessageBox("Failed to rollback to savepoint: " + error,
                                "Savepoint Error", wxOK | wxICON_ERROR, this);
                }
            });
        });
}

void SqlEditorFrame::OnReleaseSavepoint(wxCommandEvent&) {
    if (!connection_manager_ || !savepoint_choice_) {
        return;
    }
    
    int selection = savepoint_choice_->GetSelection();
    if (selection == wxNOT_FOUND) {
        return;
    }
    
    std::string sp_name = savepoints_[selection];
    std::string sql = "RELEASE SAVEPOINT " + sp_name + ";";
    
    connection_manager_->ExecuteQueryAsync(sql,
        [this, sp_name, selection](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, sp_name, selection, error]() {
                if (ok) {
                    // Remove this and subsequent savepoints
                    savepoints_.erase(savepoints_.begin() + selection, savepoints_.end());
                    if (savepoint_choice_) {
                        savepoint_choice_->Clear();
                        for (const auto& sp : savepoints_) {
                            savepoint_choice_->Append(sp);
                        }
                        if (!savepoints_.empty()) {
                            savepoint_choice_->SetSelection(savepoints_.size() - 1);
                        }
                    }
                    UpdateStatus("Savepoint " + sp_name + " released");
                } else {
                    wxMessageBox("Failed to release savepoint: " + error,
                                "Savepoint Error", wxOK | wxICON_ERROR, this);
                }
            });
        });
}

void SqlEditorFrame::OnToggleAutoCommit(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->SetAutoCommit(auto_commit_check_ && auto_commit_check_->GetValue());
    UpdateSessionControls();
}

void SqlEditorFrame::OnIsolationLevelChanged(wxCommandEvent&) {
    // Isolation level will be applied on next BEGIN
    // Could show a notification that it will take effect on next transaction
}

void SqlEditorFrame::UpdateTransactionUI() {
    if (!transaction_indicator_) {
        return;
    }
    
    bool inTransaction = connection_manager_ && connection_manager_->IsInTransaction();
    
    if (transaction_failed_) {
        // Transaction failed state
        transaction_indicator_->SetLabel("TX Failed");
        transaction_indicator_->SetBackgroundColour(wxColour(220, 53, 69));  // Red
        transaction_indicator_->SetForegroundColour(wxColour(255, 255, 255));  // White
    } else if (inTransaction) {
        // Transaction active
        transaction_indicator_->SetLabel("TX Active");
        transaction_indicator_->SetBackgroundColour(wxColour(255, 193, 7));  // Yellow
        transaction_indicator_->SetForegroundColour(wxColour(0, 0, 0));  // Black
    } else {
        // Auto-commit mode
        transaction_indicator_->SetLabel("Auto");
        transaction_indicator_->SetBackgroundColour(wxColour(108, 117, 125));  // Gray
        transaction_indicator_->SetForegroundColour(wxColour(255, 255, 255));  // White
    }
    
    // Update window title
    wxString title = "SQL Editor";
    if (connection_manager_ && connection_manager_->IsConnected()) {
        const ConnectionProfile* profile = GetSelectedProfile();
        if (profile && !profile->database.empty()) {
            title = wxString::Format("%s@%s", profile->database, profile->host);
        }
    }
    if (inTransaction) {
        title += " [TX]";
    }
    SetTitle(title);
}

bool SqlEditorFrame::ConfirmCloseWithTransaction() {
    auto elapsed = std::chrono::steady_clock::now() - transaction_start_time_;
    auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    
    wxString message = wxString::Format(
        "You have an active transaction with uncommitted changes.\n\n"
        "Transaction started: %ld seconds ago\n"
        "Statements executed: %d\n\n"
        "Click 'Yes' to commit changes, 'No' to rollback, or 'Cancel' to keep the window open.",
        elapsedSec,
        transaction_statement_count_
    );
    
    int result = wxMessageBox(message, "Uncommitted Transaction",
                             wxYES_NO | wxCANCEL | wxICON_WARNING, this);
    
    switch (result) {
        case wxYES:
            // Commit and close
            if (connection_manager_->Commit()) {
                return true;  // Close window
            } else {
                wxMessageBox(connection_manager_->LastError(), "Commit Failed", 
                           wxOK | wxICON_ERROR, this);
                return false;  // Stay open
            }
            
        case wxNO:
            // Rollback and close
            if (connection_manager_->Rollback()) {
                return true;  // Close window
            } else {
                wxMessageBox(connection_manager_->LastError(), "Rollback Failed",
                           wxOK | wxICON_ERROR, this);
                return false;  // Stay open
            }
            
        case wxCANCEL:
        default:
            // Cancel close, keep window open
            return false;
    }
}

void SqlEditorFrame::OnTogglePaging(wxCommandEvent&) {
    UpdatePagingControls();
}

void SqlEditorFrame::OnToggleStream(wxCommandEvent&) {
    UpdatePagingControls();
}

void SqlEditorFrame::OnRowLimitChanged(wxCommandEvent&) {
    if (row_limit_ctrl_) {
        row_limit_ = row_limit_ctrl_->GetValue();
    }
}

void SqlEditorFrame::OnPrevPage(wxCommandEvent&) {
    if (!paging_active_ || current_statement_.empty()) {
        return;
    }
    if (stream_check_ && stream_check_->GetValue()) {
        return;
    }
    if (current_page_ <= 0) {
        return;
    }
    current_page_--;
    ExecutePagedStatement(current_statement_, current_page_, false);
}

void SqlEditorFrame::OnNextPage(wxCommandEvent&) {
    if (!paging_active_ || current_statement_.empty()) {
        return;
    }
    bool stream_append = stream_check_ && stream_check_->GetValue();
    current_page_++;
    ExecutePagedStatement(current_statement_, current_page_, stream_append);
}

void SqlEditorFrame::OnPageSizeChanged(wxCommandEvent&) {
    if (!page_size_ctrl_) {
        return;
    }
    page_size_ = page_size_ctrl_->GetValue();
    if (paging_active_ && !current_statement_.empty()) {
        current_page_ = 0;
        ExecutePagedStatement(current_statement_, current_page_, false);
    }
    UpdatePagingControls();
}

void SqlEditorFrame::OnExportCsv(wxCommandEvent&) {
    if (!has_result_) {
        return;
    }

    wxFileDialog dialog(this, "Export CSV", "", "",
                        "CSV files (*.csv)|*.csv|All files (*.*)|*.*",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    ExportOptions options;
    options.includeHeaders = true;
    options.maxBinaryBytes = 0;
    options.includeBinarySize = false;

    std::string error;
    if (!ExportResultToCsv(last_result_, dialog.GetPath().ToStdString(), &error, options)) {
        wxMessageBox(error, "Export Error", wxOK | wxICON_ERROR, this);
        return;
    }
    UpdateStatus("Exported CSV");
}

void SqlEditorFrame::OnExportJson(wxCommandEvent&) {
    if (!has_result_) {
        return;
    }

    wxFileDialog dialog(this, "Export JSON", "", "",
                        "JSON files (*.json)|*.json|All files (*.*)|*.*",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    ExportOptions options;
    options.includeHeaders = true;
    options.maxBinaryBytes = 0;
    options.includeBinarySize = false;

    std::string error;
    if (!ExportResultToJson(last_result_, dialog.GetPath().ToStdString(), &error, options)) {
        wxMessageBox(error, "Export Error", wxOK | wxICON_ERROR, this);
        return;
    }
    UpdateStatus("Exported JSON");
}

void SqlEditorFrame::OnApplyStreamSettings(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    NetworkOptions options = connection_manager_->GetNetworkOptions();
    if (stream_window_ctrl_) {
        options.streamWindowBytes = static_cast<uint32_t>(stream_window_ctrl_->GetValue());
    }
    if (stream_chunk_ctrl_) {
        options.streamChunkBytes = static_cast<uint32_t>(stream_chunk_ctrl_->GetValue());
    }
    connection_manager_->SetNetworkOptions(options);
    UpdateStatus("Stream settings updated (reconnect to apply)");
}

void SqlEditorFrame::OnResultSelection(wxCommandEvent&) {
    if (!result_choice_) {
        return;
    }
    int selection = result_choice_->GetSelection();
    ShowResultAtIndex(selection);
}

void SqlEditorFrame::OnHistoryLoad(wxCommandEvent&) {
    if (!history_choice_ || !editor_) {
        return;
    }
    int selection = history_choice_->GetSelection();
    if (selection == wxNOT_FOUND) {
        return;
    }
    if (selection < 0 || static_cast<size_t>(selection) >= statement_history_.size()) {
        return;
    }
    editor_->SetValue(statement_history_[static_cast<size_t>(selection)]);
    UpdateStatus("Loaded history entry");
}

void SqlEditorFrame::OnExplain(wxCommandEvent&) {
    std::string statement = GetExplainTargetSql();
    if (statement.empty()) {
        wxMessageBox("Select or enter a statement to explain.", "Explain",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }
    StartSpecialQuery("EXPLAIN " + statement, "EXPLAIN");
}

void SqlEditorFrame::OnSblr(wxCommandEvent&) {
    std::string statement = GetExplainTargetSql();
    if (statement.empty()) {
        wxMessageBox("Select or enter a statement to inspect.", "SBLR",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }
    StartSpecialQuery("SHOW SBLR " + statement, "SBLR");
}

void SqlEditorFrame::OnClose(wxCloseEvent& event) {
    // Check for active transaction
    if (connection_manager_ && connection_manager_->IsInTransaction()) {
        if (!ConfirmCloseWithTransaction()) {
            event.Veto();  // Cancel the close
            return;
        }
        // User chose to commit or rollback - transaction handled in ConfirmCloseWithTransaction
    }
    if (notification_timer_) {
        notification_timer_->Stop();
    }
    if (status_timer_) {
        status_timer_->Stop();
    }

    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

void SqlEditorFrame::PopulateConnections() {
    if (!connection_choice_) {
        return;
    }

    active_profile_index_ = -1;
    connection_choice_->Clear();
    if (!connections_ || connections_->empty()) {
        connection_choice_->Append("No connections configured");
        connection_choice_->SetSelection(0);
        connection_choice_->Enable(false);
        return;
    }

    connection_choice_->Enable(true);
    for (const auto& profile : *connections_) {
        std::string label = profile.name.empty() ? profile.database : profile.name;
        connection_choice_->Append(label);
    }
    connection_choice_->SetSelection(0);
}

const ConnectionProfile* SqlEditorFrame::GetSelectedProfile() const {
    if (!connections_ || connections_->empty() || !connection_choice_) {
        return nullptr;
    }
    int selection = connection_choice_->GetSelection();
    if (selection == wxNOT_FOUND) {
        return nullptr;
    }
    if (selection < 0 || static_cast<size_t>(selection) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(selection)];
}

bool SqlEditorFrame::EnsureConnected(const ConnectionProfile& profile) {
    if (!connection_manager_) {
        return false;
    }

    bool was_connected = connection_manager_->IsConnected();
    int selection = connection_choice_ ? connection_choice_->GetSelection() : -1;
    bool profile_changed = selection != active_profile_index_;

    if (!connection_manager_->IsConnected() || profile_changed) {
        connection_manager_->Disconnect();
        if (!connection_manager_->Connect(profile)) {
            active_profile_index_ = -1;
            return false;
        }
        active_profile_index_ = selection;
    }

    if (!was_connected || profile_changed) {
        UpdateSessionControls();
        if (metadata_model_) {
            metadata_model_->SetFixturePath(profile.fixturePath);
            metadata_model_->Refresh();
        }
    }
    return true;
}

bool SqlEditorFrame::ExecuteStatements(const std::string& sql) {
    std::string trimmed = Trim(sql);
    if (trimmed.empty()) {
        wxMessageBox("Enter a query to execute.", "Execution Error", wxOK | wxICON_INFORMATION, this);
        return false;
    }

    const ConnectionProfile* profile = GetSelectedProfile();
    if (!profile) {
        wxMessageBox("Select a connection profile first.", "Execution Error", wxOK | wxICON_WARNING, this);
        return false;
    }

    if (!EnsureConnected(*profile)) {
        wxMessageBox(connection_manager_->LastError(), "Connection Error", wxOK | wxICON_ERROR, this);
        return false;
    }

    StatementSplitter splitter;
    auto split = splitter.Split(sql);
    if (split.statements.empty()) {
        wxMessageBox("No statements found after parsing.", "Execution Error", wxOK | wxICON_WARNING, this);
        return false;
    }

    AddToHistory(trimmed);
    pending_query_length_ = trimmed.size();
    return ExecuteStatementBatch(split.statements);
}

bool SqlEditorFrame::ExecuteStatementBatch(const std::vector<std::string>& statements) {
    if (query_running_) {
        wxMessageBox("A query is already running.", "Execution Error", wxOK | wxICON_WARNING, this);
        return false;
    }

    ResetGrid();
    ClearMessages();
    last_result_ = QueryResult{};
    has_result_ = false;
    result_sets_.clear();
    active_result_index_ = -1;
    paged_result_index_ = -1;
    if (result_choice_) {
        result_choice_->Clear();
    }
    if (plan_view_) {
        plan_view_->Clear();
    }
    if (sblr_view_) {
        sblr_view_->Clear();
    }
    paging_active_ = false;
    current_statement_.clear();
    pending_rows_affected_ = 0;
    pending_last_tag_.clear();
    pending_last_result_ = QueryResult{};
    pending_statements_ = statements;
    pending_statement_index_ = 0;
    pending_metadata_refresh_ = false;
    for (const auto& statement : statements) {
        if (IsDdlStatement(statement)) {
            pending_metadata_refresh_ = true;
            break;
        }
    }
    query_running_ = true;
    stream_append_ = stream_check_ && stream_check_->GetValue();
    batch_start_time_ = std::chrono::steady_clock::now();

    UpdateStatus("Running...");
    UpdateSessionControls();
    UpdateExportControls();
    ExecuteNextStatement();
    return true;
}

bool SqlEditorFrame::ExecutePagedStatement(const std::string& statement, int page_index, bool stream_append) {
    if (!connection_manager_) {
        return false;
    }

    if (page_index < 0) {
        page_index = 0;
    }

    if (page_index == 0) {
        ResultEntry entry;
        entry.statement = statement;
        entry.isPaged = true;
        result_sets_.push_back(std::move(entry));
        paged_result_index_ = static_cast<int>(result_sets_.size() - 1);
        UpdateResultChoiceSelection(paged_result_index_);
    }

    int64_t offset = static_cast<int64_t>(page_index) * page_size_;
    std::string paged_sql = BuildPagedQuery(statement, offset, page_size_);
    query_running_ = true;
    statement_start_time_ = std::chrono::steady_clock::now();
    UpdateStatus("Running...");
    UpdateSessionControls();
    StartAsyncQuery(paged_sql, true, true, stream_append, paged_result_index_, statement);
    page_label_->SetLabel("Page " + std::to_string(page_index + 1));
    return true;
}

void SqlEditorFrame::ExecuteNextStatement() {
    if (pending_statement_index_ >= pending_statements_.size()) {
        query_running_ = false;
        UpdateSessionControls();
        UpdatePagingControls();
        return;
    }

    const std::string& statement = pending_statements_[pending_statement_index_];
    bool is_last = (pending_statement_index_ + 1 == pending_statements_.size());

    if (is_last && paging_check_ && paging_check_->GetValue() && IsPagedStatement(statement)) {
        current_statement_ = statement;
        current_page_ = 0;
        paging_active_ = true;
        ExecutePagedStatement(statement, current_page_, stream_append_);
        UpdatePagingControls();
        return;
    }

    ResultEntry entry;
    entry.statement = statement;
    entry.isPaged = false;
    result_sets_.push_back(std::move(entry));
    int result_index = static_cast<int>(result_sets_.size() - 1);
    UpdateResultChoiceSelection(result_index);
    StartAsyncQuery(statement, is_last, false, false, result_index, statement);
}

void SqlEditorFrame::StartAsyncQuery(const std::string& sql, bool is_last, bool is_paged,
                                     bool stream_append, int result_index,
                                     const std::string& statement) {
    if (!connection_manager_) {
        return;
    }

    statement_start_time_ = std::chrono::steady_clock::now();
    connection_manager_->SetProgressCallback(
        [this](uint64_t rows, uint64_t bytes) {
            CallAfter([this, rows, bytes]() {
                if (progress_label_) {
                    progress_label_->SetLabel("Progress: " + std::to_string(rows) +
                                              " rows, " + std::to_string(bytes) + " bytes");
                }
            });
        });
    QueryOptions options;
    options.streaming = stream_check_ && stream_check_->GetValue();
    active_query_job_ = connection_manager_->ExecuteQueryAsync(
        sql, options,
        [this, is_last, is_paged, stream_append, result_index, statement](bool ok,
                                                                          QueryResult result,
                                                                          const std::string& error) {
            auto result_ptr = std::make_shared<QueryResult>(std::move(result));
            auto error_ptr = std::make_shared<std::string>(error);
            CallAfter([this, ok, result_ptr, error_ptr, is_last, is_paged, stream_append,
                       result_index, statement]() {
                HandleQueryResult(ok, *result_ptr, *error_ptr, is_last, is_paged,
                                  stream_append, result_index, statement);
            });
        });
}

void SqlEditorFrame::HandleQueryResult(bool ok, const QueryResult& result,
                                       const std::string& error, bool is_last, bool is_paged,
                                       bool stream_append, int result_index,
                                       const std::string& statement) {
    if (connection_manager_) {
        connection_manager_->SetProgressCallback(nullptr);
    }
    if (progress_label_) {
        progress_label_->SetLabel("Progress: n/a");
    }
    double elapsed_ms = 0.0;
    if (statement_start_time_.time_since_epoch().count() != 0) {
        elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - statement_start_time_).count();
    }
    last_statement_ms_ = elapsed_ms;
    if (stream_metrics_label_ && (stream_check_ && stream_check_->GetValue())) {
        double rows_per_sec = 0.0;
        if (elapsed_ms > 0.0) {
            rows_per_sec = (static_cast<double>(result.rows.size()) / elapsed_ms) * 1000.0;
        }
        stream_metrics_label_->SetLabel("Last stream rows: " +
                                        std::to_string(result.rows.size()) +
                                        " time: " +
                                        std::to_string(static_cast<int64_t>(elapsed_ms)) +
                                        " ms rate: " +
                                        std::to_string(static_cast<int64_t>(rows_per_sec)) +
                                        " r/s");
    }
    
    // Increment transaction statement count if in a transaction
    if (ok && connection_manager_ && connection_manager_->IsInTransaction()) {
        transaction_statement_count_++;
    }
    ResultEntry* entry = nullptr;
    if (result_index >= 0 && static_cast<size_t>(result_index) < result_sets_.size()) {
        entry = &result_sets_[static_cast<size_t>(result_index)];
        entry->elapsedMs = elapsed_ms;
        entry->ok = ok;
        entry->error = error;
    }

    if (!ok) {
        pending_metadata_refresh_ = false;
        if (entry) {
            entry->result = result;
        }
        query_running_ = false;
        wxMessageBox(error, "Execution Error", wxOK | wxICON_ERROR, this);
        ClearMessages();
        AppendMessages(result, error);
        if (result_notebook_) {
            result_notebook_->SetSelection(messages_page_index_);
        }
        UpdateStatus("Execution failed");
        UpdateResultControls();
        UpdateSessionControls();
        UpdatePagingControls();
        UpdateExportControls();
        return;
    }

    if (entry) {
        if (is_paged && stream_append && current_page_ > 0) {
            entry->result.rows.insert(entry->result.rows.end(),
                                      result.rows.begin(), result.rows.end());
            entry->result.rowsAffected += result.rowsAffected;
            entry->result.commandTag = result.commandTag;
            entry->result.messages.insert(entry->result.messages.end(),
                                          result.messages.begin(), result.messages.end());
            if (!result.errorStack.empty()) {
                entry->result.errorStack = result.errorStack;
            }
            entry->result.columns = result.columns;
        } else {
            entry->result = result;
        }
        row_limit_hit_ = false;
        if (!is_paged && row_limit_ > 0) {
            ApplyRowLimit(&entry->result, &row_limit_hit_);
        }
        entry->result.stats.elapsedMs = elapsed_ms;
        entry->result.stats.rowsReturned = static_cast<int64_t>(entry->result.rows.size());
    }

    UpdateResultControls();

    if (is_paged) {
        if (active_result_index_ == -1 || active_result_index_ == result_index) {
            ShowResultAtIndex(result_index);
        }
        size_t row_count = entry ? entry->result.rows.size() : result.rows.size();
        std::string status = "Rows: " + std::to_string(row_count);
        if (result.rowsAffected > 0) {
            status += " | Affected: " + std::to_string(result.rowsAffected);
        }
        if (!result.commandTag.empty()) {
            status += " | " + result.commandTag;
        }
        if (stream_append) {
            status += " | Page: " + std::to_string(current_page_ + 1);
        }
        status += " | Len: " + std::to_string(statement.size());
        if (elapsed_ms > 0.0) {
            status += " | Time: " + std::to_string(static_cast<int64_t>(elapsed_ms)) + " ms";
        }
        if (row_limit_ > 0) {
            status += " | Limit: " + std::to_string(row_limit_);
            if (row_limit_hit_) {
                status += " (hit)";
            }
        }
        UpdateStatus(status);
        query_running_ = false;
        UpdateSessionControls();
        UpdatePagingControls();
        UpdateExportControls();
        return;
    }

    pending_rows_affected_ += result.rowsAffected;
    pending_last_tag_ = result.commandTag;
    if (is_last) {
        pending_last_result_ = result;
        if (row_limit_ > 0) {
            bool limit_hit = false;
            ApplyRowLimit(&pending_last_result_, &limit_hit);
            row_limit_hit_ = limit_hit;
        }
        pending_last_result_.stats.elapsedMs = elapsed_ms;
        pending_last_result_.stats.rowsReturned =
            static_cast<int64_t>(pending_last_result_.rows.size());
    }

    if (active_result_index_ == -1 || active_result_index_ == result_index) {
        ShowResultAtIndex(result_index);
    }

    if (!is_last) {
        pending_statement_index_++;
        ExecuteNextStatement();
        return;
    }

    paging_active_ = false;
    current_statement_.clear();

    std::string status = "Statements: " + std::to_string(pending_statements_.size());
    if (!pending_last_result_.rows.empty()) {
        status += " | Rows: " + std::to_string(pending_last_result_.rows.size());
    }
    if (pending_rows_affected_ > 0) {
        status += " | Affected: " + std::to_string(pending_rows_affected_);
    }
    if (!pending_last_tag_.empty()) {
        status += " | " + pending_last_tag_;
    }
    if (batch_start_time_.time_since_epoch().count() != 0) {
        auto total_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - batch_start_time_).count();
        status += " | Time: " + std::to_string(static_cast<int64_t>(total_ms)) + " ms";
    }
    if (pending_query_length_ > 0) {
        status += " | Len: " + std::to_string(pending_query_length_);
    }
    if (row_limit_ > 0) {
        status += " | Limit: " + std::to_string(row_limit_);
        if (row_limit_hit_) {
            status += " (hit)";
        }
    }
    UpdateStatus(status);
    if (pending_metadata_refresh_ && metadata_model_) {
        metadata_model_->Refresh();
    }
    pending_metadata_refresh_ = false;
    query_running_ = false;
    UpdateSessionControls();
    UpdatePagingControls();
    UpdateExportControls();
}

bool SqlEditorFrame::IsPagedStatement(const std::string& statement) const {
    std::string trimmed = Trim(statement);
    std::string lower = trimmed;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower.rfind("select", 0) == 0 || lower.rfind("with", 0) == 0;
}

bool SqlEditorFrame::IsDdlStatement(const std::string& statement) const {
    std::string trimmed = Trim(statement);
    if (trimmed.empty()) {
        return false;
    }
    trimmed = StripLeadingComments(trimmed);
    std::string lower = ToLowerCopy(trimmed);
    std::string token = FirstToken(lower);
    if (token.empty()) {
        return false;
    }
    static const char* kDdlTokens[] = {
        "create", "alter", "drop", "truncate", "comment", "grant", "revoke", "recreate", "rename"
    };
    for (const char* ddl : kDdlTokens) {
        if (token == ddl) {
            return true;
        }
    }
    return false;
}

std::string SqlEditorFrame::BuildPagedQuery(const std::string& statement,
                                            int64_t offset,
                                            int64_t limit) const {
    std::string trimmed = Trim(statement);
    return "SELECT * FROM (" + trimmed + ") AS scratchrobin_q LIMIT " +
           std::to_string(limit) + " OFFSET " + std::to_string(offset);
}

void SqlEditorFrame::ApplyRowLimit(QueryResult* result, bool* limit_hit) {
    if (!result || row_limit_ <= 0) {
        return;
    }
    if (result->rows.size() > static_cast<size_t>(row_limit_)) {
        result->rows.resize(static_cast<size_t>(row_limit_));
        if (limit_hit) {
            *limit_hit = true;
        }
    }
}

void SqlEditorFrame::PopulateGrid(const QueryResult& result, bool append) {
    if (!result_grid_ || !result_table_) {
        return;
    }

    if (!append || !has_result_) {
        result_table_->Reset(result.columns, result.rows);
        last_result_ = result;
        has_result_ = true;
    } else {
        result_table_->AppendRows(result.rows);
        last_result_.rows.insert(last_result_.rows.end(), result.rows.begin(), result.rows.end());
        last_result_.rowsAffected += result.rowsAffected;
        last_result_.commandTag = result.commandTag;
        last_result_.messages.insert(last_result_.messages.end(),
                                     result.messages.begin(), result.messages.end());
        if (!result.errorStack.empty()) {
            last_result_.errorStack = result.errorStack;
        }
    }
    last_result_.stats.elapsedMs = last_statement_ms_;
    last_result_.stats.rowsReturned = static_cast<int64_t>(last_result_.rows.size());

    result_grid_->AutoSizeColumns(false);
}

void SqlEditorFrame::ResetGrid() {
    if (!result_table_) {
        return;
    }
    result_table_->Clear();
    last_result_ = QueryResult{};
    has_result_ = false;
}

void SqlEditorFrame::UpdateStatus(const std::string& message) {
    if (status_label_) {
        status_label_->SetLabel(message);
    }
}

void SqlEditorFrame::UpdateExportControls() {
    bool enable = has_result_ && !query_running_;
    if (export_csv_button_) {
        export_csv_button_->Enable(enable);
    }
    if (export_json_button_) {
        export_json_button_->Enable(enable);
    }
}

void SqlEditorFrame::UpdateResultControls() {
    if (!result_choice_) {
        return;
    }
    int previous_selection = result_choice_->GetSelection();
    result_choice_->Clear();
    for (size_t i = 0; i < result_sets_.size(); ++i) {
        const auto& entry = result_sets_[i];
        std::string summary = Trim(entry.statement);
        size_t newline = summary.find('\n');
        if (newline != std::string::npos) {
            summary = summary.substr(0, newline);
        }
        if (summary.size() > 60) {
            summary = summary.substr(0, 57) + "...";
        }
        std::string label = std::to_string(i + 1) + ": " + summary;
        if (!entry.ok) {
            label += " [ERROR]";
        } else if (!entry.result.commandTag.empty()) {
            label += " [" + entry.result.commandTag + "]";
        } else if (entry.result.rowsAffected > 0) {
            label += " [Affected " + std::to_string(entry.result.rowsAffected) + "]";
        } else if (!entry.result.rows.empty()) {
            label += " [" + std::to_string(entry.result.rows.size()) + " rows]";
        }
        result_choice_->Append(label);
    }
    if (active_result_index_ >= 0 &&
        static_cast<size_t>(active_result_index_) < result_sets_.size()) {
        result_choice_->SetSelection(active_result_index_);
    } else if (previous_selection != wxNOT_FOUND &&
               previous_selection >= 0 &&
               static_cast<size_t>(previous_selection) < result_sets_.size()) {
        result_choice_->SetSelection(previous_selection);
        active_result_index_ = previous_selection;
    }
    result_choice_->Enable(!result_sets_.empty() && !query_running_);
}

void SqlEditorFrame::ClearMessages() {
    if (message_log_) {
        message_log_->Clear();
    }
}

void SqlEditorFrame::AppendMessages(const QueryResult& result, const std::string& error) {
    if (!message_log_) {
        return;
    }
    if (!error.empty()) {
        AppendMessageLine("ERROR: " + error);
    }
    for (const auto& message : result.messages) {
        std::string prefix = message.severity.empty() ? "NOTICE" : message.severity;
        std::transform(prefix.begin(), prefix.end(), prefix.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        AppendMessageLine(prefix + ": " + message.message);
        if (!message.detail.empty()) {
            AppendMessageLine("  " + message.detail);
        }
    }
    if (!result.errorStack.empty()) {
        AppendMessageLine("ERROR STACK:");
        for (const auto& line : result.errorStack) {
            AppendMessageLine("  " + line);
        }
    }
}

void SqlEditorFrame::AppendMessageLine(const std::string& line) {
    if (!message_log_) {
        return;
    }
    message_log_->AppendText(line + "\n");
}

void SqlEditorFrame::AppendNotificationLine(const std::string& line) {
    if (!notifications_log_) {
        return;
    }
    notifications_log_->AppendText(line + "\n");
    if (notification_autoscroll_check_ && notification_autoscroll_check_->GetValue()) {
        notifications_log_->ShowPosition(notifications_log_->GetLastPosition());
    }
}

void SqlEditorFrame::DisplayStatusSnapshot(const StatusSnapshot& snapshot) {
    UpdateStatusCategoryChoices(snapshot);
    BuildStatusCards(snapshot);
}

void SqlEditorFrame::SetStatusMessage(const std::string& message) {
    if (status_message_label_) {
        status_message_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void SqlEditorFrame::ApplyStatusDefaults(const ConnectionProfile* profile, bool restartTimer) {
    if (status_timer_) {
        status_timer_->Stop();
    }
    if (!profile) {
        if (status_poll_check_) {
            status_poll_check_->SetValue(false);
        }
        if (status_poll_interval_ctrl_) {
            status_poll_interval_ctrl_->SetValue(2000);
        }
        if (status_type_choice_) {
            status_type_choice_->SetSelection(0);
        }
        if (status_category_choice_) {
            status_category_choice_->SetSelection(0);
        }
        if (status_diff_check_) {
            status_diff_check_->SetValue(false);
        }
        if (status_diff_ignore_unchanged_check_) {
            status_diff_ignore_unchanged_check_->SetValue(true);
        }
        if (status_diff_ignore_empty_check_) {
            status_diff_ignore_empty_check_->SetValue(true);
        }
        status_category_preference_ = "All";
        UpdateDiffOptionControls();
        return;
    }
    if (status_poll_check_) {
        status_poll_check_->SetValue(profile->statusAutoPollEnabled);
    }
    if (status_poll_interval_ctrl_) {
        int interval = profile->statusPollIntervalMs > 0 ? profile->statusPollIntervalMs : 2000;
        if (interval < 250) {
            interval = 250;
        }
        status_poll_interval_ctrl_->SetValue(interval);
    }
    if (status_type_choice_) {
        int selection = 0;
        switch (profile->statusDefaultKind) {
            case StatusRequestKind::ConnectionInfo: selection = 1; break;
            case StatusRequestKind::DatabaseInfo: selection = 2; break;
            case StatusRequestKind::Statistics: selection = 3; break;
            case StatusRequestKind::ServerInfo:
            default: selection = 0; break;
        }
        status_type_choice_->SetSelection(selection);
    }
    status_category_order_ = profile->statusCategoryOrder;
    status_category_preference_ = profile->statusCategoryFilter.empty()
                                      ? "All"
                                      : profile->statusCategoryFilter;
    if (status_category_choice_) {
        int restore = status_category_choice_->FindString(
            wxString::FromUTF8(status_category_preference_));
        status_category_choice_->SetSelection(restore == wxNOT_FOUND ? 0 : restore);
    }
    if (status_diff_check_) {
        status_diff_check_->SetValue(profile->statusDiffEnabled);
    }
    if (status_diff_ignore_unchanged_check_) {
        status_diff_ignore_unchanged_check_->SetValue(profile->statusDiffIgnoreUnchanged);
    }
    if (status_diff_ignore_empty_check_) {
        status_diff_ignore_empty_check_->SetValue(profile->statusDiffIgnoreEmpty);
    }
    UpdateDiffOptionControls();
    if (status_timer_) {
        status_timer_->Stop();
        if (restartTimer && status_poll_check_ && status_poll_check_->GetValue()) {
            status_timer_->Start(status_poll_interval_ctrl_
                                     ? status_poll_interval_ctrl_->GetValue()
                                     : 2000);
        }
    }
}

std::string SqlEditorFrame::BuildStatusJson(const StatusSnapshot& snapshot,
                                            const std::string& category,
                                            bool diff_only) const {
    auto escape = [](const std::string& input) {
        std::string out;
        out.reserve(input.size() + 8);
        for (char c : input) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"': out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += c; break;
            }
        }
        return out;
    };

    auto category_of = [](const std::string& key) {
        size_t delim = key.find_first_of(".:");
        if (delim == std::string::npos) {
            return std::string("General");
        }
        return key.substr(0, delim);
    };

    std::map<std::string, std::string> prev_map;
    if (diff_only) {
        for (const auto& entry : previous_status_.entries) {
            prev_map[entry.key] = entry.value;
        }
    }
    bool ignore_unchanged = status_diff_ignore_unchanged_check_
                                ? status_diff_ignore_unchanged_check_->GetValue()
                                : true;
    bool ignore_empty = status_diff_ignore_empty_check_
                            ? status_diff_ignore_empty_check_->GetValue()
                            : true;

    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"request_type\": \"" << escape(StatusRequestKindToString(snapshot.kind)) << "\",\n";
    if (diff_only) {
        oss << "  \"diff\": [\n";
        bool first = true;
        for (const auto& entry : snapshot.entries) {
            if (!category.empty() && category != "All" && category_of(entry.key) != category) {
                continue;
            }
            auto it = prev_map.find(entry.key);
            std::string old_value = it == prev_map.end() ? "" : it->second;
            if (ignore_empty && entry.value.empty() && old_value.empty()) {
                continue;
            }
            if (ignore_unchanged && it != prev_map.end() && old_value == entry.value) {
                continue;
            }
            if (!first) {
                oss << ",\n";
            }
            first = false;
            oss << "    {\"key\": \"" << escape(entry.key) << "\", \"old\": \""
                << escape(old_value) << "\", \"new\": \"" << escape(entry.value) << "\"}";
        }
        oss << "\n  ]\n";
    } else {
        oss << "  \"entries\": [\n";
        bool first = true;
        for (const auto& entry : snapshot.entries) {
            if (!category.empty() && category != "All" && category_of(entry.key) != category) {
                continue;
            }
            if (!first) {
                oss << ",\n";
            }
            first = false;
            oss << "    {\"key\": \"" << escape(entry.key) << "\", \"value\": \""
                << escape(entry.value) << "\"}";
        }
        oss << "\n  ]\n";
    }
    oss << "}\n";
    return oss.str();
}

std::string SqlEditorFrame::SelectedStatusCategory() const {
    if (!status_category_choice_) {
        return "All";
    }
    return status_category_choice_->GetStringSelection().ToStdString();
}

void SqlEditorFrame::UpdateStatusCategoryChoices(const StatusSnapshot& snapshot) {
    if (!status_category_choice_) {
        return;
    }
    std::string previous = status_category_preference_.empty()
                               ? status_category_choice_->GetStringSelection().ToStdString()
                               : status_category_preference_;
    status_category_choice_->Clear();
    status_category_choice_->Append("All");

    std::map<std::string, bool> seen;
    auto add_category = [&](const std::string& name) {
        if (name.empty() || seen[name]) {
            return;
        }
        seen[name] = true;
        status_category_choice_->Append(wxString::FromUTF8(name));
    };

    for (const auto& category : status_category_order_) {
        if (category == "Request") {
            continue;
        }
        add_category(category);
    }
    for (const auto& entry : snapshot.entries) {
        std::string category = "General";
        size_t delim = entry.key.find_first_of(".:");
        if (delim != std::string::npos) {
            category = entry.key.substr(0, delim);
        }
        add_category(category);
    }

    int restore = status_category_choice_->FindString(wxString::FromUTF8(previous));
    if (restore == wxNOT_FOUND) {
        status_category_choice_->SetSelection(0);
    } else {
        status_category_choice_->SetSelection(restore);
    }
}

void SqlEditorFrame::AddStatusHistory(const StatusSnapshot& snapshot) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char time_buf[32] = {0};
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
    std::string label = std::string(time_buf) + " | " + StatusRequestKindToString(snapshot.kind);
    status_history_.push_back({label, snapshot});
    if (status_history_.size() > status_history_limit_) {
        status_history_.erase(status_history_.begin(),
                              status_history_.begin() +
                              static_cast<long>(status_history_.size() - status_history_limit_));
    }
    RefreshStatusHistory();
    if (status_history_list_ && !status_history_.empty()) {
        status_history_list_->SetSelection(static_cast<int>(status_history_.size() - 1));
    }
}

void SqlEditorFrame::RefreshStatusHistory() {
    if (!status_history_list_) {
        return;
    }
    status_history_list_->Clear();
    for (const auto& entry : status_history_) {
        status_history_list_->Append(wxString::FromUTF8(entry.label));
    }
}

void SqlEditorFrame::ShowHistorySnapshot(size_t index) {
    if (index >= status_history_.size()) {
        return;
    }
    if (index > 0) {
        previous_status_ = status_history_[index - 1].snapshot;
    } else {
        previous_status_ = StatusSnapshot{};
    }
    last_status_ = status_history_[index].snapshot;
    has_status_ = true;
    DisplayStatusSnapshot(last_status_);
}

void SqlEditorFrame::PersistStatusPreferences() {
    if (!connections_ || !connection_choice_) {
        return;
    }
    auto* editable_connections = const_cast<std::vector<ConnectionProfile>*>(connections_);
    if (!editable_connections) {
        return;
    }
    int selection = connection_choice_->GetSelection();
    if (selection < 0 || static_cast<size_t>(selection) >= editable_connections->size()) {
        return;
    }
    ConnectionProfile& profile = (*editable_connections)[static_cast<size_t>(selection)];
    status_category_preference_ = SelectedStatusCategory();
    profile.statusCategoryFilter =
        (status_category_preference_ == "All") ? "" : status_category_preference_;
    profile.statusDiffEnabled = status_diff_check_ && status_diff_check_->GetValue();
    profile.statusDiffIgnoreUnchanged = status_diff_ignore_unchanged_check_
                                            ? status_diff_ignore_unchanged_check_->GetValue()
                                            : true;
    profile.statusDiffIgnoreEmpty = status_diff_ignore_empty_check_
                                        ? status_diff_ignore_empty_check_->GetValue()
                                        : true;
    if (status_poll_check_) {
        profile.statusAutoPollEnabled = status_poll_check_->GetValue();
    }
    if (status_poll_interval_ctrl_) {
        profile.statusPollIntervalMs = status_poll_interval_ctrl_->GetValue();
    }
    if (status_type_choice_) {
        switch (status_type_choice_->GetSelection()) {
            case 1: profile.statusDefaultKind = StatusRequestKind::ConnectionInfo; break;
            case 2: profile.statusDefaultKind = StatusRequestKind::DatabaseInfo; break;
            case 3: profile.statusDefaultKind = StatusRequestKind::Statistics; break;
            default: profile.statusDefaultKind = StatusRequestKind::ServerInfo; break;
        }
    }

    ConfigStore store;
    wxFileName config_root(wxStandardPaths::Get().GetUserConfigDir(), "");
    config_root.AppendDir("scratchrobin");
    if (!config_root.DirExists()) {
        config_root.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    wxFileName connection_path(config_root);
    connection_path.SetFullName("connections.toml");
    store.SaveConnections(connection_path.GetFullPath().ToStdString(), *editable_connections);
}

void SqlEditorFrame::UpdateDiffOptionControls() {
    bool enabled = status_diff_check_ && status_diff_check_->GetValue();
    if (status_diff_ignore_unchanged_check_) {
        status_diff_ignore_unchanged_check_->Enable(enabled);
    }
    if (status_diff_ignore_empty_check_) {
        status_diff_ignore_empty_check_->Enable(enabled);
    }
}

void SqlEditorFrame::ClearStatusCards() {
    if (!status_cards_panel_ || !status_cards_sizer_) {
        return;
    }
    status_cards_panel_->Freeze();
    status_cards_sizer_->Clear(true);
    status_cards_panel_->Layout();
    status_cards_panel_->FitInside();
    status_cards_panel_->Thaw();
}

void SqlEditorFrame::BuildStatusCards(const StatusSnapshot& snapshot) {
    if (!status_cards_panel_ || !status_cards_sizer_) {
        return;
    }
    status_cards_panel_->Freeze();
    status_cards_sizer_->Clear(true);

    auto category_of = [](const std::string& key) {
        size_t delim = key.find_first_of(".:");
        if (delim == std::string::npos) {
            return std::string("General");
        }
        return key.substr(0, delim);
    };

    bool diff_only = status_diff_check_ && status_diff_check_->GetValue();
    bool ignore_unchanged = status_diff_ignore_unchanged_check_
                                ? status_diff_ignore_unchanged_check_->GetValue()
                                : true;
    bool ignore_empty = status_diff_ignore_empty_check_
                            ? status_diff_ignore_empty_check_->GetValue()
                            : true;
    std::string filter_category = SelectedStatusCategory();
    std::map<std::string, std::string> prev_map;
    if (diff_only) {
        for (const auto& entry : previous_status_.entries) {
            prev_map[entry.key] = entry.value;
        }
    }

    std::map<std::string, std::vector<StatusEntry>> grouped;
    grouped["Request"].push_back({"Type", StatusRequestKindToString(snapshot.kind)});
    for (const auto& entry : snapshot.entries) {
        std::string category = category_of(entry.key);
        if (!filter_category.empty() && filter_category != "All" && filter_category != category) {
            continue;
        }
        std::string key = entry.key;
        size_t delim = key.find_first_of(".:");
        if (delim != std::string::npos) {
            key = key.substr(delim + 1);
        }
        if (diff_only) {
            auto it = prev_map.find(entry.key);
            std::string old_value = it == prev_map.end() ? "" : it->second;
            if (ignore_empty && entry.value.empty() && old_value.empty()) {
                continue;
            }
            if (ignore_unchanged && it != prev_map.end() && old_value == entry.value) {
                continue;
            }
            grouped["Changes"].push_back({key, old_value + " → " + entry.value});
        } else {
            grouped[category].push_back({key, entry.value});
        }
    }

    std::vector<std::string> ordered_categories;
    if (diff_only) {
        ordered_categories.push_back("Request");
        if (!grouped["Changes"].empty()) {
            ordered_categories.push_back("Changes");
        }
    } else {
        if (!status_category_order_.empty()) {
            ordered_categories = status_category_order_;
        }
        if (std::find(ordered_categories.begin(), ordered_categories.end(), "Request") ==
            ordered_categories.end()) {
            ordered_categories.insert(ordered_categories.begin(), "Request");
        }
        for (const auto& group : grouped) {
            if (std::find(ordered_categories.begin(), ordered_categories.end(), group.first) ==
                ordered_categories.end()) {
                ordered_categories.push_back(group.first);
            }
        }
    }

    for (const auto& category : ordered_categories) {
        auto it = grouped.find(category);
        if (it == grouped.end() || it->second.empty()) {
            continue;
        }
        const auto& group = *it;
        auto* box = new wxStaticBox(status_cards_panel_, wxID_ANY, wxString::FromUTF8(group.first));
        auto* boxSizer = new wxStaticBoxSizer(box, wxVERTICAL);
        auto* grid = new wxFlexGridSizer(2, 6, 12);
        grid->AddGrowableCol(1);
        for (const auto& entry : group.second) {
            grid->Add(new wxStaticText(box, wxID_ANY, wxString::FromUTF8(entry.key)), 0,
                      wxALIGN_CENTER_VERTICAL);
            grid->Add(new wxStaticText(box, wxID_ANY, wxString::FromUTF8(entry.value)), 0,
                      wxALIGN_CENTER_VERTICAL);
        }
        boxSizer->Add(grid, 1, wxEXPAND | wxALL, 8);
        status_cards_sizer_->Add(boxSizer, 0, wxEXPAND | wxALL, 6);
    }

    status_cards_panel_->Layout();
    status_cards_panel_->FitInside();
    status_cards_panel_->Thaw();
}

std::string SqlEditorFrame::FormatNotificationPayload(const NotificationEvent& ev) const {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char time_buf[32] = {0};
    std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", std::localtime(&now_time));

    std::string payload_text;
    bool printable = true;
    for (uint8_t ch : ev.payload) {
        if (ch < 9 || (ch > 13 && ch < 32)) {
            printable = false;
            break;
        }
    }
    if (printable) {
        payload_text.assign(ev.payload.begin(), ev.payload.end());
    } else {
        std::ostringstream oss;
        oss << "0x";
        for (uint8_t ch : ev.payload) {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(ch);
        }
        payload_text = oss.str();
    }

    std::string detail = "[" + std::string(time_buf) + "] ";
    detail += "[" + ev.channel + "] " + payload_text;
    if (ev.changeType != 0 || ev.rowId != 0) {
        detail += " (change=" + std::to_string(ev.changeType) +
                  " row=" + std::to_string(ev.rowId) + ")";
    }
    if (ev.processId != 0) {
        detail += " pid=" + std::to_string(ev.processId);
    }
    return detail;
}

bool SqlEditorFrame::ShouldDisplayNotification(const NotificationEvent& ev) const {
    if (!notification_filter_ctrl_) {
        return true;
    }
    std::string filter = notification_filter_ctrl_->GetValue().ToStdString();
    if (filter.empty()) {
        return true;
    }
    std::string payload(ev.payload.begin(), ev.payload.end());
    std::string haystack = ev.channel + " " + payload;
    auto to_lower = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    };
    return to_lower(haystack).find(to_lower(filter)) != std::string::npos;
}

void SqlEditorFrame::AddToHistory(const std::string& sql) {
    std::string trimmed = Trim(sql);
    if (trimmed.empty()) {
        return;
    }
    if (!statement_history_.empty() && statement_history_.back() == trimmed) {
        return;
    }
    statement_history_.push_back(trimmed);
    if (statement_history_.size() > history_max_items_) {
        statement_history_.erase(statement_history_.begin(),
                                 statement_history_.begin() +
                                 static_cast<long>(statement_history_.size() - history_max_items_));
    }
    UpdateHistoryControls();
}

void SqlEditorFrame::UpdateHistoryControls() {
    if (!history_choice_ || !history_load_button_) {
        return;
    }
    history_choice_->Clear();
    for (const auto& entry : statement_history_) {
        std::string summary = entry;
        size_t newline = summary.find('\n');
        if (newline != std::string::npos) {
            summary = summary.substr(0, newline);
        }
        if (summary.size() > 60) {
            summary = summary.substr(0, 57) + "...";
        }
        history_choice_->Append(summary);
    }
    if (!statement_history_.empty()) {
        history_choice_->SetSelection(static_cast<int>(statement_history_.size() - 1));
    }
    bool enable = !statement_history_.empty() && !query_running_;
    history_choice_->Enable(enable);
    history_load_button_->Enable(enable);
}

std::string SqlEditorFrame::GetExplainTargetSql() const {
    if (!editor_) {
        return std::string();
    }
    std::string selection = editor_->GetStringSelection().ToStdString();
    std::string source = selection.empty() ? editor_->GetValue().ToStdString() : selection;
    source = Trim(source);
    if (source.empty()) {
        return std::string();
    }
    StatementSplitter splitter;
    auto split = splitter.Split(source);
    if (!split.statements.empty()) {
        return split.statements.front();
    }
    return source;
}

void SqlEditorFrame::StartSpecialQuery(const std::string& sql, const std::string& mode) {
    if (!connection_manager_) {
        return;
    }
    if (query_running_) {
        wxMessageBox("A query is already running.", "Execution Error", wxOK | wxICON_WARNING, this);
        return;
    }
    const ConnectionProfile* profile = GetSelectedProfile();
    if (!profile) {
        wxMessageBox("Select a connection profile first.", "Execution Error", wxOK | wxICON_WARNING, this);
        return;
    }
    if (!EnsureConnected(*profile)) {
        wxMessageBox(connection_manager_->LastError(), "Connection Error", wxOK | wxICON_ERROR, this);
        return;
    }

    query_running_ = true;
    statement_start_time_ = std::chrono::steady_clock::now();
    ClearMessages();
    UpdateStatus(mode + " running...");
    UpdateSessionControls();
    UpdatePagingControls();
    UpdateExportControls();

    active_query_job_ = connection_manager_->ExecuteQueryAsync(
        sql,
        [this, mode](bool ok, QueryResult result, const std::string& error) {
            auto result_ptr = std::make_shared<QueryResult>(std::move(result));
            auto error_ptr = std::make_shared<std::string>(error);
            CallAfter([this, ok, result_ptr, error_ptr, mode]() {
                HandleSpecialResult(ok, *result_ptr, *error_ptr, mode);
            });
        });
}

void SqlEditorFrame::HandleSpecialResult(bool ok, const QueryResult& result,
                                         const std::string& error, const std::string& mode) {
    double elapsed_ms = 0.0;
    if (statement_start_time_.time_since_epoch().count() != 0) {
        elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - statement_start_time_).count();
    }

    if (!ok) {
        wxMessageBox(error, mode + " Error", wxOK | wxICON_ERROR, this);
        AppendMessages(result, error);
        if (result_notebook_) {
            result_notebook_->SetSelection(messages_page_index_);
        }
        UpdateStatus(mode + " failed");
        query_running_ = false;
        UpdateSessionControls();
        UpdatePagingControls();
        UpdateExportControls();
        return;
    }

    std::string text = ResultToText(result);
    if (mode == "EXPLAIN") {
        if (plan_view_) {
            plan_view_->SetValue(text);
        }
        if (result_notebook_) {
            result_notebook_->SetSelection(plan_page_index_);
        }
    } else if (mode == "SBLR") {
        if (sblr_view_) {
            sblr_view_->SetValue(text);
        }
        if (result_notebook_) {
            result_notebook_->SetSelection(sblr_page_index_);
        }
    }

    AppendMessages(result, std::string());
    std::string status = mode + " ready";
    if (!result.rows.empty()) {
        status += " | Rows: " + std::to_string(result.rows.size());
    }
    if (elapsed_ms > 0.0) {
        status += " | Time: " + std::to_string(static_cast<int64_t>(elapsed_ms)) + " ms";
    }
    UpdateStatus(status);
    query_running_ = false;
    UpdateSessionControls();
    UpdatePagingControls();
    UpdateExportControls();
}

std::string SqlEditorFrame::ResultToText(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No rows returned.";
    }

    std::ostringstream out;
    if (!result.columns.empty()) {
        for (size_t i = 0; i < result.columns.size(); ++i) {
            if (i > 0) {
                out << " | ";
            }
            out << result.columns[i].name;
        }
        out << "\n";
    }

    FormatOptions format_options;
    for (const auto& row : result.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) {
                out << " | ";
            }
            std::string type = (i < result.columns.size()) ? result.columns[i].type : std::string();
            out << FormatValueForDisplay(row[i], type, format_options);
        }
        out << "\n";
    }
    return out.str();
}

void SqlEditorFrame::UpdateResultChoiceSelection(int index) {
    if (index < 0 || static_cast<size_t>(index) >= result_sets_.size()) {
        return;
    }
    active_result_index_ = index;
    UpdateResultControls();
    if (!query_running_) {
        ShowResultAtIndex(index);
    }
}

void SqlEditorFrame::ShowResultAtIndex(int index) {
    if (index < 0 || static_cast<size_t>(index) >= result_sets_.size()) {
        return;
    }
    active_result_index_ = index;
    if (result_choice_) {
        result_choice_->SetSelection(index);
    }
    const auto& entry = result_sets_[static_cast<size_t>(index)];
    PopulateGrid(entry.result, false);
    ClearMessages();
    AppendMessages(entry.result, entry.ok ? std::string() : entry.error);
    if (!entry.ok && result_notebook_) {
        result_notebook_->SetSelection(messages_page_index_);
    } else if (result_notebook_) {
        result_notebook_->SetSelection(results_page_index_);
    }

    std::string status = "Result " + std::to_string(index + 1);
    if (!entry.result.commandTag.empty()) {
        status += " | " + entry.result.commandTag;
    }
    if (!entry.result.rows.empty()) {
        status += " | Rows: " + std::to_string(entry.result.rows.size());
    }
    if (entry.result.rowsAffected > 0) {
        status += " | Affected: " + std::to_string(entry.result.rowsAffected);
    }
    if (entry.elapsedMs > 0.0) {
        status += " | Time: " + std::to_string(static_cast<int64_t>(entry.elapsedMs)) + " ms";
    }
    UpdateStatus(status);
}

void SqlEditorFrame::UpdateSessionControls() {
    bool has_connections = connections_ && !connections_->empty();
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    bool auto_commit = connection_manager_ ? connection_manager_->IsAutoCommit() : true;
    bool in_transaction = connection_manager_ ? connection_manager_->IsInTransaction() : false;
    auto caps = connection_manager_ ? connection_manager_->Capabilities()
                                    : BackendCapabilities{};

    if (connection_choice_) {
        connection_choice_->Enable(has_connections && !query_running_);
    }
    if (connect_button_) {
        connect_button_->Enable(has_connections && !connected && !query_running_);
    }
    if (disconnect_button_) {
        disconnect_button_->Enable(connected && !query_running_);
    }
    if (auto_commit_check_) {
        auto_commit_check_->SetValue(auto_commit);
        auto_commit_check_->Enable(connected && caps.supportsTransactions && !query_running_);
    }
    if (begin_button_) {
        begin_button_->Enable(connected && caps.supportsTransactions && auto_commit && !in_transaction && !query_running_);
    }
    if (commit_button_) {
        commit_button_->Enable(connected && caps.supportsTransactions && in_transaction && !query_running_);
    }
    if (rollback_button_) {
        rollback_button_->Enable(connected && caps.supportsTransactions && in_transaction && !query_running_);
    }
    if (savepoint_button_) {
        savepoint_button_->Enable(connected && caps.supportsTransactions && in_transaction && !query_running_);
    }
    if (savepoint_choice_) {
        savepoint_choice_->Enable(connected && caps.supportsTransactions && in_transaction && !savepoints_.empty() && !query_running_);
    }
    if (isolation_choice_) {
        isolation_choice_->Enable(connected && caps.supportsTransactions && !in_transaction && !query_running_);
    }
    if (run_button_) {
        run_button_->Enable(connected && !query_running_);
    }
    if (copy_button_) {
        bool copy_enabled = connected && !query_running_ &&
                            (caps.supportsCopyIn || caps.supportsCopyOut || caps.supportsCopyBoth);
        copy_button_->Enable(copy_enabled);
    }
    if (stream_window_ctrl_) {
        stream_window_ctrl_->Enable(!query_running_);
    }
    if (stream_chunk_ctrl_) {
        stream_chunk_ctrl_->Enable(!query_running_);
    }
    if (stream_apply_button_) {
        stream_apply_button_->Enable(!query_running_);
    }
    if (prepared_edit_button_) {
        prepared_edit_button_->Enable(connected && caps.supportsPreparedStatements && !query_running_);
    }
    if (prepared_prepare_button_) {
        prepared_prepare_button_->Enable(connected && caps.supportsPreparedStatements && !query_running_);
    }
    if (prepared_execute_button_) {
        prepared_execute_button_->Enable(connected && caps.supportsPreparedStatements && !query_running_ &&
                                         active_prepared_ != nullptr);
    }
    if (prepared_status_label_) {
        prepared_status_label_->SetLabel(connected && caps.supportsPreparedStatements
                                             ? "Prepared statements available."
                                             : "Prepared statements not supported.");
    }
    if (notification_subscribe_button_) {
        notification_subscribe_button_->Enable(connected && caps.supportsNotifications && !query_running_);
    }
    if (notification_unsubscribe_button_) {
        notification_unsubscribe_button_->Enable(connected && caps.supportsNotifications && !query_running_);
    }
    if (notification_fetch_button_) {
        notification_fetch_button_->Enable(connected && caps.supportsNotifications && !query_running_);
    }
    if (notification_poll_check_) {
        notification_poll_check_->Enable(connected && caps.supportsNotifications);
    }
    if (notification_poll_interval_ctrl_) {
        notification_poll_interval_ctrl_->Enable(connected && caps.supportsNotifications &&
                                                 !notification_fetch_pending_);
    }
    if (notification_poll_check_ && !connected) {
        notification_poll_check_->SetValue(false);
        if (notification_timer_) {
            notification_timer_->Stop();
        }
    }
    if (status_type_choice_) {
        status_type_choice_->Enable(connected && caps.supportsStatus && !query_running_);
    }
    if (status_fetch_button_) {
        status_fetch_button_->Enable(connected && caps.supportsStatus && !query_running_);
    }
    if (status_clear_button_) {
        status_clear_button_->Enable(connected);
    }
    if (status_copy_button_) {
        status_copy_button_->Enable(has_status_);
    }
    if (status_save_button_) {
        status_save_button_->Enable(has_status_);
    }
    if (status_category_choice_) {
        status_category_choice_->Enable(connected && caps.supportsStatus);
    }
    if (status_diff_check_) {
        status_diff_check_->Enable(has_status_);
    }
    if (status_poll_check_) {
        status_poll_check_->Enable(connected && caps.supportsStatus);
    }
    if (status_poll_interval_ctrl_) {
        status_poll_interval_ctrl_->Enable(connected && caps.supportsStatus && !status_fetch_pending_);
    }
    if (status_poll_check_ && !connected) {
        status_poll_check_->SetValue(false);
        if (status_timer_) {
            status_timer_->Stop();
        }
    }
    if (cancel_button_) {
        cancel_button_->Enable(connected && query_running_ && caps.supportsCancel);
    }
    if (explain_button_) {
        explain_button_->Enable(connected && caps.supportsExplain && !query_running_);
    }
    if (sblr_button_) {
        sblr_button_->Enable(connected && caps.supportsSblr && !query_running_);
    }

    UpdateTransactionUI();
    UpdateExportControls();
    UpdateResultControls();
    UpdateHistoryControls();
}

void SqlEditorFrame::UpdatePagingControls() {
    auto caps = connection_manager_ ? connection_manager_->Capabilities()
                                    : BackendCapabilities{};
    bool paging_supported = caps.supportsPaging;
    bool paging_enabled = paging_check_ && paging_check_->GetValue() && paging_supported;
    bool can_page = paging_active_ && paging_enabled && !query_running_;
    bool stream_enabled = stream_check_ && stream_check_->GetValue();

    if (stream_status_label_) {
        if (!paging_supported) {
            stream_status_label_->SetLabel("Streaming: unavailable");
        } else if (stream_enabled) {
            stream_status_label_->SetLabel("Streaming: on");
        } else {
            stream_status_label_->SetLabel("Streaming: off");
        }
    }
    if (prev_page_button_) {
        prev_page_button_->Enable(can_page && !stream_enabled && current_page_ > 0);
    }
    if (next_page_button_) {
        next_page_button_->Enable(can_page);
    }
    if (page_size_ctrl_) {
        page_size_ctrl_->Enable(paging_enabled && !query_running_);
    }
    if (row_limit_ctrl_) {
        row_limit_ctrl_->Enable(!paging_enabled && !query_running_);
    }
    if (paging_check_) {
        paging_check_->Enable(paging_supported && !query_running_);
    }
    if (stream_check_) {
        stream_check_->Enable(paging_supported && !query_running_);
    }
    if (page_label_) {
        if (stream_enabled && paging_active_) {
            page_label_->SetLabel("Loaded pages: " + std::to_string(current_page_ + 1));
        } else {
            page_label_->SetLabel("Page " + std::to_string(current_page_ + 1));
        }
    }
}

} // namespace scratchrobin
