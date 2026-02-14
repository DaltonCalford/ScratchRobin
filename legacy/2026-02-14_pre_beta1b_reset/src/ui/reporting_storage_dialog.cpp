// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "reporting_storage_dialog.h"

#include "../reporting/result_storage.h"
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

// ReportingStorageDialog implementation

wxBEGIN_EVENT_TABLE(ReportingStorageDialog, wxDialog)
    EVT_CHOICE(wxID_ANY, ReportingStorageDialog::OnStorageTypeChanged)
    EVT_CHECKBOX(wxID_ANY, ReportingStorageDialog::OnEnableChanged)
wxEND_EVENT_TABLE()

ReportingStorageDialog::ReportingStorageDialog(wxWindow* parent,
                                                ProjectConfig::ReportingStorage* config)
    : wxDialog(parent, wxID_ANY, "Reporting Storage Configuration", wxDefaultPosition,
               wxSize(600, 500)),
      config_(config) {
    CreateControls();
    LoadFromConfig();
    UpdateUIState();
}

void ReportingStorageDialog::CreateControls() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Enable checkbox
    enable_checkbox_ = new wxCheckBox(this, wxID_ANY, "Enable Persistent Result Storage");
    main_sizer->Add(enable_checkbox_, 0, wxALL, 12);
    
    // Storage type selection
    auto* type_sizer = new wxBoxSizer(wxHORIZONTAL);
    type_sizer->Add(new wxStaticText(this, wxID_ANY, "Storage Type:"), 0, 
                    wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    storage_type_choice_ = new wxChoice(this, wxID_ANY);
    storage_type_choice_->Append("Embedded (SQLite)");
    storage_type_choice_->Append("External Database");
    storage_type_choice_->Append("S3-Compatible Storage");
    type_sizer->Add(storage_type_choice_, 1);
    main_sizer->Add(type_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Notebook for type-specific settings
    type_notebook_ = new wxNotebook(this, wxID_ANY);
    
    // Embedded panel
    embedded_panel_ = new wxPanel(type_notebook_, wxID_ANY);
    auto* embedded_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* path_sizer = new wxBoxSizer(wxHORIZONTAL);
    path_sizer->Add(new wxStaticText(embedded_panel_, wxID_ANY, "Database Path:"), 
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    embedded_path_ctrl_ = new wxTextCtrl(embedded_panel_, wxID_ANY);
    path_sizer->Add(embedded_path_ctrl_, 1, wxRIGHT, 8);
    auto* browse_btn = new wxButton(embedded_panel_, wxID_ANY, "Browse...");
    browse_btn->Bind(wxEVT_BUTTON, &ReportingStorageDialog::OnBrowsePath, this);
    path_sizer->Add(browse_btn);
    embedded_sizer->Add(path_sizer, 0, wxEXPAND | wxBOTTOM, 12);
    
    embedded_sizer->Add(new wxStaticText(embedded_panel_, wxID_ANY, 
        "Relative to project root. Leave empty for default (.scratchrobin/reporting_results.db)"),
        0, wxBOTTOM, 12);
    
    auto* retention_sizer = new wxBoxSizer(wxHORIZONTAL);
    retention_sizer->Add(new wxStaticText(embedded_panel_, wxID_ANY, 
                                           "Retention Period (days):"), 
                          0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    retention_days_ctrl_ = new wxSpinCtrl(embedded_panel_, wxID_ANY);
    retention_days_ctrl_->SetRange(1, 3650);  // 1 day to 10 years
    retention_days_ctrl_->SetValue(90);
    retention_sizer->Add(retention_days_ctrl_);
    embedded_sizer->Add(retention_sizer, 0, wxBOTTOM, 12);
    
    compress_checkbox_ = new wxCheckBox(embedded_panel_, wxID_ANY, 
                                        "Compress stored results");
    embedded_sizer->Add(compress_checkbox_, 0, wxBOTTOM, 8);
    
    encrypt_checkbox_ = new wxCheckBox(embedded_panel_, wxID_ANY, 
                                       "Encrypt stored results");
    embedded_sizer->Add(encrypt_checkbox_, 0, wxBOTTOM, 8);
    
    embedded_panel_->SetSizer(embedded_sizer);
    type_notebook_->AddPage(embedded_panel_, "Embedded");
    
    // External panel
    external_panel_ = new wxPanel(type_notebook_, wxID_ANY);
    auto* external_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* conn_sizer = new wxBoxSizer(wxHORIZONTAL);
    conn_sizer->Add(new wxStaticText(external_panel_, wxID_ANY, 
                                      "Connection Profile:"), 
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    connection_ref_ctrl_ = new wxTextCtrl(external_panel_, wxID_ANY);
    conn_sizer->Add(connection_ref_ctrl_, 1, wxRIGHT, 8);
    auto* test_btn = new wxButton(external_panel_, wxID_ANY, "Test");
    test_btn->Bind(wxEVT_BUTTON, &ReportingStorageDialog::OnTestConnection, this);
    conn_sizer->Add(test_btn);
    external_sizer->Add(conn_sizer, 0, wxEXPAND | wxBOTTOM, 12);
    
    auto* schema_sizer = new wxBoxSizer(wxHORIZONTAL);
    schema_sizer->Add(new wxStaticText(external_panel_, wxID_ANY, "Schema Name:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    schema_name_ctrl_ = new wxTextCtrl(external_panel_, wxID_ANY, "reporting");
    schema_sizer->Add(schema_name_ctrl_, 1);
    external_sizer->Add(schema_sizer, 0, wxEXPAND | wxBOTTOM, 12);
    
    auto* prefix_sizer = new wxBoxSizer(wxHORIZONTAL);
    prefix_sizer->Add(new wxStaticText(external_panel_, wxID_ANY, "Table Prefix:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    table_prefix_ctrl_ = new wxTextCtrl(external_panel_, wxID_ANY, "rpt_");
    prefix_sizer->Add(table_prefix_ctrl_, 1);
    external_sizer->Add(prefix_sizer, 0, wxEXPAND | wxBOTTOM, 12);
    
    external_panel_->SetSizer(external_sizer);
    type_notebook_->AddPage(external_panel_, "External Database");
    
    // S3 panel
    s3_panel_ = new wxPanel(type_notebook_, wxID_ANY);
    auto* s3_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* bucket_sizer = new wxBoxSizer(wxHORIZONTAL);
    bucket_sizer->Add(new wxStaticText(s3_panel_, wxID_ANY, "Bucket Name:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    s3_bucket_ctrl_ = new wxTextCtrl(s3_panel_, wxID_ANY);
    bucket_sizer->Add(s3_bucket_ctrl_, 1);
    s3_sizer->Add(bucket_sizer, 0, wxEXPAND | wxBOTTOM, 12);
    
    auto* region_sizer = new wxBoxSizer(wxHORIZONTAL);
    region_sizer->Add(new wxStaticText(s3_panel_, wxID_ANY, "Region:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    s3_region_ctrl_ = new wxTextCtrl(s3_panel_, wxID_ANY, "us-east-1");
    region_sizer->Add(s3_region_ctrl_, 1);
    s3_sizer->Add(region_sizer, 0, wxEXPAND | wxBOTTOM, 12);
    
    auto* access_sizer = new wxBoxSizer(wxHORIZONTAL);
    access_sizer->Add(new wxStaticText(s3_panel_, wxID_ANY, "Access Key:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    s3_access_key_ctrl_ = new wxTextCtrl(s3_panel_, wxID_ANY);
    access_sizer->Add(s3_access_key_ctrl_, 1);
    s3_sizer->Add(access_sizer, 0, wxEXPAND | wxBOTTOM, 12);
    
    auto* secret_sizer = new wxBoxSizer(wxHORIZONTAL);
    secret_sizer->Add(new wxStaticText(s3_panel_, wxID_ANY, "Secret Key:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    s3_secret_key_ctrl_ = new wxTextCtrl(s3_panel_, wxID_ANY, wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxTE_PASSWORD);
    secret_sizer->Add(s3_secret_key_ctrl_, 1);
    s3_sizer->Add(secret_sizer, 0, wxEXPAND | wxBOTTOM, 12);
    
    s3_panel_->SetSizer(s3_sizer);
    type_notebook_->AddPage(s3_panel_, "S3 Storage");
    
    main_sizer->Add(type_notebook_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Buttons
    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer(1);
    btn_sizer->Add(new wxButton(this, wxID_OK, "Save"), 0, wxRIGHT, 8);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"));
    main_sizer->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    SetSizer(main_sizer);
}

void ReportingStorageDialog::LoadFromConfig() {
    if (!config_) return;
    
    enable_checkbox_->SetValue(config_->enabled);
    
    if (config_->storage_type == "embedded") {
        storage_type_choice_->SetSelection(0);
    } else if (config_->storage_type == "external") {
        storage_type_choice_->SetSelection(1);
    } else if (config_->storage_type == "s3") {
        storage_type_choice_->SetSelection(2);
    }
    
    embedded_path_ctrl_->SetValue(config_->database_path);
    retention_days_ctrl_->SetValue(static_cast<int>(config_->retention_days));
    compress_checkbox_->SetValue(config_->compress_results);
    encrypt_checkbox_->SetValue(config_->encrypt_results);
    
    connection_ref_ctrl_->SetValue(config_->connection_ref);
    schema_name_ctrl_->SetValue(config_->schema_name);
    table_prefix_ctrl_->SetValue(config_->table_prefix);
}

void ReportingStorageDialog::SaveToConfig() {
    if (!config_) return;
    
    config_->enabled = enable_checkbox_->GetValue();
    
    int type_sel = storage_type_choice_->GetSelection();
    if (type_sel == 0) {
        config_->storage_type = "embedded";
    } else if (type_sel == 1) {
        config_->storage_type = "external";
    } else if (type_sel == 2) {
        config_->storage_type = "s3";
    }
    
    config_->database_path = embedded_path_ctrl_->GetValue().ToStdString();
    config_->retention_days = static_cast<uint32_t>(retention_days_ctrl_->GetValue());
    config_->compress_results = compress_checkbox_->GetValue();
    config_->encrypt_results = encrypt_checkbox_->GetValue();
    
    config_->connection_ref = connection_ref_ctrl_->GetValue().ToStdString();
    config_->schema_name = schema_name_ctrl_->GetValue().ToStdString();
    config_->table_prefix = table_prefix_ctrl_->GetValue().ToStdString();
}

void ReportingStorageDialog::UpdateUIState() {
    bool enabled = enable_checkbox_->GetValue();
    storage_type_choice_->Enable(enabled);
    type_notebook_->Enable(enabled);
    
    if (enabled) {
        int type = storage_type_choice_->GetSelection();
        type_notebook_->SetSelection(type);
    }
}

void ReportingStorageDialog::OnStorageTypeChanged(wxCommandEvent& event) {
    int type = storage_type_choice_->GetSelection();
    type_notebook_->SetSelection(type);
}

void ReportingStorageDialog::OnEnableChanged(wxCommandEvent& event) {
    UpdateUIState();
}

void ReportingStorageDialog::OnBrowsePath(wxCommandEvent& event) {
    wxFileDialog dialog(this, "Select Database File", "", "",
                        "SQLite databases (*.db;*.sqlite)|*.db;*.sqlite|All files (*.*)|*.*",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() == wxID_OK) {
        embedded_path_ctrl_->SetValue(dialog.GetPath());
    }
}

void ReportingStorageDialog::OnTestConnection(wxCommandEvent& event) {
    // Test connection to external database
    wxMessageBox("Connection test not yet implemented", "Test Connection", 
                 wxOK | wxICON_INFORMATION, this);
}

bool ReportingStorageDialog::ShowModalAndSave() {
    if (ShowModal() == wxID_OK) {
        SaveToConfig();
        return true;
    }
    return false;
}

// StoredResultsHistoryPanel implementation

wxBEGIN_EVENT_TABLE(StoredResultsHistoryPanel, wxPanel)
wxEND_EVENT_TABLE()

StoredResultsHistoryPanel::StoredResultsHistoryPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    CreateControls();
}

void StoredResultsHistoryPanel::CreateControls() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(this, wxID_ANY, "Stored Results History"), 
               0, wxALL, 8);
    
    // Results list
    sizer->Add(new wxStaticText(this, wxID_ANY, "Select a result to view, export, or compare."),
               0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    // Toolbar
    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
    auto* refresh_btn = new wxButton(this, wxID_ANY, "Refresh");
    auto* delete_btn = new wxButton(this, wxID_ANY, "Delete");
    auto* export_btn = new wxButton(this, wxID_ANY, "Export");
    auto* compare_btn = new wxButton(this, wxID_ANY, "Compare");
    auto* trend_btn = new wxButton(this, wxID_ANY, "View Trend");
    
    toolbar->Add(refresh_btn, 0, wxRIGHT, 8);
    toolbar->Add(delete_btn, 0, wxRIGHT, 8);
    toolbar->Add(export_btn, 0, wxRIGHT, 8);
    toolbar->Add(compare_btn, 0, wxRIGHT, 8);
    toolbar->Add(trend_btn);
    
    sizer->Add(toolbar, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    SetSizer(sizer);
}

void StoredResultsHistoryPanel::LoadResults(const std::string& question_id) {
    current_question_id_ = question_id;
    RefreshResults();
}

void StoredResultsHistoryPanel::RefreshResults() {
    // Load results from storage
}

void StoredResultsHistoryPanel::OnResultSelected(wxCommandEvent& event) {}
void StoredResultsHistoryPanel::OnDeleteResult(wxCommandEvent& event) {}
void StoredResultsHistoryPanel::OnExportResult(wxCommandEvent& event) {}
void StoredResultsHistoryPanel::OnCompareResults(wxCommandEvent& event) {}
void StoredResultsHistoryPanel::OnViewTrend(wxCommandEvent& event) {}

// ResultComparisonDialog implementation

wxBEGIN_EVENT_TABLE(ResultComparisonDialog, wxDialog)
wxEND_EVENT_TABLE()

ResultComparisonDialog::ResultComparisonDialog(wxWindow* parent,
                                                const std::string& result_id_1,
                                                const std::string& result_id_2)
    : wxDialog(parent, wxID_ANY, "Compare Results", wxDefaultPosition, wxSize(800, 600)),
      result_id_1_(result_id_1),
      result_id_2_(result_id_2) {
    CreateControls();
    LoadComparison();
}

void ResultComparisonDialog::CreateControls() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Result Comparison"), 0, wxALL, 12);
    SetSizer(sizer);
}

void ResultComparisonDialog::LoadComparison() {
    // Load and compare results
}

void ResultComparisonDialog::ExportComparison() {
    // Export comparison to file
}

// LongDurationReportDialog implementation

wxBEGIN_EVENT_TABLE(LongDurationReportDialog, wxDialog)
wxEND_EVENT_TABLE()

LongDurationReportDialog::LongDurationReportDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Long Duration Report", wxDefaultPosition, 
               wxSize(600, 500)) {
    CreateControls();
}

void LongDurationReportDialog::CreateControls() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, wxID_ANY, "Configure Long Duration Report"),
               0, wxALL, 12);
    SetSizer(sizer);
}

void LongDurationReportDialog::OnQuestionSelected(wxCommandEvent& event) {}
void LongDurationReportDialog::OnPreview(wxCommandEvent& event) {}
void LongDurationReportDialog::OnExport(wxCommandEvent& event) {}

bool LongDurationReportDialog::ShowModalAndGetConfig(ReportConfig* config) {
    if (ShowModal() == wxID_OK && config) {
        *config = config_;
        return true;
    }
    return false;
}

} // namespace scratchrobin
