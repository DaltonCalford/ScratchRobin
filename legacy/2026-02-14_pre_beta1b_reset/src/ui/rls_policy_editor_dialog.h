/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_RLS_POLICY_EDITOR_DIALOG_H
#define SCRATCHROBIN_RLS_POLICY_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxTextCtrl;
class wxChoice;
class wxCheckBox;

namespace scratchrobin {

class RlsPolicyEditorDialog : public wxDialog {
public:
    enum class Mode {
        Create,
        Edit
    };

    explicit RlsPolicyEditorDialog(wxWindow* parent, Mode mode);

    void SetPolicyName(const std::string& name);
    void SetTableName(const std::string& name);
    void SetPolicyType(const std::string& type);
    void SetRoles(const std::string& roles);
    void SetUsingExpr(const std::string& expr);
    void SetWithCheckExpr(const std::string& expr);
    void SetPolicyMode(const std::string& mode);
    void SetEnableRlsOnTable(bool enable);

    std::string GetStatement() const;

private:
    void BuildLayout();
    void UpdateStatementPreview();
    std::string BuildStatement() const;

    Mode mode_;
    std::string statement_;

    wxTextCtrl* policy_name_ctrl_ = nullptr;
    wxTextCtrl* table_name_ctrl_ = nullptr;
    wxChoice* policy_type_choice_ = nullptr;
    wxTextCtrl* roles_ctrl_ = nullptr;
    wxTextCtrl* using_expr_ctrl_ = nullptr;
    wxTextCtrl* with_check_expr_ctrl_ = nullptr;
    wxChoice* policy_mode_choice_ = nullptr;
    wxCheckBox* enable_rls_check_ = nullptr;
    wxTextCtrl* preview_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_RLS_POLICY_EDITOR_DIALOG_H
