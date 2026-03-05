/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/sql_editor_frame.h"

#include <cstdint>
#include <string>

#include <wx/button.h>
#include <wx/font.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

namespace scratchrobin::ui {

namespace {
constexpr char kDialect[] = "scratchbird-native";
}

SqlEditorFrame::SqlEditorFrame(backend::SessionClient* session_client,
                               backend::ScratchbirdRuntimeConfig* runtime_config)
    : wxFrame(nullptr, wxID_ANY, "Execute SQL", wxDefaultPosition,
              wxSize(1080, 760)),
      session_client_(session_client),
      runtime_config_(runtime_config) {
  BuildUi();
  BindEvents();

  CreateStatusBar(2);
  SetStatusText("Ready", 0);
  SetStatusText("ScratchBird SQL Editor", 1);
}

void SqlEditorFrame::BuildUi() {
  auto* root = new wxPanel(this, wxID_ANY);
  auto* root_sizer = new wxBoxSizer(wxVERTICAL);

  auto* toolbar_panel = new wxPanel(root, wxID_ANY);
  auto* toolbar_sizer = new wxBoxSizer(wxHORIZONTAL);

  auto* execute_btn =
      new wxButton(toolbar_panel, static_cast<int>(ControlId::kExecuteButton),
                   "Execute\tF8");
  auto* commit_btn =
      new wxButton(toolbar_panel, static_cast<int>(ControlId::kCommitButton), "Commit");
  auto* rollback_btn = new wxButton(toolbar_panel,
                                    static_cast<int>(ControlId::kRollbackButton),
                                    "Rollback");
  auto* clear_btn =
      new wxButton(toolbar_panel, static_cast<int>(ControlId::kClearButton), "Clear");

  toolbar_sizer->Add(execute_btn, 0, wxRIGHT, 6);
  toolbar_sizer->Add(commit_btn, 0, wxRIGHT, 6);
  toolbar_sizer->Add(rollback_btn, 0, wxRIGHT, 6);
  toolbar_sizer->Add(clear_btn, 0, wxRIGHT, 6);
  toolbar_panel->SetSizer(toolbar_sizer);

  auto* splitter = new wxSplitterWindow(root, wxID_ANY);
  splitter->SetMinimumPaneSize(180);

  auto* sql_panel = new wxPanel(splitter, wxID_ANY);
  auto* sql_sizer = new wxBoxSizer(wxVERTICAL);
  sql_editor_ = new wxStyledTextCtrl(sql_panel, wxID_ANY);
  sql_editor_->StyleSetForeground(wxSTC_STYLE_DEFAULT, *wxBLACK);
  sql_editor_->StyleSetBackground(wxSTC_STYLE_DEFAULT, *wxWHITE);
  sql_editor_->StyleSetFont(wxSTC_STYLE_DEFAULT,
                            wxFontInfo(11).Family(wxFONTFAMILY_TELETYPE));
  sql_editor_->SetText("select id, name from projects;");
  sql_sizer->Add(sql_editor_, 1, wxEXPAND);
  sql_panel->SetSizer(sql_sizer);

  auto* results_panel = new wxPanel(splitter, wxID_ANY);
  auto* results_sizer = new wxBoxSizer(wxVERTICAL);

  notebook_ = new wxNotebook(results_panel, wxID_ANY);

  auto* data_page = new wxPanel(notebook_, wxID_ANY);
  auto* data_sizer = new wxBoxSizer(wxVERTICAL);
  result_grid_ = new wxGrid(data_page, wxID_ANY);
  result_grid_->CreateGrid(0, 0);
  result_grid_->EnableEditing(false);
  result_grid_->SetRowLabelSize(70);
  data_sizer->Add(result_grid_, 1, wxEXPAND);
  data_page->SetSizer(data_sizer);

  auto* log_page = new wxPanel(notebook_, wxID_ANY);
  auto* log_sizer = new wxBoxSizer(wxVERTICAL);
  output_log_ = new wxTextCtrl(log_page, wxID_ANY, "", wxDefaultPosition,
                               wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
  log_sizer->Add(output_log_, 1, wxEXPAND);
  log_page->SetSizer(log_sizer);

  notebook_->AddPage(data_page, "Data", true);
  notebook_->AddPage(log_page, "Output", false);

  results_sizer->Add(notebook_, 1, wxEXPAND);
  results_panel->SetSizer(results_sizer);

  splitter->SplitHorizontally(sql_panel, results_panel, 340);

  root_sizer->Add(toolbar_panel, 0, wxEXPAND | wxALL, 6);
  root_sizer->Add(splitter, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  root->SetSizer(root_sizer);

  wxAcceleratorEntry entries[1];
  entries[0].Set(wxACCEL_NORMAL, WXK_F8, static_cast<int>(ControlId::kExecuteButton));
  wxAcceleratorTable accel(1, entries);
  SetAcceleratorTable(accel);
}

void SqlEditorFrame::BindEvents() {
  Bind(wxEVT_BUTTON, &SqlEditorFrame::OnExecute, this,
       static_cast<int>(ControlId::kExecuteButton));
  Bind(wxEVT_BUTTON, &SqlEditorFrame::OnCommit, this,
       static_cast<int>(ControlId::kCommitButton));
  Bind(wxEVT_BUTTON, &SqlEditorFrame::OnRollback, this,
       static_cast<int>(ControlId::kRollbackButton));
  Bind(wxEVT_BUTTON, &SqlEditorFrame::OnClear, this,
       static_cast<int>(ControlId::kClearButton));
}

void SqlEditorFrame::RenderResult(const backend::QueryResponse& response) {
  if (result_grid_->GetNumberRows() > 0) {
    result_grid_->DeleteRows(0, result_grid_->GetNumberRows());
  }
  if (result_grid_->GetNumberCols() > 0) {
    result_grid_->DeleteCols(0, result_grid_->GetNumberCols());
  }

  const int cols = static_cast<int>(response.result_set.columns.size());
  if (cols > 0) {
    result_grid_->AppendCols(cols);
    for (int c = 0; c < cols; ++c) {
      result_grid_->SetColLabelValue(c, response.result_set.columns[c]);
    }
  }

  const int rows = static_cast<int>(response.result_set.rows.size());
  if (rows > 0) {
    result_grid_->AppendRows(rows);
    for (int r = 0; r < rows; ++r) {
      const auto& row = response.result_set.rows[r];
      for (int c = 0; c < static_cast<int>(row.size()) && c < cols; ++c) {
        result_grid_->SetCellValue(r, c, row[c]);
      }
    }
  }

  result_grid_->AutoSizeColumns(false);
}

void SqlEditorFrame::AppendLogLine(const wxString& line) {
  if (!output_log_) {
    return;
  }
  output_log_->AppendText(line);
  output_log_->AppendText("\n");
}

void SqlEditorFrame::OnExecute(wxCommandEvent&) {
  const std::string sql = sql_editor_->GetText().ToStdString();
  if (sql.empty()) {
    wxMessageBox("SQL text is empty.", "No SQL", wxOK | wxICON_WARNING, this);
    return;
  }

  if (!runtime_config_) {
    wxMessageBox("Runtime configuration is unavailable.", "Configuration Error",
                 wxOK | wxICON_ERROR, this);
    return;
  }

  session_client_->ConfigureRuntime(*runtime_config_);
  const auto response =
      session_client_->ExecuteSql(runtime_config_->port, kDialect, sql);

  RenderResult(response);

  if (!response.status.ok) {
    SetStatusText("Execution failed", 0);
    AppendLogLine("error: " + wxString::FromUTF8(response.status.message));
    AppendLogLine("path: " + wxString::FromUTF8(response.execution_path));
    notebook_->SetSelection(1);
    return;
  }

  SetStatusText("Execution succeeded", 0);
  AppendLogLine("ok: " + wxString::FromUTF8(response.execution_path));
  AppendLogLine("mode: " + wxString::FromUTF8(backend::ToString(runtime_config_->mode)));
  notebook_->SetSelection(0);
}

void SqlEditorFrame::OnCommit(wxCommandEvent&) {
  AppendLogLine("commit requested (transaction command wiring pending)");
}

void SqlEditorFrame::OnRollback(wxCommandEvent&) {
  AppendLogLine("rollback requested (transaction command wiring pending)");
}

void SqlEditorFrame::OnClear(wxCommandEvent&) {
  sql_editor_->ClearAll();
  output_log_->Clear();
}

}  // namespace scratchrobin::ui
