/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "query_builder_dialog.h"

#include <memory>
#include <vector>
#include <string>

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/checklst.h>
#include <wx/listctrl.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <wx/msgdlg.h>

namespace scratchrobin::ui {

// Forward declaration of QueryModel
struct QueryBuilderDialog::QueryModel {
  std::vector<std::string> tables;
  std::vector<std::string> columns;
  std::vector<std::string> joins;
  std::vector<std::string> conditions;
  std::vector<std::string> order_by;
  std::vector<std::string> group_by;
  int limit = 0;
  bool distinct = false;
};

// Control IDs
enum {
  ID_ADD_TABLE = wxID_HIGHEST + 1,
  ID_REMOVE_TABLE,
  ID_ADD_JOIN,
  ID_REMOVE_JOIN,
  ID_ADD_CONDITION,
  ID_REMOVE_CONDITION,
  ID_GENERATE_SQL,
  ID_PREVIEW,
  ID_TABLE_SELECTED,
  ID_AVAILABLE_TABLES,
  ID_SELECTED_TABLES,
};

wxBEGIN_EVENT_TABLE(QueryBuilderDialog, wxDialog)
  EVT_BUTTON(ID_ADD_TABLE, QueryBuilderDialog::OnAddTable)
  EVT_BUTTON(ID_REMOVE_TABLE, QueryBuilderDialog::OnRemoveTable)
  EVT_BUTTON(ID_ADD_JOIN, QueryBuilderDialog::OnAddJoin)
  EVT_BUTTON(ID_ADD_CONDITION, QueryBuilderDialog::OnAddCondition)
  EVT_BUTTON(ID_GENERATE_SQL, QueryBuilderDialog::OnGenerateSQL)
  EVT_BUTTON(ID_PREVIEW, QueryBuilderDialog::OnPreview)
  EVT_BUTTON(wxID_OK, QueryBuilderDialog::OnOK)
  EVT_BUTTON(wxID_CANCEL, QueryBuilderDialog::OnCancel)
  EVT_LISTBOX_DCLICK(ID_AVAILABLE_TABLES, QueryBuilderDialog::OnAddTable)
  EVT_LISTBOX(ID_SELECTED_TABLES, QueryBuilderDialog::OnTableSelected)
wxEND_EVENT_TABLE()

QueryBuilderDialog::QueryBuilderDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Query Builder"), wxDefaultPosition,
               wxSize(800, 600)),
      model_(std::make_unique<QueryModel>()) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

QueryBuilderDialog::~QueryBuilderDialog() = default;

void QueryBuilderDialog::CreateControls() {
  // Tables panel controls
  available_tables_ = new wxListBox(this, ID_AVAILABLE_TABLES);
  selected_tables_ = new wxListBox(this, ID_SELECTED_TABLES);
  add_table_btn_ = new wxButton(this, ID_ADD_TABLE, _(">"));
  remove_table_btn_ = new wxButton(this, ID_REMOVE_TABLE, _("<"));

  // Columns panel
  columns_list_ = new wxCheckListBox(this, wxID_ANY);

  // Joins panel
  joins_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                               wxLC_REPORT | wxLC_SINGLE_SEL);
  joins_list_->InsertColumn(0, _("Join Type"));
  joins_list_->InsertColumn(1, _("Left Table"));
  joins_list_->InsertColumn(2, _("Left Column"));
  joins_list_->InsertColumn(3, _("Right Table"));
  joins_list_->InsertColumn(4, _("Right Column"));
  add_join_btn_ = new wxButton(this, ID_ADD_JOIN, _("Add Join..."));
  remove_join_btn_ = new wxButton(this, ID_REMOVE_JOIN, _("Remove"));

  // Conditions panel
  conditions_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxLC_REPORT | wxLC_SINGLE_SEL);
  conditions_list_->InsertColumn(0, _("Column"));
  conditions_list_->InsertColumn(1, _("Operator"));
  conditions_list_->InsertColumn(2, _("Value"));
  conditions_list_->InsertColumn(3, _("Connector"));
  add_condition_btn_ = new wxButton(this, ID_ADD_CONDITION, _("Add Condition..."));
  remove_condition_btn_ = new wxButton(this, ID_REMOVE_CONDITION, _("Remove"));
  condition_op_choice_ = new wxChoice(this, wxID_ANY);
  condition_op_choice_->Append("=");
  condition_op_choice_->Append("<>");
  condition_op_choice_->Append("<");
  condition_op_choice_->Append(">");
  condition_op_choice_->Append("<=");
  condition_op_choice_->Append(">=");
  condition_op_choice_->Append("LIKE");
  condition_op_choice_->Append("IN");
  condition_op_choice_->Append("IS NULL");
  condition_op_choice_->Append("IS NOT NULL");
  condition_op_choice_->SetSelection(0);

  // Preview panel
  sql_preview_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY);
  query_name_ctrl_ = new wxTextCtrl(this, wxID_ANY, _("New Query"));

  // Populate with sample tables (in real implementation, load from database)
  available_tables_->Append(_("customers"));
  available_tables_->Append(_("orders"));
  available_tables_->Append(_("products"));
  available_tables_->Append(_("categories"));
  available_tables_->Append(_("employees"));
}

void QueryBuilderDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

  // Notebook for query building tabs
  wxNotebook* notebook = new wxNotebook(this, wxID_ANY);

  // Tables tab
  wxPanel* tables_panel = new wxPanel(notebook);
  wxBoxSizer* tables_sizer = new wxBoxSizer(wxHORIZONTAL);

  // Available tables
  wxBoxSizer* available_sizer = new wxBoxSizer(wxVERTICAL);
  available_sizer->Add(new wxStaticText(tables_panel, wxID_ANY, _("Available Tables:")), 0, wxBOTTOM, 5);
  available_sizer->Add(available_tables_, 1, wxEXPAND);
  tables_sizer->Add(available_sizer, 1, wxEXPAND | wxALL, 5);

  // Buttons
  wxBoxSizer* btn_sizer = new wxBoxSizer(wxVERTICAL);
  btn_sizer->AddStretchSpacer(1);
  btn_sizer->Add(add_table_btn_, 0, wxALL, 2);
  btn_sizer->Add(remove_table_btn_, 0, wxALL, 2);
  btn_sizer->AddStretchSpacer(1);
  tables_sizer->Add(btn_sizer, 0, wxALIGN_CENTER_VERTICAL);

  // Selected tables
  wxBoxSizer* selected_sizer = new wxBoxSizer(wxVERTICAL);
  selected_sizer->Add(new wxStaticText(tables_panel, wxID_ANY, _("Selected Tables:")), 0, wxBOTTOM, 5);
  selected_sizer->Add(selected_tables_, 1, wxEXPAND);
  tables_sizer->Add(selected_sizer, 1, wxEXPAND | wxALL, 5);

  tables_panel->SetSizer(tables_sizer);
  notebook->AddPage(tables_panel, _("Tables"));

  // Columns tab
  wxPanel* columns_panel = new wxPanel(notebook);
  wxBoxSizer* columns_sizer = new wxBoxSizer(wxVERTICAL);
  columns_sizer->Add(new wxStaticText(columns_panel, wxID_ANY, _("Select Columns:")), 0, wxBOTTOM, 5);
  columns_sizer->Add(columns_list_, 1, wxEXPAND);
  columns_panel->SetSizer(columns_sizer);
  notebook->AddPage(columns_panel, _("Columns"));

  // Joins tab
  wxPanel* joins_panel = new wxPanel(notebook);
  wxBoxSizer* joins_sizer = new wxBoxSizer(wxVERTICAL);
  joins_sizer->Add(new wxStaticText(joins_panel, wxID_ANY, _("Table Joins:")), 0, wxBOTTOM, 5);
  joins_sizer->Add(joins_list_, 1, wxEXPAND);
  wxBoxSizer* joins_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  joins_btn_sizer->Add(add_join_btn_, 0, wxRIGHT, 5);
  joins_btn_sizer->Add(remove_join_btn_, 0);
  joins_sizer->Add(joins_btn_sizer, 0, wxTOP, 5);
  joins_panel->SetSizer(joins_sizer);
  notebook->AddPage(joins_panel, _("Joins"));

  // Conditions tab
  wxPanel* conditions_panel = new wxPanel(notebook);
  wxBoxSizer* conditions_sizer = new wxBoxSizer(wxVERTICAL);
  conditions_sizer->Add(new wxStaticText(conditions_panel, wxID_ANY, _("WHERE Conditions:")), 0, wxBOTTOM, 5);
  conditions_sizer->Add(conditions_list_, 1, wxEXPAND);
  wxBoxSizer* conditions_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  conditions_btn_sizer->Add(add_condition_btn_, 0, wxRIGHT, 5);
  conditions_btn_sizer->Add(remove_condition_btn_, 0);
  conditions_sizer->Add(conditions_btn_sizer, 0, wxTOP, 5);
  conditions_panel->SetSizer(conditions_sizer);
  notebook->AddPage(conditions_panel, _("Conditions"));

  main_sizer->Add(notebook, 1, wxEXPAND | wxALL, 5);

  // Preview section
  wxStaticBoxSizer* preview_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("SQL Preview"));
  preview_sizer->Add(sql_preview_, 1, wxEXPAND | wxALL, 5);
  main_sizer->Add(preview_sizer, 0, wxEXPAND | wxALL, 5);

  // Query name
  wxBoxSizer* name_sizer = new wxBoxSizer(wxHORIZONTAL);
  name_sizer->Add(new wxStaticText(this, wxID_ANY, _("Query Name:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  name_sizer->Add(query_name_ctrl_, 1, wxEXPAND);
  main_sizer->Add(name_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

  // Button sizer
  wxStdDialogButtonSizer* dialog_btn_sizer = new wxStdDialogButtonSizer();
  dialog_btn_sizer->AddButton(new wxButton(this, ID_PREVIEW, _("&Preview")));
  dialog_btn_sizer->AddButton(new wxButton(this, ID_GENERATE_SQL, _("&Generate SQL")));
  dialog_btn_sizer->AddButton(new wxButton(this, wxID_OK, _("&OK")));
  dialog_btn_sizer->AddButton(new wxButton(this, wxID_CANCEL, _("&Cancel")));
  dialog_btn_sizer->Realize();
  main_sizer->Add(dialog_btn_sizer, 0, wxEXPAND | wxALL, 5);

  SetSizer(main_sizer);
}

void QueryBuilderDialog::BindEvents() {
  // Additional event bindings if needed
}

std::string QueryBuilderDialog::GetGeneratedSQL() const {
  return generated_sql_;
}

void QueryBuilderDialog::SetInitialSQL(const std::string& sql) {
  initial_sql_ = sql;
  sql_preview_->SetValue(sql);
}

void QueryBuilderDialog::PopulateTableList() {
  // Already populated in CreateControls with sample data
}

void QueryBuilderDialog::UpdatePreview() {
  BuildQuery();
  sql_preview_->SetValue(generated_sql_);
}

void QueryBuilderDialog::BuildQuery() {
  std::string sql = "SELECT ";

  // Columns
  if (model_->columns.empty()) {
    sql += "*";
  } else {
    for (size_t i = 0; i < model_->columns.size(); ++i) {
      if (i > 0) sql += ", ";
      sql += model_->columns[i];
    }
  }

  // Tables
  if (!model_->tables.empty()) {
    sql += " FROM ";
    for (size_t i = 0; i < model_->tables.size(); ++i) {
      if (i > 0) sql += ", ";
      sql += model_->tables[i];
    }
  }

  // Joins
  for (const auto& join : model_->joins) {
    sql += " " + join;
  }

  // Conditions
  if (!model_->conditions.empty()) {
    sql += " WHERE ";
    for (size_t i = 0; i < model_->conditions.size(); ++i) {
      if (i > 0) sql += " AND ";
      sql += model_->conditions[i];
    }
  }

  generated_sql_ = sql;
}

void QueryBuilderDialog::OnAddTable(wxCommandEvent& event) {
  int selection = available_tables_->GetSelection();
  if (selection != wxNOT_FOUND) {
    wxString table = available_tables_->GetString(selection);
    selected_tables_->Append(table);
    model_->tables.push_back(table.ToStdString());
    
    // Update columns list with sample columns
    columns_list_->Clear();
    columns_list_->Append(table + ".*");
    columns_list_->Check(0);
    
    UpdatePreview();
  }
}

void QueryBuilderDialog::OnRemoveTable(wxCommandEvent& event) {
  int selection = selected_tables_->GetSelection();
  if (selection != wxNOT_FOUND) {
    selected_tables_->Delete(selection);
    if (selection < static_cast<int>(model_->tables.size())) {
      model_->tables.erase(model_->tables.begin() + selection);
    }
    UpdatePreview();
  }
}

void QueryBuilderDialog::OnAddJoin(wxCommandEvent& event) {
  // TODO: Implement join dialog
  wxMessageBox(_("Join dialog not yet implemented"), _("Add Join"),
               wxOK | wxICON_INFORMATION);
}

void QueryBuilderDialog::OnAddCondition(wxCommandEvent& event) {
  // TODO: Implement condition dialog
  wxMessageBox(_("Condition dialog not yet implemented"), _("Add Condition"),
               wxOK | wxICON_INFORMATION);
}

void QueryBuilderDialog::OnGenerateSQL(wxCommandEvent& event) {
  UpdatePreview();
}

void QueryBuilderDialog::OnPreview(wxCommandEvent& event) {
  UpdatePreview();
  wxMessageBox(wxString::FromUTF8(generated_sql_), _("SQL Preview"),
               wxOK | wxICON_INFORMATION);
}

void QueryBuilderDialog::OnOK(wxCommandEvent& event) {
  UpdatePreview();
  if (generated_sql_.empty()) {
    wxMessageBox(_("Please build a query first"), _("Validation Error"),
                 wxOK | wxICON_WARNING);
    return;
  }
  EndModal(wxID_OK);
}

void QueryBuilderDialog::OnCancel(wxCommandEvent& event) {
  EndModal(wxID_CANCEL);
}

void QueryBuilderDialog::OnTableSelected(wxCommandEvent& event) {
  // Update available columns based on selected table
  columns_list_->Clear();
  int selection = selected_tables_->GetSelection();
  if (selection != wxNOT_FOUND) {
    wxString table = selected_tables_->GetString(selection);
    // Add sample columns (in real implementation, load from schema)
    columns_list_->Append(table + ".id");
    columns_list_->Append(table + ".name");
    columns_list_->Append(table + ".created_at");
  }
}

}  // namespace scratchrobin::ui
