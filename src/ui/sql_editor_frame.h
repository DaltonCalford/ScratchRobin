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
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>
#include <wx/textctrl.h>

#include "backend/scratchbird_runtime_config.h"
#include "backend/session_client.h"

namespace scratchrobin::ui {

class SqlEditorFrame final : public wxFrame {
 public:
  SqlEditorFrame(backend::SessionClient* session_client,
                 backend::ScratchbirdRuntimeConfig* runtime_config);

 private:
  enum class ControlId {
    kExecuteButton = wxID_HIGHEST + 300,
    kCommitButton,
    kRollbackButton,
    kClearButton,
  };

  void BuildUi();
  void BindEvents();
  void RenderResult(const backend::QueryResponse& response);
  void AppendLogLine(const wxString& line);

  void OnExecute(wxCommandEvent& event);
  void OnCommit(wxCommandEvent& event);
  void OnRollback(wxCommandEvent& event);
  void OnClear(wxCommandEvent& event);

  backend::SessionClient* session_client_;
  backend::ScratchbirdRuntimeConfig* runtime_config_;

  wxStyledTextCtrl* sql_editor_{nullptr};
  wxNotebook* notebook_{nullptr};
  wxGrid* result_grid_{nullptr};
  wxTextCtrl* output_log_{nullptr};
};

}  // namespace scratchrobin::ui
