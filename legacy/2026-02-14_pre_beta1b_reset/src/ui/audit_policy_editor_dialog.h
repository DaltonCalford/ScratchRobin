/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_AUDIT_POLICY_EDITOR_DIALOG_H
#define SCRATCHROBIN_AUDIT_POLICY_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxTextCtrl;
class wxChoice;
class wxCheckBox;

namespace scratchrobin {

class AuditPolicyEditorDialog : public wxDialog {
public:
    enum class Mode {
        Create,
        Edit
    };

    explicit AuditPolicyEditorDialog(wxWindow* parent, Mode mode);

    void SetPolicyId(const std::string& id);
    void SetScopeType(const std::string& scope);
    void SetScopeId(const std::string& scope_id);
    void SetCategory(const std::string& category);
    void SetEventCode(const std::string& code);
    void SetMinSeverity(const std::string& severity);
    void SetAuditCondition(const std::string& condition);
    void SetAuditSelect(bool value);
    void SetAuditInsert(bool value);
    void SetAuditUpdate(bool value);
    void SetAuditDelete(bool value);
    void SetEnabled(bool value);

    std::string GetStatement() const;

private:
    void BuildLayout();
    void UpdateStatementPreview();
    std::string BuildStatement() const;

    Mode mode_;
    std::string statement_;

    wxTextCtrl* policy_id_ctrl_ = nullptr;
    wxChoice* scope_type_choice_ = nullptr;
    wxTextCtrl* scope_id_ctrl_ = nullptr;
    wxTextCtrl* category_ctrl_ = nullptr;
    wxTextCtrl* event_code_ctrl_ = nullptr;
    wxTextCtrl* min_severity_ctrl_ = nullptr;
    wxCheckBox* audit_select_ctrl_ = nullptr;
    wxCheckBox* audit_insert_ctrl_ = nullptr;
    wxCheckBox* audit_update_ctrl_ = nullptr;
    wxCheckBox* audit_delete_ctrl_ = nullptr;
    wxTextCtrl* audit_condition_ctrl_ = nullptr;
    wxCheckBox* enabled_ctrl_ = nullptr;
    wxTextCtrl* preview_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_AUDIT_POLICY_EDITOR_DIALOG_H
