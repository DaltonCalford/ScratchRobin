/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "ddl_execution_dialog.h"

#include "core/connection_manager.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/gauge.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace scratchrobin {

namespace {

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

wxBEGIN_EVENT_TABLE(DdlExecutionDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, DdlExecutionDialog::OnExecute)
    EVT_CLOSE(DdlExecutionDialog::OnClose)
wxEND_EVENT_TABLE()

DdlExecutionDialog::DdlExecutionDialog(wxWindow* parent,
                                       ConnectionManager* connectionManager,
                                       const std::vector<ConnectionProfile>* connections,
                                       const std::string& ddl)
    : wxDialog(parent, wxID_ANY, "Execute DDL",
               wxDefaultPosition, wxSize(700, 500)),
      connection_manager_(connectionManager),
      connections_(connections),
      ddl_(ddl) {
    BuildLayout();
}

void DdlExecutionDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Connection selection
    auto* connSizer = new wxBoxSizer(wxHORIZONTAL);
    connSizer->Add(new wxStaticText(this, wxID_ANY, "Target Connection:"), 0,
                   wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    connection_choice_ = new wxChoice(this, wxID_ANY);
    if (connections_) {
        for (const auto& profile : *connections_) {
            connection_choice_->Append(ProfileLabel(profile));
        }
    }
    connSizer->Add(connection_choice_, 1, wxEXPAND);
    root->Add(connSizer, 0, wxEXPAND | wxALL, 12);
    
    // Status
    status_text_ = new wxStaticText(this, wxID_ANY, "Ready to execute DDL");
    root->Add(status_text_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Progress
    auto* progressSizer = new wxBoxSizer(wxHORIZONTAL);
    progress_gauge_ = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition,
                                   wxSize(-1, 20));
    progressSizer->Add(progress_gauge_, 1, wxEXPAND | wxRIGHT, 8);
    count_text_ = new wxStaticText(this, wxID_ANY, "0 / 0");
    progressSizer->Add(count_text_, 0, wxALIGN_CENTER_VERTICAL);
    root->Add(progressSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Log
    root->Add(new wxStaticText(this, wxID_ANY, "Execution Log:"), 0,
              wxLEFT | wxRIGHT | wxBOTTOM, 4);
    log_list_ = new wxListBox(this, wxID_ANY);
    root->Add(log_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    execute_btn_ = new wxButton(this, wxID_ANY, "Execute DDL");
    cancel_btn_ = new wxButton(this, wxID_ANY, "Cancel");
    cancel_btn_->Enable(false);
    close_btn_ = new wxButton(this, wxID_CLOSE, "Close");
    
    btnSizer->Add(execute_btn_, 0, wxRIGHT, 8);
    btnSizer->Add(cancel_btn_, 0, wxRIGHT, 8);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(close_btn_, 0);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
}

bool DdlExecutionDialog::Execute() {
    if (is_executing_) return false;
    
    // Validate connection selection
    if (!connection_manager_ || !connections_) {
        AddLogMessage("No connections available", true);
        return false;
    }
    
    int sel = connection_choice_->GetSelection();
    if (sel < 0 || sel >= static_cast<int>(connections_->size())) {
        AddLogMessage("Please select a connection", true);
        return false;
    }
    
    // Split DDL into statements
    std::vector<std::string> statements = SplitDdlIntoStatements(ddl_);
    if (statements.empty()) {
        AddLogMessage("No SQL statements found in DDL", true);
        return false;
    }
    
    result_.statementsTotal = static_cast<int>(statements.size());
    result_.statementsExecuted = 0;
    result_.statementsFailed = 0;
    result_.errors.clear();
    
    is_executing_ = true;
    should_cancel_ = false;
    execute_btn_->Enable(false);
    cancel_btn_->Enable(true);
    connection_choice_->Enable(false);
    
    progress_gauge_->SetRange(result_.statementsTotal);
    progress_gauge_->SetValue(0);
    
    // Connect to database
    if (!ConnectToDatabase()) {
        is_executing_ = false;
        execute_btn_->Enable(true);
        cancel_btn_->Enable(false);
        connection_choice_->Enable(true);
        return false;
    }
    
    AddLogMessage("Connected to database");
    AddLogMessage("Executing " + std::to_string(statements.size()) + " statements...");
    
    // Execute statements
    for (size_t i = 0; i < statements.size() && !should_cancel_; ++i) {
        const std::string& stmt = statements[i];
        result_.lastStatement = stmt;
        
        UpdateProgress(static_cast<int>(i), result_.statementsTotal, stmt);
        
        // Execute the statement
        QueryResult result;
        if (connection_manager_->ExecuteQuery(stmt, &result)) {
            result_.statementsExecuted++;
            AddLogMessage("OK: " + stmt.substr(0, 60) + (stmt.length() > 60 ? "..." : ""));
        } else {
            result_.statementsFailed++;
            std::string error = connection_manager_->LastError();
            result_.errors.push_back("Failed: " + stmt + " - " + error);
            AddLogMessage("FAILED: " + stmt.substr(0, 40) + " - " + error, true);
        }
    }
    
    is_executing_ = false;
    execute_btn_->Enable(true);
    cancel_btn_->Enable(false);
    connection_choice_->Enable(true);
    
    // Final update
    UpdateProgress(result_.statementsTotal, result_.statementsTotal, "");
    
    // Summary
    std::string summary = "Execution complete: " + 
                          std::to_string(result_.statementsExecuted) + " succeeded, " +
                          std::to_string(result_.statementsFailed) + " failed";
    AddLogMessage(summary);
    status_text_->SetLabel(summary);
    
    if (should_cancel_) {
        AddLogMessage("Execution was cancelled by user");
    }
    
    result_.success = (result_.statementsFailed == 0) && !should_cancel_;
    return result_.success;
}

bool DdlExecutionDialog::ConnectToDatabase() {
    if (!connection_manager_ || !connections_) return false;
    
    int sel = connection_choice_->GetSelection();
    if (sel < 0 || sel >= static_cast<int>(connections_->size())) return false;
    
    const auto& profile = (*connections_)[sel];
    
    status_text_->SetLabel("Connecting to " + ProfileLabel(profile) + "...");
    
    connection_manager_->Disconnect();
    if (!connection_manager_->Connect(profile)) {
        std::string error = connection_manager_->LastError();
        status_text_->SetLabel("Connection failed");
        AddLogMessage("Connection failed: " + error, true);
        return false;
    }
    
    return true;
}

std::vector<std::string> DdlExecutionDialog::SplitDdlIntoStatements(const std::string& ddl) {
    std::vector<std::string> statements;
    std::string current;
    
    std::istringstream stream(ddl);
    std::string line;
    bool inBlockComment = false;
    
    while (std::getline(stream, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(start, end - start + 1);
        
        // Skip empty lines and single-line comments
        if (trimmed.empty() || trimmed.substr(0, 2) == "--" || trimmed.substr(0, 2) == "//") {
            continue;
        }
        
        // Handle block comments
        if (trimmed.substr(0, 2) == "/*") {
            inBlockComment = true;
        }
        if (inBlockComment) {
            size_t closePos = trimmed.find("*/");
            if (closePos != std::string::npos) {
                inBlockComment = false;
            }
            continue;
        }
        
        current += trimmed + " ";
        
        // Check for statement terminator
        if (!current.empty() && current[current.length() - 2] == ';') {
            // Trim trailing whitespace and add
            size_t last = current.find_last_not_of(" \t\r\n");
            if (last != std::string::npos) {
                statements.push_back(current.substr(0, last + 1));
            }
            current.clear();
        }
    }
    
    // Add any remaining statement
    if (!current.empty()) {
        size_t last = current.find_last_not_of(" \t\r\n");
        if (last != std::string::npos) {
            statements.push_back(current.substr(0, last + 1));
        }
    }
    
    return statements;
}

void DdlExecutionDialog::UpdateProgress(int current, int total, const std::string& statement) {
    CallAfter([this, current, total, statement]() {
        progress_gauge_->SetValue(current);
        count_text_->SetLabel(std::to_string(current) + " / " + std::to_string(total));
        if (!statement.empty()) {
            status_text_->SetLabel("Executing: " + statement.substr(0, 50) + 
                                   (statement.length() > 50 ? "..." : ""));
        }
    });
}

void DdlExecutionDialog::AddLogMessage(const std::string& message, bool isError) {
    CallAfter([this, message, isError]() {
        wxString prefix = isError ? wxString::FromUTF8("[ERROR] ") : wxString::FromUTF8("[INFO] ");
        log_list_->Append(prefix + wxString::FromUTF8(message));
        // Scroll to bottom
        if (log_list_->GetCount() > 0) {
            log_list_->SetSelection(log_list_->GetCount() - 1);
        }
    });
}

void DdlExecutionDialog::OnExecute(wxCommandEvent& event) {
    if (is_executing_) {
        should_cancel_ = true;
        cancel_btn_->Enable(false);
        AddLogMessage("Cancelling...");
    } else {
        Execute();
    }
}

void DdlExecutionDialog::OnCancel(wxCommandEvent& event) {
    if (is_executing_) {
        should_cancel_ = true;
        event.Skip();
    } else {
        EndModal(wxID_CANCEL);
    }
}

void DdlExecutionDialog::OnClose(wxCloseEvent& event) {
    if (is_executing_) {
        should_cancel_ = true;
        // Don't close immediately, wait for execution to stop
        event.Veto();
    } else {
        event.Skip();
    }
}

} // namespace scratchrobin
