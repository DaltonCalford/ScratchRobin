/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "object_metadata_dialog_new.h"

#include <map>
#include <vector>

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/clipbrd.h>

#include "backend/session_client.h"

namespace scratchrobin::ui {

// Metadata cache structure
struct ObjectMetadataDialogNew::MetadataCache {
  std::string name;
  std::string type;
  std::string owner;
  std::string created;
  std::string modified;
  std::string description;
  std::string ddl;
  std::vector<std::map<std::string, std::string>> columns;
  std::vector<std::map<std::string, std::string>> constraints;
  std::vector<std::map<std::string, std::string>> indexes;
  std::vector<std::map<std::string, std::string>> permissions;
};

// Control IDs
enum {
  ID_REFRESH = wxID_HIGHEST + 1,
  ID_EXPORT_DDL,
  ID_COPY_CLIPBOARD,
};

wxBEGIN_EVENT_TABLE(ObjectMetadataDialogNew, wxDialog)
  EVT_BUTTON(ID_REFRESH, ObjectMetadataDialogNew::OnRefresh)
  EVT_BUTTON(ID_EXPORT_DDL, ObjectMetadataDialogNew::OnExportDDL)
  EVT_BUTTON(ID_COPY_CLIPBOARD, ObjectMetadataDialogNew::OnCopyToClipboard)
  EVT_BUTTON(wxID_CLOSE, ObjectMetadataDialogNew::OnClose)
wxEND_EVENT_TABLE()

ObjectMetadataDialogNew::ObjectMetadataDialogNew(
    wxWindow* parent,
    scratchrobin::backend::SessionClient* session_client,
    const std::string& object_name,
    const std::string& object_type)
    : wxDialog(parent, wxID_ANY, 
               wxString::Format(_("Object Metadata - %s"), object_name),
               wxDefaultPosition, wxSize(700, 550)),
      session_client_(session_client),
      object_name_(object_name),
      object_type_(object_type),
      cache_(std::make_unique<MetadataCache>()) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

ObjectMetadataDialogNew::~ObjectMetadataDialogNew() = default;

void ObjectMetadataDialogNew::CreateControls() {
  // Create notebook for tabs
  notebook_ = new wxNotebook(this, wxID_ANY);

  // General tab
  wxPanel* general_panel = new wxPanel(notebook_);
  name_label_ = new wxStaticText(general_panel, wxID_ANY, wxEmptyString);
  type_label_ = new wxStaticText(general_panel, wxID_ANY, wxEmptyString);
  owner_label_ = new wxStaticText(general_panel, wxID_ANY, wxEmptyString);
  created_label_ = new wxStaticText(general_panel, wxID_ANY, wxEmptyString);
  modified_label_ = new wxStaticText(general_panel, wxID_ANY, wxEmptyString);
  description_label_ = new wxStaticText(general_panel, wxID_ANY, wxEmptyString);
  
  wxFlexGridSizer* general_sizer = new wxFlexGridSizer(2, 2, 10, 10);
  general_sizer->AddGrowableCol(1, 1);
  general_sizer->Add(new wxStaticText(general_panel, wxID_ANY, _("Name:")), 0, wxALIGN_CENTER_VERTICAL);
  general_sizer->Add(name_label_, 1, wxEXPAND);
  general_sizer->Add(new wxStaticText(general_panel, wxID_ANY, _("Type:")), 0, wxALIGN_CENTER_VERTICAL);
  general_sizer->Add(type_label_, 1, wxEXPAND);
  general_sizer->Add(new wxStaticText(general_panel, wxID_ANY, _("Owner:")), 0, wxALIGN_CENTER_VERTICAL);
  general_sizer->Add(owner_label_, 1, wxEXPAND);
  general_sizer->Add(new wxStaticText(general_panel, wxID_ANY, _("Created:")), 0, wxALIGN_CENTER_VERTICAL);
  general_sizer->Add(created_label_, 1, wxEXPAND);
  general_sizer->Add(new wxStaticText(general_panel, wxID_ANY, _("Modified:")), 0, wxALIGN_CENTER_VERTICAL);
  general_sizer->Add(modified_label_, 1, wxEXPAND);
  general_sizer->Add(new wxStaticText(general_panel, wxID_ANY, _("Description:")), 0, wxALIGN_TOP);
  general_sizer->Add(description_label_, 1, wxEXPAND);
  
  general_panel->SetSizer(general_sizer);
  notebook_->AddPage(general_panel, _("General"));

  // Columns tab (for tables/views)
  wxPanel* columns_panel = new wxPanel(notebook_);
  columns_list_ = new wxListCtrl(columns_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);
  columns_list_->InsertColumn(0, _("Name"), wxLIST_FORMAT_LEFT, 150);
  columns_list_->InsertColumn(1, _("Data Type"), wxLIST_FORMAT_LEFT, 120);
  columns_list_->InsertColumn(2, _("Nullable"), wxLIST_FORMAT_CENTER, 70);
  columns_list_->InsertColumn(3, _("Default"), wxLIST_FORMAT_LEFT, 150);
  columns_list_->InsertColumn(4, _("Description"), wxLIST_FORMAT_LEFT, 200);
  
  wxBoxSizer* columns_sizer = new wxBoxSizer(wxVERTICAL);
  columns_sizer->Add(columns_list_, 1, wxEXPAND);
  columns_panel->SetSizer(columns_sizer);
  notebook_->AddPage(columns_panel, _("Columns"));

  // Constraints tab
  wxPanel* constraints_panel = new wxPanel(notebook_);
  constraints_list_ = new wxListCtrl(constraints_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);
  constraints_list_->InsertColumn(0, _("Name"), wxLIST_FORMAT_LEFT, 150);
  constraints_list_->InsertColumn(1, _("Type"), wxLIST_FORMAT_LEFT, 100);
  constraints_list_->InsertColumn(2, _("Columns"), wxLIST_FORMAT_LEFT, 200);
  constraints_list_->InsertColumn(3, _("Reference"), wxLIST_FORMAT_LEFT, 200);
  
  wxBoxSizer* constraints_sizer = new wxBoxSizer(wxVERTICAL);
  constraints_sizer->Add(constraints_list_, 1, wxEXPAND);
  constraints_panel->SetSizer(constraints_sizer);
  notebook_->AddPage(constraints_panel, _("Constraints"));

  // Indexes tab
  wxPanel* indexes_panel = new wxPanel(notebook_);
  indexes_list_ = new wxListCtrl(indexes_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);
  indexes_list_->InsertColumn(0, _("Name"), wxLIST_FORMAT_LEFT, 150);
  indexes_list_->InsertColumn(1, _("Type"), wxLIST_FORMAT_LEFT, 100);
  indexes_list_->InsertColumn(2, _("Columns"), wxLIST_FORMAT_LEFT, 200);
  indexes_list_->InsertColumn(3, _("Unique"), wxLIST_FORMAT_CENTER, 60);
  indexes_list_->InsertColumn(4, _("Active"), wxLIST_FORMAT_CENTER, 60);
  
  wxBoxSizer* indexes_sizer = new wxBoxSizer(wxVERTICAL);
  indexes_sizer->Add(indexes_list_, 1, wxEXPAND);
  indexes_panel->SetSizer(indexes_sizer);
  notebook_->AddPage(indexes_panel, _("Indexes"));

  // Dependencies tab
  wxPanel* dependencies_panel = new wxPanel(notebook_);
  dependencies_tree_ = new wxTreeCtrl(dependencies_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT);
  
  wxBoxSizer* deps_sizer = new wxBoxSizer(wxVERTICAL);
  deps_sizer->Add(dependencies_tree_, 1, wxEXPAND);
  dependencies_panel->SetSizer(deps_sizer);
  notebook_->AddPage(dependencies_panel, _("Dependencies"));

  // DDL tab
  wxPanel* ddl_panel = new wxPanel(notebook_);
  ddl_text_ = new wxTextCtrl(ddl_panel, wxID_ANY, wxEmptyString,
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
  ddl_text_->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
  
  wxBoxSizer* ddl_sizer = new wxBoxSizer(wxVERTICAL);
  ddl_sizer->Add(ddl_text_, 1, wxEXPAND);
  ddl_panel->SetSizer(ddl_sizer);
  notebook_->AddPage(ddl_panel, _("DDL"));

  // Permissions tab
  wxPanel* permissions_panel = new wxPanel(notebook_);
  permissions_list_ = new wxListCtrl(permissions_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);
  permissions_list_->InsertColumn(0, _("Grantee"), wxLIST_FORMAT_LEFT, 120);
  permissions_list_->InsertColumn(1, _("Privilege"), wxLIST_FORMAT_LEFT, 100);
  permissions_list_->InsertColumn(2, _("Grant Option"), wxLIST_FORMAT_CENTER, 100);
  permissions_list_->InsertColumn(3, _("Granted By"), wxLIST_FORMAT_LEFT, 120);
  
  wxBoxSizer* perms_sizer = new wxBoxSizer(wxVERTICAL);
  perms_sizer->Add(permissions_list_, 1, wxEXPAND);
  permissions_panel->SetSizer(perms_sizer);
  notebook_->AddPage(permissions_panel, _("Permissions"));
}

void ObjectMetadataDialogNew::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Add notebook
  main_sizer->Add(notebook_, 1, wxEXPAND | wxALL, 5);
  
  // Button sizer
  wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  btn_sizer->Add(new wxButton(this, ID_REFRESH, _("&Refresh")), 0, wxRIGHT, 5);
  btn_sizer->Add(new wxButton(this, ID_EXPORT_DDL, _("&Export DDL...")), 0, wxRIGHT, 5);
  btn_sizer->Add(new wxButton(this, ID_COPY_CLIPBOARD, _("&Copy DDL")), 0, wxRIGHT, 5);
  btn_sizer->AddStretchSpacer(1);
  btn_sizer->Add(new wxButton(this, wxID_CLOSE, _("&Close")), 0);
  
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void ObjectMetadataDialogNew::BindEvents() {
  // Additional event bindings if needed
}

void ObjectMetadataDialogNew::LoadMetadata() {
  if (is_loading_) return;
  is_loading_ = true;
  
  // Load metadata based on object type
  if (object_type_ == "TABLE") {
    LoadTableMetadata();
  } else if (object_type_ == "VIEW") {
    LoadViewMetadata();
  } else if (object_type_ == "PROCEDURE") {
    LoadProcedureMetadata();
  } else if (object_type_ == "FUNCTION") {
    LoadFunctionMetadata();
  } else if (object_type_ == "TRIGGER") {
    LoadTriggerMetadata();
  } else if (object_type_ == "INDEX") {
    LoadIndexMetadata();
  } else if (object_type_ == "DOMAIN") {
    LoadDomainMetadata();
  }
  
  // Update UI
  UpdateGeneralTab();
  UpdateColumnsTab();
  UpdateConstraintsTab();
  UpdateIndexesTab();
  UpdateDDLTtab();
  UpdatePermissionsTab();
  
  is_loading_ = false;
}

void ObjectMetadataDialogNew::LoadTableMetadata() {
  // Populate with sample data (in real implementation, query database)
  cache_->name = object_name_;
  cache_->type = "TABLE";
  cache_->owner = "SYSDBA";
  cache_->created = "2024-01-15 10:30:00";
  cache_->modified = "2024-03-20 14:45:00";
  cache_->description = "Sample table for demonstration";
  
  // Sample columns
  cache_->columns.push_back({
      {"name", "ID"},
      {"type", "BIGINT"},
      {"nullable", "NO"},
      {"default", "AUTO_INCREMENT"},
      {"description", "Primary key"}
  });
  cache_->columns.push_back({
      {"name", "NAME"},
      {"type", "VARCHAR(100)"},
      {"nullable", "NO"},
      {"default", ""},
      {"description", "Object name"}
  });
  cache_->columns.push_back({
      {"name", "CREATED_AT"},
      {"type", "TIMESTAMP"},
      {"nullable", "YES"},
      {"default", "CURRENT_TIMESTAMP"},
      {"description", "Creation timestamp"}
  });
  
  // Sample DDL
  cache_->ddl = wxString::Format(
      "CREATE TABLE %s (\n"
      "  ID BIGINT NOT NULL AUTO_INCREMENT,\n"
      "  NAME VARCHAR(100) NOT NULL,\n"
      "  CREATED_AT TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
      "  CONSTRAINT PK_%s PRIMARY KEY (ID)\n"
      ");",
      object_name_, object_name_).ToStdString();
}

void ObjectMetadataDialogNew::LoadViewMetadata() {
  cache_->name = object_name_;
  cache_->type = "VIEW";
  cache_->owner = "SYSDBA";
  cache_->created = "2024-02-01 09:00:00";
  cache_->modified = "2024-02-01 09:00:00";
  cache_->ddl = "CREATE VIEW " + object_name_ + " AS\nSELECT * FROM some_table;";
}

void ObjectMetadataDialogNew::LoadProcedureMetadata() {
  cache_->name = object_name_;
  cache_->type = "PROCEDURE";
  cache_->owner = "SYSDBA";
  cache_->created = "2024-01-20 11:00:00";
  cache_->ddl = "CREATE PROCEDURE " + object_name_ + "\nAS\nBEGIN\n  -- Procedure body\nEND;";
}

void ObjectMetadataDialogNew::LoadFunctionMetadata() {
  cache_->name = object_name_;
  cache_->type = "FUNCTION";
  cache_->owner = "SYSDBA";
  cache_->ddl = "CREATE FUNCTION " + object_name_ + "()\nRETURNS INTEGER\nAS\nBEGIN\n  RETURN 1;\nEND;";
}

void ObjectMetadataDialogNew::LoadTriggerMetadata() {
  cache_->name = object_name_;
  cache_->type = "TRIGGER";
  cache_->owner = "SYSDBA";
  cache_->ddl = "CREATE TRIGGER " + object_name_ + "\nBEFORE INSERT ON some_table\nAS\nBEGIN\n  -- Trigger body\nEND;";
}

void ObjectMetadataDialogNew::LoadIndexMetadata() {
  cache_->name = object_name_;
  cache_->type = "INDEX";
  cache_->owner = "SYSDBA";
  cache_->ddl = "CREATE INDEX " + object_name_ + " ON some_table (column1, column2);";
}

void ObjectMetadataDialogNew::LoadDomainMetadata() {
  cache_->name = object_name_;
  cache_->type = "DOMAIN";
  cache_->owner = "SYSDBA";
  cache_->ddl = "CREATE DOMAIN " + object_name_ + " AS VARCHAR(50);";
}

void ObjectMetadataDialogNew::UpdateGeneralTab() {
  name_label_->SetLabel(cache_->name);
  type_label_->SetLabel(cache_->type);
  owner_label_->SetLabel(cache_->owner);
  created_label_->SetLabel(cache_->created);
  modified_label_->SetLabel(cache_->modified);
  description_label_->SetLabel(cache_->description);
}

void ObjectMetadataDialogNew::UpdateColumnsTab() {
  columns_list_->DeleteAllItems();
  
  for (size_t i = 0; i < cache_->columns.size(); ++i) {
    const auto& col = cache_->columns[i];
    long idx = columns_list_->InsertItem(i, col.at("name"));
    columns_list_->SetItem(idx, 1, col.at("type"));
    columns_list_->SetItem(idx, 2, col.at("nullable"));
    columns_list_->SetItem(idx, 3, col.at("default"));
    columns_list_->SetItem(idx, 4, col.at("description"));
  }
}

void ObjectMetadataDialogNew::UpdateConstraintsTab() {
  constraints_list_->DeleteAllItems();
  // TODO: Load constraints from database
}

void ObjectMetadataDialogNew::UpdateIndexesTab() {
  indexes_list_->DeleteAllItems();
  // TODO: Load indexes from database
}

void ObjectMetadataDialogNew::UpdateDependenciesTab() {
  dependencies_tree_->DeleteAllItems();
  wxTreeItemId root = dependencies_tree_->AddRoot(_("Dependencies"));
  
  wxTreeItemId uses = dependencies_tree_->AppendItem(root, _("Uses"));
  wxTreeItemId used_by = dependencies_tree_->AppendItem(root, _("Used By"));
  
  // TODO: Load actual dependencies
  dependencies_tree_->AppendItem(uses, _("Sample table"));
  dependencies_tree_->AppendItem(used_by, _("Sample view"));
}

void ObjectMetadataDialogNew::UpdateDDLTtab() {
  ddl_text_->SetValue(cache_->ddl);
}

void ObjectMetadataDialogNew::UpdatePermissionsTab() {
  permissions_list_->DeleteAllItems();
  // TODO: Load permissions from database
}

void ObjectMetadataDialogNew::OnRefresh(wxCommandEvent& event) {
  LoadMetadata();
}

void ObjectMetadataDialogNew::OnExportDDL(wxCommandEvent& event) {
  // TODO: Implement DDL export to file
  wxMessageBox(_("DDL export not yet implemented"), _("Export DDL"),
               wxOK | wxICON_INFORMATION);
}

void ObjectMetadataDialogNew::OnCopyToClipboard(wxCommandEvent& event) {
  if (wxTheClipboard->Open()) {
    wxTheClipboard->SetData(new wxTextDataObject(ddl_text_->GetValue()));
    wxTheClipboard->Close();
    wxMessageBox(_("DDL copied to clipboard"), _("Copy DDL"),
                 wxOK | wxICON_INFORMATION);
  }
}

void ObjectMetadataDialogNew::OnClose(wxCommandEvent& event) {
  EndModal(wxID_CLOSE);
}

void ObjectMetadataDialogNew::OnTabChanged(wxBookCtrlEvent& event) {
  // Handle tab change if needed
}

}  // namespace scratchrobin::ui
