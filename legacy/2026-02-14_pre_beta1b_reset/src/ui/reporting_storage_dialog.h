// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "../core/project.h"
#include <wx/dialog.h>
#include <wx/panel.h>

class wxCheckBox;
class wxChoice;
class wxTextCtrl;
class wxSpinCtrl;
class wxNotebook;
class wxPanel;

namespace scratchrobin {

/**
 * @brief Dialog for configuring reporting result storage
 */
class ReportingStorageDialog : public wxDialog {
public:
    ReportingStorageDialog(wxWindow* parent, ProjectConfig::ReportingStorage* config);
    
    bool ShowModalAndSave();
    
private:
    void CreateControls();
    void LoadFromConfig();
    void SaveToConfig();
    void OnStorageTypeChanged(wxCommandEvent& event);
    void OnEnableChanged(wxCommandEvent& event);
    void OnBrowsePath(wxCommandEvent& event);
    void OnTestConnection(wxCommandEvent& event);
    void UpdateUIState();
    
    // UI Controls
    wxCheckBox* enable_checkbox_ = nullptr;
    wxChoice* storage_type_choice_ = nullptr;
    wxNotebook* type_notebook_ = nullptr;
    
    // Embedded storage controls
    wxPanel* embedded_panel_ = nullptr;
    wxTextCtrl* embedded_path_ctrl_ = nullptr;
    wxSpinCtrl* retention_days_ctrl_ = nullptr;
    wxCheckBox* compress_checkbox_ = nullptr;
    wxCheckBox* encrypt_checkbox_ = nullptr;
    
    // External storage controls
    wxPanel* external_panel_ = nullptr;
    wxTextCtrl* connection_ref_ctrl_ = nullptr;
    wxTextCtrl* schema_name_ctrl_ = nullptr;
    wxTextCtrl* table_prefix_ctrl_ = nullptr;
    
    // S3 storage controls
    wxPanel* s3_panel_ = nullptr;
    wxTextCtrl* s3_bucket_ctrl_ = nullptr;
    wxTextCtrl* s3_region_ctrl_ = nullptr;
    wxTextCtrl* s3_access_key_ctrl_ = nullptr;
    wxTextCtrl* s3_secret_key_ctrl_ = nullptr;
    
    // Stats display
    wxPanel* stats_panel_ = nullptr;
    
    ProjectConfig::ReportingStorage* config_;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Panel for viewing stored results history
 */
class StoredResultsHistoryPanel : public wxPanel {
public:
    StoredResultsHistoryPanel(wxWindow* parent);
    
    void LoadResults(const std::string& question_id);
    void RefreshResults();
    
private:
    void CreateControls();
    void OnResultSelected(wxCommandEvent& event);
    void OnDeleteResult(wxCommandEvent& event);
    void OnExportResult(wxCommandEvent& event);
    void OnCompareResults(wxCommandEvent& event);
    void OnViewTrend(wxCommandEvent& event);
    
    std::string current_question_id_;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Dialog for comparing two result sets
 */
class ResultComparisonDialog : public wxDialog {
public:
    ResultComparisonDialog(wxWindow* parent,
                           const std::string& result_id_1,
                           const std::string& result_id_2);
    
private:
    void CreateControls();
    void LoadComparison();
    void ExportComparison();
    
    std::string result_id_1_;
    std::string result_id_2_;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Dialog for configuring long-duration report
 */
class LongDurationReportDialog : public wxDialog {
public:
    struct ReportConfig {
        std::string name;
        std::string question_id;
        std::string aggregation_column;
        std::string aggregation_function;
        std::string group_by_column;
        std::time_t start_date;
        std::time_t end_date;
        std::string time_granularity;
        std::string export_format;
    };
    
    LongDurationReportDialog(wxWindow* parent);
    
    bool ShowModalAndGetConfig(ReportConfig* config);
    
private:
    void CreateControls();
    void OnQuestionSelected(wxCommandEvent& event);
    void OnPreview(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    
    ReportConfig config_;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin
