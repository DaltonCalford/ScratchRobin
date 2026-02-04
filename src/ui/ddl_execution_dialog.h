/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DDL_EXECUTION_DIALOG_H
#define SCRATCHROBIN_DDL_EXECUTION_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxTextCtrl;
class wxChoice;
class wxGauge;
class wxStaticText;
class wxButton;
class wxListBox;

namespace scratchrobin {

class ConnectionManager;
class ConnectionProfile;

// DDL execution status
struct DdlExecutionResult {
    bool success = false;
    int statementsTotal = 0;
    int statementsExecuted = 0;
    int statementsFailed = 0;
    std::vector<std::string> errors;
    std::string lastStatement;
};

class DdlExecutionDialog : public wxDialog {
public:
    DdlExecutionDialog(wxWindow* parent,
                       ConnectionManager* connectionManager,
                       const std::vector<ConnectionProfile>* connections,
                       const std::string& ddl);

    // Execute DDL and show progress
    bool Execute();

private:
    void BuildLayout();
    void OnConnectionChanged(wxCommandEvent& event);
    void OnExecute(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    
    bool ConnectToDatabase();
    DdlExecutionResult ExecuteDdlStatements();
    std::vector<std::string> SplitDdlIntoStatements(const std::string& ddl);
    void UpdateProgress(int current, int total, const std::string& statement);
    void AddLogMessage(const std::string& message, bool isError = false);

    ConnectionManager* connection_manager_;
    const std::vector<ConnectionProfile>* connections_;
    std::string ddl_;
    
    wxChoice* connection_choice_ = nullptr;
    wxStaticText* status_text_ = nullptr;
    wxGauge* progress_gauge_ = nullptr;
    wxStaticText* count_text_ = nullptr;
    wxListBox* log_list_ = nullptr;
    wxButton* execute_btn_ = nullptr;
    wxButton* cancel_btn_ = nullptr;
    wxButton* close_btn_ = nullptr;
    
    bool is_executing_ = false;
    bool should_cancel_ = false;
    DdlExecutionResult result_;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DDL_EXECUTION_DIALOG_H
