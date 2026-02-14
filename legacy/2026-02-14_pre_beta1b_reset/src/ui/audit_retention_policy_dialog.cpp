/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "audit_retention_policy_dialog.h"

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

AuditRetentionPolicyDialog::AuditRetentionPolicyDialog(wxWindow* parent, Mode mode)
    : wxDialog(parent, wxID_ANY,
               mode == Mode::Create ? "Create Audit Retention Policy" : "Edit Audit Retention Policy",
               wxDefaultPosition, wxSize(620, 520),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    BuildLayout();
    UpdateStatementPreview();
}

void AuditRetentionPolicyDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    auto* form = new wxFlexGridSizer(2, 8, 12);
    form->AddGrowableCol(1, 1);

    form->Add(new wxStaticText(this, wxID_ANY, "Policy ID"), 0, wxALIGN_CENTER_VERTICAL);
    policy_id_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(policy_id_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Category"), 0, wxALIGN_CENTER_VERTICAL);
    category_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(category_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Severity Min"), 0, wxALIGN_CENTER_VERTICAL);
    severity_min_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(severity_min_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Retention Period"), 0, wxALIGN_CENTER_VERTICAL);
    retention_period_ctrl_ = new wxTextCtrl(this, wxID_ANY, "30 days");
    form->Add(retention_period_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Archive After"), 0, wxALIGN_CENTER_VERTICAL);
    archive_after_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(archive_after_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Delete After"), 0, wxALIGN_CENTER_VERTICAL);
    delete_after_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    form->Add(delete_after_ctrl_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Storage Class"), 0, wxALIGN_CENTER_VERTICAL);
    storage_class_choice_ = new wxChoice(this, wxID_ANY);
    storage_class_choice_->Append("HOT");
    storage_class_choice_->Append("WARM");
    storage_class_choice_->Append("COLD");
    storage_class_choice_->Append("ARCHIVE");
    storage_class_choice_->SetSelection(0);
    form->Add(storage_class_choice_, 1, wxEXPAND);

    form->Add(new wxStaticText(this, wxID_ANY, "Active"), 0, wxALIGN_CENTER_VERTICAL);
    active_ctrl_ = new wxCheckBox(this, wxID_ANY, "Is Active");
    active_ctrl_->SetValue(true);
    form->Add(active_ctrl_, 1, wxEXPAND);

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
    category_ctrl_->Bind(wxEVT_TEXT, update);
    severity_min_ctrl_->Bind(wxEVT_TEXT, update);
    retention_period_ctrl_->Bind(wxEVT_TEXT, update);
    archive_after_ctrl_->Bind(wxEVT_TEXT, update);
    delete_after_ctrl_->Bind(wxEVT_TEXT, update);
    storage_class_choice_->Bind(wxEVT_CHOICE, update);
    active_ctrl_->Bind(wxEVT_CHECKBOX, update);
}

std::string AuditRetentionPolicyDialog::BuildStatement() const {
    std::string policy_id = Trim(policy_id_ctrl_ ? policy_id_ctrl_->GetValue().ToStdString() : "");
    std::string category = Trim(category_ctrl_ ? category_ctrl_->GetValue().ToStdString() : "");
    std::string severity = Trim(severity_min_ctrl_ ? severity_min_ctrl_->GetValue().ToStdString() : "");
    std::string retention = Trim(retention_period_ctrl_ ? retention_period_ctrl_->GetValue().ToStdString() : "");
    std::string archive_after = Trim(archive_after_ctrl_ ? archive_after_ctrl_->GetValue().ToStdString() : "");
    std::string delete_after = Trim(delete_after_ctrl_ ? delete_after_ctrl_->GetValue().ToStdString() : "");
    std::string storage_class = storage_class_choice_
                                    ? storage_class_choice_->GetStringSelection().ToStdString()
                                    : "HOT";
    bool active = active_ctrl_ ? active_ctrl_->GetValue() : true;

    std::ostringstream sql;
    if (mode_ == Mode::Create) {
        sql << "INSERT INTO sys.audit_retention_policy (\n"
            << "    policy_id, category, severity_min, retention_period, archive_after,\n"
            << "    delete_after, storage_class, is_active\n"
            << ") VALUES (\n"
            << "    " << (policy_id.empty() ? "gen_uuid_v7()" : SqlStringOrNull(policy_id)) << ",\n"
            << "    " << SqlStringOrNull(category) << ",\n"
            << "    " << (severity.empty() ? "NULL" : severity) << ",\n"
            << "    " << SqlStringOrNull(retention) << ",\n"
            << "    " << SqlStringOrNull(archive_after) << ",\n"
            << "    " << SqlStringOrNull(delete_after) << ",\n"
            << "    '" << storage_class << "',\n"
            << "    " << SqlBool(active) << "\n"
            << ");";
    } else {
        sql << "UPDATE sys.audit_retention_policy SET\n"
            << "    category = " << SqlStringOrNull(category) << ",\n"
            << "    severity_min = " << (severity.empty() ? "NULL" : severity) << ",\n"
            << "    retention_period = " << SqlStringOrNull(retention) << ",\n"
            << "    archive_after = " << SqlStringOrNull(archive_after) << ",\n"
            << "    delete_after = " << SqlStringOrNull(delete_after) << ",\n"
            << "    storage_class = '" << storage_class << "',\n"
            << "    is_active = " << SqlBool(active) << "\n"
            << "WHERE policy_id = " << (policy_id.empty() ? "policy_id" : SqlStringOrNull(policy_id))
            << ";";
    }
    return sql.str();
}

void AuditRetentionPolicyDialog::UpdateStatementPreview() {
    statement_ = BuildStatement();
    if (preview_ctrl_) {
        preview_ctrl_->SetValue(statement_);
    }
}

void AuditRetentionPolicyDialog::SetPolicyId(const std::string& id) {
    if (policy_id_ctrl_) {
        policy_id_ctrl_->SetValue(id);
    }
}

void AuditRetentionPolicyDialog::SetCategory(const std::string& category) {
    if (category_ctrl_) {
        category_ctrl_->SetValue(category);
    }
}

void AuditRetentionPolicyDialog::SetSeverityMin(const std::string& severity) {
    if (severity_min_ctrl_) {
        severity_min_ctrl_->SetValue(severity);
    }
}

void AuditRetentionPolicyDialog::SetRetentionPeriod(const std::string& period) {
    if (retention_period_ctrl_) {
        retention_period_ctrl_->SetValue(period);
    }
}

void AuditRetentionPolicyDialog::SetArchiveAfter(const std::string& period) {
    if (archive_after_ctrl_) {
        archive_after_ctrl_->SetValue(period);
    }
}

void AuditRetentionPolicyDialog::SetDeleteAfter(const std::string& period) {
    if (delete_after_ctrl_) {
        delete_after_ctrl_->SetValue(period);
    }
}

void AuditRetentionPolicyDialog::SetStorageClass(const std::string& storage_class) {
    if (!storage_class_choice_) {
        return;
    }
    int index = storage_class_choice_->FindString(storage_class);
    if (index != wxNOT_FOUND) {
        storage_class_choice_->SetSelection(index);
    }
}

void AuditRetentionPolicyDialog::SetActive(bool active) {
    if (active_ctrl_) {
        active_ctrl_->SetValue(active);
    }
}

std::string AuditRetentionPolicyDialog::GetStatement() const {
    return statement_.empty() ? BuildStatement() : statement_;
}

} // namespace scratchrobin
