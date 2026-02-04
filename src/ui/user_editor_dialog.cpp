/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "user_editor_dialog.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

namespace {

std::string EscapeSqlString(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        if (c == '\'') {
            result += "''";
        } else {
            result += c;
        }
    }
    return result;
}

} // namespace

wxBEGIN_EVENT_TABLE(UserEditorDialog, wxDialog)
    EVT_BUTTON(wxID_OK, UserEditorDialog::OnOk)
wxEND_EVENT_TABLE()

UserEditorDialog::UserEditorDialog(wxWindow* parent, UserEditorMode mode)
    : wxDialog(parent, wxID_ANY, mode == UserEditorMode::Create ? "Create User" : "Alter User",
               wxDefaultPosition, wxSize(450, 350)),
      mode_(mode) {
    BuildLayout();
}

void UserEditorDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Username
    auto* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    nameSizer->Add(new wxStaticText(this, wxID_ANY, "Username:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    nameSizer->Add(name_ctrl_, 1, wxEXPAND);
    root->Add(nameSizer, 0, wxEXPAND | wxALL, 12);
    
    // Password (create mode always, alter mode optional)
    auto* passSizer = new wxBoxSizer(wxHORIZONTAL);
    passSizer->Add(new wxStaticText(this, wxID_ANY, "Password:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    password_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    passSizer->Add(password_ctrl_, 1, wxEXPAND);
    root->Add(passSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    if (mode_ == UserEditorMode::Alter) {
        change_password_chk_ = new wxCheckBox(this, wxID_ANY, "Change password");
        root->Add(change_password_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }
    
    // Superuser
    superuser_chk_ = new wxCheckBox(this, wxID_ANY, "Superuser");
    root->Add(superuser_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Default schema
    auto* schemaSizer = new wxBoxSizer(wxHORIZONTAL);
    schemaSizer->Add(new wxStaticText(this, wxID_ANY, "Default Schema:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    schema_ctrl_ = new wxTextCtrl(this, wxID_ANY, "public");
    schemaSizer->Add(schema_ctrl_, 1, wxEXPAND);
    root->Add(schemaSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Auth provider
    auto* authSizer = new wxBoxSizer(wxHORIZONTAL);
    authSizer->Add(new wxStaticText(this, wxID_ANY, "Auth Provider:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    auth_choice_ = new wxChoice(this, wxID_ANY);
    auth_choice_->Append("LOCAL");
    auth_choice_->Append("LDAP");
    auth_choice_->Append("Kerberos");
    auth_choice_->Append("OAuth");
    auth_choice_->SetSelection(0);
    authSizer->Add(auth_choice_, 1, wxEXPAND);
    root->Add(authSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, wxID_OK, mode_ == UserEditorMode::Create ? "Create" : "Alter"), 0);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
    
    if (mode_ == UserEditorMode::Alter) {
        name_ctrl_->Enable(false);
    }
}

void UserEditorDialog::SetUserName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
    }
}

void UserEditorDialog::SetIsSuperUser(bool value) {
    if (superuser_chk_) {
        superuser_chk_->SetValue(value);
    }
}

void UserEditorDialog::SetDefaultSchema(const std::string& schema) {
    if (schema_ctrl_) {
        schema_ctrl_->SetValue(schema);
    }
}

void UserEditorDialog::SetAuthProvider(const std::string& provider) {
    if (auth_choice_) {
        int idx = auth_choice_->FindString(provider);
        if (idx != wxNOT_FOUND) {
            auth_choice_->SetSelection(idx);
        }
    }
}

std::string UserEditorDialog::GetUserName() const {
    return name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : "";
}

std::string UserEditorDialog::GetPassword() const {
    return password_ctrl_ ? password_ctrl_->GetValue().ToStdString() : "";
}

bool UserEditorDialog::GetIsSuperUser() const {
    return superuser_chk_ ? superuser_chk_->GetValue() : false;
}

std::string UserEditorDialog::GetDefaultSchema() const {
    return schema_ctrl_ ? schema_ctrl_->GetValue().ToStdString() : "";
}

std::string UserEditorDialog::GetAuthProvider() const {
    return auth_choice_ ? auth_choice_->GetStringSelection().ToStdString() : "";
}

bool UserEditorDialog::GetChangePassword() const {
    return change_password_chk_ ? change_password_chk_->GetValue() : true;
}

void UserEditorDialog::OnOk(wxCommandEvent& event) {
    if (GetUserName().empty()) {
        wxMessageBox("Username is required.", "Validation Error", wxOK | wxICON_ERROR, this);
        return;
    }
    if (mode_ == UserEditorMode::Create && GetPassword().empty()) {
        wxMessageBox("Password is required for new users.", "Validation Error", wxOK | wxICON_ERROR, this);
        return;
    }
    event.Skip();
}

std::string UserEditorDialog::BuildSql(const std::string& backend) const {
    std::string sql;
    std::string name = GetUserName();
    std::string password = GetPassword();
    bool superuser = GetIsSuperUser();
    std::string schema = GetDefaultSchema();
    std::string auth = GetAuthProvider();
    
    if (backend == "native" || backend == "scratchbird") {
        if (mode_ == UserEditorMode::Create) {
            sql = "CREATE USER " + name + " WITH PASSWORD '" + EscapeSqlString(password) + "'";
            if (superuser) sql += " SUPERUSER";
            if (!schema.empty() && schema != "public") sql += " DEFAULT SCHEMA " + schema;
            if (!auth.empty() && auth != "LOCAL") sql += " AUTH PROVIDER " + auth;
            sql += ";";
        } else {
            sql = "ALTER USER " + name;
            if (GetChangePassword() && !password.empty()) {
                sql += " WITH PASSWORD '" + EscapeSqlString(password) + "'";
            }
            sql += superuser ? " SUPERUSER" : " NOSUPERUSER";
            if (!schema.empty()) sql += " DEFAULT SCHEMA " + schema;
            sql += ";";
        }
    } else if (backend == "postgresql") {
        if (mode_ == UserEditorMode::Create) {
            sql = "CREATE ROLE " + name + " WITH LOGIN PASSWORD '" + EscapeSqlString(password) + "'";
            if (superuser) sql += " SUPERUSER";
            sql += ";";
        } else {
            sql = "ALTER ROLE " + name;
            if (GetChangePassword() && !password.empty()) {
                sql += " WITH PASSWORD '" + EscapeSqlString(password) + "'";
            }
            sql += superuser ? " SUPERUSER" : " NOSUPERUSER";
            sql += ";";
        }
    } else if (backend == "mysql") {
        if (mode_ == UserEditorMode::Create) {
            sql = "CREATE USER '" + name + "'@'localhost' IDENTIFIED BY '" + EscapeSqlString(password) + "';";
        } else {
            sql = "ALTER USER '" + name + "'@'localhost' ";
            if (GetChangePassword() && !password.empty()) {
                sql += "IDENTIFIED BY '" + EscapeSqlString(password) + "'";
            }
            sql += ";";
        }
    } else if (backend == "firebird") {
        if (mode_ == UserEditorMode::Create) {
            sql = "CREATE USER " + name + " PASSWORD '" + EscapeSqlString(password) + "';";
        } else {
            sql = "ALTER USER " + name + " ";
            if (GetChangePassword() && !password.empty()) {
                sql += "PASSWORD '" + EscapeSqlString(password) + "'";
            }
            sql += ";";
        }
    }
    
    return sql;
}

} // namespace scratchrobin
