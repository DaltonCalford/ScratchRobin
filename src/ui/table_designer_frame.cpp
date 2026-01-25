#include "table_designer_frame.h"

#include "diagram_frame.h"
#include "domain_manager_frame.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "result_grid_table.h"
#include "schema_manager_frame.h"
#include "sql_editor_frame.h"
#include "table_editor_dialog.h"
#include "users_roles_frame.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

constexpr int kMenuConnect = wxID_HIGHEST + 200;
constexpr int kMenuDisconnect = wxID_HIGHEST + 201;
constexpr int kMenuRefresh = wxID_HIGHEST + 202;
constexpr int kMenuCreate = wxID_HIGHEST + 203;
constexpr int kMenuEdit = wxID_HIGHEST + 204;
constexpr int kMenuDrop = wxID_HIGHEST + 205;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 206;

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

std::string EscapeSqlLiteral(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == '\'') {
            out.push_back('\'');
        }
        out.push_back(ch);
    }
    return out;
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

} // namespace

wxBEGIN_EVENT_TABLE(TableDesignerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, TableDesignerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, TableDesignerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, TableDesignerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, TableDesignerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, TableDesignerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, TableDesignerFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, TableDesignerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, TableDesignerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, TableDesignerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, TableDesignerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, TableDesignerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, TableDesignerFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, TableDesignerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, TableDesignerFrame::OnDrop)
    EVT_CLOSE(TableDesignerFrame::OnClose)
wxEND_EVENT_TABLE()

TableDesignerFrame::TableDesignerFrame(WindowManager* windowManager,
                                       ConnectionManager* connectionManager,
                                       const std::vector<ConnectionProfile>* connections,
                                       const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Tables", wxDefaultPosition, wxSize(1100, 720)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
    BuildMenu();
    BuildLayout();
    PopulateConnections();
    UpdateControls();

    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
}

void TableDesignerFrame::BuildMenu() {
    WindowChromeConfig chrome;
    if (app_config_) {
        chrome = app_config_->chrome.monitoring;
    }
    if (!chrome.showMenu) {
        return;
    }

    MenuBuildOptions options;
    options.includeConnections = chrome.replicateMenu;
    options.includeEdit = true;
    options.includeView = true;
    options.includeWindow = true;
    options.includeHelp = true;
    auto* menu_bar = BuildMenuBar(options, window_manager_, this);
    SetMenuBar(menu_bar);
}

void TableDesignerFrame::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* topPanel = new wxPanel(this, wxID_ANY);
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Connection:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connection_choice_ = new wxChoice(topPanel, kConnectionChoiceId);
    topSizer->Add(connection_choice_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connect_button_ = new wxButton(topPanel, kMenuConnect, "Connect");
    topSizer->Add(connect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    disconnect_button_ = new wxButton(topPanel, kMenuDisconnect, "Disconnect");
    topSizer->Add(disconnect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    refresh_button_ = new wxButton(topPanel, kMenuRefresh, "Refresh");
    topSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL);
    topPanel->SetSizer(topSizer);
    rootSizer->Add(topPanel, 0, wxEXPAND | wxALL, 8);

    auto* actionPanel = new wxPanel(this, wxID_ANY);
    auto* actionSizer = new wxBoxSizer(wxHORIZONTAL);
    create_button_ = new wxButton(actionPanel, kMenuCreate, "Create");
    edit_button_ = new wxButton(actionPanel, kMenuEdit, "Alter");
    drop_button_ = new wxButton(actionPanel, kMenuDrop, "Drop");
    actionSizer->Add(create_button_, 0, wxRIGHT, 6);
    actionSizer->Add(edit_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(new wxStaticText(listPanel, wxID_ANY, "Tables"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    tables_grid_ = new wxGrid(listPanel, wxID_ANY);
    tables_grid_->EnableEditing(false);
    tables_grid_->SetRowLabelSize(40);
    tables_table_ = new ResultGridTable();
    tables_grid_->SetTable(tables_table_, true);
    listSizer->Add(tables_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    auto* detailPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailSizer = new wxBoxSizer(wxVERTICAL);
    auto* notebook = new wxNotebook(detailPanel, wxID_ANY);

    auto* definitionTab = new wxPanel(notebook, wxID_ANY);
    auto* definitionSizer = new wxBoxSizer(wxVERTICAL);
    details_text_ = new wxTextCtrl(definitionTab, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    definitionSizer->Add(details_text_, 1, wxEXPAND | wxALL, 8);
    definitionTab->SetSizer(definitionSizer);

    auto* columnsTab = new wxPanel(notebook, wxID_ANY);
    auto* columnsSizer = new wxBoxSizer(wxVERTICAL);
    columns_grid_ = new wxGrid(columnsTab, wxID_ANY);
    columns_grid_->EnableEditing(false);
    columns_grid_->SetRowLabelSize(40);
    columns_table_ = new ResultGridTable();
    columns_grid_->SetTable(columns_table_, true);
    columnsSizer->Add(columns_grid_, 1, wxEXPAND | wxALL, 8);
    columnsTab->SetSizer(columnsSizer);

    notebook->AddPage(definitionTab, "Definition");
    notebook->AddPage(columnsTab, "Columns");
    detailSizer->Add(notebook, 1, wxEXPAND);
    detailPanel->SetSizer(detailSizer);

    splitter->SplitVertically(listPanel, detailPanel, 420);
    rootSizer->Add(splitter, 1, wxEXPAND);

    auto* statusPanel = new wxPanel(this, wxID_ANY);
    auto* statusSizer = new wxBoxSizer(wxVERTICAL);
    status_text_ = new wxStaticText(statusPanel, wxID_ANY, "Ready");
    statusSizer->Add(status_text_, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    message_text_ = new wxTextCtrl(statusPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    message_text_->SetMinSize(wxSize(-1, 70));
    statusSizer->Add(message_text_, 0, wxEXPAND | wxALL, 8);
    statusPanel->SetSizer(statusSizer);
    rootSizer->Add(statusPanel, 0, wxEXPAND);

    SetSizer(rootSizer);

    tables_grid_->Bind(wxEVT_GRID_SELECT_CELL, &TableDesignerFrame::OnTableSelected, this);
}

void TableDesignerFrame::PopulateConnections() {
    if (!connection_choice_) {
        return;
    }
    connection_choice_->Clear();
    active_profile_index_ = -1;
    if (!connections_ || connections_->empty()) {
        connection_choice_->Append("No connections configured");
        connection_choice_->SetSelection(0);
        connection_choice_->Enable(false);
        return;
    }
    connection_choice_->Enable(true);
    for (const auto& profile : *connections_) {
        connection_choice_->Append(ProfileLabel(profile));
    }
    connection_choice_->SetSelection(0);
}

const ConnectionProfile* TableDesignerFrame::GetSelectedProfile() const {
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

bool TableDesignerFrame::EnsureConnected(const ConnectionProfile& profile) {
    if (!connection_manager_) {
        return false;
    }
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
    return true;
}

bool TableDesignerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    return NormalizeBackendName(profile.backend) == "native";
}

void TableDesignerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_table = !selected_table_.empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (create_button_) create_button_->Enable(connected && native && !busy);
    if (edit_button_) edit_button_->Enable(connected && native && has_table && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_table && !busy);
}

void TableDesignerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void TableDesignerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void TableDesignerFrame::RefreshTables() {
    if (!connection_manager_) {
        return;
    }
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        SetMessage("Select a connection profile first.");
        return;
    }
    if (!EnsureConnected(*profile)) {
        SetMessage(connection_manager_ ? connection_manager_->LastError() : "Connection failed.");
        return;
    }
    if (!IsNativeProfile(*profile)) {
        SetMessage("Tables are available only for ScratchBird connections.");
        return;
    }

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading tables...");
    connection_manager_->ExecuteQueryAsync(
        "SELECT name, schema_name FROM sb_catalog.sb_tables "
        "WHERE name NOT LIKE 'sb_%' ORDER BY schema_name, name",
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                tables_result_ = result;
                if (tables_table_) {
                    tables_table_->Reset(tables_result_.columns, tables_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load tables." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Tables updated");
                }
                UpdateControls();
            });
        });
}

void TableDesignerFrame::RefreshTableDetails(const std::string& table_name) {
    if (!connection_manager_ || table_name.empty()) {
        return;
    }
    std::string sql = "SHOW CREATE TABLE " + table_name;
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                table_details_result_ = result;
                if (ok) {
                    if (details_text_) {
                        details_text_->SetValue(FormatDetails(table_details_result_));
                    }
                } else if (!error.empty()) {
                    SetMessage(error);
                }
                UpdateControls();
            });
        });
}

void TableDesignerFrame::RefreshTableColumns(const std::string& table_name) {
    if (!connection_manager_ || table_name.empty()) {
        return;
    }
    std::string sql = "SHOW COLUMNS FROM " + table_name;
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                columns_result_ = result;
                if (columns_table_) {
                    columns_table_->Reset(columns_result_.columns, columns_result_.rows);
                }
                if (!ok && !error.empty()) {
                    SetMessage(error);
                }
                UpdateControls();
            });
        });
}

void TableDesignerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
    if (!connection_manager_) {
        return;
    }
    pending_queries_++;
    UpdateControls();
    UpdateStatus("Running...");
    connection_manager_->ExecuteQueryAsync(
        sql, [this, success_message](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, success_message]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok) {
                    UpdateStatus(success_message);
                    SetMessage("");
                } else {
                    UpdateStatus("Command failed");
                    SetMessage(error.empty() ? "Command failed." : error);
                }
                UpdateControls();
                RefreshTables();
                if (!selected_table_.empty()) {
                    RefreshTableDetails(selected_table_);
                    RefreshTableColumns(selected_table_);
                }
            });
        });
}

std::string TableDesignerFrame::GetSelectedTableName() const {
    if (!tables_grid_ || tables_result_.rows.empty()) {
        return std::string();
    }
    int row = tables_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= tables_result_.rows.size()) {
        return std::string();
    }
    std::string name = ExtractValue(tables_result_, row, {"name", "table_name"});
    std::string schema = ExtractValue(tables_result_, row, {"schema_name", "schema"});
    if (!name.empty() && !schema.empty() && schema != "root") {
        return schema + "." + name;
    }
    if (!name.empty()) {
        return name;
    }
    if (!tables_result_.rows[row].empty()) {
        return tables_result_.rows[row][0].text;
    }
    return std::string();
}

int TableDesignerFrame::FindColumnIndex(const QueryResult& result,
                                        const std::vector<std::string>& names) const {
    for (size_t i = 0; i < result.columns.size(); ++i) {
        std::string column = ToLowerCopy(result.columns[i].name);
        for (const auto& name : names) {
            if (column == name) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;
}

std::string TableDesignerFrame::ExtractValue(const QueryResult& result,
                                             int row,
                                             const std::vector<std::string>& names) const {
    int index = FindColumnIndex(result, names);
    if (index < 0 || row < 0 || static_cast<size_t>(row) >= result.rows.size()) {
        return std::string();
    }
    if (static_cast<size_t>(index) >= result.rows[row].size()) {
        return std::string();
    }
    return result.rows[row][static_cast<size_t>(index)].text;
}

std::string TableDesignerFrame::FormatDetails(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No table details returned.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

void TableDesignerFrame::OnConnect(wxCommandEvent&) {
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        SetMessage("Select a connection profile first.");
        return;
    }
    if (!EnsureConnected(*profile)) {
        SetMessage(connection_manager_ ? connection_manager_->LastError() : "Connection failed.");
        return;
    }
    UpdateStatus("Connected");
    UpdateControls();
    RefreshTables();
}

void TableDesignerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void TableDesignerFrame::OnRefresh(wxCommandEvent&) {
    RefreshTables();
}

void TableDesignerFrame::OnTableSelected(wxGridEvent& event) {
    selected_table_ = GetSelectedTableName();
    if (!selected_table_.empty()) {
        RefreshTableDetails(selected_table_);
        RefreshTableColumns(selected_table_);
    }
    UpdateControls();
    event.Skip();
}

void TableDesignerFrame::OnCreate(wxCommandEvent&) {
    TableEditorDialog dialog(this, TableEditorMode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Create table statement is empty.");
        return;
    }
    RunCommand(sql, "Table created");
}

void TableDesignerFrame::OnEdit(wxCommandEvent&) {
    if (selected_table_.empty()) {
        return;
    }
    TableEditorDialog dialog(this, TableEditorMode::Alter);
    dialog.SetTableName(selected_table_);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Alter table statement is empty.");
        return;
    }
    RunCommand(sql, "Table altered");
}

void TableDesignerFrame::OnDrop(wxCommandEvent&) {
    if (selected_table_.empty()) {
        return;
    }
    wxArrayString choices;
    choices.Add("Drop (default)");
    choices.Add("Drop (cascade)");
    choices.Add("Drop (restrict)");
    wxSingleChoiceDialog dialog(this, "Drop table option", "Drop Table", choices);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = "DROP TABLE " + selected_table_;
    if (dialog.GetSelection() == 1) {
        sql += " CASCADE";
    } else if (dialog.GetSelection() == 2) {
        sql += " RESTRICT";
    }
    sql += ";";
    RunCommand(sql, "Table dropped");
}

void TableDesignerFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void TableDesignerFrame::OnNewDiagram(wxCommandEvent&) {
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

void TableDesignerFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                        app_config_);
    monitor->Show(true);
}

void TableDesignerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void TableDesignerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void TableDesignerFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    domains->Show(true);
}

void TableDesignerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void TableDesignerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void TableDesignerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
