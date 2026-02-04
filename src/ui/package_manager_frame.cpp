/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "package_manager_frame.h"

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
#include "table_designer_frame.h"
#include "users_roles_frame.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/choicdlg.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/treectrl.h>

namespace scratchrobin {
namespace {

constexpr int kMenuConnect = wxID_HIGHEST + 150;
constexpr int kMenuDisconnect = wxID_HIGHEST + 151;
constexpr int kMenuRefresh = wxID_HIGHEST + 152;
constexpr int kMenuCreate = wxID_HIGHEST + 153;
constexpr int kMenuEdit = wxID_HIGHEST + 154;
constexpr int kMenuDrop = wxID_HIGHEST + 155;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 156;

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

bool IsSimpleIdentifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    if (!(std::isalpha(static_cast<unsigned char>(value[0])) || value[0] == '_')) {
        return false;
    }
    for (char ch : value) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
            return false;
        }
    }
    return true;
}

bool IsQuotedIdentifier(const std::string& value) {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

std::string QuoteIdentifier(const std::string& value) {
    if (IsSimpleIdentifier(value) || IsQuotedIdentifier(value)) {
        return value;
    }
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') {
            out.push_back('"');
        }
        out.push_back(ch);
    }
    out.push_back('"');
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

wxBEGIN_EVENT_TABLE(PackageManagerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, PackageManagerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, PackageManagerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, PackageManagerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, PackageManagerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, PackageManagerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, PackageManagerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, PackageManagerFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, PackageManagerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, PackageManagerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, PackageManagerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, PackageManagerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, PackageManagerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, PackageManagerFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, PackageManagerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, PackageManagerFrame::OnDrop)
    EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, PackageManagerFrame::OnNotebookPageChanged)
    EVT_CLOSE(PackageManagerFrame::OnClose)
wxEND_EVENT_TABLE()

PackageManagerFrame::PackageManagerFrame(WindowManager* windowManager,
                                         ConnectionManager* connectionManager,
                                         const std::vector<ConnectionProfile>* connections,
                                         const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Package Manager", wxDefaultPosition, wxSize(1100, 750)),
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

void PackageManagerFrame::BuildMenu() {
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

void PackageManagerFrame::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Connection selector at top
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

    // Toolbar: Create/Edit/Drop
    auto* actionPanel = new wxPanel(this, wxID_ANY);
    auto* actionSizer = new wxBoxSizer(wxHORIZONTAL);
    create_button_ = new wxButton(actionPanel, kMenuCreate, "Create");
    edit_button_ = new wxButton(actionPanel, kMenuEdit, "Edit");
    drop_button_ = new wxButton(actionPanel, kMenuDrop, "Drop");
    actionSizer->Add(create_button_, 0, wxRIGHT, 6);
    actionSizer->Add(edit_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Main splitter: packages grid on left, notebook on right
    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    // Left panel: Packages grid
    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(new wxStaticText(listPanel, wxID_ANY, "Packages"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    packages_grid_ = new wxGrid(listPanel, wxID_ANY);
    packages_grid_->EnableEditing(false);
    packages_grid_->SetRowLabelSize(40);
    packages_table_ = new ResultGridTable();
    packages_grid_->SetTable(packages_table_, true);
    listSizer->Add(packages_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    // Right panel: Notebook with tabs
    auto* detailPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailSizer = new wxBoxSizer(wxVERTICAL);
    
    notebook_ = new wxNotebook(detailPanel, wxID_ANY);

    // Specification tab
    auto* specTab = new wxPanel(notebook_, wxID_ANY);
    auto* specSizer = new wxBoxSizer(wxVERTICAL);
    spec_text_ = new wxTextCtrl(specTab, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    specSizer->Add(spec_text_, 1, wxEXPAND | wxALL, 8);
    specTab->SetSizer(specSizer);
    notebook_->AddPage(specTab, "Specification");

    // Body tab
    auto* bodyTab = new wxPanel(notebook_, wxID_ANY);
    auto* bodySizer = new wxBoxSizer(wxVERTICAL);
    body_text_ = new wxTextCtrl(bodyTab, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    bodySizer->Add(body_text_, 1, wxEXPAND | wxALL, 8);
    bodyTab->SetSizer(bodySizer);
    notebook_->AddPage(bodyTab, "Body");

    // Contents tab: Tree/list of procedures/functions
    auto* contentsTab = new wxPanel(notebook_, wxID_ANY);
    auto* contentsSizer = new wxBoxSizer(wxVERTICAL);
    contentsSizer->Add(new wxStaticText(contentsTab, wxID_ANY, "Procedures and Functions"), 0,
                       wxLEFT | wxRIGHT | wxTOP, 8);
    contents_tree_ = new wxTreeCtrl(contentsTab, wxID_ANY);
    contentsSizer->Add(contents_tree_, 1, wxEXPAND | wxALL, 8);
    contentsTab->SetSizer(contentsSizer);
    notebook_->AddPage(contentsTab, "Contents");

    // Dependencies tab
    auto* depsTab = new wxPanel(notebook_, wxID_ANY);
    auto* depsSizer = new wxBoxSizer(wxVERTICAL);
    depsSizer->Add(new wxStaticText(depsTab, wxID_ANY, "Package Dependencies"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    dependencies_grid_ = new wxGrid(depsTab, wxID_ANY);
    dependencies_grid_->EnableEditing(false);
    dependencies_grid_->SetRowLabelSize(40);
    dependencies_table_ = new ResultGridTable();
    dependencies_grid_->SetTable(dependencies_table_, true);
    depsSizer->Add(dependencies_grid_, 1, wxEXPAND | wxALL, 8);
    depsTab->SetSizer(depsSizer);
    notebook_->AddPage(depsTab, "Dependencies");

    detailSizer->Add(notebook_, 1, wxEXPAND);
    detailPanel->SetSizer(detailSizer);

    splitter->SplitVertically(listPanel, detailPanel, 450);
    rootSizer->Add(splitter, 1, wxEXPAND);

    // Status panel at bottom
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

    packages_grid_->Bind(wxEVT_GRID_SELECT_CELL, &PackageManagerFrame::OnPackageSelected, this);
}

void PackageManagerFrame::PopulateConnections() {
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

const ConnectionProfile* PackageManagerFrame::GetSelectedProfile() const {
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

bool PackageManagerFrame::EnsureConnected(const ConnectionProfile& profile) {
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

bool PackageManagerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    return NormalizeBackendName(profile.backend) == "native";
}

void PackageManagerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_package = !selected_package_name_.empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (create_button_) create_button_->Enable(connected && native && !busy);
    if (edit_button_) edit_button_->Enable(connected && native && has_package && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_package && !busy);
}

void PackageManagerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void PackageManagerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void PackageManagerFrame::RefreshPackages() {
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
        SetMessage("Packages are available only for ScratchBird connections.");
        return;
    }

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading packages...");
    
    std::string sql = 
        "SELECT package_name, schema_name, is_valid, created, last_modified, "
        "specification_length, body_length "
        "FROM sb_catalog.sb_packages "
        "ORDER BY schema_name, package_name";
    
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                packages_result_ = result;
                if (packages_table_) {
                    packages_table_->Reset(packages_result_.columns, packages_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load packages." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Packages updated");
                    // Clear package details
                    selected_package_name_.clear();
                    selected_schema_name_.clear();
                    if (spec_text_) spec_text_->Clear();
                    if (body_text_) body_text_->Clear();
                    if (contents_tree_) contents_tree_->DeleteAllItems();
                    if (dependencies_table_) dependencies_table_->Clear();
                }
                UpdateControls();
            });
        });
}

void PackageManagerFrame::RefreshPackageSpec() {
    if (!connection_manager_ || selected_package_name_.empty()) {
        return;
    }
    
    std::string sql = "SHOW PACKAGE SPECIFICATION " + 
                      QuoteIdentifier(selected_package_name_);
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok && spec_text_) {
                    if (!result.rows.empty() && !result.rows[0].empty()) {
                        spec_text_->SetValue(result.rows[0][0].text);
                    } else {
                        spec_text_->Clear();
                    }
                } else if (!error.empty()) {
                    if (spec_text_) spec_text_->SetValue("Error loading specification: " + error);
                }
                UpdateControls();
            });
        });
}

void PackageManagerFrame::RefreshPackageBody() {
    if (!connection_manager_ || selected_package_name_.empty()) {
        return;
    }
    
    std::string sql = "SHOW PACKAGE BODY " + 
                      QuoteIdentifier(selected_package_name_);
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok && body_text_) {
                    if (!result.rows.empty() && !result.rows[0].empty()) {
                        body_text_->SetValue(result.rows[0][0].text);
                    } else {
                        body_text_->Clear();
                    }
                } else if (!error.empty()) {
                    if (body_text_) body_text_->SetValue("Error loading body: " + error);
                }
                UpdateControls();
            });
        });
}

void PackageManagerFrame::RefreshPackageContents() {
    if (!connection_manager_ || selected_package_name_.empty() || !contents_tree_) {
        return;
    }
    
    std::string sql = 
        "SELECT routine_name, routine_type, is_procedure, is_function, "
        "return_type, parameter_count "
        "FROM sb_catalog.sb_routines "
        "WHERE package_name = '" + EscapeSqlLiteral(selected_package_name_) + "' "
        "ORDER BY routine_type, routine_name";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                contents_result_ = result;
                
                contents_tree_->DeleteAllItems();
                
                if (ok) {
                    wxTreeItemId rootId = contents_tree_->AddRoot(
                        wxString::FromUTF8(selected_package_name_));
                    wxTreeItemId proceduresId = contents_tree_->AppendItem(rootId, "Procedures");
                    wxTreeItemId functionsId = contents_tree_->AppendItem(rootId, "Functions");
                    
                    for (const auto& row : result.rows) {
                        if (row.size() >= 2) {
                            std::string routine_name = row[0].isNull ? "" : row[0].text;
                            std::string routine_type = row[1].isNull ? "" : row[1].text;
                            
                            wxString label = wxString::FromUTF8(routine_name);
                            
                            // Add parameter count if available
                            if (row.size() >= 6 && !row[5].isNull) {
                                label += " (" + wxString::FromUTF8(row[5].text) + " params)";
                            }
                            
                            if (ToLowerCopy(routine_type) == "procedure") {
                                contents_tree_->AppendItem(proceduresId, label);
                            } else {
                                // For functions, add return type if available
                                if (row.size() >= 5 && !row[4].isNull && !row[4].text.empty()) {
                                    label += " -> " + wxString::FromUTF8(row[4].text);
                                }
                                contents_tree_->AppendItem(functionsId, label);
                            }
                        }
                    }
                    
                    contents_tree_->Expand(rootId);
                    contents_tree_->Expand(proceduresId);
                    contents_tree_->Expand(functionsId);
                } else if (!error.empty()) {
                    contents_tree_->AddRoot("Error loading contents");
                }
                UpdateControls();
            });
        });
}

void PackageManagerFrame::RefreshPackageDependencies() {
    if (!connection_manager_ || selected_package_name_.empty()) {
        return;
    }
    
    std::string sql = 
        "SELECT object_name, object_type, dependency_type "
        "FROM sb_catalog.sb_dependencies "
        "WHERE package_name = '" + EscapeSqlLiteral(selected_package_name_) + "' "
        "ORDER BY dependency_type, object_type, object_name";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                dependencies_result_ = result;
                if (dependencies_table_) {
                    dependencies_table_->Reset(dependencies_result_.columns, 
                                               dependencies_result_.rows);
                }
                if (!ok && !error.empty()) {
                    // Catalog table may not exist - show friendly message
                    if (dependencies_table_) {
                        dependencies_table_->Clear();
                    }
                }
                UpdateControls();
            });
        });
}

std::string PackageManagerFrame::GetSelectedPackageName() const {
    if (!packages_grid_ || packages_result_.rows.empty()) {
        return std::string();
    }
    int row = packages_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= packages_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(packages_result_, row, 
                                     {"package_name", "package", "package name"});
    if (!value.empty()) {
        return value;
    }
    if (!packages_result_.rows[row].empty()) {
        return packages_result_.rows[row][0].text;
    }
    return std::string();
}

std::string PackageManagerFrame::GetSelectedSchemaName() const {
    if (!packages_grid_ || packages_result_.rows.empty()) {
        return std::string();
    }
    int row = packages_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= packages_result_.rows.size()) {
        return std::string();
    }
    return ExtractValue(packages_result_, row, {"schema_name", "schema", "schema name"});
}

int PackageManagerFrame::FindColumnIndex(const QueryResult& result,
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

std::string PackageManagerFrame::ExtractValue(const QueryResult& result,
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

std::string PackageManagerFrame::FormatDetails(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No details returned.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

void PackageManagerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
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
                RefreshPackages();
            });
        });
}

void PackageManagerFrame::OnConnect(wxCommandEvent&) {
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
    RefreshPackages();
}

void PackageManagerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void PackageManagerFrame::OnRefresh(wxCommandEvent&) {
    RefreshPackages();
}

void PackageManagerFrame::OnPackageSelected(wxGridEvent& event) {
    selected_package_name_ = GetSelectedPackageName();
    selected_schema_name_ = GetSelectedSchemaName();
    
    if (!selected_package_name_.empty()) {
        // Refresh all tabs based on current selection
        RefreshPackageSpec();
        RefreshPackageBody();
        RefreshPackageContents();
        RefreshPackageDependencies();
    }
    UpdateControls();
    event.Skip();
}

void PackageManagerFrame::OnNotebookPageChanged(wxBookCtrlEvent& event) {
    // Tab content is loaded on demand or when package selection changes
    event.Skip();
}

void PackageManagerFrame::OnCreate(wxCommandEvent&) {
    // Open SQL editor with CREATE PACKAGE template
    std::string sql = "CREATE PACKAGE " + QuoteIdentifier("NEW_PACKAGE") + " AS\n"
                      "  -- Package specification\n"
                      "  -- Add procedures and functions here\n"
                      "END " + QuoteIdentifier("NEW_PACKAGE") + ";";
    
    if (window_manager_) {
        auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                          app_config_, nullptr);
        editor->Show(true);
        // TODO: Set SQL text in the editor
    }
}

void PackageManagerFrame::OnEdit(wxCommandEvent&) {
    if (selected_package_name_.empty()) {
        return;
    }
    // Open SQL editor with ALTER PACKAGE template
    std::string sql = "ALTER PACKAGE " + QuoteIdentifier(selected_package_name_) + " AS\n"
                      "  -- Modified package specification\n"
                      "END " + QuoteIdentifier(selected_package_name_) + ";";
    
    if (window_manager_) {
        auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                          app_config_, nullptr);
        editor->Show(true);
        // TODO: Set SQL text in the editor
    }
}

void PackageManagerFrame::OnDrop(wxCommandEvent&) {
    if (selected_package_name_.empty()) {
        return;
    }
    
    wxArrayString choices;
    choices.Add("Drop (default)");
    choices.Add("Drop (cascade)");
    wxSingleChoiceDialog dialog(this, "Drop package option", "Drop Package", choices);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    std::string sql = "DROP PACKAGE " + QuoteIdentifier(selected_package_name_);
    if (dialog.GetSelection() == 1) {
        sql += " CASCADE";
    }
    sql += ";";
    RunCommand(sql, "Package dropped");
}

void PackageManagerFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void PackageManagerFrame::OnNewDiagram(wxCommandEvent&) {
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

void PackageManagerFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                        app_config_);
    monitor->Show(true);
}

void PackageManagerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void PackageManagerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void PackageManagerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void PackageManagerFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    domains->Show(true);
}

void PackageManagerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void PackageManagerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void PackageManagerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
