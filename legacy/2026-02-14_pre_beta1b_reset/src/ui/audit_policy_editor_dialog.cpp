/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "audit_policy_editor_dialog.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
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

std::string EscapeSqlLiteral(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == '\'') {
            out.push_back('\'');
        }
        out.push_back(ch);
    }
    return out;
}

std::string SqlStringOrNull(const std::string& value) {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return "NULL";
    }
    return "'" + EscapeSqlLiteral(trimmed) + "'";
}

std::string SqlBool(bool value) {
    return value ? "TRUE" : "FALSE";
}

} // namespace

AuditPolicyEditorDialog::AuditPolicyEditorDialog(wxWindow* parent, Mode mode)
    : wxDialog(parent, wxID_ANY,
               mode == Mode::Create ? "Create Audit Policy" : "Edit Audit Policy",
               wxDefaultPosition, wxSize(640, 560),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    BuildLayout();
    UpdateStatementPreview();
}

void AuditPolicyEditorDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    auto* form = new wxFlexGridSizer(2, 8, 12);
    form->AddGrowableCol(1, 1);

    form->Add(new wxStaticText(this, wxID_ANY, "Policy UUID"), 0, wxALIGN_CENTER_VERTICAL);
    policy_id_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(policy_id_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Scope Type"), 0, wxALIGN_CENTER_VERTICAL);
    scope_type_choice_ = new wxChoice(this, wxID_ANY);
    scope_type_choice_->Append("GLOBAL");
    scope_type_choice_->Append("DATABASE");
    scope_type_choice_->Append("SCHEMA");
    scope_type_choice_->Append("TABLE");
    scope_type_choice_->Append("USER");
    scope_type_choice_->SetSelection(0);
    form->Add(scope_type_choice_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Scope UUID"), 0, wxALIGN_CENTER_VERTICAL);
    scope_id_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(scope_id_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Category"), 0, wxALIGN_CENTER_VERTICAL);
    category_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(category_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Event Code"), 0, wxALIGN_CENTER_VERTICAL);
    event_code_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(event_code_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Min Severity"), 0, wxALIGN_CENTER_VERTICAL);
    min_severity_ctrl_ = new wxTextCtrl(this, wxID_ANY, "7");
    form->Add(min_severity_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Audit Flags"), 0, wxALIGN_CENTER_VERTICAL);
    auto* flags = new wxBoxSizer(wxHORIZONTAL);
    audit_select_ctrl_ = new wxCheckBox(this, wxID_ANY, "SELECT");
    audit_insert_ctrl_ = new wxCheckBox(this, wxID_ANY, "INSERT");
    audit_update_ctrl_ = new wxCheckBox(this, wxID_ANY, "UPDATE");
    audit_delete_ctrl_ = new wxCheckBox(this, wxID_ANY, "DELETE");
    audit_select_ctrl_->SetValue(false);
    audit_insert_ctrl_->SetValue(true);
    audit_update_ctrl_->SetValue(true);
    audit_delete_ctrl_->SetValue(true);
    flags->Add(audit_select_ctrl_, 0, wxRIGHT, 8);
    flags->Add(audit_insert_ctrl_, 0, wxRIGHT, 8);
    flags->Add(audit_update_ctrl_, 0, wxRIGHT, 8);
    flags->Add(audit_delete_ctrl_, 0);
    form->Add(flags, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Audit Condition"),
              0, wxALIGN_CENTER_VERTICAL);
    audit_condition_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                           wxTE_MULTILINE);
    form->Add(audit_condition_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Enabled"),
              0, wxALIGN_CENTER_VERTICAL);
    enabled_ctrl_ = new wxCheckBox(this, wxID_ANY, "Is Enabled");
    enabled_ctrl_->SetValue(true);
    form->Add(enabled_ctrl_, 1, wxEXPAND);

    root->Add(form, 1, wxEXPAND | wxALL, 12);

    root->Add(new wxStaticText(this, wxID_ANY, "Generated SQL"), 0, wxLEFT | wxRIGHT, 12);
    preview_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 140),
                                   wxTE_MULTILINE | wxTE_READONLY);
    root->Add(preview_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* buttons = CreateSeparatedButtonSizer(wxOK | wxCANCEL);
    root->Add(buttons, 0, wxEXPAND | wxALL, 12);

    SetSizerAndFit(root);

    auto update = [this](wxCommandEvent&) { UpdateStatementPreview(); };
    policy_id_ctrl_->Bind(wxEVT_TEXT, update);
    scope_type_choice_->Bind(wxEVT_CHOICE, update);
    scope_id_ctrl_->Bind(wxEVT_TEXT, update);
    category_ctrl_->Bind(wxEVT_TEXT, update);
    event_code_ctrl_->Bind(wxEVT_TEXT, update);
    min_severity_ctrl_->Bind(wxEVT_TEXT, update);
    audit_select_ctrl_->Bind(wxEVT_CHECKBOX, update);
    audit_insert_ctrl_->Bind(wxEVT_CHECKBOX, update);
    audit_update_ctrl_->Bind(wxEVT_CHECKBOX, update);
    audit_delete_ctrl_->Bind(wxEVT_CHECKBOX, update);
    audit_condition_ctrl_->Bind(wxEVT_TEXT, update);
    enabled_ctrl_->Bind(wxEVT_CHECKBOX, update);
}

std::string AuditPolicyEditorDialog::BuildStatement() const {
    std::string policy_id = Trim(policy_id_ctrl_ ? policy_id_ctrl_->GetValue().ToStdString() : "");
    std::string scope_type = scope_type_choice_ ? scope_type_choice_->GetStringSelection().ToStdString() : "GLOBAL";
    std::string scope_id = Trim(scope_id_ctrl_ ? scope_id_ctrl_->GetValue().ToStdString() : "");
    std::string category = Trim(category_ctrl_ ? category_ctrl_->GetValue().ToStdString() : "");
    std::string event_code = Trim(event_code_ctrl_ ? event_code_ctrl_->GetValue().ToStdString() : "");
    std::string min_sev = Trim(min_severity_ctrl_ ? min_severity_ctrl_->GetValue().ToStdString() : "7");
    std::string condition = Trim(audit_condition_ctrl_ ? audit_condition_ctrl_->GetValue().ToStdString() : "");

    bool audit_select = audit_select_ctrl_ ? audit_select_ctrl_->GetValue() : false;
    bool audit_insert = audit_insert_ctrl_ ? audit_insert_ctrl_->GetValue() : true;
    bool audit_update = audit_update_ctrl_ ? audit_update_ctrl_->GetValue() : true;
    bool audit_delete = audit_delete_ctrl_ ? audit_delete_ctrl_->GetValue() : true;
    bool enabled = enabled_ctrl_ ? enabled_ctrl_->GetValue() : true;

    std::ostringstream sql;
    if (mode_ == Mode::Create) {
        sql << "INSERT INTO sys.audit_policies (\n"
            << "    policy_uuid, scope_type, scope_uuid, category, event_code,\n"
            << "    min_severity, audit_select, audit_insert, audit_update, audit_delete,\n"
            << "    audit_condition, is_enabled, created_at\n"
            << ") VALUES (\n"
            << "    " << (policy_id.empty() ? "gen_uuid_v7()" : SqlStringOrNull(policy_id)) << ",\n"
            << "    '" << scope_type << "',\n"
            << "    " << (scope_id.empty() ? "NULL" : SqlStringOrNull(scope_id)) << ",\n"
            << "    " << SqlStringOrNull(category) << ",\n"
            << "    " << SqlStringOrNull(event_code) << ",\n"
            << "    " << (min_sev.empty() ? "7" : min_sev) << ",\n"
            << "    " << SqlBool(audit_select) << ",\n"
            << "    " << SqlBool(audit_insert) << ",\n"
            << "    " << SqlBool(audit_update) << ",\n"
            << "    " << SqlBool(audit_delete) << ",\n"
            << "    " << SqlStringOrNull(condition) << ",\n"
            << "    " << SqlBool(enabled) << ",\n"
            << "    CURRENT_TIMESTAMP\n"
            << ");";
    } else {
        sql << "UPDATE sys.audit_policies SET\n"
            << "    scope_type = '" << scope_type << "',\n"
            << "    scope_uuid = " << (scope_id.empty() ? "NULL" : SqlStringOrNull(scope_id)) << ",\n"
            << "    category = " << SqlStringOrNull(category) << ",\n"
            << "    event_code = " << SqlStringOrNull(event_code) << ",\n"
            << "    min_severity = " << (min_sev.empty() ? "7" : min_sev) << ",\n"
            << "    audit_select = " << SqlBool(audit_select) << ",\n"
            << "    audit_insert = " << SqlBool(audit_insert) << ",\n"
            << "    audit_update = " << SqlBool(audit_update) << ",\n"
            << "    audit_delete = " << SqlBool(audit_delete) << ",\n"
            << "    audit_condition = " << SqlStringOrNull(condition) << ",\n"
            << "    is_enabled = " << SqlBool(enabled) << "\n"
            << "WHERE policy_uuid = " << (policy_id.empty() ? "policy_uuid" : SqlStringOrNull(policy_id))
            << ";";
    }
    return sql.str();
}

void AuditPolicyEditorDialog::UpdateStatementPreview() {
    statement_ = BuildStatement();
    if (preview_ctrl_) {
        preview_ctrl_->SetValue(statement_);
    }
}

void AuditPolicyEditorDialog::SetPolicyId(const std::string& id) {
    if (policy_id_ctrl_) {
        policy_id_ctrl_->SetValue(id);
    }
}

void AuditPolicyEditorDialog::SetScopeType(const std::string& scope) {
    if (!scope_type_choice_) {
        return;
    }
    int index = scope_type_choice_->FindString(scope);
    if (index != wxNOT_FOUND) {
        scope_type_choice_->SetSelection(index);
    }
}

void AuditPolicyEditorDialog::SetScopeId(const std::string& scope_id) {
    if (scope_id_ctrl_) {
        scope_id_ctrl_->SetValue(scope_id);
    }
}

void AuditPolicyEditorDialog::SetCategory(const std::string& category) {
    if (category_ctrl_) {
        category_ctrl_->SetValue(category);
    }
}

void AuditPolicyEditorDialog::SetEventCode(const std::string& code) {
    if (event_code_ctrl_) {
        event_code_ctrl_->SetValue(code);
    }
}

void AuditPolicyEditorDialog::SetMinSeverity(const std::string& severity) {
    if (min_severity_ctrl_) {
        min_severity_ctrl_->SetValue(severity);
    }
}

void AuditPolicyEditorDialog::SetAuditCondition(const std::string& condition) {
    if (audit_condition_ctrl_) {
        audit_condition_ctrl_->SetValue(condition);
    }
}

void AuditPolicyEditorDialog::SetAuditSelect(bool value) {
    if (audit_select_ctrl_) {
        audit_select_ctrl_->SetValue(value);
    }
}

void AuditPolicyEditorDialog::SetAuditInsert(bool value) {
    if (audit_insert_ctrl_) {
        audit_insert_ctrl_->SetValue(value);
    }
}

void AuditPolicyEditorDialog::SetAuditUpdate(bool value) {
    if (audit_update_ctrl_) {
        audit_update_ctrl_->SetValue(value);
    }
}

void AuditPolicyEditorDialog::SetAuditDelete(bool value) {
    if (audit_delete_ctrl_) {
        audit_delete_ctrl_->SetValue(value);
    }
}

void AuditPolicyEditorDialog::SetEnabled(bool value) {
    if (enabled_ctrl_) {
        enabled_ctrl_->SetValue(value);
    }
}

std::string AuditPolicyEditorDialog::GetStatement() const {
    return statement_.empty() ? BuildStatement() : statement_;
}

} // namespace scratchrobin
