/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ERROR_DIALOG_H
#define SCRATCHROBIN_ERROR_DIALOG_H

#include <wx/dialog.h>

#include "core/error_handler.h"

class wxButton;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

class ErrorDialog : public wxDialog {
public:
    ErrorDialog(wxWindow* parent, const ErrorInfo& error);
    
    void ShowError();
    
private:
    void BuildLayout();
    void UpdateDetailsVisibility();
    void OnCopyToClipboard(wxCommandEvent& event);
    void OnToggleDetails(wxCommandEvent& event);
    void OnRetry(wxCommandEvent& event);
    
    wxString FormatErrorMessage() const;
    wxString FormatDetails() const;
    wxColour GetSeverityColor() const;
    wxString GetSeverityIcon() const;
    
    ErrorInfo error_;
    
    // UI elements
    wxStaticText* icon_label_ = nullptr;
    wxStaticText* title_label_ = nullptr;
    wxStaticText* message_label_ = nullptr;
    wxStaticText* hint_label_ = nullptr;
    wxStaticText* code_label_ = nullptr;
    wxTextCtrl* details_ctrl_ = nullptr;
    wxButton* details_button_ = nullptr;
    wxButton* retry_button_ = nullptr;
    
    bool details_visible_ = false;
    
    wxDECLARE_EVENT_TABLE();
};

// Convenience function to show error dialog
void ShowErrorDialog(wxWindow* parent, const ErrorInfo& error);
void ShowErrorDialog(wxWindow* parent, const std::string& message, 
                     const std::string& title = "Error");

} // namespace scratchrobin

#endif // SCRATCHROBIN_ERROR_DIALOG_H
