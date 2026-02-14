/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "routine_editor_dialog.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

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

RoutineEditorDialog::RoutineEditorDialog(wxWindow* parent, RoutineEditorMode mode)
    : wxDialog(parent, wxID_ANY, [mode]() -> wxString {
                   switch (mode) {
                       case RoutineEditorMode::CreateProcedure:
                           return "Create Procedure";
                       case RoutineEditorMode::CreateFunction:
                           return "Create Function";
                       case RoutineEditorMode::EditRoutine:
                           return "Edit Routine";
                   }
                   return "Routine Editor";
               }(),
               wxDefaultPosition, wxSize(720, 840),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Routine Type (radio buttons) - only in create mode
    if (mode_ != RoutineEditorMode::EditRoutine) {
        auto* typeLabel = new wxStaticText(this, wxID_ANY, "Routine Type");
        rootSizer->Add(typeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);

        auto* typeSizer = new wxBoxSizer(wxHORIZONTAL);
        procedure_radio_ = new wxRadioButton(this, wxID_ANY, "Procedure",
                                              wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
        function_radio_ = new wxRadioButton(this, wxID_ANY, "Function");

        if (mode_ == RoutineEditorMode::CreateFunction) {
            function_radio_->SetValue(true);
        } else {
            procedure_radio_->SetValue(true);
        }

        typeSizer->Add(procedure_radio_, 0, wxRIGHT, 16);
        typeSizer->Add(function_radio_, 0);
        rootSizer->Add(typeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        procedure_radio_->Bind(wxEVT_RADIOBUTTON,
            [this](wxCommandEvent&) { UpdateRoutineTypeFields(); });
        function_radio_->Bind(wxEVT_RADIOBUTTON,
            [this](wxCommandEvent&) { UpdateRoutineTypeFields(); });
    }

    // Routine Name
    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Routine Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Schema
    auto* schemaLabel = new wxStaticText(this, wxID_ANY, "Schema");
    rootSizer->Add(schemaLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    schema_choice_ = BuildChoice(this, {"public"});
    rootSizer->Add(schema_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Language
    auto* languageLabel = new wxStaticText(this, wxID_ANY, "Language");
    rootSizer->Add(languageLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    language_choice_ = BuildChoice(this, {"SQL", "PLSQL", "JAVA", "C", "PYTHON", "JAVASCRIPT"});
    rootSizer->Add(language_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Deterministic checkbox
    deterministic_ctrl_ = new wxCheckBox(this, wxID_ANY, "DETERMINISTIC");
    deterministic_ctrl_->SetToolTip("Function returns the same result for the same input");
    rootSizer->Add(deterministic_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Security Type
    auto* securityLabel = new wxStaticText(this, wxID_ANY, "SQL Security");
    rootSizer->Add(securityLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    security_choice_ = BuildChoice(this, {"INVOKER", "DEFINER"});
    rootSizer->Add(security_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Return Type (only for functions)
    return_type_label_ = new wxStaticText(this, wxID_ANY, "Return Type");
    rootSizer->Add(return_type_label_, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    return_type_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    return_type_ctrl_->SetHint("INT, VARCHAR(100), TABLE(...), etc.");
    rootSizer->Add(return_type_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Parameters section
    auto* paramsLabel = new wxStaticText(this, wxID_ANY, "Parameters");
    rootSizer->Add(paramsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    // Parameters grid
    params_grid_ = new wxGrid(this, wxID_ANY);
    params_grid_->CreateGrid(0, 4);
    params_grid_->SetColLabelValue(0, "Name");
    params_grid_->SetColLabelValue(1, "Data Type");
    params_grid_->SetColLabelValue(2, "Mode");
    params_grid_->SetColLabelValue(3, "Default Value");
    params_grid_->SetColSize(0, 140);
    params_grid_->SetColSize(1, 140);
    params_grid_->SetColSize(2, 80);
    params_grid_->SetColSize(3, 140);
    params_grid_->SetMinSize(wxSize(-1, 150));
    rootSizer->Add(params_grid_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Parameter buttons
    auto* paramButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    add_param_button_ = new wxButton(this, wxID_ANY, "Add");
    remove_param_button_ = new wxButton(this, wxID_ANY, "Remove");
    edit_param_button_ = new wxButton(this, wxID_ANY, "Edit");
    paramButtonSizer->Add(add_param_button_, 0, wxRIGHT, 8);
    paramButtonSizer->Add(remove_param_button_, 0, wxRIGHT, 8);
    paramButtonSizer->Add(edit_param_button_, 0);
    rootSizer->Add(paramButtonSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Routine Body
    auto* bodyLabel = new wxStaticText(this, wxID_ANY, "Routine Body");
    rootSizer->Add(bodyLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    body_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                wxSize(-1, 200), wxTE_MULTILINE);
    body_ctrl_->SetHint("BEGIN\n  -- your code here\nEND;");
    rootSizer->Add(body_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Buttons
    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);

    SetSizerAndFit(rootSizer);
    CentreOnParent();

    // Bind events
    add_param_button_->Bind(wxEVT_BUTTON, &RoutineEditorDialog::OnAddParameter, this);
    remove_param_button_->Bind(wxEVT_BUTTON, &RoutineEditorDialog::OnRemoveParameter, this);
    edit_param_button_->Bind(wxEVT_BUTTON, &RoutineEditorDialog::OnEditParameter, this);
    params_grid_->Bind(wxEVT_GRID_CELL_LEFT_DCLICK, &RoutineEditorDialog::OnGridDoubleClick, this);
    params_grid_->Bind(wxEVT_GRID_SELECT_CELL,
        [this](wxGridEvent&) { UpdateParameterButtons(); });

    // Initial state update
    UpdateRoutineTypeFields();
    UpdateParameterButtons();
}

std::string RoutineEditorDialog::BuildSql() const {
    switch (mode_) {
        case RoutineEditorMode::CreateProcedure:
            return BuildCreateProcedureSql();
        case RoutineEditorMode::CreateFunction:
            return BuildCreateFunctionSql();
        case RoutineEditorMode::EditRoutine:
            return BuildEditRoutineSql();
    }
    return std::string();
}

std::string RoutineEditorDialog::routine_name() const {
    return Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
}

void RoutineEditorDialog::SetRoutineName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
    }
}

void RoutineEditorDialog::SetSchema(const std::string& schema) {
    if (schema_choice_) {
        int idx = schema_choice_->FindString(schema);
        if (idx != wxNOT_FOUND) {
            schema_choice_->SetSelection(idx);
        } else {
            schema_choice_->Append(schema);
            schema_choice_->SetSelection(schema_choice_->GetCount() - 1);
        }
    }
}

void RoutineEditorDialog::SetParameters(const std::vector<RoutineParameter>& params) {
    if (!params_grid_) {
        return;
    }
    // Clear existing rows
    while (params_grid_->GetNumberRows() > 0) {
        params_grid_->DeleteRows(0);
    }
    // Add new rows
    for (const auto& param : params) {
        int row = params_grid_->GetNumberRows();
        params_grid_->AppendRows(1);
        params_grid_->SetCellValue(row, 0, param.name);
        params_grid_->SetCellValue(row, 1, param.data_type);
        params_grid_->SetCellValue(row, 2, param.mode);
        params_grid_->SetCellValue(row, 3, param.default_value);
    }
    UpdateParameterButtons();
}

void RoutineEditorDialog::SetBody(const std::string& body) {
    if (body_ctrl_) {
        body_ctrl_->SetValue(body);
    }
}

void RoutineEditorDialog::SetReturnType(const std::string& return_type) {
    if (return_type_ctrl_) {
        return_type_ctrl_->SetValue(return_type);
    }
}

void RoutineEditorDialog::SetLanguage(const std::string& language) {
    if (language_choice_) {
        int idx = language_choice_->FindString(language);
        if (idx != wxNOT_FOUND) {
            language_choice_->SetSelection(idx);
        }
    }
}

void RoutineEditorDialog::SetDeterministic(bool deterministic) {
    if (deterministic_ctrl_) {
        deterministic_ctrl_->SetValue(deterministic);
    }
}

void RoutineEditorDialog::SetSecurityType(const std::string& security_type) {
    if (security_choice_) {
        int idx = security_choice_->FindString(security_type);
        if (idx != wxNOT_FOUND) {
            security_choice_->SetSelection(idx);
        }
    }
}

std::string RoutineEditorDialog::BuildCreateProcedureSql() const {
    std::string name = routine_name();
    if (name.empty()) {
        return std::string();
    }

    std::string schema = schema_choice_ ? schema_choice_->GetStringSelection().ToStdString()
                                        : std::string("public");
    std::string body = Trim(body_ctrl_ ? body_ctrl_->GetValue().ToStdString() : std::string());

    std::ostringstream sql;
    sql << "CREATE PROCEDURE ";
    if (!schema.empty() && schema != "public") {
        sql << QuoteIdentifier(schema) << ".";
    }
    sql << QuoteIdentifier(name);

    // Parameters
    std::string params = BuildParametersClause();
    if (!params.empty()) {
        sql << "(" << params << ")";
    }
    sql << "\n";

    // Language
    if (language_choice_) {
        sql << "  LANGUAGE " << language_choice_->GetStringSelection().ToStdString() << "\n";
    }

    // Deterministic
    if (deterministic_ctrl_ && deterministic_ctrl_->IsChecked()) {
        sql << "  DETERMINISTIC\n";
    }

    // Security
    if (security_choice_) {
        sql << "  SQL SECURITY " << security_choice_->GetStringSelection().ToStdString() << "\n";
    }

    // Body
    sql << "BEGIN\n";
    if (!body.empty()) {
        sql << body;
        if (body.back() != '\n') {
            sql << "\n";
        }
    }
    sql << "END;";

    return sql.str();
}

std::string RoutineEditorDialog::BuildCreateFunctionSql() const {
    std::string name = routine_name();
    if (name.empty()) {
        return std::string();
    }

    std::string schema = schema_choice_ ? schema_choice_->GetStringSelection().ToStdString()
                                        : std::string("public");
    std::string return_type = Trim(return_type_ctrl_ ? return_type_ctrl_->GetValue().ToStdString()
                                                      : std::string());
    std::string body = Trim(body_ctrl_ ? body_ctrl_->GetValue().ToStdString() : std::string());

    if (return_type.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "CREATE FUNCTION ";
    if (!schema.empty() && schema != "public") {
        sql << QuoteIdentifier(schema) << ".";
    }
    sql << QuoteIdentifier(name);

    // Parameters (without mode for functions typically)
    std::string params = BuildParametersClause();
    if (!params.empty()) {
        sql << "(" << params << ")";
    }
    sql << "\n";

    // Return type
    sql << "  RETURNS " << return_type << "\n";

    // Language
    if (language_choice_) {
        sql << "  LANGUAGE " << language_choice_->GetStringSelection().ToStdString() << "\n";
    }

    // Deterministic
    if (deterministic_ctrl_ && deterministic_ctrl_->IsChecked()) {
        sql << "  DETERMINISTIC\n";
    }

    // Security
    if (security_choice_) {
        sql << "  SQL SECURITY " << security_choice_->GetStringSelection().ToStdString() << "\n";
    }

    // Body
    sql << "BEGIN\n";
    if (!body.empty()) {
        sql << body;
        if (body.back() != '\n') {
            sql << "\n";
        }
    }
    sql << "END;";

    return sql.str();
}

std::string RoutineEditorDialog::BuildEditRoutineSql() const {
    // For edit mode, generate ALTER statement or full replacement
    // This is a simplified version - typically would use ALTER PROCEDURE/FUNCTION
    std::string name = routine_name();
    if (name.empty()) {
        return std::string();
    }

    std::string schema = schema_choice_ ? schema_choice_->GetStringSelection().ToStdString()
                                        : std::string("public");
    std::ostringstream sql;

    // First drop existing, then create new
    sql << "-- Drop existing routine\n";
    sql << "DROP PROCEDURE IF EXISTS ";
    if (!schema.empty() && schema != "public") {
        sql << QuoteIdentifier(schema) << ".";
    }
    sql << QuoteIdentifier(name) << ";\n\n";

    // Generate new create statement based on current settings
    bool is_function = function_radio_ && function_radio_->GetValue();
    if (is_function) {
        sql << BuildCreateFunctionSql();
    } else {
        sql << BuildCreateProcedureSql();
    }

    return sql.str();
}

std::string RoutineEditorDialog::BuildParametersClause() const {
    if (!params_grid_ || params_grid_->GetNumberRows() == 0) {
        return std::string();
    }

    std::ostringstream params;
    for (int i = 0; i < params_grid_->GetNumberRows(); ++i) {
        if (i > 0) {
            params << ", ";
        }

        std::string param_name = Trim(params_grid_->GetCellValue(i, 0).ToStdString());
        std::string data_type = Trim(params_grid_->GetCellValue(i, 1).ToStdString());
        std::string mode = Trim(params_grid_->GetCellValue(i, 2).ToStdString());
        std::string default_val = Trim(params_grid_->GetCellValue(i, 3).ToStdString());

        if (!mode.empty() && mode != "IN") {
            params << mode << " ";
        }
        if (!param_name.empty()) {
            params << QuoteIdentifier(param_name) << " ";
        }
        params << data_type;
        if (!default_val.empty()) {
            params << " DEFAULT " << default_val;
        }
    }
    return params.str();
}

std::string RoutineEditorDialog::FormatRoutinePath(const std::string& value) const {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return trimmed;
    }
    if (trimmed.find('.') != std::string::npos || trimmed.find('"') != std::string::npos) {
        return trimmed;
    }
    return QuoteIdentifier(trimmed);
}

std::string RoutineEditorDialog::QuoteIdentifier(const std::string& value) const {
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

bool RoutineEditorDialog::IsSimpleIdentifier(const std::string& value) const {
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

bool RoutineEditorDialog::IsQuotedIdentifier(const std::string& value) const {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

std::string RoutineEditorDialog::Trim(const std::string& value) const {
    std::string result = value;
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), not_space));
    result.erase(std::find_if(result.rbegin(), result.rend(), not_space).base(), result.end());
    return result;
}

void RoutineEditorDialog::UpdateRoutineTypeFields() {
    bool is_function = false;
    if (mode_ == RoutineEditorMode::CreateFunction) {
        is_function = true;
    } else if (mode_ == RoutineEditorMode::CreateProcedure) {
        is_function = false;
    } else if (function_radio_) {
        is_function = function_radio_->GetValue();
    }

    // Show/hide return type for functions
    if (return_type_label_) {
        return_type_label_->Show(is_function);
    }
    if (return_type_ctrl_) {
        return_type_ctrl_->Show(is_function);
        return_type_ctrl_->Enable(is_function);
    }

    Layout();
}

void RoutineEditorDialog::UpdateParameterButtons() {
    if (!params_grid_) {
        return;
    }
    bool has_selection = params_grid_->GetSelectedRows().GetCount() > 0 ||
                         params_grid_->GetGridCursorRow() >= 0;
    if (remove_param_button_) {
        remove_param_button_->Enable(has_selection);
    }
    if (edit_param_button_) {
        edit_param_button_->Enable(has_selection);
    }
}

void RoutineEditorDialog::OnRoutineTypeChanged(wxCommandEvent& event) {
    UpdateRoutineTypeFields();
}

void RoutineEditorDialog::OnAddParameter(wxCommandEvent& event) {
    if (!params_grid_) {
        return;
    }
    int row = params_grid_->GetNumberRows();
    params_grid_->AppendRows(1);
    params_grid_->SetCellValue(row, 2, "IN");  // Default mode
    params_grid_->SetGridCursor(row, 0);
    params_grid_->EnableCellEditControl();
    UpdateParameterButtons();
}

void RoutineEditorDialog::OnRemoveParameter(wxCommandEvent& event) {
    if (!params_grid_) {
        return;
    }
    wxArrayInt rows = params_grid_->GetSelectedRows();
    if (rows.IsEmpty()) {
        int cursor_row = params_grid_->GetGridCursorRow();
        if (cursor_row >= 0 && cursor_row < params_grid_->GetNumberRows()) {
            params_grid_->DeleteRows(cursor_row);
        }
    } else {
        // Delete in reverse order to maintain indices
        for (int i = static_cast<int>(rows.GetCount()) - 1; i >= 0; --i) {
            params_grid_->DeleteRows(rows[i]);
        }
    }
    UpdateParameterButtons();
}

void RoutineEditorDialog::OnEditParameter(wxCommandEvent& event) {
    if (!params_grid_) {
        return;
    }
    int row = params_grid_->GetGridCursorRow();
    if (row >= 0 && row < params_grid_->GetNumberRows()) {
        params_grid_->EnableCellEditControl();
    }
}

void RoutineEditorDialog::OnGridDoubleClick(wxGridEvent& event) {
    if (params_grid_) {
        params_grid_->EnableCellEditControl();
    }
    event.Skip();
}

} // namespace scratchrobin
