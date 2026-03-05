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
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>

namespace scratchrobin::ui {

/**
 * Data Import/Export Dialog
 * Dialog for importing and exporting data
 */
class DataImportExportDialog : public wxDialog {
 public:
  DataImportExportDialog(wxWindow* parent);
  ~DataImportExportDialog() override;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnImport(wxCommandEvent& event);
  void OnExport(wxCommandEvent& event);
  void OnBrowseSource(wxCommandEvent& event);
  void OnBrowseTarget(wxCommandEvent& event);
  void OnCancel(wxCommandEvent& event);

  // UI Components
  wxChoice* format_choice_{nullptr};
  wxChoice* table_choice_{nullptr};
  wxTextCtrl* source_path_{nullptr};
  wxTextCtrl* target_path_{nullptr};
  wxCheckBox* header_check_{nullptr};
  wxCheckBox* overwrite_check_{nullptr};
  wxButton* browse_source_btn_{nullptr};
  wxButton* browse_target_btn_{nullptr};
  wxButton* import_btn_{nullptr};
  wxButton* export_btn_{nullptr};
  wxButton* cancel_btn_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
