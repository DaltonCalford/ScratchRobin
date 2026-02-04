/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "reverse_engineer_wizard.h"

#include "core/connection_manager.h"
#include "ui/diagram_model.h"

#include <algorithm>
#include <cctype>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace scratchrobin {

namespace {

std::string NormalizeBackendName(const std::string& raw) {
    std::string value = raw;
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
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

// ============================================================================
// Schema Selection Page
// ============================================================================

SchemaSelectionPage::SchemaSelectionPage(wxWizard* parent,
                                         ConnectionManager* connection_manager,
                                         const std::vector<ConnectionProfile>* connections)
    : wxWizardPage(parent),
      connection_manager_(connection_manager),
      connections_(connections) {
    BuildLayout();
}

void SchemaSelectionPage::BuildLayout() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(this, wxID_ANY,
        "Select a connection and schema to import from:"),
        0, wxALL, 10);
    
    // Connection selection
    auto* connSizer = new wxBoxSizer(wxHORIZONTAL);
    connSizer->Add(new wxStaticText(this, wxID_ANY, "Connection:"), 0,
                   wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    connection_choice_ = new wxChoice(this, wxID_ANY);
    if (connections_) {
        for (const auto& profile : *connections_) {
            connection_choice_->Append(ProfileLabel(profile));
        }
    }
    connSizer->Add(connection_choice_, 1, wxEXPAND);
    sizer->Add(connSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Schema selection
    auto* schemaSizer = new wxBoxSizer(wxHORIZONTAL);
    schemaSizer->Add(new wxStaticText(this, wxID_ANY, "Schema:"), 0,
                     wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    schema_choice_ = new wxChoice(this, wxID_ANY);
    schema_choice_->Append("Loading schemas...");
    schema_choice_->Enable(false);
    schemaSizer->Add(schema_choice_, 1, wxEXPAND);
    sizer->Add(schemaSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Status text
    status_text_ = new wxStaticText(this, wxID_ANY, "");
    status_text_->SetForegroundColour(wxColour(255, 100, 100));
    sizer->Add(status_text_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    SetSizer(sizer);
    
    // Bind events
    connection_choice_->Bind(wxEVT_CHOICE, &SchemaSelectionPage::OnConnectionChanged, this);
}

void SchemaSelectionPage::OnConnectionChanged(wxCommandEvent&) {
    LoadSchemas();
}

void SchemaSelectionPage::LoadSchemas() {
    if (!connection_manager_ || !connections_) {
        status_text_->SetLabel("No connections available");
        return;
    }
    
    int sel = connection_choice_->GetSelection();
    if (sel < 0 || sel >= static_cast<int>(connections_->size())) {
        return;
    }
    
    const auto& profile = (*connections_)[sel];
    
    // Connect to the database
    connection_manager_->Disconnect();
    if (!connection_manager_->Connect(profile)) {
        status_text_->SetLabel("Failed to connect: " + connection_manager_->LastError());
        return;
    }
    
    schema_choice_->Clear();
    schema_choice_->Enable(true);
    
    // Query schemas based on backend
    std::string backend = NormalizeBackendName(profile.backend);
    std::string sql;
    
    if (backend == "native" || backend == "scratchbird") {
        sql = "SELECT schema_name FROM sb_catalog.sb_schemas ORDER BY schema_name;";
    } else if (backend == "postgresql") {
        sql = "SELECT schema_name FROM information_schema.schemata "
              "WHERE schema_name NOT LIKE 'pg_%' AND schema_name != 'information_schema' "
              "ORDER BY schema_name;";
    } else if (backend == "mysql") {
        sql = "SHOW DATABASES;";
    } else if (backend == "firebird") {
        sql = "SELECT DISTINCT RDB$OWNER_NAME FROM RDB$RELATIONS "
              "WHERE RDB$SYSTEM_FLAG = 0;";
    }
    
    QueryResult result;
    if (connection_manager_->ExecuteQuery(sql, &result)) {
        for (const auto& row : result.rows) {
            if (!row.empty() && !row[0].isNull) {
                schema_choice_->Append(row[0].text);
            }
        }
        if (schema_choice_->GetCount() > 0) {
            schema_choice_->SetSelection(0);
            status_text_->SetLabel("Loaded " + std::to_string(schema_choice_->GetCount()) + " schemas");
        } else {
            status_text_->SetLabel("No schemas found");
        }
    } else {
        status_text_->SetLabel("Failed to load schemas");
    }
}

bool SchemaSelectionPage::TransferDataFromWindow() {
    if (connection_choice_->GetSelection() < 0) {
        status_text_->SetLabel("Please select a connection");
        return false;
    }
    if (schema_choice_->GetSelection() < 0) {
        status_text_->SetLabel("Please select a schema");
        return false;
    }
    return true;
}

std::string SchemaSelectionPage::GetSelectedSchema() const {
    if (schema_choice_ && schema_choice_->GetSelection() >= 0) {
        return schema_choice_->GetStringSelection().ToStdString();
    }
    return "";
}

const ConnectionProfile* SchemaSelectionPage::GetSelectedProfile() const {
    if (!connections_ || !connection_choice_) return nullptr;
    int sel = connection_choice_->GetSelection();
    if (sel < 0 || sel >= static_cast<int>(connections_->size())) return nullptr;
    return &(*connections_)[sel];
}

// ============================================================================
// Table Selection Page
// ============================================================================

TableSelectionPage::TableSelectionPage(wxWizard* parent)
    : wxWizardPage(parent) {
    BuildLayout();
}

void TableSelectionPage::BuildLayout() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(this, wxID_ANY,
        "Select tables to import into the diagram:"),
        0, wxALL, 10);
    
    // Table checklist
    tables_list_ = new wxCheckListBox(this, wxID_ANY);
    sizer->Add(tables_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Select/Deselect buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(new wxButton(this, wxID_ANY, "Select All"), 0, wxRIGHT, 5);
    btnSizer->Add(new wxButton(this, wxID_ANY, "Deselect All"), 0);
    sizer->Add(btnSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Status
    status_text_ = new wxStaticText(this, wxID_ANY, "");
    sizer->Add(status_text_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    SetSizer(sizer);
    
    // Bind buttons
    Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
        auto* btn = dynamic_cast<wxButton*>(evt.GetEventObject());
        if (btn) {
            wxString label = btn->GetLabel();
            if (label.Contains("Select All")) {
                OnSelectAll(evt);
            } else if (label.Contains("Deselect All")) {
                OnDeselectAll(evt);
            }
        }
    });
}

void TableSelectionPage::SetSchema(const std::string& schema) {
    schema_ = schema;
    LoadTables();
}

void TableSelectionPage::SetProfile(ConnectionManager* cm, const ConnectionProfile* profile) {
    connection_manager_ = cm;
    profile_ = profile;
}

void TableSelectionPage::LoadTables() {
    if (!connection_manager_ || schema_.empty()) return;
    
    tables_list_->Clear();
    
    std::string backend = profile_ ? NormalizeBackendName(profile_->backend) : "native";
    std::string sql;
    
    if (backend == "native" || backend == "scratchbird") {
        sql = "SELECT name FROM sb_catalog.sb_tables WHERE schema_name = '" + schema_ + "' "
              "AND name NOT LIKE 'sb_%' ORDER BY name;";
    } else if (backend == "postgresql") {
        sql = "SELECT table_name FROM information_schema.tables "
              "WHERE table_schema = '" + schema_ + "' AND table_type = 'BASE TABLE' "
              "ORDER BY table_name;";
    } else if (backend == "mysql") {
        sql = "SHOW TABLES FROM " + schema_ + ";";
    } else if (backend == "firebird") {
        sql = "SELECT RDB$RELATION_NAME FROM RDB$RELATIONS "
              "WHERE RDB$SYSTEM_FLAG = 0 ORDER BY RDB$RELATION_NAME;";
    }
    
    QueryResult result;
    if (connection_manager_->ExecuteQuery(sql, &result)) {
        for (const auto& row : result.rows) {
            if (!row.empty() && !row[0].isNull) {
                std::string name = row[0].text;
                // Trim spaces for Firebird
                size_t end = name.find_last_not_of(" ");
                if (end != std::string::npos) {
                    name = name.substr(0, end + 1);
                }
                tables_list_->Append(name);
            }
        }
        // Select all by default
        for (unsigned int i = 0; i < tables_list_->GetCount(); ++i) {
            tables_list_->Check(i);
        }
        status_text_->SetLabel("Found " + std::to_string(tables_list_->GetCount()) + " tables");
    } else {
        status_text_->SetLabel("Failed to load tables");
    }
}

void TableSelectionPage::OnSelectAll(wxCommandEvent&) {
    for (unsigned int i = 0; i < tables_list_->GetCount(); ++i) {
        tables_list_->Check(i);
    }
}

void TableSelectionPage::OnDeselectAll(wxCommandEvent&) {
    for (unsigned int i = 0; i < tables_list_->GetCount(); ++i) {
        tables_list_->Check(i, false);
    }
}

bool TableSelectionPage::TransferDataFromWindow() {
    bool has_selection = false;
    for (unsigned int i = 0; i < tables_list_->GetCount(); ++i) {
        if (tables_list_->IsChecked(i)) {
            has_selection = true;
            break;
        }
    }
    if (!has_selection) {
        status_text_->SetLabel("Please select at least one table");
        return false;
    }
    return true;
}

std::vector<std::string> TableSelectionPage::GetSelectedTables() const {
    std::vector<std::string> result;
    for (unsigned int i = 0; i < tables_list_->GetCount(); ++i) {
        if (tables_list_->IsChecked(i)) {
            result.push_back(tables_list_->GetString(i).ToStdString());
        }
    }
    return result;
}

// ============================================================================
// Import Options Page
// ============================================================================

ImportOptionsPage::ImportOptionsPage(wxWizard* parent)
    : wxWizardPage(parent) {
    BuildLayout();
}

void ImportOptionsPage::BuildLayout() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(this, wxID_ANY,
        "Configure import options:"),
        0, wxALL, 10);
    
    // Options checkboxes
    include_indexes_chk_ = new wxCheckBox(this, wxID_ANY, "Include indexes");
    include_constraints_chk_ = new wxCheckBox(this, wxID_ANY, "Include constraints");
    include_constraints_chk_->SetValue(true);
    include_comments_chk_ = new wxCheckBox(this, wxID_ANY, "Include comments");
    include_comments_chk_->SetValue(true);
    auto_layout_chk_ = new wxCheckBox(this, wxID_ANY, "Apply auto-layout after import");
    auto_layout_chk_->SetValue(true);
    
    sizer->Add(include_indexes_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    sizer->Add(include_constraints_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    sizer->Add(include_comments_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    sizer->Add(auto_layout_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Layout algorithm
    auto* algoSizer = new wxBoxSizer(wxHORIZONTAL);
    algoSizer->Add(new wxStaticText(this, wxID_ANY, "Layout Algorithm:"), 0,
                   wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    layout_algo_choice_ = new wxChoice(this, wxID_ANY);
    layout_algo_choice_->Append("Sugiyama (Hierarchical)");
    layout_algo_choice_->Append("Force-Directed");
    layout_algo_choice_->Append("Orthogonal");
    layout_algo_choice_->SetSelection(0);
    algoSizer->Add(layout_algo_choice_, 1, wxEXPAND);
    sizer->Add(algoSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    SetSizer(sizer);
}

diagram::ReverseEngineerOptions ImportOptionsPage::GetOptions() const {
    diagram::ReverseEngineerOptions options;
    options.include_indexes = include_indexes_chk_->GetValue();
    options.include_constraints = include_constraints_chk_->GetValue();
    options.include_comments = include_comments_chk_->GetValue();
    options.auto_layout = auto_layout_chk_->GetValue();
    
    int algo = layout_algo_choice_->GetSelection();
    if (algo == 0) options.layout_algorithm = diagram::LayoutAlgorithm::Sugiyama;
    else if (algo == 1) options.layout_algorithm = diagram::LayoutAlgorithm::ForceDirected;
    else if (algo == 2) options.layout_algorithm = diagram::LayoutAlgorithm::Orthogonal;
    
    return options;
}

// ============================================================================
// Reverse Engineer Wizard
// ============================================================================

wxBEGIN_EVENT_TABLE(ReverseEngineerWizard, wxWizard)
    EVT_WIZARD_FINISHED(wxID_ANY, ReverseEngineerWizard::OnWizardFinished)
    EVT_WIZARD_CANCEL(wxID_ANY, ReverseEngineerWizard::OnWizardCancelled)
wxEND_EVENT_TABLE()

ReverseEngineerWizard::ReverseEngineerWizard(wxWindow* parent,
                                             ConnectionManager* connection_manager,
                                             const std::vector<ConnectionProfile>* connections)
    : wxWizard(parent, wxID_ANY, "Reverse Engineer Database to Diagram",
               wxBitmap(), wxDefaultPosition, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      connection_manager_(connection_manager),
      connections_(connections) {
    
    // Create pages
    schema_page_ = new SchemaSelectionPage(this, connection_manager, connections);
    tables_page_ = new TableSelectionPage(this);
    options_page_ = new ImportOptionsPage(this);
    
    // Set up page chain
    schema_page_->SetNext(tables_page_);
    tables_page_->SetPrev(schema_page_);
    tables_page_->SetNext(options_page_);
    options_page_->SetPrev(tables_page_);
    
    // Set first page
    GetPageAreaSizer()->Add(schema_page_);
}

bool ReverseEngineerWizard::Run(DiagramModel* model) {
    target_model_ = model;
    completed_ = false;
    
    if (!RunWizard(schema_page_)) {
        return false;
    }
    
    if (!completed_) {
        return false;
    }
    
    return true;
}

void ReverseEngineerWizard::OnWizardFinished(wxWizardEvent&) {
    if (!target_model_) return;
    
    // Get options from pages
    std::string schema = schema_page_->GetSelectedSchema();
    const ConnectionProfile* profile = schema_page_->GetSelectedProfile();
    std::vector<std::string> tables = tables_page_->GetSelectedTables();
    diagram::ReverseEngineerOptions options = options_page_->GetOptions();
    options.schema_filter = schema;
    options.table_filter = tables;
    
    // Update tables page data before importing
    tables_page_->SetSchema(schema);
    tables_page_->SetProfile(connection_manager_, profile);
    
    // Show progress dialog and import
    ImportProgressDialog progressDlg(this, static_cast<int>(tables.size()));
    
    // Create reverse engineer and import
    diagram::ReverseEngineer engine(connection_manager_, profile);
    
    bool success = engine.ImportToDiagram(target_model_, options,
        [&progressDlg](const std::string& table, int current, int total) {
            progressDlg.UpdateProgress(table, current, total);
        });
    
    progressDlg.SetCompleted(success, success ? "Import completed successfully" : "Import failed");
    progressDlg.ShowModal();
    
    completed_ = success;
}

void ReverseEngineerWizard::OnWizardCancelled(wxWizardEvent&) {
    completed_ = false;
}

// ============================================================================
// Import Progress Dialog
// ============================================================================

wxBEGIN_EVENT_TABLE(ImportProgressDialog, wxDialog)
    EVT_BUTTON(wxID_CANCEL, ImportProgressDialog::OnCancel)
wxEND_EVENT_TABLE()

ImportProgressDialog::ImportProgressDialog(wxWindow* parent, int total_tables)
    : wxDialog(parent, wxID_ANY, "Importing Database Schema",
               wxDefaultPosition, wxSize(400, 200)) {
    BuildLayout();
    progress_gauge_->SetRange(total_tables);
}

void ImportProgressDialog::BuildLayout() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(this, wxID_ANY, "Importing tables..."),
               0, wxALL, 15);
    
    current_table_text_ = new wxStaticText(this, wxID_ANY, "Initializing...");
    sizer->Add(current_table_text_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    progress_gauge_ = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition,
                                   wxSize(-1, 25));
    sizer->Add(progress_gauge_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    count_text_ = new wxStaticText(this, wxID_ANY, "0 / 0");
    sizer->Add(count_text_, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    cancel_btn_ = new wxButton(this, wxID_CANCEL, "Cancel");
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(cancel_btn_, 0);
    sizer->Add(btnSizer, 0, wxEXPAND | wxALL, 15);
    
    SetSizer(sizer);
}

void ImportProgressDialog::UpdateProgress(const std::string& table_name,
                                          int current, int total) {
    CallAfter([this, table_name, current, total]() {
        current_table_text_->SetLabel("Importing: " + table_name);
        progress_gauge_->SetValue(current);
        count_text_->SetLabel(std::to_string(current) + " / " + std::to_string(total));
    });
}

void ImportProgressDialog::SetCompleted(bool success, const std::string& message) {
    CallAfter([this, success, message]() {
        current_table_text_->SetLabel(message);
        current_table_text_->SetForegroundColour(success ? wxColour(0, 150, 0) : wxColour(200, 0, 0));
        cancel_btn_->SetLabel("Close");
    });
}

void ImportProgressDialog::OnCancel(wxCommandEvent&) {
    cancelled_ = true;
    EndModal(wxID_CANCEL);
}

} // namespace scratchrobin
