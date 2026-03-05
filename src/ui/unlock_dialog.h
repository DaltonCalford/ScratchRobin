/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>

namespace scratchrobin::ui {

// Dialog for entering the decryption key/password
// Shown when encrypted passwords need to be decrypted
class UnlockDialog : public wxDialog {
public:
    UnlockDialog(wxWindow* parent, bool has_encrypted_passwords);
    
    // Show dialog and return entered key
    // Returns empty string if cancelled
    wxString ShowAndGetKey();
    
    // Check if user wants to save the key for this session
    bool rememberKeyForSession() const;

private:
    enum class ControlId {
        kPassword = wxID_HIGHEST + 400,
        kRememberSession,
        kSkipDecrypt,
    };
    
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onSkipDecrypt(wxCommandEvent& event);
    
    wxTextCtrl* password_ctrl_{nullptr};
    wxCheckBox* remember_check_{nullptr};
    wxButton* skip_btn_{nullptr};
    
    wxString entered_key_;
    bool remember_for_session_{false};
    
    DECLARE_EVENT_TABLE()
};

}  // namespace scratchrobin::ui
