/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "rls_policy_editor_dialog.h"

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

bool IsSimpleIdentifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    if (!(std::isalpha(static_cast<unsigned char>(value[0])) || value[0] == '_')) {
        return false;
    }
    for (char ch : value) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
            return false;
        }
    }
    return true;
}

std::string QuoteIdentifier(const std::string& value) {
    if (IsSimpleIdentifier(value)) {
        return value;
    }
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') {
            out.push_back('"');
        }
        out.push_back(ch);
    }
    out.push_back('"');
    return out;
}

} // namespace

RlsPolicyEditorDialog::RlsPolicyEditorDialog(wxWindow* parent, Mode mode)
    : wxDialog(parent, wxID_ANY,
               mode == Mode::Create ? "Create RLS Policy" : "Edit RLS Policy",
               wxDefaultPosition, wxSize(640, 560),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    BuildLayout();
    UpdateStatementPreview();
}

void RlsPolicyEditorDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    auto* form = new wxFlexGridSizer(2, 8, 12);
    form->AddGrowableCol(1, 1);

    form->Add(new wxStaticText(this, wxID_ANY, "Policy Name"), 0, wxALIGN_CENTER_VERTICAL);
    policy_name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(policy_name_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Table"), 0, wxALIGN_CENTER_VERTICAL);
    table_name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(table_name_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Policy Mode"), 0, wxALIGN_CENTER_VERTICAL);
    policy_mode_choice_ = new wxChoice(this, wxID_ANY);
    policy_mode_choice_->Append("PERMISSIVE");
    policy_mode_choice_->Append("RESTRICTIVE");
    policy_mode_choice_->SetSelection(0);
    form->Add(policy_mode_choice_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Policy Type"), 0, wxALIGN_CENTER_VERTICAL);
    policy_type_choice_ = new wxChoice(this, wxID_ANY);
    policy_type_choice_->Append("ALL");
    policy_type_choice_->Append("SELECT");
    policy_type_choice_->Append("INSERT");
    policy_type_choice_->Append("UPDATE");
    policy_type_choice_->Append("DELETE");
    policy_type_choice_->SetSelection(0);
    form->Add(policy_type_choice_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Roles (comma-separated)"),
              0, wxALIGN_CENTER_VERTICAL);
    roles_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(roles_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "USING Expression"),
              0, wxALIGN_CENTER_VERTICAL);
    using_expr_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                      wxTE_MULTILINE);
    form->Add(using_expr_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "WITH CHECK Expression"),
              0, wxALIGN_CENTER_VERTICAL);
    with_check_expr_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                           wxTE_MULTILINE);
    form->Add(with_check_expr_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Enable RLS on Table"),
              0, wxALIGN_CENTER_VERTICAL);
    enable_rls_check_ = new wxCheckBox(this, wxID_ANY, "Apply ALTER TABLE ... ENABLE ROW LEVEL SECURITY");
    enable_rls_check_->SetValue(true);
    form->Add(enable_rls_check_, 1, wxEXPAND);

    root->Add(form, 1, wxEXPAND | wxALL, 12);

    root->Add(new wxStaticText(this, wxID_ANY, "Generated SQL"), 0, wxLEFT | wxRIGHT, 12);
    preview_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 140),
                                   wxTE_MULTILINE | wxTE_READONLY);
    root->Add(preview_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* buttons = CreateSeparatedButtonSizer(wxOK | wxCANCEL);
    root->Add(buttons, 0, wxEXPAND | wxALL, 12);

    SetSizerAndFit(root);

    auto update = [this](wxCommandEvent&) { UpdateStatementPreview(); };
    policy_name_ctrl_->Bind(wxEVT_TEXT, update);
    table_name_ctrl_->Bind(wxEVT_TEXT, update);
    policy_mode_choice_->Bind(wxEVT_CHOICE, update);
    policy_type_choice_->Bind(wxEVT_CHOICE, update);
    roles_ctrl_->Bind(wxEVT_TEXT, update);
    using_expr_ctrl_->Bind(wxEVT_TEXT, update);
    with_check_expr_ctrl_->Bind(wxEVT_TEXT, update);
    enable_rls_check_->Bind(wxEVT_CHECKBOX, update);
}

void RlsPolicyEditorDialog::UpdateStatementPreview() {
    statement_ = BuildStatement();
    if (preview_ctrl_) {
        preview_ctrl_->SetValue(statement_);
    }
}

std::string RlsPolicyEditorDialog::BuildStatement() const {
    std::string name = Trim(policy_name_ctrl_ ? policy_name_ctrl_->GetValue().ToStdString() : "");
    std::string table = Trim(table_name_ctrl_ ? table_name_ctrl_->GetValue().ToStdString() : "");
    std::string roles = Trim(roles_ctrl_ ? roles_ctrl_->GetValue().ToStdString() : "");
    std::string using_expr = Trim(using_expr_ctrl_ ? using_expr_ctrl_->GetValue().ToStdString() : "");
    std::string with_check = Trim(with_check_expr_ctrl_ ? with_check_expr_ctrl_->GetValue().ToStdString() : "");
    std::string mode = policy_mode_choice_ ? policy_mode_choice_->GetStringSelection().ToStdString() : "PERMISSIVE";
    std::string type = policy_type_choice_ ? policy_type_choice_->GetStringSelection().ToStdString() : "ALL";

    std::ostringstream sql;
    sql << "CREATE POLICY " << QuoteIdentifier(name.empty() ? "policy_name" : name)
        << " ON " << QuoteIdentifier(table.empty() ? "table_name" : table) << "\n";
    if (!mode.empty()) {
        sql << "  AS " << mode << "\n";
    }
    if (!type.empty() && type != "ALL") {
        sql << "  FOR " << type << "\n";
    }
    if (!roles.empty()) {
        sql << "  TO " << roles << "\n";
    }
    if (!using_expr.empty()) {
        sql << "  USING (" << using_expr << ")\n";
    }
    if (!with_check.empty()) {
        sql << "  WITH CHECK (" << with_check << ")\n";
    }
    sql << ";";

    bool enable_rls = enable_rls_check_ ? enable_rls_check_->GetValue() : false;
    if (enable_rls) {
        sql << "\n\nALTER TABLE " << QuoteIdentifier(table.empty() ? "table_name" : table)
            << " ENABLE ROW LEVEL SECURITY;";
    }

    return sql.str();
}

void RlsPolicyEditorDialog::SetPolicyName(const std::string& name) {
    if (policy_name_ctrl_) {
        policy_name_ctrl_->SetValue(name);
    }
}

void RlsPolicyEditorDialog::SetTableName(const std::string& name) {
    if (table_name_ctrl_) {
        table_name_ctrl_->SetValue(name);
    }
}

void RlsPolicyEditorDialog::SetPolicyType(const std::string& type) {
    if (!policy_type_choice_) {
        return;
    }
    int index = policy_type_choice_->FindString(type);
    if (index != wxNOT_FOUND) {
        policy_type_choice_->SetSelection(index);
    }
}

void RlsPolicyEditorDialog::SetRoles(const std::string& roles) {
    if (roles_ctrl_) {
        roles_ctrl_->SetValue(roles);
    }
}

void RlsPolicyEditorDialog::SetUsingExpr(const std::string& expr) {
    if (using_expr_ctrl_) {
        using_expr_ctrl_->SetValue(expr);
    }
}

void RlsPolicyEditorDialog::SetWithCheckExpr(const std::string& expr) {
    if (with_check_expr_ctrl_) {
        with_check_expr_ctrl_->SetValue(expr);
    }
}

void RlsPolicyEditorDialog::SetPolicyMode(const std::string& mode) {
    if (!policy_mode_choice_) {
        return;
    }
    int index = policy_mode_choice_->FindString(mode);
    if (index != wxNOT_FOUND) {
        policy_mode_choice_->SetSelection(index);
    }
}

void RlsPolicyEditorDialog::SetEnableRlsOnTable(bool enable) {
    if (enable_rls_check_) {
        enable_rls_check_->SetValue(enable);
    }
}

std::string RlsPolicyEditorDialog::GetStatement() const {
    return statement_.empty() ? BuildStatement() : statement_;
}

} // namespace scratchrobin
