/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ROLE_EDITOR_DIALOG_H
#define SCRATCHROBIN_ROLE_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxTextCtrl;
class wxCheckBox;
class wxChoice;

namespace scratchrobin {

enum class RoleEditorMode {
    Create,
    Alter
};

class RoleEditorDialog : public wxDialog {
public:
    RoleEditorDialog(wxWindow* parent, RoleEditorMode mode);

    // Setters for Alter mode
    void SetRoleName(const std::string& name);
    void SetCanLogin(bool value);
    void SetIsSuperUser(bool value);
    void SetDefaultSchema(const std::string& schema);
    
    // Getters for results
    std::string GetRoleName() const;
    std::string GetPassword() const;
    bool GetCanLogin() const;
    bool GetIsSuperUser() const;
    std::string GetDefaultSchema() const;
    
    // Build SQL statement
    std::string BuildSql(const std::string& backend) const;

private:
    void BuildLayout();
    void OnOk(wxCommandEvent& event);

    RoleEditorMode mode_;
    
    wxTextCtrl* name_ctrl_ = nullptr;
    wxTextCtrl* password_ctrl_ = nullptr;
    wxCheckBox* can_login_chk_ = nullptr;
    wxCheckBox* superuser_chk_ = nullptr;
    wxTextCtrl* schema_ctrl_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_ROLE_EDITOR_DIALOG_H
