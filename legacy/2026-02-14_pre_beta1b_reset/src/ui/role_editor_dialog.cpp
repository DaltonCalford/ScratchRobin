/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "role_editor_dialog.h"

#include <wx/button.h>
#include <wx/checkbox.h>
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

wxBEGIN_EVENT_TABLE(RoleEditorDialog, wxDialog)
    EVT_BUTTON(wxID_OK, RoleEditorDialog::OnOk)
wxEND_EVENT_TABLE()

RoleEditorDialog::RoleEditorDialog(wxWindow* parent, RoleEditorMode mode)
    : wxDialog(parent, wxID_ANY, mode == RoleEditorMode::Create ? "Create Role" : "Alter Role",
               wxDefaultPosition, wxSize(400, 280)),
      mode_(mode) {
    BuildLayout();
}

void RoleEditorDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Role name
    auto* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    nameSizer->Add(new wxStaticText(this, wxID_ANY, "Role Name:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    nameSizer->Add(name_ctrl_, 1, wxEXPAND);
    root->Add(nameSizer, 0, wxEXPAND | wxALL, 12);
    
    // Can login (create login role vs non-login role)
    can_login_chk_ = new wxCheckBox(this, wxID_ANY, "Can Login (LOGIN role)");
    can_login_chk_->SetValue(false);
    root->Add(can_login_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Password (only for login roles)
    auto* passSizer = new wxBoxSizer(wxHORIZONTAL);
    passSizer->Add(new wxStaticText(this, wxID_ANY, "Password:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    password_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    passSizer->Add(password_ctrl_, 1, wxEXPAND);
    root->Add(passSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Superuser
    superuser_chk_ = new wxCheckBox(this, wxID_ANY, "Superuser privileges");
    root->Add(superuser_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Default schema
    auto* schemaSizer = new wxBoxSizer(wxHORIZONTAL);
    schemaSizer->Add(new wxStaticText(this, wxID_ANY, "Default Schema:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    schema_ctrl_ = new wxTextCtrl(this, wxID_ANY, "public");
    schemaSizer->Add(schema_ctrl_, 1, wxEXPAND);
    root->Add(schemaSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, wxID_OK, mode_ == RoleEditorMode::Create ? "Create" : "Alter"), 0);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
    
    if (mode_ == RoleEditorMode::Alter) {
        name_ctrl_->Enable(false);
    }
}

void RoleEditorDialog::SetRoleName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
    }
}

void RoleEditorDialog::SetCanLogin(bool value) {
    if (can_login_chk_) {
        can_login_chk_->SetValue(value);
    }
}

void RoleEditorDialog::SetIsSuperUser(bool value) {
    if (superuser_chk_) {
        superuser_chk_->SetValue(value);
    }
}

void RoleEditorDialog::SetDefaultSchema(const std::string& schema) {
    if (schema_ctrl_) {
        schema_ctrl_->SetValue(schema);
    }
}

std::string RoleEditorDialog::GetRoleName() const {
    return name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : "";
}

std::string RoleEditorDialog::GetPassword() const {
    return password_ctrl_ ? password_ctrl_->GetValue().ToStdString() : "";
}

bool RoleEditorDialog::GetCanLogin() const {
    return can_login_chk_ ? can_login_chk_->GetValue() : false;
}

bool RoleEditorDialog::GetIsSuperUser() const {
    return superuser_chk_ ? superuser_chk_->GetValue() : false;
}

std::string RoleEditorDialog::GetDefaultSchema() const {
    return schema_ctrl_ ? schema_ctrl_->GetValue().ToStdString() : "";
}

void RoleEditorDialog::OnOk(wxCommandEvent& event) {
    if (GetRoleName().empty()) {
        wxMessageBox("Role name is required.", "Validation Error", wxOK | wxICON_ERROR, this);
        return;
    }
    if (GetCanLogin() && mode_ == RoleEditorMode::Create && GetPassword().empty()) {
        wxMessageBox("Password is required for login roles.", "Validation Error", wxOK | wxICON_ERROR, this);
        return;
    }
    event.Skip();
}

std::string RoleEditorDialog::BuildSql(const std::string& backend) const {
    std::string sql;
    std::string name = GetRoleName();
    std::string password = GetPassword();
    bool can_login = GetCanLogin();
    bool superuser = GetIsSuperUser();
    std::string schema = GetDefaultSchema();
    
    if (backend == "native" || backend == "scratchbird") {
        if (mode_ == RoleEditorMode::Create) {
            if (can_login) {
                sql = "CREATE ROLE " + name + " WITH LOGIN PASSWORD '" + EscapeSqlString(password) + "'";
            } else {
                sql = "CREATE ROLE " + name + " NOLOGIN";
            }
            if (superuser) sql += " SUPERUSER";
            if (!schema.empty() && schema != "public") sql += " DEFAULT SCHEMA " + schema;
            sql += ";";
        } else {
            sql = "ALTER ROLE " + name;
            sql += can_login ? " WITH LOGIN" : " WITH NOLOGIN";
            sql += superuser ? " SUPERUSER" : " NOSUPERUSER";
            sql += ";";
        }
    } else if (backend == "postgresql") {
        if (mode_ == RoleEditorMode::Create) {
            sql = "CREATE ROLE " + name;
            if (can_login) sql += " WITH LOGIN";
            if (can_login && !password.empty()) sql += " PASSWORD '" + EscapeSqlString(password) + "'";
            if (superuser) sql += " SUPERUSER";
            sql += ";";
        } else {
            sql = "ALTER ROLE " + name;
            sql += can_login ? " WITH LOGIN" : " WITH NOLOGIN";
            sql += superuser ? " SUPERUSER" : " NOSUPERUSER";
            sql += ";";
        }
    } else if (backend == "mysql") {
        if (mode_ == RoleEditorMode::Create) {
            if (can_login) {
                sql = "CREATE USER '" + name + "'@'localhost' IDENTIFIED BY '" + EscapeSqlString(password) + "';";
            } else {
                sql = "CREATE ROLE '" + name + "';";
            }
        } else {
            // MySQL doesn't have a direct ALTER ROLE for login vs non-login
            sql = "-- Alter role: manual intervention required for MySQL";
        }
    } else if (backend == "firebird") {
        if (mode_ == RoleEditorMode::Create) {
            sql = "CREATE ROLE " + name + ";";
        } else {
            sql = "-- Firebird: roles cannot be altered after creation; drop and recreate";
        }
    }
    
    return sql;
}

} // namespace scratchrobin
