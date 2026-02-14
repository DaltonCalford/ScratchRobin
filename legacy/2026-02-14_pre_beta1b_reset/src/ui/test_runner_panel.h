/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_TEST_RUNNER_PANEL_H
#define SCRATCHROBIN_TEST_RUNNER_PANEL_H

#include <wx/panel.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/treectrl.h>

#include "core/testing_framework.h"

class wxListCtrl;
class wxTextCtrl;
class wxGauge;
class wxButton;
class wxCheckBox;
class wxChoice;
class wxTreeCtrl;
class wxSplitterWindow;

namespace scratchrobin {

// Forward declarations
class DatabaseConnection;

// ============================================================================
// Test Runner Panel - Execute and manage tests
// ============================================================================
class TestRunnerPanel : public wxPanel {
public:
    TestRunnerPanel(wxWindow* parent);
    ~TestRunnerPanel();
    
    void LoadTestSuite(const TestSuite& suite);
    void RunSelectedTests();
    void RunAllTests();
    
private:
    void BuildLayout();
    void BuildToolbar();
    void BuildTestTree();
    void BuildResultsPanel();
    void BuildDetailsPanel();
    
    void UpdateStatusDisplay();
    void UpdateResultsDisplay();
    void DisplayTestDetails(const TestCase& test);
    void ExportResults(const wxString& format);
    
    // Event handlers
    void OnRun(wxCommandEvent& event);
    void OnRunSelected(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnTestSelect(wxTreeEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnFilterChange(wxCommandEvent& event);
    void OnCheckAll(wxCommandEvent& event);
    void OnUncheckAll(wxCommandEvent& event);
    
    // Actions
    void ExecuteTest(const TestCase& test);
    void ExecuteSuite(const TestSuite& suite);
    void DisplayFailureDetails(const TestResult& result);
    void GenerateReport(const wxString& format);
    
    // Members
    TestRunner runner_;
    TestSuite current_suite_;
    std::vector<TestResult> results_;
    bool is_running_ = false;
    int current_test_index_ = 0;
    wxTimer update_timer_;
    
    // UI Components
    wxButton* run_button_ = nullptr;
    wxButton* run_selected_button_ = nullptr;
    wxButton* stop_button_ = nullptr;
    wxButton* refresh_button_ = nullptr;
    wxButton* export_button_ = nullptr;
    wxButton* check_all_button_ = nullptr;
    wxButton* uncheck_all_button_ = nullptr;
    
    wxChoice* filter_choice_ = nullptr;
    wxGauge* progress_gauge_ = nullptr;
    wxTreeCtrl* test_tree_ = nullptr;
    
    wxListCtrl* results_list_ = nullptr;
    wxTextCtrl* details_text_ = nullptr;
    wxTextCtrl* log_text_ = nullptr;
    
    // Status labels
    wxStaticText* passed_label_ = nullptr;
    wxStaticText* failed_label_ = nullptr;
    wxStaticText* skipped_label_ = nullptr;
    wxStaticText* total_label_ = nullptr;
    wxStaticText* time_label_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// Test Configuration Dialog
// ============================================================================
class TestConfigDialog : public wxDialog {
public:
    TestConfigDialog(wxWindow* parent, TestRunnerConfig* config);
    
private:
    void BuildLayout();
    void OnSave(wxCommandEvent& event);
    void OnReset(wxCommandEvent& event);
    
    TestRunnerConfig* config_;
};

// ============================================================================
// Test Report Viewer
// ============================================================================
class TestReportViewer : public wxDialog {
public:
    TestReportViewer(wxWindow* parent, const std::string& report, ReportFormat format);
    
private:
    void BuildLayout();
    void OnSave(wxCommandEvent& event);
    void OnPrint(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    
    wxString report_content_;
    ReportFormat format_;
};

// ============================================================================
// Performance Benchmark Dialog
// ============================================================================
class BenchmarkDialog : public wxDialog {
public:
    BenchmarkDialog(wxWindow* parent, DatabaseConnection* connection);
    
private:
    void BuildLayout();
    void OnRunBenchmark(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void DisplayResults(const TestRunner::BenchmarkResult& result);
    
    DatabaseConnection* connection_;
};

// ============================================================================
// Data Quality Rules Editor
// ============================================================================
class DataQualityRulesDialog : public wxDialog {
public:
    DataQualityRulesDialog(wxWindow* parent, std::vector<DataQualityRule>* rules);
    
private:
    void BuildLayout();
    void OnAddRule(wxCommandEvent& event);
    void OnEditRule(wxCommandEvent& event);
    void OnDeleteRule(wxCommandEvent& event);
    void OnImport(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    
    std::vector<DataQualityRule>* rules_;
    wxListCtrl* rules_list_ = nullptr;
};

// ============================================================================
// Security Test Configuration
// ============================================================================
class SecurityTestDialog : public wxDialog {
public:
    SecurityTestDialog(wxWindow* parent, SecurityTestConfig* config);
    
private:
    void BuildLayout();
    void OnRunTest(wxCommandEvent& event);
    void OnSelectAllChecks(wxCommandEvent& event);
    void OnSaveProfile(wxCommandEvent& event);
    void OnLoadProfile(wxCommandEvent& event);
    
    SecurityTestConfig* config_;
    std::vector<wxCheckBox*> check_boxes_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TEST_RUNNER_PANEL_H
