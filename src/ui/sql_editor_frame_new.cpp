/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "sql_editor_frame_new.h"

#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/toolbar.h>
#include <wx/menu.h>
#include <wx/fdrepdlg.h>
#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>

#include "backend/session_client.h"
#include "backend/scratchbird_runtime_config.h"
#include "components/sql_editor.h"
#include "components/data_grid.h"

namespace scratchrobin::ui {

// Menu IDs
enum {
  ID_EXECUTE = wxID_HIGHEST + 1,
  ID_EXECUTE_ALL,
  ID_CANCEL,
};

wxBEGIN_EVENT_TABLE(SqlEditorFrameNew, wxFrame)
  EVT_MENU(wxID_OPEN, SqlEditorFrameNew::OnOpen)
  EVT_MENU(wxID_SAVE, SqlEditorFrameNew::OnSave)
  EVT_MENU(wxID_SAVEAS, SqlEditorFrameNew::OnSaveAs)
  EVT_MENU(wxID_CLOSE, SqlEditorFrameNew::OnClose)
  EVT_MENU(wxID_EXIT, SqlEditorFrameNew::OnClose)
  EVT_MENU(wxID_CUT, SqlEditorFrameNew::OnCut)
  EVT_MENU(wxID_COPY, SqlEditorFrameNew::OnCopy)
  EVT_MENU(wxID_PASTE, SqlEditorFrameNew::OnPaste)
  EVT_MENU(wxID_UNDO, SqlEditorFrameNew::OnUndo)
  EVT_MENU(wxID_REDO, SqlEditorFrameNew::OnRedo)
  EVT_MENU(wxID_FIND, SqlEditorFrameNew::OnFind)
  EVT_MENU(wxID_REPLACE, SqlEditorFrameNew::OnReplace)
  EVT_MENU(wxID_PREFERENCES, SqlEditorFrameNew::OnPreferences)
  EVT_MENU(wxID_HELP, SqlEditorFrameNew::OnHelp)
  EVT_MENU(ID_EXECUTE, SqlEditorFrameNew::OnExecute)
  EVT_MENU(ID_EXECUTE_ALL, SqlEditorFrameNew::OnExecuteAll)
  EVT_MENU(ID_CANCEL, SqlEditorFrameNew::OnCancel)
wxEND_EVENT_TABLE()

SqlEditorFrameNew::SqlEditorFrameNew(wxWindow* parent,
                                     scratchrobin::backend::SessionClient* session_client,
                                     scratchrobin::backend::ScratchbirdRuntimeConfig* runtime_config)
    : wxFrame(parent, wxID_ANY, _("SQL Editor"), wxDefaultPosition, wxSize(900, 700)),
      session_client_(session_client),
      runtime_config_(runtime_config) {
  CreateControls();
  SetupLayout();
  CreateMenuBar();
  CreateMainToolBar();
}

SqlEditorFrameNew::~SqlEditorFrameNew() {
  // Cleanup handled by wxWidgets
}

void SqlEditorFrameNew::CreateControls() {
  // Create splitter window
  splitter_ = new wxSplitterWindow(this, wxID_ANY);
  
  // Create SQL editor (top panel)
  sql_editor_ = new SqlEditor(splitter_, wxID_ANY);
  
  // Create results grid (bottom panel)
  results_grid_ = new DataGrid(splitter_, wxID_ANY);
  
  // Split the window
  splitter_->SplitHorizontally(sql_editor_, results_grid_, 400);
  splitter_->SetMinimumPaneSize(100);
}

void SqlEditorFrameNew::SetupLayout() {
  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(splitter_, 1, wxEXPAND);
  SetSizer(sizer);
}

void SqlEditorFrameNew::CreateMenuBar() {
  wxMenuBar* menu_bar = new wxMenuBar();
  
  // File menu
  wxMenu* file_menu = new wxMenu();
  file_menu->Append(wxID_OPEN, _("&Open...\tCtrl+O"));
  file_menu->Append(wxID_SAVE, _("&Save\tCtrl+S"));
  file_menu->Append(wxID_SAVEAS, _("Save &As...\tCtrl+Shift+S"));
  file_menu->AppendSeparator();
  file_menu->Append(wxID_CLOSE, _("&Close\tCtrl+W"));
  menu_bar->Append(file_menu, _("&File"));
  
  // Edit menu
  wxMenu* edit_menu = new wxMenu();
  edit_menu->Append(wxID_UNDO, _("&Undo\tCtrl+Z"));
  edit_menu->Append(wxID_REDO, _("&Redo\tCtrl+Y"));
  edit_menu->AppendSeparator();
  edit_menu->Append(wxID_CUT, _("Cu&t\tCtrl+X"));
  edit_menu->Append(wxID_COPY, _("&Copy\tCtrl+C"));
  edit_menu->Append(wxID_PASTE, _("&Paste\tCtrl+V"));
  edit_menu->AppendSeparator();
  edit_menu->Append(wxID_FIND, _("&Find...\tCtrl+F"));
  edit_menu->Append(wxID_REPLACE, _("&Replace...\tCtrl+H"));
  edit_menu->AppendSeparator();
  edit_menu->Append(wxID_PREFERENCES, _("&Preferences..."));
  menu_bar->Append(edit_menu, _("&Edit"));
  
  // Query menu
  wxMenu* query_menu = new wxMenu();
  query_menu->Append(ID_EXECUTE, _("&Execute\tF5"));
  query_menu->Append(ID_EXECUTE_ALL, _("Execute &All\tF6"));
  query_menu->Append(ID_CANCEL, _("&Cancel Execution\tEsc"));
  menu_bar->Append(query_menu, _("&Query"));
  
  // Help menu
  wxMenu* help_menu = new wxMenu();
  help_menu->Append(wxID_HELP, _("&Documentation\tF1"));
  menu_bar->Append(help_menu, _("&Help"));
  
  SetMenuBar(menu_bar);
}

void SqlEditorFrameNew::CreateMainToolBar() {
  wxToolBar* toolbar = CreateToolBar(wxTB_FLAT | wxTB_HORIZONTAL);
  toolbar->AddTool(wxID_OPEN, _("Open"), wxArtProvider::GetBitmap(wxART_FILE_OPEN));
  toolbar->AddTool(wxID_SAVE, _("Save"), wxArtProvider::GetBitmap(wxART_FILE_SAVE));
  toolbar->AddSeparator();
  toolbar->AddTool(wxID_CUT, _("Cut"), wxArtProvider::GetBitmap(wxART_CUT));
  toolbar->AddTool(wxID_COPY, _("Copy"), wxArtProvider::GetBitmap(wxART_COPY));
  toolbar->AddTool(wxID_PASTE, _("Paste"), wxArtProvider::GetBitmap(wxART_PASTE));
  toolbar->AddSeparator();
  toolbar->AddTool(ID_EXECUTE, _("Execute"), wxArtProvider::GetBitmap(wxART_GO_FORWARD));
  toolbar->AddTool(ID_CANCEL, _("Cancel"), wxArtProvider::GetBitmap(wxART_STOP));
  toolbar->Realize();
}

void SqlEditorFrameNew::SetSqlText(const std::string& sql) {
  // TODO: Implement via sql_editor_->SetText(sql);
}

std::string SqlEditorFrameNew::GetSqlText() const {
  // TODO: Implement via sql_editor_->GetText();
  return "";
}

void SqlEditorFrameNew::ClearSqlText() {
  // TODO: Implement via sql_editor_->Clear();
}

void SqlEditorFrameNew::ExecuteCurrentQuery() {
  // TODO: Implement query execution
}

void SqlEditorFrameNew::ExecuteAllQueries() {
  // TODO: Implement query execution
}

void SqlEditorFrameNew::CancelExecution() {
  // TODO: Implement query cancellation
}

void SqlEditorFrameNew::OpenFile(const std::string& filepath) {
  // TODO: Implement file open
}

bool SqlEditorFrameNew::SaveFile() {
  // TODO: Implement file save
  return false;
}

bool SqlEditorFrameNew::SaveFileAs(const std::string& filepath) {
  // TODO: Implement file save as
  return false;
}

// Event handlers
void SqlEditorFrameNew::OnExecute(wxCommandEvent& event) {
  ExecuteCurrentQuery();
}

void SqlEditorFrameNew::OnExecuteAll(wxCommandEvent& event) {
  ExecuteAllQueries();
}

void SqlEditorFrameNew::OnCancel(wxCommandEvent& event) {
  CancelExecution();
}

void SqlEditorFrameNew::OnOpen(wxCommandEvent& event) {
  wxFileDialog dlg(this, _("Open SQL file"), "", "",
                   "SQL files (*.sql)|*.sql|All files (*.*)|*.*",
                   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (dlg.ShowModal() == wxID_OK) {
    OpenFile(dlg.GetPath().ToStdString());
  }
}

void SqlEditorFrameNew::OnSave(wxCommandEvent& event) {
  SaveFile();
}

void SqlEditorFrameNew::OnSaveAs(wxCommandEvent& event) {
  wxFileDialog dlg(this, _("Save SQL file"), "", "",
                   "SQL files (*.sql)|*.sql|All files (*.*)|*.*",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() == wxID_OK) {
    SaveFileAs(dlg.GetPath().ToStdString());
  }
}

void SqlEditorFrameNew::OnClose(wxCommandEvent& event) {
  Close();
}

void SqlEditorFrameNew::OnCut(wxCommandEvent& event) {
  // TODO: Implement cut
}

void SqlEditorFrameNew::OnCopy(wxCommandEvent& event) {
  // TODO: Implement copy
}

void SqlEditorFrameNew::OnPaste(wxCommandEvent& event) {
  // TODO: Implement paste
}

void SqlEditorFrameNew::OnUndo(wxCommandEvent& event) {
  // TODO: Implement undo
}

void SqlEditorFrameNew::OnRedo(wxCommandEvent& event) {
  // TODO: Implement redo
}

void SqlEditorFrameNew::OnFind(wxCommandEvent& event) {
  wxFindReplaceDialog dlg(this, nullptr, _("Find"), wxFR_NOWHOLEWORD);
  dlg.Show(true);
}

void SqlEditorFrameNew::OnReplace(wxCommandEvent& event) {
  wxFindReplaceDialog dlg(this, nullptr, _("Replace"), wxFR_REPLACEDIALOG);
  dlg.Show(true);
}

void SqlEditorFrameNew::OnPreferences(wxCommandEvent& event) {
  // TODO: Show preferences dialog
}

void SqlEditorFrameNew::OnHelp(wxCommandEvent& event) {
  // TODO: Show help
}

}  // namespace scratchrobin::ui
