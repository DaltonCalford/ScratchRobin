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

#include <string>
#include <vector>

#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "backend/preview_object_metadata_store.h"

namespace scratchrobin::ui {

class ObjectMetadataDialog final : public wxDialog {
 public:
  enum class Mode {
    kCreate = 0,
    kEdit = 1,
    kView = 2,
  };

  ObjectMetadataDialog(wxWindow* parent, Mode mode,
                       const backend::PreviewObjectMetadata& seed_metadata);

  backend::PreviewObjectMetadata GetMetadata() const;

 private:
  struct KindFieldControl {
    std::string property_name;
    std::string property_type;
    std::string notes;
    bool required{false};
    bool is_boolean{false};
    std::vector<wxString> enum_values;
    wxWindow* control{nullptr};
  };

  enum class ControlId {
    kAddProperty = wxID_HIGHEST + 470,
    kRemoveProperty,
    kObjectKind,
  };

  void BuildUi();
  void BindEvents();
  void ApplyMode();
  void PopulateFromSeed();
  bool ValidateInput(wxString* error_message) const;
  void RebuildKindFieldControls(bool preserve_values);
  void PopulateKindFieldControlsFromSeed();
  std::string SelectedObjectKind() const;
  std::string ReadKindFieldValue(const KindFieldControl& field) const;
  void WriteKindFieldValue(const KindFieldControl& field, const std::string& value);

  void OnAddProperty(wxCommandEvent& event);
  void OnRemoveProperty(wxCommandEvent& event);
  void OnObjectKindChanged(wxCommandEvent& event);
  void OnOk(wxCommandEvent& event);

  Mode mode_;
  backend::PreviewObjectMetadata seed_;

  wxTextCtrl* schema_path_ctrl_{nullptr};
  wxTextCtrl* object_name_ctrl_{nullptr};
  wxChoice* object_kind_choice_{nullptr};
  wxTextCtrl* description_ctrl_{nullptr};
  wxStaticText* kind_summary_label_{nullptr};
  wxPanel* kind_fields_panel_{nullptr};
  wxFlexGridSizer* kind_fields_sizer_{nullptr};
  wxGrid* properties_grid_{nullptr};
  wxButton* add_property_btn_{nullptr};
  wxButton* remove_property_btn_{nullptr};
  std::vector<KindFieldControl> kind_fields_;
};

}  // namespace scratchrobin::ui
