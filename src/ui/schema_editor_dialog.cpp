/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "schema_editor_dialog.h"

#include <algorithm>
#include <cctype>

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

std::string QuoteIdentifier(const std::string& value) {
    if (value.empty()) {
        return value;
    }
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
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

wxChoice* BuildChoice(wxWindow* parent, const std::vector<std::string>& options) {
    auto* choice = new wxChoice(parent, wxID_ANY);
    for (const auto& option : options) {
        choice->Append(option);
    }
    if (!options.empty()) {
        choice->SetSelection(0);
    }
    return choice;
}

} // namespace

SchemaEditorDialog::SchemaEditorDialog(wxWindow* parent, SchemaEditorMode mode)
    : wxDialog(parent, wxID_ANY, mode == SchemaEditorMode::Create ? "Create Schema" : "Alter Schema",
               wxDefaultPosition, wxSize(520, 420),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Schema Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    if (mode_ == SchemaEditorMode::Create) {
        if_not_exists_ctrl_ = new wxCheckBox(this, wxID_ANY, "IF NOT EXISTS");
        rootSizer->Add(if_not_exists_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        auto* ownerLabel = new wxStaticText(this, wxID_ANY, "Authorization (Owner)");
        rootSizer->Add(ownerLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        owner_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(owner_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    } else {
        auto* actionLabel = new wxStaticText(this, wxID_ANY, "Action");
        rootSizer->Add(actionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_action_choice_ = BuildChoice(this, {"RENAME", "OWNER TO", "SET PATH"});
        rootSizer->Add(alter_action_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        alter_value_label_ = new wxStaticText(this, wxID_ANY, "Value");
        rootSizer->Add(alter_value_label_, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_value_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(alter_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }

    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(rootSizer);
    CentreOnParent();

    if (alter_action_choice_) {
        alter_action_choice_->Bind(wxEVT_CHOICE,
            [this](wxCommandEvent&) { UpdateAlterActionFields(); });
        UpdateAlterActionFields();
    }
}

std::string SchemaEditorDialog::BuildSql() const {
    return mode_ == SchemaEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string SchemaEditorDialog::schema_name() const {
    return Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
}

void SchemaEditorDialog::SetSchemaName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
        if (mode_ == SchemaEditorMode::Alter) {
            name_ctrl_->Enable(false);
        }
    }
}

std::string SchemaEditorDialog::BuildCreateSql() const {
    std::string name = schema_name();
    std::string owner = Trim(owner_ctrl_ ? owner_ctrl_->GetValue().ToStdString() : std::string());
    if (name.empty() && owner.empty()) {
        return std::string();
    }

    std::string sql = "CREATE SCHEMA ";
    if (if_not_exists_ctrl_ && if_not_exists_ctrl_->IsChecked()) {
        sql += "IF NOT EXISTS ";
    }

    if (!name.empty()) {
        sql += QuoteIdentifier(name) + " ";
    }

    if (!owner.empty()) {
        sql += "AUTHORIZATION " + QuoteIdentifier(owner) + " ";
    }

    sql += ";";
    return sql;
}

std::string SchemaEditorDialog::BuildAlterSql() const {
    std::string name = schema_name();
    if (name.empty()) {
        return std::string();
    }
    if (!alter_action_choice_ || !alter_value_ctrl_) {
        return std::string();
    }

    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    std::string value = Trim(alter_value_ctrl_->GetValue().ToStdString());
    if (value.empty()) {
        return std::string();
    }

    std::string sql = "ALTER SCHEMA " + QuoteIdentifier(name) + " ";
    if (action == "RENAME") {
        sql += "RENAME TO " + QuoteIdentifier(value);
    } else if (action == "OWNER TO") {
        sql += "OWNER TO " + QuoteIdentifier(value);
    } else if (action == "SET PATH") {
        sql += "SET PATH " + value;
    } else {
        return std::string();
    }
    sql += ";";
    return sql;
}

void SchemaEditorDialog::UpdateAlterActionFields() {
    if (!alter_action_choice_ || !alter_value_label_ || !alter_value_ctrl_) {
        return;
    }
    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    wxString label = "Value";
    wxString hint;
    if (action == "RENAME") {
        label = "New Schema Name";
    } else if (action == "OWNER TO") {
        label = "Owner";
    } else if (action == "SET PATH") {
        label = "Schema Path";
    }
    alter_value_label_->SetLabel(label);
    alter_value_ctrl_->SetHint(hint);
}

} // namespace scratchrobin
