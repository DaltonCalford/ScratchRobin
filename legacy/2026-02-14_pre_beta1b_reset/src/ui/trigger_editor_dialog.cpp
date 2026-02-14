/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "trigger_editor_dialog.h"

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

TriggerEditorDialog::TriggerEditorDialog(wxWindow* parent, TriggerEditorMode mode)
    : wxDialog(parent, wxID_ANY,
               mode == TriggerEditorMode::Create ? "Create Trigger" : "Edit Trigger",
               wxDefaultPosition, wxSize(640, 800),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Trigger Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    if (mode_ == TriggerEditorMode::Create) {
        auto* schemaLabel = new wxStaticText(this, wxID_ANY, "Schema");
        rootSizer->Add(schemaLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        schema_choice_ = BuildChoice(this, {"PUBLIC", "SYS", "CATALOG"});
        rootSizer->Add(schema_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* tableLabel = new wxStaticText(this, wxID_ANY, "Table Name");
        rootSizer->Add(tableLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        table_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        table_ctrl_->SetHint("schema.table_name or table_name");
        rootSizer->Add(table_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* timingLabel = new wxStaticText(this, wxID_ANY, "Timing");
        rootSizer->Add(timingLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        timing_choice_ = BuildChoice(this, {"BEFORE", "AFTER", "INSTEAD OF"});
        rootSizer->Add(timing_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* eventsLabel = new wxStaticText(this, wxID_ANY, "Events");
        rootSizer->Add(eventsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);

        auto* eventsSizer = new wxBoxSizer(wxHORIZONTAL);
        insert_event_ctrl_ = new wxCheckBox(this, wxID_ANY, "INSERT");
        update_event_ctrl_ = new wxCheckBox(this, wxID_ANY, "UPDATE");
        delete_event_ctrl_ = new wxCheckBox(this, wxID_ANY, "DELETE");
        eventsSizer->Add(insert_event_ctrl_, 0, wxRIGHT, 16);
        eventsSizer->Add(update_event_ctrl_, 0, wxRIGHT, 16);
        eventsSizer->Add(delete_event_ctrl_, 0, wxRIGHT, 16);
        rootSizer->Add(eventsSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* forEachLabel = new wxStaticText(this, wxID_ANY, "For Each");
        rootSizer->Add(forEachLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        for_each_choice_ = BuildChoice(this, {"ROW", "STATEMENT"});
        rootSizer->Add(for_each_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* whenLabel = new wxStaticText(this, wxID_ANY, "When Condition (optional)");
        rootSizer->Add(whenLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        when_condition_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                              wxDefaultSize, wxTE_MULTILINE);
        when_condition_ctrl_->SetHint("NEW.status = 'ACTIVE' OR OLD.value IS NULL");
        rootSizer->Add(when_condition_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* bodyLabel = new wxStaticText(this, wxID_ANY, "Trigger Body");
        rootSizer->Add(bodyLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        trigger_body_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                            wxSize(-1, 200), wxTE_MULTILINE);
        trigger_body_ctrl_->SetHint(
            "BEGIN\n"
            "  -- Add your trigger logic here\n"
            "  INSERT INTO audit_log (table_name, action, changed_at)\n"
            "  VALUES ('table_name', 'INSERT', CURRENT_TIMESTAMP);\n"
            "END;");
        rootSizer->Add(trigger_body_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    } else {
        auto* actionLabel = new wxStaticText(this, wxID_ANY, "Action");
        rootSizer->Add(actionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_action_choice_ = BuildChoice(this, {
            "RENAME TO",
            "ENABLE",
            "DISABLE",
            "SET SCHEMA"
        });
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

std::string TriggerEditorDialog::BuildSql() const {
    return mode_ == TriggerEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string TriggerEditorDialog::trigger_name() const {
    return Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
}

void TriggerEditorDialog::SetTriggerName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
        if (mode_ == TriggerEditorMode::Edit) {
            name_ctrl_->Enable(false);
        }
    }
}

void TriggerEditorDialog::SetTableName(const std::string& name) {
    if (table_ctrl_) {
        table_ctrl_->SetValue(name);
    }
}

void TriggerEditorDialog::SetSchemaName(const std::string& name) {
    if (schema_choice_ && !name.empty()) {
        int index = schema_choice_->FindString(name);
        if (index != wxNOT_FOUND) {
            schema_choice_->SetSelection(index);
        }
    }
}

std::string TriggerEditorDialog::BuildCreateSql() const {
    std::string name = trigger_name();
    if (name.empty()) {
        return std::string();
    }

    std::string table = Trim(table_ctrl_ ? table_ctrl_->GetValue().ToStdString()
                                         : std::string());
    if (table.empty()) {
        return std::string();
    }

    std::string events = BuildEventsClause();
    if (events.empty()) {
        return std::string();
    }

    std::string body = Trim(trigger_body_ctrl_ ? trigger_body_ctrl_->GetValue().ToStdString()
                                               : std::string());
    if (body.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "CREATE TRIGGER " << FormatTriggerPath(name) << "\n";

    std::string timing = timing_choice_ ? timing_choice_->GetStringSelection().ToStdString()
                                        : "BEFORE";
    sql << "  " << timing << " " << events << "\n";
    sql << "  ON " << FormatTablePath(table) << "\n";

    if (for_each_choice_) {
        std::string for_each = for_each_choice_->GetStringSelection().ToStdString();
        sql << "  FOR EACH " << for_each << "\n";
    }

    std::string when_condition = Trim(when_condition_ctrl_ ? when_condition_ctrl_->GetValue().ToStdString()
                                                           : std::string());
    if (!when_condition.empty()) {
        sql << "  WHEN (" << when_condition << ")\n";
    }

    sql << "BEGIN\n";

    // Add the trigger body, ensuring proper indentation
    std::istringstream body_stream(body);
    std::string line;
    while (std::getline(body_stream, line)) {
        // Check if line already has proper formatting (starts with spaces or is a BEGIN/END)
        std::string trimmed_line = Trim(line);
        if (!trimmed_line.empty()) {
            // Add 4 spaces indentation if not already indented and not a BEGIN/END marker
            if (!line.empty() && line[0] != ' ' && line[0] != '\t' &&
                trimmed_line != "BEGIN" && trimmed_line != "END;" && trimmed_line != "END") {
                sql << "    " << line << "\n";
            } else {
                sql << "  " << line << "\n";
            }
        }
    }

    // Ensure the body ends with END
    std::string trimmed_body = Trim(body);
    if (!trimmed_body.empty() &&
        trimmed_body.length() < 4 ||
        (trimmed_body.substr(trimmed_body.length() - 3) != "END" &&
         trimmed_body.substr(trimmed_body.length() - 4) != "END;")) {
        sql << "  END;\n";
    }

    sql << ";";
    return sql.str();
}

std::string TriggerEditorDialog::BuildAlterSql() const {
    std::string name = trigger_name();
    if (name.empty()) {
        return std::string();
    }

    if (!alter_action_choice_) {
        return std::string();
    }

    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    std::string value = Trim(alter_value_ctrl_ ? alter_value_ctrl_->GetValue().ToStdString()
                                               : std::string());

    std::ostringstream sql;
    sql << "ALTER TRIGGER " << FormatTriggerPath(name) << " ";

    if (action == "RENAME TO") {
        if (value.empty()) {
            return std::string();
        }
        sql << "RENAME TO " << QuoteIdentifier(value);
    } else if (action == "ENABLE") {
        sql << "ENABLE";
    } else if (action == "DISABLE") {
        sql << "DISABLE";
    } else if (action == "SET SCHEMA") {
        if (value.empty()) {
            return std::string();
        }
        sql << "SET SCHEMA " << QuoteIdentifier(value);
    } else {
        return std::string();
    }

    sql << ";";
    return sql.str();
}

std::string TriggerEditorDialog::FormatTriggerPath(const std::string& value) const {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return trimmed;
    }
    if (trimmed.find('.') != std::string::npos || trimmed.find('"') != std::string::npos) {
        return trimmed;
    }
    return QuoteIdentifier(trimmed);
}

std::string TriggerEditorDialog::FormatTablePath(const std::string& value) const {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return trimmed;
    }
    if (trimmed.find('.') != std::string::npos || trimmed.find('"') != std::string::npos) {
        return trimmed;
    }
    return QuoteIdentifier(trimmed);
}

std::string TriggerEditorDialog::BuildEventsClause() const {
    std::vector<std::string> events;

    if (insert_event_ctrl_ && insert_event_ctrl_->IsChecked()) {
        events.push_back("INSERT");
    }
    if (update_event_ctrl_ && update_event_ctrl_->IsChecked()) {
        events.push_back("UPDATE");
    }
    if (delete_event_ctrl_ && delete_event_ctrl_->IsChecked()) {
        events.push_back("DELETE");
    }

    if (events.empty()) {
        return std::string();
    }

    std::ostringstream out;
    for (size_t i = 0; i < events.size(); ++i) {
        if (i > 0) {
            out << " OR ";
        }
        out << events[i];
    }
    return out.str();
}

void TriggerEditorDialog::UpdateAlterActionFields() {
    if (!alter_action_choice_ || !alter_value_label_ || !alter_value_ctrl_) {
        return;
    }

    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    bool needs_value = false;
    wxString label = "Value";

    if (action == "RENAME TO") {
        needs_value = true;
        label = "New Trigger Name";
    } else if (action == "SET SCHEMA") {
        needs_value = true;
        label = "New Schema Name";
    }

    alter_value_label_->SetLabel(label);
    alter_value_label_->Show(needs_value);
    alter_value_ctrl_->Show(needs_value);
    alter_value_ctrl_->Enable(needs_value);

    if (!needs_value) {
        alter_value_ctrl_->Clear();
    }

    Layout();
}

} // namespace scratchrobin
