/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_TRIGGER_EDITOR_DIALOG_H
#define SCRATCHROBIN_TRIGGER_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class TriggerEditorMode {
    Create,
    Edit
};

class TriggerEditorDialog : public wxDialog {
public:
    TriggerEditorDialog(wxWindow* parent, TriggerEditorMode mode);

    std::string BuildSql() const;

    std::string trigger_name() const;
    void SetTriggerName(const std::string& name);
    void SetTableName(const std::string& name);
    void SetSchemaName(const std::string& name);

private:
    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    std::string FormatTriggerPath(const std::string& value) const;
    std::string FormatTablePath(const std::string& value) const;
    std::string BuildEventsClause() const;
    void UpdateAlterActionFields();

    TriggerEditorMode mode_;

    // Create mode controls
    wxTextCtrl* name_ctrl_ = nullptr;
    wxChoice* schema_choice_ = nullptr;
    wxTextCtrl* table_ctrl_ = nullptr;
    wxChoice* timing_choice_ = nullptr;
    wxCheckBox* insert_event_ctrl_ = nullptr;
    wxCheckBox* update_event_ctrl_ = nullptr;
    wxCheckBox* delete_event_ctrl_ = nullptr;
    wxChoice* for_each_choice_ = nullptr;
    wxTextCtrl* when_condition_ctrl_ = nullptr;
    wxTextCtrl* trigger_body_ctrl_ = nullptr;

    // Edit mode controls
    wxChoice* alter_action_choice_ = nullptr;
    wxStaticText* alter_value_label_ = nullptr;
    wxTextCtrl* alter_value_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TRIGGER_EDITOR_DIALOG_H
