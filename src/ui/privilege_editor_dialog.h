/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_PRIVILEGE_EDITOR_DIALOG_H
#define SCRATCHROBIN_PRIVILEGE_EDITOR_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxTextCtrl;
class wxCheckListBox;
class wxChoice;
class wxRadioBox;

namespace scratchrobin {

enum class PrivilegeOperation {
    Grant,
    Revoke
};

class PrivilegeEditorDialog : public wxDialog {
public:
    PrivilegeEditorDialog(wxWindow* parent, PrivilegeOperation operation);

    // Setters
    void SetPrincipal(const std::string& principal);
    void SetObjectType(const std::string& type);  // TABLE, SCHEMA, etc.
    void SetObjectName(const std::string& name);
    
    // Getters
    std::string GetPrincipal() const;
    std::string GetObjectType() const;
    std::string GetObjectName() const;
    std::vector<std::string> GetSelectedPrivileges() const;
    bool GetGrantOption() const;
    std::string GetGrantOptionText() const;
    
    // Build SQL statement
    std::string BuildSql(const std::string& backend) const;

private:
    void BuildLayout();
    void OnOk(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);
    void OnDeselectAll(wxCommandEvent& event);
    void UpdatePrivilegeList();

    PrivilegeOperation operation_;
    
    wxTextCtrl* principal_ctrl_ = nullptr;
    wxChoice* object_type_choice_ = nullptr;
    wxTextCtrl* object_name_ctrl_ = nullptr;
    wxCheckListBox* privileges_list_ = nullptr;
    wxRadioBox* grant_option_box_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PRIVILEGE_EDITOR_DIALOG_H
