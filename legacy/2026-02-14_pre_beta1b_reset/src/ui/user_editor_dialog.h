/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_USER_EDITOR_DIALOG_H
#define SCRATCHROBIN_USER_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxTextCtrl;
class wxCheckBox;
class wxChoice;

namespace scratchrobin {

enum class UserEditorMode {
    Create,
    Alter
};

class UserEditorDialog : public wxDialog {
public:
    UserEditorDialog(wxWindow* parent, UserEditorMode mode);

    // Setters for Alter mode
    void SetUserName(const std::string& name);
    void SetIsSuperUser(bool value);
    void SetDefaultSchema(const std::string& schema);
    void SetAuthProvider(const std::string& provider);
    
    // Getters for results
    std::string GetUserName() const;
    std::string GetPassword() const;
    bool GetIsSuperUser() const;
    std::string GetDefaultSchema() const;
    std::string GetAuthProvider() const;
    bool GetChangePassword() const;
    
    // Build SQL statement
    std::string BuildSql(const std::string& backend) const;

private:
    void BuildLayout();
    void OnOk(wxCommandEvent& event);

    UserEditorMode mode_;
    
    wxTextCtrl* name_ctrl_ = nullptr;
    wxTextCtrl* password_ctrl_ = nullptr;
    wxCheckBox* change_password_chk_ = nullptr;
    wxCheckBox* superuser_chk_ = nullptr;
    wxTextCtrl* schema_ctrl_ = nullptr;
    wxChoice* auth_choice_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_USER_EDITOR_DIALOG_H
