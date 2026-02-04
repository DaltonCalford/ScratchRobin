/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "privilege_editor_dialog.h"

#include <wx/button.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
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

const char* kTablePrivileges[] = {
    "SELECT",
    "INSERT",
    "UPDATE",
    "DELETE",
    "TRUNCATE",
    "REFERENCES",
    "TRIGGER",
    nullptr
};

const char* kSchemaPrivileges[] = {
    "USAGE",
    "CREATE",
    nullptr
};

const char* kSequencePrivileges[] = {
    "USAGE",
    "SELECT",
    "UPDATE",
    nullptr
};

const char* kDatabasePrivileges[] = {
    "CREATE",
    "CONNECT",
    "TEMPORARY",
    nullptr
};

const char* kFunctionPrivileges[] = {
    "EXECUTE",
    nullptr
};

const char** GetPrivilegesForType(const std::string& type) {
    if (type == "TABLE") return kTablePrivileges;
    if (type == "SCHEMA") return kSchemaPrivileges;
    if (type == "SEQUENCE") return kSequencePrivileges;
    if (type == "DATABASE") return kDatabasePrivileges;
    if (type == "FUNCTION" || type == "PROCEDURE") return kFunctionPrivileges;
    return kTablePrivileges;  // Default
}

} // namespace

wxBEGIN_EVENT_TABLE(PrivilegeEditorDialog, wxDialog)
    EVT_BUTTON(wxID_OK, PrivilegeEditorDialog::OnOk)
wxEND_EVENT_TABLE()

PrivilegeEditorDialog::PrivilegeEditorDialog(wxWindow* parent, PrivilegeOperation operation)
    : wxDialog(parent, wxID_ANY, 
               operation == PrivilegeOperation::Grant ? "Grant Privileges" : "Revoke Privileges",
               wxDefaultPosition, wxSize(450, 500)),
      operation_(operation) {
    BuildLayout();
}

void PrivilegeEditorDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Principal (user/role)
    auto* principalSizer = new wxBoxSizer(wxHORIZONTAL);
    principalSizer->Add(new wxStaticText(this, wxID_ANY, "Principal:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    principal_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    principalSizer->Add(principal_ctrl_, 1, wxEXPAND);
    root->Add(principalSizer, 0, wxEXPAND | wxALL, 12);
    
    // Object type
    auto* typeSizer = new wxBoxSizer(wxHORIZONTAL);
    typeSizer->Add(new wxStaticText(this, wxID_ANY, "Object Type:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    object_type_choice_ = new wxChoice(this, wxID_ANY);
    object_type_choice_->Append("TABLE");
    object_type_choice_->Append("SCHEMA");
    object_type_choice_->Append("SEQUENCE");
    object_type_choice_->Append("DATABASE");
    object_type_choice_->Append("FUNCTION");
    object_type_choice_->SetSelection(0);
    typeSizer->Add(object_type_choice_, 1, wxEXPAND);
    root->Add(typeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Object name
    auto* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    nameSizer->Add(new wxStaticText(this, wxID_ANY, "Object Name:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    object_name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    nameSizer->Add(object_name_ctrl_, 1, wxEXPAND);
    root->Add(nameSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Privileges list
    root->Add(new wxStaticText(this, wxID_ANY, "Privileges:"), 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    privileges_list_ = new wxCheckListBox(this, wxID_ANY);
    UpdatePrivilegeList();
    root->Add(privileges_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Select/Deselect buttons
    auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
    btnRow->Add(new wxButton(this, wxID_ANY, "Select All"), 0, wxRIGHT, 4);
    btnRow->Add(new wxButton(this, wxID_ANY, "Deselect All"), 0);
    root->Add(btnRow, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Grant option (for GRANT only)
    if (operation_ == PrivilegeOperation::Grant) {
        wxArrayString options;
        options.Add("No grant option");
        options.Add("WITH GRANT OPTION");
        options.Add("WITH ADMIN OPTION");
        grant_option_box_ = new wxRadioBox(this, wxID_ANY, "Grant Options", wxDefaultPosition, wxDefaultSize, options, 1);
        grant_option_box_->SetSelection(0);
        root->Add(grant_option_box_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }
    
    // OK/Cancel buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, wxID_OK, 
                   operation_ == PrivilegeOperation::Grant ? "Grant" : "Revoke"), 0);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
}

void PrivilegeEditorDialog::UpdatePrivilegeList() {
    if (!privileges_list_) return;
    
    privileges_list_->Clear();
    
    std::string type = GetObjectType();
    const char** privileges = GetPrivilegesForType(type);
    
    for (int i = 0; privileges[i] != nullptr; ++i) {
        privileges_list_->Append(privileges[i]);
    }
}

void PrivilegeEditorDialog::SetPrincipal(const std::string& principal) {
    if (principal_ctrl_) {
        principal_ctrl_->SetValue(principal);
    }
}

void PrivilegeEditorDialog::SetObjectType(const std::string& type) {
    if (object_type_choice_) {
        int idx = object_type_choice_->FindString(type);
        if (idx != wxNOT_FOUND) {
            object_type_choice_->SetSelection(idx);
            UpdatePrivilegeList();
        }
    }
}

void PrivilegeEditorDialog::SetObjectName(const std::string& name) {
    if (object_name_ctrl_) {
        object_name_ctrl_->SetValue(name);
    }
}

std::string PrivilegeEditorDialog::GetPrincipal() const {
    return principal_ctrl_ ? principal_ctrl_->GetValue().ToStdString() : "";
}

std::string PrivilegeEditorDialog::GetObjectType() const {
    return object_type_choice_ ? object_type_choice_->GetStringSelection().ToStdString() : "TABLE";
}

std::string PrivilegeEditorDialog::GetObjectName() const {
    return object_name_ctrl_ ? object_name_ctrl_->GetValue().ToStdString() : "";
}

std::vector<std::string> PrivilegeEditorDialog::GetSelectedPrivileges() const {
    std::vector<std::string> result;
    if (!privileges_list_) return result;
    
    for (unsigned int i = 0; i < privileges_list_->GetCount(); ++i) {
        if (privileges_list_->IsChecked(i)) {
            result.push_back(privileges_list_->GetString(i).ToStdString());
        }
    }
    return result;
}

bool PrivilegeEditorDialog::GetGrantOption() const {
    return grant_option_box_ ? grant_option_box_->GetSelection() > 0 : false;
}

std::string PrivilegeEditorDialog::GetGrantOptionText() const {
    if (!grant_option_box_) return "";
    int sel = grant_option_box_->GetSelection();
    if (sel == 1) return " WITH GRANT OPTION";
    if (sel == 2) return " WITH ADMIN OPTION";
    return "";
}

void PrivilegeEditorDialog::OnOk(wxCommandEvent& event) {
    if (GetPrincipal().empty()) {
        wxMessageBox("Principal (user/role) is required.", "Validation Error", wxOK | wxICON_ERROR, this);
        return;
    }
    if (GetObjectName().empty()) {
        wxMessageBox("Object name is required.", "Validation Error", wxOK | wxICON_ERROR, this);
        return;
    }
    if (GetSelectedPrivileges().empty()) {
        wxMessageBox("At least one privilege must be selected.", "Validation Error", wxOK | wxICON_ERROR, this);
        return;
    }
    event.Skip();
}

std::string PrivilegeEditorDialog::BuildSql(const std::string& backend) const {
    std::string sql;
    std::string principal = GetPrincipal();
    std::string object_type = GetObjectType();
    std::string object_name = GetObjectName();
    std::vector<std::string> privileges = GetSelectedPrivileges();
    
    if (privileges.empty()) return "";
    
    // Build privilege list
    std::string priv_list;
    for (size_t i = 0; i < privileges.size(); ++i) {
        if (i > 0) priv_list += ", ";
        priv_list += privileges[i];
    }
    
    if (operation_ == PrivilegeOperation::Grant) {
        sql = "GRANT " + priv_list + " ON " + object_type + " " + object_name + 
              " TO " + principal + GetGrantOptionText() + ";";
    } else {
        sql = "REVOKE " + priv_list + " ON " + object_type + " " + object_name + 
              " FROM " + principal + ";";
    }
    
    return sql;
}

} // namespace scratchrobin
