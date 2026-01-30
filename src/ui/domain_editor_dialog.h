/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DOMAIN_EDITOR_DIALOG_H
#define SCRATCHROBIN_DOMAIN_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class DomainEditorMode {
    Create,
    Alter
};

class DomainEditorDialog : public wxDialog {
public:
    DomainEditorDialog(wxWindow* parent, DomainEditorMode mode);

    std::string BuildSql() const;
    std::string domain_name() const;

    void SetDomainName(const std::string& name);

private:
    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    std::string BuildConstraintsClause() const;
    std::string BuildWithClause() const;
    std::string FormatDomainPath(const std::string& value) const;
    void UpdateKindFields();
    void UpdateAlterActionFields();

    DomainEditorMode mode_;

    wxTextCtrl* name_ctrl_ = nullptr;
    wxCheckBox* if_not_exists_ctrl_ = nullptr;
    wxChoice* kind_choice_ = nullptr;

    wxTextCtrl* base_type_ctrl_ = nullptr;
    wxTextCtrl* record_fields_ctrl_ = nullptr;
    wxTextCtrl* enum_values_ctrl_ = nullptr;
    wxTextCtrl* set_element_ctrl_ = nullptr;
    wxTextCtrl* variant_types_ctrl_ = nullptr;

    wxTextCtrl* inherits_ctrl_ = nullptr;
    wxTextCtrl* collation_ctrl_ = nullptr;
    wxCheckBox* not_null_ctrl_ = nullptr;
    wxTextCtrl* default_ctrl_ = nullptr;
    wxTextCtrl* check_ctrl_ = nullptr;
    wxTextCtrl* extra_constraints_ctrl_ = nullptr;
    wxTextCtrl* with_clause_ctrl_ = nullptr;

    wxChoice* alter_action_choice_ = nullptr;
    wxStaticText* alter_value_label_ = nullptr;
    wxTextCtrl* alter_value_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DOMAIN_EDITOR_DIALOG_H
