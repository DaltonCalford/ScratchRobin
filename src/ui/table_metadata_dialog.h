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

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>

#include "backend/preview_metadata_store.h"

namespace scratchrobin::ui {

class TableMetadataDialog final : public wxDialog {
 public:
  enum class Mode {
    kCreate = 0,
    kEdit = 1,
    kView = 2,
  };

  TableMetadataDialog(wxWindow* parent, Mode mode,
                      const backend::PreviewTableMetadata& seed_metadata);

  backend::PreviewTableMetadata GetMetadata() const;

 private:
  enum class ControlId {
    kAddColumn = wxID_HIGHEST + 410,
    kRemoveColumn,
    kApplyTypeHelper,
    kAddIndex,
    kRemoveIndex,
    kAddTrigger,
    kRemoveTrigger,
  };

  void BuildUi();
  void BindEvents();
  void ApplyMode();
  void PopulateFromSeed();
  bool ValidateInput(wxString* error_message) const;

  void OnAddColumn(wxCommandEvent& event);
  void OnRemoveColumn(wxCommandEvent& event);
  void OnApplyTypeHelper(wxCommandEvent& event);
  void OnAddIndex(wxCommandEvent& event);
  void OnRemoveIndex(wxCommandEvent& event);
  void OnAddTrigger(wxCommandEvent& event);
  void OnRemoveTrigger(wxCommandEvent& event);
  void OnOk(wxCommandEvent& event);

  Mode mode_;
  backend::PreviewTableMetadata seed_;

  wxTextCtrl* schema_path_ctrl_{nullptr};
  wxTextCtrl* table_name_ctrl_{nullptr};
  wxTextCtrl* description_ctrl_{nullptr};
  wxNotebook* notebook_{nullptr};

  wxGrid* columns_grid_{nullptr};
  wxChoice* type_helper_choice_{nullptr};
  wxButton* add_column_btn_{nullptr};
  wxButton* remove_column_btn_{nullptr};
  wxButton* apply_type_btn_{nullptr};

  wxGrid* indexes_grid_{nullptr};
  wxButton* add_index_btn_{nullptr};
  wxButton* remove_index_btn_{nullptr};

  wxGrid* triggers_grid_{nullptr};
  wxButton* add_trigger_btn_{nullptr};
  wxButton* remove_trigger_btn_{nullptr};
};

}  // namespace scratchrobin::ui
