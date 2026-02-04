/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "error_dialog.h"

#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

wxBEGIN_EVENT_TABLE(ErrorDialog, wxDialog)
    EVT_BUTTON(wxID_HIGHEST + 100, ErrorDialog::OnCopyToClipboard)
    EVT_BUTTON(wxID_HIGHEST + 101, ErrorDialog::OnToggleDetails)
    EVT_BUTTON(wxID_HIGHEST + 102, ErrorDialog::OnRetry)
wxEND_EVENT_TABLE()

ErrorDialog::ErrorDialog(wxWindow* parent, const ErrorInfo& error)
    : wxDialog(parent, wxID_ANY, "Error", wxDefaultPosition, wxSize(500, 300)),
      error_(error) {
    
    // Set dialog title based on severity
    switch (error.severity) {
        case ErrorSeverity::Fatal:
            SetTitle("Fatal Error");
            break;
        case ErrorSeverity::Error:
            SetTitle("Error");
            break;
        case ErrorSeverity::Warning:
            SetTitle("Warning");
            break;
        case ErrorSeverity::Notice:
            SetTitle("Notice");
            break;
    }
    
    BuildLayout();
    UpdateDetailsVisibility();
}

void ErrorDialog::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    
    // Header with icon and title
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Severity icon
    icon_label_ = new wxStaticText(this, wxID_ANY, GetSeverityIcon());
    icon_label_->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    headerSizer->Add(icon_label_, 0, wxALL, 10);
    
    // Title and message
    auto* textSizer = new wxBoxSizer(wxVERTICAL);
    
    title_label_ = new wxStaticText(this, wxID_ANY, 
        ErrorMapper::GetUserMessage(error_.code));
    title_label_->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    textSizer->Add(title_label_, 0, wxEXPAND | wxTOP | wxRIGHT, 10);
    
    // Main error message
    message_label_ = new wxStaticText(this, wxID_ANY, error_.message);
    message_label_->Wrap(400);
    textSizer->Add(message_label_, 0, wxEXPAND | wxTOP | wxRIGHT, 5);
    
    // Hint (if available)
    if (!error_.hint.empty()) {
        hint_label_ = new wxStaticText(this, wxID_ANY, 
            "Hint: " + error_.hint);
        hint_label_->Wrap(400);
        hint_label_->SetForegroundColour(wxColour(0, 100, 0));
        textSizer->Add(hint_label_, 0, wxEXPAND | wxTOP | wxRIGHT, 10);
    }
    
    headerSizer->Add(textSizer, 1, wxEXPAND);
    rootSizer->Add(headerSizer, 0, wxEXPAND | wxALL, 10);
    
    // Error code info
    wxString codeText;
    if (!error_.code.empty() && error_.code != "SR-0000") {
        codeText = "Error Code: " + error_.code;
        if (!error_.sqlState.empty()) {
            codeText += " (SQLSTATE: " + error_.sqlState + ")";
        } else if (!error_.backendCode.empty()) {
            codeText += " (" + error_.backend + ": " + error_.backendCode + ")";
        }
    }
    if (!codeText.IsEmpty()) {
        code_label_ = new wxStaticText(this, wxID_ANY, codeText);
        code_label_->SetForegroundColour(wxColour(100, 100, 100));
        code_label_->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        rootSizer->Add(code_label_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    }
    
    // Details text (collapsible)
    details_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, 
                                   wxSize(-1, 150), wxTE_MULTILINE | wxTE_READONLY);
    details_ctrl_->SetValue(FormatDetails());
    rootSizer->Add(details_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Button row
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left side: Utility buttons
    auto* copyButton = new wxButton(this, wxID_HIGHEST + 100, "Copy to Clipboard");
    buttonSizer->Add(copyButton, 0, wxRIGHT, 5);
    
    details_button_ = new wxButton(this, wxID_HIGHEST + 101, "Show Details");
    buttonSizer->Add(details_button_, 0, wxRIGHT, 5);
    
    // Retry button (only for retryable errors)
    if (error_.IsRetryable()) {
        retry_button_ = new wxButton(this, wxID_HIGHEST + 102, "Retry");
        buttonSizer->Add(retry_button_, 0, wxRIGHT, 5);
    }
    
    buttonSizer->AddStretchSpacer(1);
    
    // Right side: OK button
    auto* okButton = new wxButton(this, wxID_OK, "OK");
    okButton->SetDefault();
    buttonSizer->Add(okButton, 0);
    
    rootSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    SetSizer(rootSizer);
    Fit();
}

void ErrorDialog::UpdateDetailsVisibility() {
    if (!details_ctrl_ || !details_button_) return;
    
    details_ctrl_->Show(details_visible_);
    details_button_->SetLabel(details_visible_ ? "Hide Details" : "Show Details");
    
    if (details_visible_) {
        SetSize(wxSize(500, 450));
    } else {
        SetSize(wxSize(500, 300));
        Fit();
    }
    
    Layout();
}

wxString ErrorDialog::FormatErrorMessage() const {
    wxString text;
    text += title_label_->GetLabel() + "\n\n";
    text += message_label_->GetLabel() + "\n";
    if (hint_label_) {
        text += "\nHint: " + hint_label_->GetLabel() + "\n";
    }
    if (code_label_) {
        text += "\n" + code_label_->GetLabel() + "\n";
    }
    return text;
}

wxString ErrorDialog::FormatDetails() const {
    wxString details;
    
    details += "Error Details:\n";
    details += "==============\n\n";
    
    if (!error_.code.empty()) {
        details += "ScratchRobin Code: " + error_.code + "\n";
    }
    if (!error_.backend.empty()) {
        details += "Backend: " + error_.backend + "\n";
    }
    if (!error_.sqlState.empty()) {
        details += "SQLSTATE: " + error_.sqlState + "\n";
    }
    if (!error_.backendCode.empty()) {
        details += "Backend Code: " + error_.backendCode + "\n";
    }
    if (!error_.connection.empty()) {
        details += "Connection: " + error_.connection + "\n";
    }
    
    details += "\nMessage:\n";
    details += error_.message + "\n";
    
    if (!error_.detail.empty() && error_.detail != error_.message) {
        details += "\nDetail:\n";
        details += error_.detail + "\n";
    }
    
    if (!error_.hint.empty()) {
        details += "\nHint:\n";
        details += error_.hint + "\n";
    }
    
    if (!error_.sql.empty()) {
        details += "\nSQL Statement:\n";
        details += error_.sql + "\n";
    }
    
    details += "\nTimestamp:\n";
    auto time_t = std::chrono::system_clock::to_time_t(error_.timestamp);
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
    details += buffer;
    details += "\n";
    
    return details;
}

wxColour ErrorDialog::GetSeverityColor() const {
    switch (error_.severity) {
        case ErrorSeverity::Fatal:
            return wxColour(220, 53, 69);   // Red
        case ErrorSeverity::Error:
            return wxColour(255, 193, 7);   // Yellow
        case ErrorSeverity::Warning:
            return wxColour(255, 165, 0);   // Orange
        case ErrorSeverity::Notice:
            return wxColour(108, 117, 125); // Gray
    }
    return wxColour(0, 0, 0);
}

wxString ErrorDialog::GetSeverityIcon() const {
    switch (error_.severity) {
        case ErrorSeverity::Fatal:
            return "\u274C";  // ❌
        case ErrorSeverity::Error:
            return "\u26A0";  // ⚠️
        case ErrorSeverity::Warning:
            return "\u26A1";  // ⚡
        case ErrorSeverity::Notice:
            return "\u2139";  // ℹ️
    }
    return "";
}

void ErrorDialog::OnCopyToClipboard(wxCommandEvent&) {
    wxString text = FormatDetails();
    
    if (wxTheClipboard && wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
    }
}

void ErrorDialog::OnToggleDetails(wxCommandEvent&) {
    details_visible_ = !details_visible_;
    UpdateDetailsVisibility();
}

void ErrorDialog::OnRetry(wxCommandEvent&) {
    EndModal(wxID_RETRY);
}

void ErrorDialog::ShowError() {
    ShowModal();
}

// =============================================================================
// Convenience Functions
// =============================================================================

void ShowErrorDialog(wxWindow* parent, const ErrorInfo& error) {
    ErrorDialog dialog(parent, error);
    dialog.ShowError();
}

void ShowErrorDialog(wxWindow* parent, const std::string& message, 
                     const std::string& title) {
    ErrorInfo error;
    error.message = message;
    error.severity = ErrorSeverity::Error;
    error.category = ErrorCategory::Unknown;
    error.code = "SR-0000";
    error.timestamp = std::chrono::system_clock::now();
    
    ErrorDialog dialog(parent, error);
    dialog.SetTitle(title);
    dialog.ShowError();
}

} // namespace scratchrobin
