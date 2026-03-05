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

#include <wx/frame.h>
#include <wx/splitter.h>
#include <string>

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::backend {
struct ScratchbirdRuntimeConfig;
}

namespace scratchrobin::ui {

/**
 * SQL Editor Frame - New Implementation
 * Standalone window for SQL editing and execution
 */
class SqlEditorFrameNew : public wxFrame {
 public:
  SqlEditorFrameNew(wxWindow* parent,
                    scratchrobin::backend::SessionClient* session_client,
                    scratchrobin::backend::ScratchbirdRuntimeConfig* runtime_config);
  ~SqlEditorFrameNew() override;

  // SQL text operations
  void SetSqlText(const std::string& sql);
  std::string GetSqlText() const;
  void ClearSqlText();

  // Execution
  void ExecuteCurrentQuery();
  void ExecuteAllQueries();
  void CancelExecution();

  // File operations
  void OpenFile(const std::string& filepath);
  bool SaveFile();
  bool SaveFileAs(const std::string& filepath);

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void CreateMenuBar();
  void CreateMainToolBar();

  // Event handlers
  void OnExecute(wxCommandEvent& event);
  void OnExecuteAll(wxCommandEvent& event);
  void OnCancel(wxCommandEvent& event);
  void OnOpen(wxCommandEvent& event);
  void OnSave(wxCommandEvent& event);
  void OnSaveAs(wxCommandEvent& event);
  void OnClose(wxCommandEvent& event);
  void OnCut(wxCommandEvent& event);
  void OnCopy(wxCommandEvent& event);
  void OnPaste(wxCommandEvent& event);
  void OnUndo(wxCommandEvent& event);
  void OnRedo(wxCommandEvent& event);
  void OnFind(wxCommandEvent& event);
  void OnReplace(wxCommandEvent& event);
  void OnPreferences(wxCommandEvent& event);
  void OnHelp(wxCommandEvent& event);

  // Backend
  scratchrobin::backend::SessionClient* session_client_;
  scratchrobin::backend::ScratchbirdRuntimeConfig* runtime_config_;

  // UI Components
  class SqlEditor* sql_editor_{nullptr};
  class DataGrid* results_grid_{nullptr};
  wxSplitterWindow* splitter_{nullptr};

  // State
  std::string current_file_path_;
  bool is_modified_{false};
  bool is_executing_{false};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
