/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_TABLESPACE_EDITOR_DIALOG_H
#define SCRATCHROBIN_TABLESPACE_EDITOR_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxButton;
class wxCheckBox;
class wxChoice;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class TablespaceEditorMode {
    Create,
    Edit
};

class TablespaceEditorDialog : public wxDialog {
public:
    TablespaceEditorDialog(wxWindow* parent, TablespaceEditorMode mode);

    std::string BuildSql() const;
    std::string tablespace_name() const;

    void SetTablespaceName(const std::string& name);
    void SetOwner(const std::string& owner);
    void SetLocation(const std::string& location);
    void SetSize(const std::string& size);
    void SetAutoextend(bool enabled, const std::string& increment, const std::string& maxsize);

private:
    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    std::string FormatSize(int64_t value, const std::string& unit) const;
    int64_t ParseSize(const std::string& value) const;
    void UpdateAutoextendFields();
    void OnBrowseLocation(wxCommandEvent& event);

    TablespaceEditorMode mode_;

    // Basic info
    wxTextCtrl* name_ctrl_ = nullptr;
    wxChoice* owner_choice_ = nullptr;
    wxTextCtrl* location_ctrl_ = nullptr;
    wxButton* browse_button_ = nullptr;

    // Size settings
    wxSpinCtrl* size_ctrl_ = nullptr;
    wxChoice* size_unit_choice_ = nullptr;

    // Auto-extend settings
    wxCheckBox* autoextend_ctrl_ = nullptr;
    wxSpinCtrl* increment_ctrl_ = nullptr;
    wxChoice* increment_unit_choice_ = nullptr;

    // Max size settings
    wxCheckBox* unlimited_max_ctrl_ = nullptr;
    wxSpinCtrl* max_size_ctrl_ = nullptr;
    wxChoice* max_size_unit_choice_ = nullptr;

    wxStaticText* increment_label_ = nullptr;
    wxStaticText* max_size_label_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TABLESPACE_EDITOR_DIALOG_H
