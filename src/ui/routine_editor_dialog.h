/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ROUTINE_EDITOR_DIALOG_H
#define SCRATCHROBIN_ROUTINE_EDITOR_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxButton;
class wxCheckBox;
class wxChoice;
class wxGrid;
class wxGridEvent;
class wxRadioButton;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class RoutineEditorMode {
    CreateProcedure,
    CreateFunction,
    EditRoutine
};

struct RoutineParameter {
    std::string name;
    std::string data_type;
    std::string mode;      // IN, OUT, INOUT
    std::string default_value;
};

class RoutineEditorDialog : public wxDialog {
public:
    RoutineEditorDialog(wxWindow* parent, RoutineEditorMode mode);

    std::string BuildSql() const;
    std::string routine_name() const;

    void SetRoutineName(const std::string& name);
    void SetSchema(const std::string& schema);
    void SetParameters(const std::vector<RoutineParameter>& params);
    void SetBody(const std::string& body);
    void SetReturnType(const std::string& return_type);
    void SetLanguage(const std::string& language);
    void SetDeterministic(bool deterministic);
    void SetSecurityType(const std::string& security_type);

private:
    std::string BuildCreateProcedureSql() const;
    std::string BuildCreateFunctionSql() const;
    std::string BuildEditRoutineSql() const;
    std::string BuildParametersClause() const;
    std::string FormatRoutinePath(const std::string& value) const;
    std::string QuoteIdentifier(const std::string& value) const;
    bool IsSimpleIdentifier(const std::string& value) const;
    bool IsQuotedIdentifier(const std::string& value) const;
    std::string Trim(const std::string& value) const;

    void UpdateRoutineTypeFields();
    void UpdateParameterButtons();

    void OnRoutineTypeChanged(wxCommandEvent& event);
    void OnAddParameter(wxCommandEvent& event);
    void OnRemoveParameter(wxCommandEvent& event);
    void OnEditParameter(wxCommandEvent& event);
    void OnGridDoubleClick(wxGridEvent& event);

    RoutineEditorMode mode_;

    // Routine Type
    wxRadioButton* procedure_radio_ = nullptr;
    wxRadioButton* function_radio_ = nullptr;

    // Basic fields
    wxTextCtrl* name_ctrl_ = nullptr;
    wxChoice* schema_choice_ = nullptr;

    // Language and options
    wxChoice* language_choice_ = nullptr;
    wxCheckBox* deterministic_ctrl_ = nullptr;
    wxChoice* security_choice_ = nullptr;

    // Function-specific
    wxStaticText* return_type_label_ = nullptr;
    wxTextCtrl* return_type_ctrl_ = nullptr;

    // Parameters
    wxGrid* params_grid_ = nullptr;
    wxButton* add_param_button_ = nullptr;
    wxButton* remove_param_button_ = nullptr;
    wxButton* edit_param_button_ = nullptr;

    // Body
    wxTextCtrl* body_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_ROUTINE_EDITOR_DIALOG_H
