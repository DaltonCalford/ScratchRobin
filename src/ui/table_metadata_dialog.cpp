/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/table_metadata_dialog.h"

#include <algorithm>
#include <string>

#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace scratchrobin::ui {

namespace {

constexpr int kColumnName = 0;
constexpr int kColumnDomain = 1;
constexpr int kColumnType = 2;
constexpr int kColumnNullable = 3;
constexpr int kColumnPk = 4;
constexpr int kColumnFk = 5;
constexpr int kColumnRefSchema = 6;
constexpr int kColumnRefTable = 7;
constexpr int kColumnRefColumn = 8;
constexpr int kColumnDefault = 9;
constexpr int kColumnNotes = 10;

constexpr int kIndexName = 0;
constexpr int kIndexUnique = 1;
constexpr int kIndexMethod = 2;
constexpr int kIndexTarget = 3;
constexpr int kIndexNotes = 4;

constexpr int kTriggerName = 0;
constexpr int kTriggerTiming = 1;
constexpr int kTriggerEvent = 2;
constexpr int kTriggerNotes = 3;

wxString BoolToCell(const bool value) {
  return value ? "1" : "0";
}

bool CellToBool(const wxString& value) {
  const wxString normalized = value.Lower().Trim().Trim(false);
  return normalized == "1" || normalized == "true" || normalized == "yes" ||
         normalized == "y";
}

wxArrayString BuildTypeChoices() {
  wxArrayString choices;
  choices.Add("INT64");
  choices.Add("INT32");
  choices.Add("INT16");
  choices.Add("UUID");
  choices.Add("BOOLEAN");
  choices.Add("VARCHAR(255)");
  choices.Add("TEXT");
  choices.Add("TIMESTAMP");
  choices.Add("DATE");
  choices.Add("DECIMAL(18,2)");
  choices.Add("FLOAT64");
  choices.Add("BLOB");
  return choices;
}

bool RowHasAnyValue(const wxGrid* grid, const int row, const int columns) {
  for (int col = 0; col < columns; ++col) {
    if (!grid->GetCellValue(row, col).Trim().Trim(false).IsEmpty()) {
      return true;
    }
  }
  return false;
}

void DeleteFocusedOrSelectedRows(wxGrid* grid) {
  if (!grid || grid->GetNumberRows() <= 0) {
    return;
  }

  wxArrayInt rows = grid->GetSelectedRows();
  if (rows.IsEmpty()) {
    int row = grid->GetGridCursorRow();
    if (row < 0 || row >= grid->GetNumberRows()) {
      row = grid->GetNumberRows() - 1;
    }
    grid->DeleteRows(row, 1);
    return;
  }

  std::vector<int> sorted_rows;
  sorted_rows.reserve(rows.size());
  for (const int row : rows) {
    if (row >= 0 && row < grid->GetNumberRows()) {
      sorted_rows.push_back(row);
    }
  }
  std::sort(sorted_rows.begin(), sorted_rows.end(), std::greater<>());

  for (const int row : sorted_rows) {
    if (row >= 0 && row < grid->GetNumberRows()) {
      grid->DeleteRows(row, 1);
    }
  }
}

}  // namespace

TableMetadataDialog::TableMetadataDialog(wxWindow* parent, const Mode mode,
                                         const backend::PreviewTableMetadata& seed_metadata)
    : wxDialog(parent, wxID_ANY, "Table Metadata", wxDefaultPosition, wxSize(920, 680),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode),
      seed_(seed_metadata) {
  BuildUi();
  BindEvents();
  PopulateFromSeed();
  ApplyMode();
}

backend::PreviewTableMetadata TableMetadataDialog::GetMetadata() const {
  backend::PreviewTableMetadata metadata;
  metadata.schema_path = schema_path_ctrl_->GetValue().ToStdString();
  metadata.table_name = table_name_ctrl_->GetValue().ToStdString();
  metadata.description = description_ctrl_->GetValue().ToStdString();

  const int rows = columns_grid_->GetNumberRows();
  for (int row = 0; row < rows; ++row) {
    backend::PreviewColumnMetadata column;
    column.column_name = columns_grid_->GetCellValue(row, kColumnName).ToStdString();
    column.data_type = columns_grid_->GetCellValue(row, kColumnType).ToStdString();
    column.nullable = CellToBool(columns_grid_->GetCellValue(row, kColumnNullable));
    column.default_value = columns_grid_->GetCellValue(row, kColumnDefault).ToStdString();
    column.notes = columns_grid_->GetCellValue(row, kColumnNotes).ToStdString();
    column.domain_name = columns_grid_->GetCellValue(row, kColumnDomain).ToStdString();
    column.is_primary_key = CellToBool(columns_grid_->GetCellValue(row, kColumnPk));
    column.is_foreign_key = CellToBool(columns_grid_->GetCellValue(row, kColumnFk));
    column.references_schema_path =
        columns_grid_->GetCellValue(row, kColumnRefSchema).ToStdString();
    column.references_table_name =
        columns_grid_->GetCellValue(row, kColumnRefTable).ToStdString();
    column.references_column_name =
        columns_grid_->GetCellValue(row, kColumnRefColumn).ToStdString();
    if (!column.column_name.empty()) {
      metadata.columns.push_back(std::move(column));
    }
  }

  const int index_rows = indexes_grid_->GetNumberRows();
  for (int row = 0; row < index_rows; ++row) {
    backend::PreviewIndexMetadata idx;
    idx.index_name = indexes_grid_->GetCellValue(row, kIndexName).ToStdString();
    idx.unique = CellToBool(indexes_grid_->GetCellValue(row, kIndexUnique));
    idx.method = indexes_grid_->GetCellValue(row, kIndexMethod).ToStdString();
    idx.target = indexes_grid_->GetCellValue(row, kIndexTarget).ToStdString();
    idx.notes = indexes_grid_->GetCellValue(row, kIndexNotes).ToStdString();
    if (!idx.index_name.empty()) {
      metadata.indexes.push_back(std::move(idx));
    }
  }

  const int trigger_rows = triggers_grid_->GetNumberRows();
  for (int row = 0; row < trigger_rows; ++row) {
    backend::PreviewTriggerMetadata trigger;
    trigger.trigger_name = triggers_grid_->GetCellValue(row, kTriggerName).ToStdString();
    trigger.timing = triggers_grid_->GetCellValue(row, kTriggerTiming).ToStdString();
    trigger.event = triggers_grid_->GetCellValue(row, kTriggerEvent).ToStdString();
    trigger.notes = triggers_grid_->GetCellValue(row, kTriggerNotes).ToStdString();
    if (!trigger.trigger_name.empty()) {
      metadata.triggers.push_back(std::move(trigger));
    }
  }

  return metadata;
}

void TableMetadataDialog::BuildUi() {
  auto* root = new wxPanel(this, wxID_ANY);
  auto* root_sizer = new wxBoxSizer(wxVERTICAL);

  auto* form_grid = new wxFlexGridSizer(2, 6, 8);
  form_grid->AddGrowableCol(1, 1);

  form_grid->Add(new wxStaticText(root, wxID_ANY, "Schema Path"), 0,
                 wxALIGN_CENTER_VERTICAL);
  schema_path_ctrl_ = new wxTextCtrl(root, wxID_ANY);
  form_grid->Add(schema_path_ctrl_, 1, wxEXPAND);

  form_grid->Add(new wxStaticText(root, wxID_ANY, "Table Name"), 0,
                 wxALIGN_CENTER_VERTICAL);
  table_name_ctrl_ = new wxTextCtrl(root, wxID_ANY);
  form_grid->Add(table_name_ctrl_, 1, wxEXPAND);

  form_grid->Add(new wxStaticText(root, wxID_ANY, "Description"), 0,
                 wxALIGN_CENTER_VERTICAL);
  description_ctrl_ = new wxTextCtrl(root, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxTE_MULTILINE);
  form_grid->Add(description_ctrl_, 1, wxEXPAND);

  notebook_ = new wxNotebook(root, wxID_ANY);

  auto* columns_page = new wxPanel(notebook_, wxID_ANY);
  auto* columns_sizer = new wxBoxSizer(wxVERTICAL);

  auto* columns_label = new wxStaticText(
      columns_page, wxID_ANY,
      "Columns (domain preferred; type required when domain is empty)");

  auto* type_helper_row = new wxBoxSizer(wxHORIZONTAL);
  type_helper_row->Add(new wxStaticText(columns_page, wxID_ANY, "Type helper"), 0,
                       wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  type_helper_choice_ =
      new wxChoice(columns_page, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                   BuildTypeChoices());
  if (type_helper_choice_->GetCount() > 0) {
    type_helper_choice_->SetSelection(0);
  }
  apply_type_btn_ =
      new wxButton(columns_page, static_cast<int>(ControlId::kApplyTypeHelper),
                   "Apply to Selected");
  type_helper_row->Add(type_helper_choice_, 0, wxRIGHT, 6);
  type_helper_row->Add(apply_type_btn_, 0, wxRIGHT, 6);
  type_helper_row->AddStretchSpacer(1);

  columns_grid_ = new wxGrid(columns_page, wxID_ANY);
  columns_grid_->CreateGrid(0, 11);
  columns_grid_->SetColLabelValue(kColumnName, "Column");
  columns_grid_->SetColLabelValue(kColumnDomain, "Domain");
  columns_grid_->SetColLabelValue(kColumnType, "Type");
  columns_grid_->SetColLabelValue(kColumnNullable, "Nullable");
  columns_grid_->SetColLabelValue(kColumnPk, "PK");
  columns_grid_->SetColLabelValue(kColumnFk, "FK");
  columns_grid_->SetColLabelValue(kColumnRefSchema, "Ref Schema");
  columns_grid_->SetColLabelValue(kColumnRefTable, "Ref Table");
  columns_grid_->SetColLabelValue(kColumnRefColumn, "Ref Column");
  columns_grid_->SetColLabelValue(kColumnDefault, "Default");
  columns_grid_->SetColLabelValue(kColumnNotes, "Notes");
  columns_grid_->SetColSize(kColumnName, 170);
  columns_grid_->SetColSize(kColumnDomain, 150);
  columns_grid_->SetColSize(kColumnType, 140);
  columns_grid_->SetColSize(kColumnNullable, 80);
  columns_grid_->SetColSize(kColumnPk, 50);
  columns_grid_->SetColSize(kColumnFk, 50);
  columns_grid_->SetColSize(kColumnRefSchema, 130);
  columns_grid_->SetColSize(kColumnRefTable, 130);
  columns_grid_->SetColSize(kColumnRefColumn, 130);
  columns_grid_->SetColSize(kColumnDefault, 160);
  columns_grid_->SetColSize(kColumnNotes, 220);

  auto* type_attr = new wxGridCellAttr();
  type_attr->SetEditor(new wxGridCellChoiceEditor(BuildTypeChoices()));
  columns_grid_->SetColAttr(kColumnType, type_attr);
  columns_grid_->SetColFormatBool(kColumnNullable);
  columns_grid_->SetColFormatBool(kColumnPk);
  columns_grid_->SetColFormatBool(kColumnFk);

  auto* columns_button_row = new wxBoxSizer(wxHORIZONTAL);
  add_column_btn_ = new wxButton(columns_page, static_cast<int>(ControlId::kAddColumn),
                                 "Add Column");
  remove_column_btn_ = new wxButton(columns_page, static_cast<int>(ControlId::kRemoveColumn),
                                    "Remove Selected");
  columns_button_row->Add(add_column_btn_, 0, wxRIGHT, 6);
  columns_button_row->Add(remove_column_btn_, 0);
  columns_button_row->AddStretchSpacer(1);

  columns_sizer->Add(columns_label, 0, wxBOTTOM, 6);
  columns_sizer->Add(type_helper_row, 0, wxEXPAND | wxBOTTOM, 6);
  columns_sizer->Add(columns_grid_, 1, wxEXPAND | wxBOTTOM, 6);
  columns_sizer->Add(columns_button_row, 0, wxEXPAND);
  columns_page->SetSizer(columns_sizer);

  auto* indexes_page = new wxPanel(notebook_, wxID_ANY);
  auto* indexes_sizer = new wxBoxSizer(wxVERTICAL);
  indexes_sizer->Add(
      new wxStaticText(indexes_page, wxID_ANY, "Indexes (target = column list or expression)"),
      0, wxBOTTOM, 6);
  indexes_grid_ = new wxGrid(indexes_page, wxID_ANY);
  indexes_grid_->CreateGrid(0, 5);
  indexes_grid_->SetColLabelValue(kIndexName, "Index");
  indexes_grid_->SetColLabelValue(kIndexUnique, "Unique");
  indexes_grid_->SetColLabelValue(kIndexMethod, "Method");
  indexes_grid_->SetColLabelValue(kIndexTarget, "Target");
  indexes_grid_->SetColLabelValue(kIndexNotes, "Notes");
  indexes_grid_->SetColSize(kIndexName, 180);
  indexes_grid_->SetColSize(kIndexUnique, 70);
  indexes_grid_->SetColSize(kIndexMethod, 120);
  indexes_grid_->SetColSize(kIndexTarget, 220);
  indexes_grid_->SetColSize(kIndexNotes, 240);
  indexes_grid_->SetColFormatBool(kIndexUnique);

  auto* indexes_button_row = new wxBoxSizer(wxHORIZONTAL);
  add_index_btn_ = new wxButton(indexes_page, static_cast<int>(ControlId::kAddIndex),
                                "Add Index");
  remove_index_btn_ = new wxButton(indexes_page,
                                   static_cast<int>(ControlId::kRemoveIndex),
                                   "Remove Selected");
  indexes_button_row->Add(add_index_btn_, 0, wxRIGHT, 6);
  indexes_button_row->Add(remove_index_btn_, 0);
  indexes_button_row->AddStretchSpacer(1);

  indexes_sizer->Add(indexes_grid_, 1, wxEXPAND | wxBOTTOM, 6);
  indexes_sizer->Add(indexes_button_row, 0, wxEXPAND);
  indexes_page->SetSizer(indexes_sizer);

  auto* triggers_page = new wxPanel(notebook_, wxID_ANY);
  auto* triggers_sizer = new wxBoxSizer(wxVERTICAL);
  triggers_sizer->Add(
      new wxStaticText(triggers_page, wxID_ANY, "Triggers (timing/event metadata)"), 0,
      wxBOTTOM, 6);
  triggers_grid_ = new wxGrid(triggers_page, wxID_ANY);
  triggers_grid_->CreateGrid(0, 4);
  triggers_grid_->SetColLabelValue(kTriggerName, "Trigger");
  triggers_grid_->SetColLabelValue(kTriggerTiming, "Timing");
  triggers_grid_->SetColLabelValue(kTriggerEvent, "Event");
  triggers_grid_->SetColLabelValue(kTriggerNotes, "Notes");
  triggers_grid_->SetColSize(kTriggerName, 200);
  triggers_grid_->SetColSize(kTriggerTiming, 120);
  triggers_grid_->SetColSize(kTriggerEvent, 120);
  triggers_grid_->SetColSize(kTriggerNotes, 320);

  auto* triggers_button_row = new wxBoxSizer(wxHORIZONTAL);
  add_trigger_btn_ =
      new wxButton(triggers_page, static_cast<int>(ControlId::kAddTrigger),
                   "Add Trigger");
  remove_trigger_btn_ =
      new wxButton(triggers_page, static_cast<int>(ControlId::kRemoveTrigger),
                   "Remove Selected");
  triggers_button_row->Add(add_trigger_btn_, 0, wxRIGHT, 6);
  triggers_button_row->Add(remove_trigger_btn_, 0);
  triggers_button_row->AddStretchSpacer(1);

  triggers_sizer->Add(triggers_grid_, 1, wxEXPAND | wxBOTTOM, 6);
  triggers_sizer->Add(triggers_button_row, 0, wxEXPAND);
  triggers_page->SetSizer(triggers_sizer);

  notebook_->AddPage(columns_page, "Columns");
  notebook_->AddPage(indexes_page, "Indexes");
  notebook_->AddPage(triggers_page, "Triggers");

  auto* button_sizer = new wxStdDialogButtonSizer();
  auto* ok_button = new wxButton(root, wxID_OK);
  auto* cancel_button = new wxButton(root, wxID_CANCEL);
  button_sizer->AddButton(ok_button);
  button_sizer->AddButton(cancel_button);
  button_sizer->Realize();

  root_sizer->Add(form_grid, 0, wxEXPAND | wxALL, 8);
  root_sizer->Add(notebook_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
  root_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 8);

  root->SetSizer(root_sizer);
}

void TableMetadataDialog::BindEvents() {
  Bind(wxEVT_BUTTON, &TableMetadataDialog::OnAddColumn, this,
       static_cast<int>(ControlId::kAddColumn));
  Bind(wxEVT_BUTTON, &TableMetadataDialog::OnRemoveColumn, this,
       static_cast<int>(ControlId::kRemoveColumn));
  Bind(wxEVT_BUTTON, &TableMetadataDialog::OnApplyTypeHelper, this,
       static_cast<int>(ControlId::kApplyTypeHelper));
  Bind(wxEVT_BUTTON, &TableMetadataDialog::OnAddIndex, this,
       static_cast<int>(ControlId::kAddIndex));
  Bind(wxEVT_BUTTON, &TableMetadataDialog::OnRemoveIndex, this,
       static_cast<int>(ControlId::kRemoveIndex));
  Bind(wxEVT_BUTTON, &TableMetadataDialog::OnAddTrigger, this,
       static_cast<int>(ControlId::kAddTrigger));
  Bind(wxEVT_BUTTON, &TableMetadataDialog::OnRemoveTrigger, this,
       static_cast<int>(ControlId::kRemoveTrigger));
  Bind(wxEVT_BUTTON, &TableMetadataDialog::OnOk, this, wxID_OK);
}

void TableMetadataDialog::ApplyMode() {
  switch (mode_) {
    case Mode::kCreate:
      SetTitle("Create Table Metadata");
      break;
    case Mode::kEdit:
      SetTitle("Edit Table Metadata");
      break;
    case Mode::kView:
      SetTitle("View Table Metadata");
      break;
  }

  if (mode_ != Mode::kView) {
    return;
  }

  schema_path_ctrl_->SetEditable(false);
  table_name_ctrl_->SetEditable(false);
  description_ctrl_->SetEditable(false);
  columns_grid_->EnableEditing(false);
  indexes_grid_->EnableEditing(false);
  triggers_grid_->EnableEditing(false);
  type_helper_choice_->Disable();
  apply_type_btn_->Disable();
  add_column_btn_->Disable();
  remove_column_btn_->Disable();
  add_index_btn_->Disable();
  remove_index_btn_->Disable();
  add_trigger_btn_->Disable();
  remove_trigger_btn_->Disable();

  if (wxWindow* ok = FindWindow(wxID_OK)) {
    ok->SetLabel("Close");
  }
  if (wxWindow* cancel = FindWindow(wxID_CANCEL)) {
    cancel->Hide();
  }
}

void TableMetadataDialog::PopulateFromSeed() {
  schema_path_ctrl_->SetValue(seed_.schema_path);
  table_name_ctrl_->SetValue(seed_.table_name);
  description_ctrl_->SetValue(seed_.description);

  if (!seed_.columns.empty()) {
    columns_grid_->AppendRows(static_cast<int>(seed_.columns.size()));
  }
  for (int row = 0; row < static_cast<int>(seed_.columns.size()); ++row) {
    const auto& col = seed_.columns[row];
    columns_grid_->SetCellValue(row, kColumnName, col.column_name);
    columns_grid_->SetCellValue(row, kColumnDomain, col.domain_name);
    columns_grid_->SetCellValue(row, kColumnType, col.data_type);
    columns_grid_->SetCellValue(row, kColumnNullable, BoolToCell(col.nullable));
    columns_grid_->SetCellValue(row, kColumnPk, BoolToCell(col.is_primary_key));
    columns_grid_->SetCellValue(row, kColumnFk, BoolToCell(col.is_foreign_key));
    columns_grid_->SetCellValue(row, kColumnRefSchema, col.references_schema_path);
    columns_grid_->SetCellValue(row, kColumnRefTable, col.references_table_name);
    columns_grid_->SetCellValue(row, kColumnRefColumn, col.references_column_name);
    columns_grid_->SetCellValue(row, kColumnDefault, col.default_value);
    columns_grid_->SetCellValue(row, kColumnNotes, col.notes);
  }

  if (!seed_.indexes.empty()) {
    indexes_grid_->AppendRows(static_cast<int>(seed_.indexes.size()));
  }
  for (int row = 0; row < static_cast<int>(seed_.indexes.size()); ++row) {
    const auto& idx = seed_.indexes[row];
    indexes_grid_->SetCellValue(row, kIndexName, idx.index_name);
    indexes_grid_->SetCellValue(row, kIndexUnique, BoolToCell(idx.unique));
    indexes_grid_->SetCellValue(row, kIndexMethod, idx.method);
    indexes_grid_->SetCellValue(row, kIndexTarget, idx.target);
    indexes_grid_->SetCellValue(row, kIndexNotes, idx.notes);
  }

  if (!seed_.triggers.empty()) {
    triggers_grid_->AppendRows(static_cast<int>(seed_.triggers.size()));
  }
  for (int row = 0; row < static_cast<int>(seed_.triggers.size()); ++row) {
    const auto& trigger = seed_.triggers[row];
    triggers_grid_->SetCellValue(row, kTriggerName, trigger.trigger_name);
    triggers_grid_->SetCellValue(row, kTriggerTiming, trigger.timing);
    triggers_grid_->SetCellValue(row, kTriggerEvent, trigger.event);
    triggers_grid_->SetCellValue(row, kTriggerNotes, trigger.notes);
  }
}

bool TableMetadataDialog::ValidateInput(wxString* error_message) const {
  if (schema_path_ctrl_->GetValue().Trim().Trim(false).IsEmpty()) {
    if (error_message) {
      *error_message = "Schema path is required.";
    }
    return false;
  }
  if (table_name_ctrl_->GetValue().Trim().Trim(false).IsEmpty()) {
    if (error_message) {
      *error_message = "Table name is required.";
    }
    return false;
  }

  const int rows = columns_grid_->GetNumberRows();
  for (int row = 0; row < rows; ++row) {
    const wxString col_name = columns_grid_->GetCellValue(row, kColumnName).Trim().Trim(false);
    if (col_name.IsEmpty()) {
      const bool has_text_content =
          !columns_grid_->GetCellValue(row, kColumnDomain).Trim().Trim(false).IsEmpty() ||
          !columns_grid_->GetCellValue(row, kColumnType).Trim().Trim(false).IsEmpty() ||
          !columns_grid_->GetCellValue(row, kColumnRefSchema).Trim().Trim(false).IsEmpty() ||
          !columns_grid_->GetCellValue(row, kColumnRefTable).Trim().Trim(false).IsEmpty() ||
          !columns_grid_->GetCellValue(row, kColumnRefColumn).Trim().Trim(false).IsEmpty() ||
          !columns_grid_->GetCellValue(row, kColumnDefault).Trim().Trim(false).IsEmpty() ||
          !columns_grid_->GetCellValue(row, kColumnNotes).Trim().Trim(false).IsEmpty();
      const bool has_key_flags =
          CellToBool(columns_grid_->GetCellValue(row, kColumnPk)) ||
          CellToBool(columns_grid_->GetCellValue(row, kColumnFk));
      if (has_text_content || has_key_flags) {
        if (error_message) {
          *error_message = "Every populated column row must include a column name.";
        }
        return false;
      }
      continue;
    }
    const wxString col_type = columns_grid_->GetCellValue(row, kColumnType).Trim().Trim(false);
    const wxString col_domain =
        columns_grid_->GetCellValue(row, kColumnDomain).Trim().Trim(false);
    if (col_type.IsEmpty() && col_domain.IsEmpty()) {
      if (error_message) {
        *error_message = "Column '" + col_name +
                         "' is missing both domain and type.";
      }
      return false;
    }
    const bool is_fk = CellToBool(columns_grid_->GetCellValue(row, kColumnFk));
    const wxString ref_table =
        columns_grid_->GetCellValue(row, kColumnRefTable).Trim().Trim(false);
    if (is_fk && ref_table.IsEmpty()) {
      if (error_message) {
        *error_message = "FK column '" + col_name + "' must include a referenced table.";
      }
      return false;
    }
  }

  const int index_rows = indexes_grid_->GetNumberRows();
  for (int row = 0; row < index_rows; ++row) {
    const wxString index_name =
        indexes_grid_->GetCellValue(row, kIndexName).Trim().Trim(false);
    if (index_name.IsEmpty() &&
        RowHasAnyValue(indexes_grid_, row, indexes_grid_->GetNumberCols())) {
      if (error_message) {
        *error_message = "Every populated index row must include an index name.";
      }
      return false;
    }
  }

  const int trigger_rows = triggers_grid_->GetNumberRows();
  for (int row = 0; row < trigger_rows; ++row) {
    const wxString trigger_name =
        triggers_grid_->GetCellValue(row, kTriggerName).Trim().Trim(false);
    if (trigger_name.IsEmpty() &&
        RowHasAnyValue(triggers_grid_, row, triggers_grid_->GetNumberCols())) {
      if (error_message) {
        *error_message = "Every populated trigger row must include a trigger name.";
      }
      return false;
    }
  }
  return true;
}

void TableMetadataDialog::OnAddColumn(wxCommandEvent&) {
  const int row = columns_grid_->GetNumberRows();
  columns_grid_->AppendRows(1);
  columns_grid_->SetCellValue(row, kColumnNullable, "1");
  columns_grid_->SetCellValue(row, kColumnPk, "0");
  columns_grid_->SetCellValue(row, kColumnFk, "0");
  columns_grid_->SetGridCursor(row, kColumnName);
  columns_grid_->MakeCellVisible(row, kColumnName);
}

void TableMetadataDialog::OnRemoveColumn(wxCommandEvent&) {
  DeleteFocusedOrSelectedRows(columns_grid_);
}

void TableMetadataDialog::OnApplyTypeHelper(wxCommandEvent&) {
  if (!columns_grid_ || !type_helper_choice_ || type_helper_choice_->GetSelection() == wxNOT_FOUND) {
    return;
  }

  const wxString selected_type = type_helper_choice_->GetStringSelection();
  if (selected_type.IsEmpty()) {
    return;
  }

  wxArrayInt rows = columns_grid_->GetSelectedRows();
  if (rows.IsEmpty()) {
    const int row = columns_grid_->GetGridCursorRow();
    if (row >= 0 && row < columns_grid_->GetNumberRows()) {
      columns_grid_->SetCellValue(row, kColumnType, selected_type);
      columns_grid_->SetGridCursor(row, kColumnType);
      columns_grid_->MakeCellVisible(row, kColumnType);
    }
    return;
  }

  for (const int row : rows) {
    if (row < 0 || row >= columns_grid_->GetNumberRows()) {
      continue;
    }
    columns_grid_->SetCellValue(row, kColumnType, selected_type);
  }
}

void TableMetadataDialog::OnAddIndex(wxCommandEvent&) {
  const int row = indexes_grid_->GetNumberRows();
  indexes_grid_->AppendRows(1);
  indexes_grid_->SetGridCursor(row, kIndexName);
  indexes_grid_->MakeCellVisible(row, kIndexName);
}

void TableMetadataDialog::OnRemoveIndex(wxCommandEvent&) {
  DeleteFocusedOrSelectedRows(indexes_grid_);
}

void TableMetadataDialog::OnAddTrigger(wxCommandEvent&) {
  const int row = triggers_grid_->GetNumberRows();
  triggers_grid_->AppendRows(1);
  triggers_grid_->SetGridCursor(row, kTriggerName);
  triggers_grid_->MakeCellVisible(row, kTriggerName);
}

void TableMetadataDialog::OnRemoveTrigger(wxCommandEvent&) {
  DeleteFocusedOrSelectedRows(triggers_grid_);
}

void TableMetadataDialog::OnOk(wxCommandEvent&) {
  if (mode_ == Mode::kView) {
    EndModal(wxID_OK);
    return;
  }

  wxString error;
  if (!ValidateInput(&error)) {
    wxMessageBox(error, "Invalid Metadata", wxOK | wxICON_WARNING, this);
    return;
  }

  EndModal(wxID_OK);
}

}  // namespace scratchrobin::ui
