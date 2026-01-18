#include "sql_editor_frame.h"
#include "core/result_exporter.h"
#include "core/statement_splitter.h"
#include "core/value_formatter.h"
#include "result_grid_table.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>

namespace scratchrobin {

namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

} // namespace

SqlEditorFrame::SqlEditorFrame(WindowManager* windowManager,
                               ConnectionManager* connectionManager,
                               const std::vector<ConnectionProfile>* connections,
                               const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "SQL Editor", wxDefaultPosition, wxSize(900, 700)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    if (app_config_ && app_config_->historyMaxItems > 0) {
        history_max_items_ = static_cast<size_t>(app_config_->historyMaxItems);
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

    gridSizer->Add(result_notebook_, 1, wxEXPAND | wxALL, 4);
    gridPanel->SetSizer(gridSizer);

    splitter->SplitHorizontally(editorPanel, gridPanel, 350);
    rootSizer->Add(splitter, 1, wxEXPAND);

    SetSizer(rootSizer);

    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }

    Bind(wxEVT_CLOSE_WINDOW, &SqlEditorFrame::OnClose, this);
    run_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExecuteQuery, this);
    cancel_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnCancelQuery, this);
    connect_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnConnect, this);
    disconnect_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnDisconnect, this);
    connection_choice_->Bind(wxEVT_CHOICE, &SqlEditorFrame::OnConnectionChanged, this);
    auto_commit_check_->Bind(wxEVT_CHECKBOX, &SqlEditorFrame::OnToggleAutoCommit, this);
    begin_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnBegin, this);
    commit_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnCommit, this);
    rollback_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnRollback, this);
    prev_page_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnPrevPage, this);
    next_page_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnNextPage, this);
    page_size_ctrl_->Bind(wxEVT_SPINCTRL, &SqlEditorFrame::OnPageSizeChanged, this);
    paging_check_->Bind(wxEVT_CHECKBOX, &SqlEditorFrame::OnTogglePaging, this);
    stream_check_->Bind(wxEVT_CHECKBOX, &SqlEditorFrame::OnToggleStream, this);
    export_csv_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExportCsv, this);
    export_json_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExportJson, this);
    result_choice_->Bind(wxEVT_CHOICE, &SqlEditorFrame::OnResultSelection, this);
    history_load_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnHistoryLoad, this);
    explain_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExplain, this);
    sblr_button_->Bind(wxEVT_BUTTON, &SqlEditorFrame::OnSblr, this);

    PopulateConnections();
    UpdateSessionControls();
    UpdatePagingControls();
    UpdateResultControls();
    UpdateHistoryControls();
    UpdateExportControls();
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
    }
    active_query_job_.Cancel();
    UpdateStatus("Cancel requested");
    UpdateSessionControls();
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
    if (!connection_manager_->BeginTransaction()) {
        wxMessageBox(connection_manager_->LastError(), "Transaction Error", wxOK | wxICON_ERROR, this);
        return;
    }
    UpdateStatus("Transaction started");
    UpdateSessionControls();
}

void SqlEditorFrame::OnCommit(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    if (!connection_manager_->Commit()) {
        wxMessageBox(connection_manager_->LastError(), "Transaction Error", wxOK | wxICON_ERROR, this);
        return;
    }
    UpdateStatus("Committed");
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
    UpdateStatus("Rolled back");
    UpdateSessionControls();
}

void SqlEditorFrame::OnToggleAutoCommit(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->SetAutoCommit(auto_commit_check_ && auto_commit_check_->GetValue());
    UpdateSessionControls();
}

void SqlEditorFrame::OnTogglePaging(wxCommandEvent&) {
    UpdatePagingControls();
}

void SqlEditorFrame::OnToggleStream(wxCommandEvent&) {
    UpdatePagingControls();
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

void SqlEditorFrame::OnClose(wxCloseEvent&) {
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
    active_query_job_ = connection_manager_->ExecuteQueryAsync(
        sql,
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
    double elapsed_ms = 0.0;
    if (statement_start_time_.time_since_epoch().count() != 0) {
        elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - statement_start_time_).count();
    }
    last_statement_ms_ = elapsed_ms;
    ResultEntry* entry = nullptr;
    if (result_index >= 0 && static_cast<size_t>(result_index) < result_sets_.size()) {
        entry = &result_sets_[static_cast<size_t>(result_index)];
        entry->elapsedMs = elapsed_ms;
        entry->ok = ok;
        entry->error = error;
    }

    if (!ok) {
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
        if (elapsed_ms > 0.0) {
            status += " | Time: " + std::to_string(static_cast<int64_t>(elapsed_ms)) + " ms";
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
        pending_last_result_.stats.elapsedMs = elapsed_ms;
        pending_last_result_.stats.rowsReturned = static_cast<int64_t>(result.rows.size());
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
    UpdateStatus(status);
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

std::string SqlEditorFrame::BuildPagedQuery(const std::string& statement,
                                            int64_t offset,
                                            int64_t limit) const {
    std::string trimmed = Trim(statement);
    return "SELECT * FROM (" + trimmed + ") AS scratchrobin_q LIMIT " +
           std::to_string(limit) + " OFFSET " + std::to_string(offset);
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
        begin_button_->Enable(connected && caps.supportsTransactions && auto_commit && !query_running_);
    }
    if (commit_button_) {
        commit_button_->Enable(connected && caps.supportsTransactions && !auto_commit && !query_running_);
    }
    if (rollback_button_) {
        rollback_button_->Enable(connected && caps.supportsTransactions && !auto_commit && !query_running_);
    }
    if (run_button_) {
        run_button_->Enable(connected && !query_running_);
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

    if (prev_page_button_) {
        prev_page_button_->Enable(can_page && !stream_enabled && current_page_ > 0);
    }
    if (next_page_button_) {
        next_page_button_->Enable(can_page);
    }
    if (page_size_ctrl_) {
        page_size_ctrl_->Enable(paging_enabled && !query_running_);
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
