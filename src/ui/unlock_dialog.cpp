/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/unlock_dialog.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/statbmp.h>
#include <wx/msgdlg.h>
#include <wx/artprov.h>

namespace scratchrobin::ui {

BEGIN_EVENT_TABLE(UnlockDialog, wxDialog)
    EVT_BUTTON(wxID_OK, UnlockDialog::onOK)
    EVT_BUTTON(wxID_CANCEL, UnlockDialog::onCancel)
END_EVENT_TABLE()

UnlockDialog::UnlockDialog(wxWindow* parent, bool has_encrypted_passwords)
    : wxDialog(parent, wxID_ANY, wxT("Unlock Configuration"), wxDefaultPosition,
               wxSize(450, 250), wxDEFAULT_DIALOG_STYLE | wxCENTER) {
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Header with icon
    wxBoxSizer* header_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxStaticBitmap* icon = new wxStaticBitmap(this, wxID_ANY,
        wxArtProvider::GetBitmap(wxART_WARNING, wxART_MESSAGE_BOX, wxSize(48, 48)));
    header_sizer->Add(icon, 0, wxALL, 15);
    
    wxBoxSizer* text_sizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* title = new wxStaticText(this, wxID_ANY, 
        wxT("Enter Decryption Key"));
    wxFont title_font = title->GetFont();
    title_font.SetPointSize(14);
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(title_font);
    text_sizer->Add(title, 0, wxTOP | wxRIGHT, 10);
    
    wxString desc_text;
    if (has_encrypted_passwords) {
        desc_text = wxT("This configuration contains encrypted passwords.\n"
                       "Enter your decryption key to unlock them.");
    } else {
        desc_text = wxT("Enter a decryption key for this session.\n"
                       "This key will be used to encrypt saved passwords.");
    }
    wxStaticText* desc = new wxStaticText(this, wxID_ANY, desc_text);
    desc->Wrap(300);
    text_sizer->Add(desc, 0, wxTOP | wxRIGHT, 10);
    
    header_sizer->Add(text_sizer, 1, wxEXPAND);
    main_sizer->Add(header_sizer, 0, wxEXPAND);
    
    // Password input
    wxBoxSizer* password_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* password_label = new wxStaticText(this, wxID_ANY, wxT("Key:"));
    password_sizer->Add(password_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    password_ctrl_ = new wxTextCtrl(this, static_cast<int>(ControlId::kPassword),
                                    wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                    wxTE_PASSWORD);
    password_sizer->Add(password_ctrl_, 1, wxEXPAND);
    main_sizer->Add(password_sizer, 0, wxEXPAND | wxALL, 15);
    
    // Remember checkbox
    remember_check_ = new wxCheckBox(this, static_cast<int>(ControlId::kRememberSession),
                                     wxT("Remember key for this session"));
    remember_check_->SetValue(true);
    main_sizer->Add(remember_check_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Buttons
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddStretchSpacer();
    
    if (has_encrypted_passwords) {
        skip_btn_ = new wxButton(this, static_cast<int>(ControlId::kSkipDecrypt),
                                 wxT("Skip (No Auto-Connect)"));
        button_sizer->Add(skip_btn_, 0, wxRIGHT, 5);
    }
    
    wxButton* ok_btn = new wxButton(this, wxID_OK, wxT("Unlock"));
    ok_btn->SetDefault();
    button_sizer->Add(ok_btn, 0, wxRIGHT, 5);
    
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, wxT("Cancel"));
    button_sizer->Add(cancel_btn, 0);
    
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 15);
    
    SetSizer(main_sizer);
    
    // Focus password field
    password_ctrl_->SetFocus();
}

wxString UnlockDialog::ShowAndGetKey() {
    if (ShowModal() == wxID_OK) {
        return entered_key_;
    }
    return wxEmptyString;
}

bool UnlockDialog::rememberKeyForSession() const {
    return remember_for_session_;
}

void UnlockDialog::onOK(wxCommandEvent& event) {
    entered_key_ = password_ctrl_->GetValue();
    
    if (entered_key_.IsEmpty()) {
        wxMessageBox(wxT("Please enter a decryption key."), 
                     wxT("Key Required"), wxOK | wxICON_WARNING, this);
        return;
    }
    
    remember_for_session_ = remember_check_->IsChecked();
    EndModal(wxID_OK);
}

void UnlockDialog::onCancel(wxCommandEvent& event) {
    entered_key_.Clear();
    EndModal(wxID_CANCEL);
}

}  // namespace scratchrobin::ui
