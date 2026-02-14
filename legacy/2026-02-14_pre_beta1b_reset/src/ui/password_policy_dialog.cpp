/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "password_policy_dialog.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

} // namespace

PasswordPolicyDialog::PasswordPolicyDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Password Policy",
               wxDefaultPosition, wxSize(620, 520),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    BuildLayout();
    UpdateCommandPreview();
}

void PasswordPolicyDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    auto* form = new wxFlexGridSizer(2, 8, 12);
    form->AddGrowableCol(1, 1);

    form->Add(new wxStaticText(this, wxID_ANY, "Min Length"), 0, wxALIGN_CENTER_VERTICAL);
    min_length_ctrl_ = new wxTextCtrl(this, wxID_ANY, "12");
    form->Add(min_length_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Max Length"), 0, wxALIGN_CENTER_VERTICAL);
    max_length_ctrl_ = new wxTextCtrl(this, wxID_ANY, "128");
    form->Add(max_length_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Require Uppercase"), 0, wxALIGN_CENTER_VERTICAL);
    require_upper_ctrl_ = new wxCheckBox(this, wxID_ANY, "Uppercase");
    require_upper_ctrl_->SetValue(true);
    form->Add(require_upper_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Require Lowercase"), 0, wxALIGN_CENTER_VERTICAL);
    require_lower_ctrl_ = new wxCheckBox(this, wxID_ANY, "Lowercase");
    require_lower_ctrl_->SetValue(true);
    form->Add(require_lower_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Require Digit"), 0, wxALIGN_CENTER_VERTICAL);
    require_digit_ctrl_ = new wxCheckBox(this, wxID_ANY, "Digit");
    require_digit_ctrl_->SetValue(true);
    form->Add(require_digit_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Require Special"), 0, wxALIGN_CENTER_VERTICAL);
    require_special_ctrl_ = new wxCheckBox(this, wxID_ANY, "Special");
    require_special_ctrl_->SetValue(true);
    form->Add(require_special_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Min Categories"), 0, wxALIGN_CENTER_VERTICAL);
    min_categories_ctrl_ = new wxTextCtrl(this, wxID_ANY, "3");
    form->Add(min_categories_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Disallow Username"), 0, wxALIGN_CENTER_VERTICAL);
    no_username_ctrl_ = new wxCheckBox(this, wxID_ANY, "No username in password");
    no_username_ctrl_->SetValue(true);
    form->Add(no_username_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Disallow Dictionary"), 0, wxALIGN_CENTER_VERTICAL);
    no_dictionary_ctrl_ = new wxCheckBox(this, wxID_ANY, "No dictionary words");
    no_dictionary_ctrl_->SetValue(true);
    form->Add(no_dictionary_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Min Entropy Bits"), 0, wxALIGN_CENTER_VERTICAL);
    min_entropy_ctrl_ = new wxTextCtrl(this, wxID_ANY, "60");
    form->Add(min_entropy_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Password History Count"), 0, wxALIGN_CENTER_VERTICAL);
    history_count_ctrl_ = new wxTextCtrl(this, wxID_ANY, "10");
    form->Add(history_count_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Min Age (days)"), 0, wxALIGN_CENTER_VERTICAL);
    min_age_ctrl_ = new wxTextCtrl(this, wxID_ANY, "1");
    form->Add(min_age_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Max Age (days)"), 0, wxALIGN_CENTER_VERTICAL);
    max_age_ctrl_ = new wxTextCtrl(this, wxID_ANY, "90");
    form->Add(max_age_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Warning Days"), 0, wxALIGN_CENTER_VERTICAL);
    warning_days_ctrl_ = new wxTextCtrl(this, wxID_ANY, "14");
    form->Add(warning_days_ctrl_, 1, wxEXPAND);

    root->Add(form, 1, wxEXPAND | wxALL, 12);

    root->Add(new wxStaticText(this, wxID_ANY, "Generated Command"), 0, wxLEFT | wxRIGHT, 12);
    preview_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 140),
                                   wxTE_MULTILINE | wxTE_READONLY);
    root->Add(preview_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* buttons = CreateSeparatedButtonSizer(wxOK | wxCANCEL);
    root->Add(buttons, 0, wxEXPAND | wxALL, 12);

    SetSizerAndFit(root);

    auto update = [this](wxCommandEvent&) { UpdateCommandPreview(); };
    min_length_ctrl_->Bind(wxEVT_TEXT, update);
    max_length_ctrl_->Bind(wxEVT_TEXT, update);
    require_upper_ctrl_->Bind(wxEVT_CHECKBOX, update);
    require_lower_ctrl_->Bind(wxEVT_CHECKBOX, update);
    require_digit_ctrl_->Bind(wxEVT_CHECKBOX, update);
    require_special_ctrl_->Bind(wxEVT_CHECKBOX, update);
    min_categories_ctrl_->Bind(wxEVT_TEXT, update);
    no_username_ctrl_->Bind(wxEVT_CHECKBOX, update);
    no_dictionary_ctrl_->Bind(wxEVT_CHECKBOX, update);
    min_entropy_ctrl_->Bind(wxEVT_TEXT, update);
    history_count_ctrl_->Bind(wxEVT_TEXT, update);
    min_age_ctrl_->Bind(wxEVT_TEXT, update);
    max_age_ctrl_->Bind(wxEVT_TEXT, update);
    warning_days_ctrl_->Bind(wxEVT_TEXT, update);
}

std::string PasswordPolicyDialog::BuildCommand() const {
    std::ostringstream cmd;
    cmd << "sb_security password-policy";

    auto add_flag = [&cmd](const std::string& name, const std::string& value) {
        std::string trimmed = Trim(value);
        if (!trimmed.empty()) {
            cmd << " --" << name << " " << trimmed;
        }
    };

    add_flag("min-length", min_length_ctrl_ ? min_length_ctrl_->GetValue().ToStdString() : "");
    add_flag("max-length", max_length_ctrl_ ? max_length_ctrl_->GetValue().ToStdString() : "");
    if (require_upper_ctrl_ && require_upper_ctrl_->GetValue()) cmd << " --require-upper";
    if (require_lower_ctrl_ && require_lower_ctrl_->GetValue()) cmd << " --require-lower";
    if (require_digit_ctrl_ && require_digit_ctrl_->GetValue()) cmd << " --require-digit";
    if (require_special_ctrl_ && require_special_ctrl_->GetValue()) cmd << " --require-special";
    add_flag("min-categories", min_categories_ctrl_ ? min_categories_ctrl_->GetValue().ToStdString() : "");
    if (no_username_ctrl_ && no_username_ctrl_->GetValue()) cmd << " --no-username";
    if (no_dictionary_ctrl_ && no_dictionary_ctrl_->GetValue()) cmd << " --no-dictionary";
    add_flag("min-entropy", min_entropy_ctrl_ ? min_entropy_ctrl_->GetValue().ToStdString() : "");
    add_flag("history-count", history_count_ctrl_ ? history_count_ctrl_->GetValue().ToStdString() : "");
    add_flag("min-age-days", min_age_ctrl_ ? min_age_ctrl_->GetValue().ToStdString() : "");
    add_flag("max-age-days", max_age_ctrl_ ? max_age_ctrl_->GetValue().ToStdString() : "");
    add_flag("warning-days", warning_days_ctrl_ ? warning_days_ctrl_->GetValue().ToStdString() : "");

    cmd << "\n\n# NOTE: Password policy changes are security-governed in ScratchBird.\n"
        << "# Apply via cluster policy update or sb_security tooling.";
    return cmd.str();
}

void PasswordPolicyDialog::UpdateCommandPreview() {
    command_ = BuildCommand();
    if (preview_ctrl_) {
        preview_ctrl_->SetValue(command_);
    }
}

std::string PasswordPolicyDialog::GetCommand() const {
    return command_.empty() ? BuildCommand() : command_;
}

} // namespace scratchrobin
