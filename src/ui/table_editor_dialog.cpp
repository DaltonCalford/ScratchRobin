/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "table_editor_dialog.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

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

bool IsQuotedIdentifier(const std::string& value) {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
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
    if (IsSimpleIdentifier(value) || IsQuotedIdentifier(value)) {
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

std::vector<std::string> SplitLines(const std::string& value) {
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string line;
    while (std::getline(stream, line)) {
        line = Trim(line);
        if (line.empty()) {
            continue;
        }
        if (!line.empty() && line.back() == ',') {
            line.pop_back();
            line = Trim(line);
        }
        if (!line.empty()) {
            parts.push_back(line);
        }
    }
    return parts;
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

TableEditorDialog::TableEditorDialog(wxWindow* parent, TableEditorMode mode)
    : wxDialog(parent, wxID_ANY, mode == TableEditorMode::Create ? "Create Table" : "Alter Table",
               wxDefaultPosition, wxSize(640, 720),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Table Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    if (mode_ == TableEditorMode::Create) {
        if_not_exists_ctrl_ = new wxCheckBox(this, wxID_ANY, "IF NOT EXISTS");
        rootSizer->Add(if_not_exists_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        auto* columnsLabel = new wxStaticText(this, wxID_ANY, "Columns (one per line)");
        rootSizer->Add(columnsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        columns_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                       wxSize(-1, 160), wxTE_MULTILINE);
        columns_ctrl_->SetHint("id UUID PRIMARY KEY\nname VARCHAR(80) NOT NULL");
        rootSizer->Add(columns_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* constraintsLabel = new wxStaticText(this, wxID_ANY, "Table Constraints (one per line)");
        rootSizer->Add(constraintsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        constraints_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                           wxSize(-1, 120), wxTE_MULTILINE);
        constraints_ctrl_->SetHint("CONSTRAINT pk_name PRIMARY KEY (id)");
        rootSizer->Add(constraints_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* optionsLabel = new wxStaticText(this, wxID_ANY, "Table Options (raw)");
        rootSizer->Add(optionsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        options_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                       wxSize(-1, 80), wxTE_MULTILINE);
        options_ctrl_->SetHint("TABLESPACE main_ts\nON COMMIT DELETE ROWS");
        rootSizer->Add(options_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    } else {
        auto* actionLabel = new wxStaticText(this, wxID_ANY, "Action");
        rootSizer->Add(actionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_action_choice_ = BuildChoice(this, {
            "ADD COLUMN",
            "DROP COLUMN",
            "RENAME COLUMN",
            "RENAME TABLE",
            "ADD CONSTRAINT",
            "DROP CONSTRAINT",
            "RENAME CONSTRAINT",
            "SET TABLESPACE",
            "SET SCHEMA"
        });
        rootSizer->Add(alter_action_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        alter_value_label_ = new wxStaticText(this, wxID_ANY, "Value");
        rootSizer->Add(alter_value_label_, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_value_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(alter_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        alter_value_label_2_ = new wxStaticText(this, wxID_ANY, "New Name");
        rootSizer->Add(alter_value_label_2_, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_value_ctrl_2_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(alter_value_ctrl_2_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
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

std::string TableEditorDialog::BuildSql() const {
    return mode_ == TableEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string TableEditorDialog::table_name() const {
    return Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
}

void TableEditorDialog::SetTableName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
        if (mode_ == TableEditorMode::Alter) {
            name_ctrl_->Enable(false);
        }
    }
}

std::string TableEditorDialog::BuildCreateSql() const {
    std::string name = table_name();
    if (name.empty()) {
        return std::string();
    }

    auto column_lines = SplitLines(columns_ctrl_ ? columns_ctrl_->GetValue().ToStdString()
                                                 : std::string());
    auto constraint_lines = SplitLines(constraints_ctrl_ ? constraints_ctrl_->GetValue().ToStdString()
                                                         : std::string());
    if (column_lines.empty() && constraint_lines.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "CREATE TABLE ";
    if (if_not_exists_ctrl_ && if_not_exists_ctrl_->IsChecked()) {
        sql << "IF NOT EXISTS ";
    }
    sql << FormatTablePath(name) << " (\n";

    bool first = true;
    for (const auto& line : column_lines) {
        if (!first) {
            sql << ",\n";
        }
        sql << "  " << line;
        first = false;
    }
    for (const auto& line : constraint_lines) {
        if (!first) {
            sql << ",\n";
        }
        sql << "  " << line;
        first = false;
    }
    sql << "\n)";

    std::string options = Trim(options_ctrl_ ? options_ctrl_->GetValue().ToStdString()
                                             : std::string());
    if (!options.empty()) {
        sql << "\n" << options;
    }
    sql << ";";
    return sql.str();
}

std::string TableEditorDialog::BuildAlterSql() const {
    std::string name = table_name();
    if (name.empty()) {
        return std::string();
    }
    if (!alter_action_choice_ || !alter_value_ctrl_) {
        return std::string();
    }

    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    std::string value = Trim(alter_value_ctrl_->GetValue().ToStdString());
    std::string value2 = Trim(alter_value_ctrl_2_ ? alter_value_ctrl_2_->GetValue().ToStdString()
                                                 : std::string());

    if ((action == "RENAME COLUMN" || action == "RENAME CONSTRAINT") && value2.empty()) {
        return std::string();
    }
    if (value.empty() && action != "RENAME TABLE") {
        return std::string();
    }
    if (action == "RENAME TABLE" && value.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "ALTER TABLE " << FormatTablePath(name) << " ";

    if (action == "ADD COLUMN") {
        sql << "ADD " << value;
    } else if (action == "DROP COLUMN") {
        sql << "DROP COLUMN " << QuoteIdentifier(value);
    } else if (action == "RENAME COLUMN") {
        sql << "RENAME COLUMN " << QuoteIdentifier(value) << " TO " << QuoteIdentifier(value2);
    } else if (action == "RENAME TABLE") {
        sql << "RENAME TO " << QuoteIdentifier(value);
    } else if (action == "ADD CONSTRAINT") {
        sql << "ADD " << value;
    } else if (action == "DROP CONSTRAINT") {
        sql << "DROP CONSTRAINT " << QuoteIdentifier(value);
    } else if (action == "RENAME CONSTRAINT") {
        sql << "RENAME CONSTRAINT " << QuoteIdentifier(value) << " TO " << QuoteIdentifier(value2);
    } else if (action == "SET TABLESPACE") {
        sql << "SET TABLESPACE " << value;
    } else if (action == "SET SCHEMA") {
        sql << "SET SCHEMA " << value;
    } else {
        return std::string();
    }

    sql << ";";
    return sql.str();
}

std::string TableEditorDialog::FormatTablePath(const std::string& value) const {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return trimmed;
    }
    if (trimmed.find('.') != std::string::npos || trimmed.find('"') != std::string::npos) {
        return trimmed;
    }
    return QuoteIdentifier(trimmed);
}

void TableEditorDialog::UpdateAlterActionFields() {
    if (!alter_action_choice_ || !alter_value_label_ || !alter_value_ctrl_ ||
        !alter_value_label_2_ || !alter_value_ctrl_2_) {
        return;
    }
    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    bool show_second = (action == "RENAME COLUMN" || action == "RENAME CONSTRAINT");
    wxString label = "Value";
    wxString hint;
    if (action == "ADD COLUMN") {
        label = "Column Definition";
    } else if (action == "DROP COLUMN") {
        label = "Column Name";
    } else if (action == "RENAME COLUMN") {
        label = "Old Column Name";
    } else if (action == "RENAME TABLE") {
        label = "New Table Name";
    } else if (action == "ADD CONSTRAINT") {
        label = "Constraint Clause";
    } else if (action == "DROP CONSTRAINT") {
        label = "Constraint Name";
    } else if (action == "RENAME CONSTRAINT") {
        label = "Old Constraint Name";
    } else if (action == "SET TABLESPACE") {
        label = "Tablespace";
    } else if (action == "SET SCHEMA") {
        label = "Schema Path";
    }

    alter_value_label_->SetLabel(label);
    alter_value_ctrl_->SetHint(hint);
    alter_value_label_2_->Show(show_second);
    alter_value_ctrl_2_->Show(show_second);

    if (show_second) {
        alter_value_label_2_->SetLabel("New Name");
    }

    Layout();
}

} // namespace scratchrobin
