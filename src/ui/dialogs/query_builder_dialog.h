/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/checklst.h>
#include <wx/listctrl.h>
#include <wx/choice.h>
#include <memory>
#include <string>

namespace scratchrobin::ui {

/**
 * Query Builder Dialog
 * Visual query builder for constructing SQL queries
 */
class QueryBuilderDialog : public wxDialog {
 public:
  QueryBuilderDialog(wxWindow* parent);
  ~QueryBuilderDialog() override;

  // Get the generated SQL query
  std::string GetGeneratedSQL() const;

  // Set initial SQL (for editing existing query)
  void SetInitialSQL(const std::string& sql);

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnGenerateSQL(wxCommandEvent& event);
  void OnPreview(wxCommandEvent& event);
  void OnOK(wxCommandEvent& event);
  void OnCancel(wxCommandEvent& event);
  void OnAddTable(wxCommandEvent& event);
  void OnRemoveTable(wxCommandEvent& event);
  void OnAddJoin(wxCommandEvent& event);
  void OnAddCondition(wxCommandEvent& event);
  void OnTableSelected(wxCommandEvent& event);

  // Internal methods
  void PopulateTableList();
  void UpdatePreview();
  void BuildQuery();

  // UI Components - Tables panel
  wxListBox* available_tables_{nullptr};
  wxListBox* selected_tables_{nullptr};
  wxButton* add_table_btn_{nullptr};
  wxButton* remove_table_btn_{nullptr};

  // UI Components - Columns panel
  wxCheckListBox* columns_list_{nullptr};

  // UI Components - Joins panel
  wxListCtrl* joins_list_{nullptr};
  wxButton* add_join_btn_{nullptr};
  wxButton* remove_join_btn_{nullptr};

  // UI Components - Conditions panel
  wxListCtrl* conditions_list_{nullptr};
  wxButton* add_condition_btn_{nullptr};
  wxButton* remove_condition_btn_{nullptr};
  wxChoice* condition_op_choice_{nullptr};

  // UI Components - Preview panel
  wxTextCtrl* sql_preview_{nullptr};
  wxTextCtrl* query_name_ctrl_{nullptr};

  // State
  std::string generated_sql_;
  std::string initial_sql_;
  struct QueryModel;
  std::unique_ptr<QueryModel> model_;

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
