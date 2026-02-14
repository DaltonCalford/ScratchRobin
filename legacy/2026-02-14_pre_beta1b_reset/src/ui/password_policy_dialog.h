/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_PASSWORD_POLICY_DIALOG_H
#define SCRATCHROBIN_PASSWORD_POLICY_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxTextCtrl;
class wxCheckBox;

namespace scratchrobin {

class PasswordPolicyDialog : public wxDialog {
public:
    explicit PasswordPolicyDialog(wxWindow* parent);

    std::string GetCommand() const;

private:
    void BuildLayout();
    void UpdateCommandPreview();
    std::string BuildCommand() const;

    std::string command_;

    wxTextCtrl* min_length_ctrl_ = nullptr;
    wxTextCtrl* max_length_ctrl_ = nullptr;
    wxCheckBox* require_upper_ctrl_ = nullptr;
    wxCheckBox* require_lower_ctrl_ = nullptr;
    wxCheckBox* require_digit_ctrl_ = nullptr;
    wxCheckBox* require_special_ctrl_ = nullptr;
    wxTextCtrl* min_categories_ctrl_ = nullptr;
    wxCheckBox* no_username_ctrl_ = nullptr;
    wxCheckBox* no_dictionary_ctrl_ = nullptr;
    wxTextCtrl* min_entropy_ctrl_ = nullptr;
    wxTextCtrl* history_count_ctrl_ = nullptr;
    wxTextCtrl* min_age_ctrl_ = nullptr;
    wxTextCtrl* max_age_ctrl_ = nullptr;
    wxTextCtrl* warning_days_ctrl_ = nullptr;
    wxTextCtrl* preview_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PASSWORD_POLICY_DIALOG_H
