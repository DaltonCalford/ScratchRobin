/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_SCHEMA_EDITOR_DIALOG_H
#define SCRATCHROBIN_SCHEMA_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class SchemaEditorMode {
    Create,
    Alter
};

class SchemaEditorDialog : public wxDialog {
public:
    SchemaEditorDialog(wxWindow* parent, SchemaEditorMode mode);

    std::string BuildSql() const;
    std::string schema_name() const;

    void SetSchemaName(const std::string& name);

private:
    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    void UpdateAlterActionFields();

    SchemaEditorMode mode_;

    wxTextCtrl* name_ctrl_ = nullptr;
    wxCheckBox* if_not_exists_ctrl_ = nullptr;
    wxTextCtrl* owner_ctrl_ = nullptr;

    wxChoice* alter_action_choice_ = nullptr;
    wxStaticText* alter_value_label_ = nullptr;
    wxTextCtrl* alter_value_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SCHEMA_EDITOR_DIALOG_H
