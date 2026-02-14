/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "view_editor_dialog.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <vector>

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/panel.h>
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

ViewEditorDialog::ViewEditorDialog(wxWindow* parent, ViewEditorMode mode)
    : wxDialog(parent, wxID_ANY,
               mode == ViewEditorMode::Create ? "Create View" : "Edit View",
               wxDefaultPosition, wxSize(800, 700),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    BuildLayout();
    CentreOnParent();
}

void ViewEditorDialog::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // View Name
    auto* nameLabel = new wxStaticText(this, wxID_ANY, "View Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Schema
    auto* schemaLabel = new wxStaticText(this, wxID_ANY, "Schema");
    rootSizer->Add(schemaLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    schema_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    schema_ctrl_->SetHint("Default schema");
    rootSizer->Add(schema_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    if (mode_ == ViewEditorMode::Create) {
        // OR REPLACE option
        or_replace_ctrl_ = new wxCheckBox(this, wxID_ANY, "OR REPLACE");
        rootSizer->Add(or_replace_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        // IF NOT EXISTS option
        if_not_exists_ctrl_ = new wxCheckBox(this, wxID_ANY, "IF NOT EXISTS");
        rootSizer->Add(if_not_exists_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        // View Type
        auto* typeLabel = new wxStaticText(this, wxID_ANY, "View Type");
        rootSizer->Add(typeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        view_type_choice_ = BuildChoice(this, {"REGULAR", "MATERIALIZED"});
        rootSizer->Add(view_type_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        // Check Option
        auto* checkOptionLabel = new wxStaticText(this, wxID_ANY, "Check Option");
        rootSizer->Add(checkOptionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        check_option_choice_ = BuildChoice(this, {"NONE", "LOCAL", "CASCADED"});
        rootSizer->Add(check_option_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        // Is Updatable
        is_updatable_ctrl_ = new wxCheckBox(this, wxID_ANY, "Updatable View");
        rootSizer->Add(is_updatable_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        // Notebook for tabs
        notebook_ = new wxNotebook(this, wxID_ANY);
        BuildDefinitionTab(notebook_);
        BuildColumnsTab(notebook_);
        BuildDependenciesTab(notebook_);
        rootSizer->Add(notebook_, 1, wxEXPAND | wxALL, 12);

    } else {
        // Edit mode - disable name and schema
        if (name_ctrl_) name_ctrl_->Enable(false);
        if (schema_ctrl_) schema_ctrl_->Enable(false);

        // Alter action
        auto* actionLabel = new wxStaticText(this, wxID_ANY, "Action");
        rootSizer->Add(actionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_action_choice_ = BuildChoice(this, {
            "RENAME TO",
            "SET SCHEMA",
            "ALTER DEFINITION"
        });
        rootSizer->Add(alter_action_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        alter_value_label_ = new wxStaticText(this, wxID_ANY, "Value");
        rootSizer->Add(alter_value_label_, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_value_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(alter_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        // Definition editor for alter
        auto* definitionLabel = new wxStaticText(this, wxID_ANY, "View Definition");
        rootSizer->Add(definitionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        definition_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                          wxSize(-1, 300), wxTE_MULTILINE);
        definition_ctrl_->SetHint("SELECT ...");
        rootSizer->Add(definition_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }

    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizer(rootSizer);

    if (view_type_choice_) {
        view_type_choice_->Bind(wxEVT_CHOICE,
            [this](wxCommandEvent&) { UpdateViewTypeFields(); });
        UpdateViewTypeFields();
    }

    if (alter_action_choice_) {
        alter_action_choice_->Bind(wxEVT_CHOICE,
            [this](wxCommandEvent&) { UpdateAlterActionFields(); });
        UpdateAlterActionFields();
    }
}

void ViewEditorDialog::BuildDefinitionTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* label = new wxStaticText(panel, wxID_ANY, "SQL Definition");
    sizer->Add(label, 0, wxBOTTOM, 8);

    definition_ctrl_ = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition,
                                      wxDefaultSize, wxTE_MULTILINE);
    definition_ctrl_->SetHint("SELECT column1, column2\nFROM table_name\nWHERE condition;");
    sizer->Add(definition_ctrl_, 1, wxEXPAND);

    panel->SetSizer(sizer);
    notebook->AddPage(panel, "Definition");
}

void ViewEditorDialog::BuildColumnsTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* label = new wxStaticText(panel, wxID_ANY,
        "View columns will be shown here after parsing the definition.\n"
        "This tab shows the columns that will be exposed by the view.");
    sizer->Add(label, 0, wxBOTTOM, 8);

    columns_grid_ = new wxGrid(panel, wxID_ANY);
    columns_grid_->CreateGrid(0, 4);
    columns_grid_->SetColLabelValue(0, "Column Name");
    columns_grid_->SetColLabelValue(1, "Data Type");
    columns_grid_->SetColLabelValue(2, "Nullable");
    columns_grid_->SetColLabelValue(3, "Default");
    columns_grid_->EnableEditing(false);
    sizer->Add(columns_grid_, 1, wxEXPAND);

    panel->SetSizer(sizer);
    notebook->AddPage(panel, "Columns");
}

void ViewEditorDialog::BuildDependenciesTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* label = new wxStaticText(panel, wxID_ANY,
        "Dependencies will be shown here after parsing the definition.\n"
        "This tab shows tables and views referenced by the view.");
    sizer->Add(label, 0, wxBOTTOM, 8);

    dependencies_grid_ = new wxGrid(panel, wxID_ANY);
    dependencies_grid_->CreateGrid(0, 3);
    dependencies_grid_->SetColLabelValue(0, "Name");
    dependencies_grid_->SetColLabelValue(1, "Type");
    dependencies_grid_->SetColLabelValue(2, "Schema");
    dependencies_grid_->EnableEditing(false);
    sizer->Add(dependencies_grid_, 1, wxEXPAND);

    panel->SetSizer(sizer);
    notebook->AddPage(panel, "Dependencies");
}

std::string ViewEditorDialog::BuildSql() const {
    return mode_ == ViewEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string ViewEditorDialog::view_name() const {
    return Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
}

void ViewEditorDialog::SetViewName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
    }
}

void ViewEditorDialog::SetSchema(const std::string& schema) {
    if (schema_ctrl_) {
        schema_ctrl_->SetValue(schema);
    }
}

void ViewEditorDialog::SetViewDefinition(const std::string& definition) {
    if (definition_ctrl_) {
        definition_ctrl_->SetValue(definition);
    }
}

void ViewEditorDialog::SetColumns(const std::vector<ViewColumnInfo>& columns) {
    if (!columns_grid_) return;

    // Clear existing rows
    if (columns_grid_->GetNumberRows() > 0) {
        columns_grid_->DeleteRows(0, columns_grid_->GetNumberRows());
    }

    // Add new rows
    for (const auto& col : columns) {
        int row = columns_grid_->GetNumberRows();
        columns_grid_->AppendRows(1);
        columns_grid_->SetCellValue(row, 0, col.name);
        columns_grid_->SetCellValue(row, 1, col.data_type);
        columns_grid_->SetCellValue(row, 2, col.is_nullable ? "YES" : "NO");
        columns_grid_->SetCellValue(row, 3, col.default_value);
    }

    columns_grid_->AutoSizeColumns();
}

void ViewEditorDialog::SetDependencies(const std::vector<ViewDependencyInfo>& dependencies) {
    if (!dependencies_grid_) return;

    // Clear existing rows
    if (dependencies_grid_->GetNumberRows() > 0) {
        dependencies_grid_->DeleteRows(0, dependencies_grid_->GetNumberRows());
    }

    // Add new rows
    for (const auto& dep : dependencies) {
        int row = dependencies_grid_->GetNumberRows();
        dependencies_grid_->AppendRows(1);
        dependencies_grid_->SetCellValue(row, 0, dep.name);
        dependencies_grid_->SetCellValue(row, 1, dep.type);
        dependencies_grid_->SetCellValue(row, 2, dep.schema);
    }

    dependencies_grid_->AutoSizeColumns();
}

std::string ViewEditorDialog::BuildCreateSql() const {
    std::string name = view_name();
    if (name.empty()) {
        return std::string();
    }

    std::string definition = Trim(definition_ctrl_ ? definition_ctrl_->GetValue().ToStdString()
                                                   : std::string());
    if (definition.empty()) {
        return std::string();
    }

    std::string schema = Trim(schema_ctrl_ ? schema_ctrl_->GetValue().ToStdString()
                                           : std::string());

    std::string view_type = view_type_choice_ ? view_type_choice_->GetStringSelection().ToStdString()
                                              : "REGULAR";
    bool is_materialized = (view_type == "MATERIALIZED");

    std::ostringstream sql;
    sql << "CREATE ";

    if (or_replace_ctrl_ && or_replace_ctrl_->IsChecked()) {
        sql << "OR REPLACE ";
    }

    if (is_materialized) {
        sql << "MATERIALIZED VIEW ";
    } else {
        sql << "VIEW ";
    }

    if (if_not_exists_ctrl_ && if_not_exists_ctrl_->IsChecked()) {
        sql << "IF NOT EXISTS ";
    }

    // Fully qualified name
    if (!schema.empty()) {
        sql << QuoteIdentifier(schema) << ".";
    }
    sql << QuoteIdentifier(name);

    sql << " AS\n";

    // Definition
    sql << definition;

    // Check option (only for regular views)
    if (!is_materialized && check_option_choice_) {
        std::string check_option = check_option_choice_->GetStringSelection().ToStdString();
        if (check_option == "LOCAL") {
            sql << "\nWITH LOCAL CHECK OPTION";
        } else if (check_option == "CASCADED") {
            sql << "\nWITH CASCADED CHECK OPTION";
        }
    }

    sql << ";";
    return sql.str();
}

std::string ViewEditorDialog::BuildAlterSql() const {
    std::string name = view_name();
    if (name.empty()) {
        return std::string();
    }

    if (!alter_action_choice_ || !alter_value_ctrl_) {
        return std::string();
    }

    std::string schema = Trim(schema_ctrl_ ? schema_ctrl_->GetValue().ToStdString()
                                           : std::string());

    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    std::string value = Trim(alter_value_ctrl_->GetValue().ToStdString());

    std::ostringstream sql;

    if (action == "RENAME TO") {
        if (value.empty()) {
            return std::string();
        }
        sql << "ALTER VIEW ";
        if (!schema.empty()) {
            sql << QuoteIdentifier(schema) << ".";
        }
        sql << QuoteIdentifier(name) << " RENAME TO " << QuoteIdentifier(value);
    } else if (action == "SET SCHEMA") {
        if (value.empty()) {
            return std::string();
        }
        sql << "ALTER VIEW ";
        if (!schema.empty()) {
            sql << QuoteIdentifier(schema) << ".";
        }
        sql << QuoteIdentifier(name) << " SET SCHEMA " << QuoteIdentifier(value);
    } else if (action == "ALTER DEFINITION") {
        std::string new_definition = Trim(definition_ctrl_ ? definition_ctrl_->GetValue().ToStdString()
                                                           : std::string());
        if (new_definition.empty()) {
            return std::string();
        }
        // Most databases require DROP and CREATE for altering view definition
        // Some support CREATE OR REPLACE VIEW
        sql << "CREATE OR REPLACE VIEW ";
        if (!schema.empty()) {
            sql << QuoteIdentifier(schema) << ".";
        }
        sql << QuoteIdentifier(name) << " AS\n" << new_definition << ";";
    } else {
        return std::string();
    }

    sql << ";";
    return sql.str();
}

std::string ViewEditorDialog::FormatViewPath(const std::string& value) const {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return trimmed;
    }
    if (trimmed.find('.') != std::string::npos || trimmed.find('"') != std::string::npos) {
        return trimmed;
    }
    return QuoteIdentifier(trimmed);
}

void ViewEditorDialog::UpdateViewTypeFields() {
    if (!view_type_choice_ || !check_option_choice_ || !is_updatable_ctrl_) {
        return;
    }

    std::string view_type = view_type_choice_->GetStringSelection().ToStdString();
    bool is_materialized = (view_type == "MATERIALIZED");

    // Check option and updatable only apply to regular views
    check_option_choice_->Enable(!is_materialized);
    is_updatable_ctrl_->Enable(!is_materialized);

    // Uncheck updatable for materialized views
    if (is_materialized) {
        is_updatable_ctrl_->SetValue(false);
        check_option_choice_->SetSelection(0);  // NONE
    }
}

void ViewEditorDialog::UpdateAlterActionFields() {
    if (!alter_action_choice_ || !alter_value_label_ || !alter_value_ctrl_) {
        return;
    }

    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    wxString label = "Value";

    if (action == "RENAME TO") {
        label = "New View Name";
    } else if (action == "SET SCHEMA") {
        label = "New Schema";
    } else if (action == "ALTER DEFINITION") {
        label = "New Definition (shown in editor)";
        alter_value_ctrl_->Enable(false);
    }

    alter_value_label_->SetLabel(label);
    if (action != "ALTER DEFINITION") {
        alter_value_ctrl_->Enable(true);
    }
}

void ViewEditorDialog::RefreshColumnsFromDefinition() {
    // Parse the SQL definition to extract column information
    // This is a simplified parser - in production, this would use
    // a proper SQL parser to get accurate column info
    if (!definition_ctrl_) return;

    std::string definition = definition_ctrl_->GetValue().ToStdString();
    std::vector<ViewColumnInfo> columns;

    // Very simple regex to find SELECT columns
    // This is just a placeholder - real implementation would need
    // a proper SQL parser or database introspection
    std::regex column_regex(R"((\w+)\s*[,\s])", std::regex::icase);
    std::smatch match;
    std::string::const_iterator search_start(definition.cbegin());

    // Clear and repopulate grid
    SetColumns(columns);
}

void ViewEditorDialog::ParseDependenciesFromDefinition() {
    // Parse the SQL definition to find referenced tables/views
    if (!definition_ctrl_) return;

    std::string definition = definition_ctrl_->GetValue().ToStdString();
    std::vector<ViewDependencyInfo> dependencies;

    // Simple regex to find FROM and JOIN clauses
    // This is a placeholder - real implementation would need
    // a proper SQL parser
    std::regex from_regex(R"(FROM\s+(\w+))", std::regex::icase);
    std::regex join_regex(R"(JOIN\s+(\w+))", std::regex::icase);

    std::smatch match;
    std::string::const_iterator search_start(definition.cbegin());

    // Find FROM clauses
    while (std::regex_search(search_start, definition.cend(), match, from_regex)) {
        ViewDependencyInfo dep;
        dep.name = match[1].str();
        dep.type = "TABLE";
        dependencies.push_back(dep);
        search_start = match.suffix().first;
    }

    // Find JOIN clauses
    search_start = definition.cbegin();
    while (std::regex_search(search_start, definition.cend(), match, join_regex)) {
        ViewDependencyInfo dep;
        dep.name = match[1].str();
        dep.type = "TABLE";
        dependencies.push_back(dep);
        search_start = match.suffix().first;
    }

    SetDependencies(dependencies);
}

} // namespace scratchrobin
