/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/components/data_cell_editor.h"

#include <wx/textctrl.h>
#include <wx/msgdlg.h>

namespace scratchrobin::ui {

// DataCellEditor Implementation

DataCellEditor::DataCellEditor() = default;

DataCellEditor::~DataCellEditor() = default;

void DataCellEditor::Create(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler) {
  text_ctrl_ = new wxTextCtrl(parent, id, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB);
  
  text_ctrl_->SetMaxLength(max_length_ > 0 ? max_length_ : 0);
  
  wxGridCellEditor::Create(parent, id, evtHandler);
}

void DataCellEditor::BeginEdit(int row, int col, wxGrid* grid) {
  if (text_ctrl_) {
    text_ctrl_->SetValue(grid->GetCellValue(row, col));
    text_ctrl_->SetFocus();
    text_ctrl_->SelectAll();
  }
}

bool DataCellEditor::EndEdit(int row, int col, const wxGrid* grid,
                              const wxString& oldval, wxString* newval) {
  if (!text_ctrl_) {
    return false;
  }
  
  wxString value = text_ctrl_->GetValue();
  
  // Validate null
  if (value.IsEmpty() && !allow_null_) {
    wxString error = _("This field cannot be empty.");
    wxMessageBox(error, _("Validation Error"), wxOK | wxICON_WARNING);
    return false;
  }
  
  // Run custom validator if set
  if (custom_validator_) {
    wxString error;
    if (!custom_validator_(value, &error)) {
      wxMessageBox(error, _("Validation Error"), wxOK | wxICON_WARNING);
      return false;
    }
  }
  
  // Validate data type
  wxString error;
  if (!ValidateValue(value, &error)) {
    wxMessageBox(error, _("Validation Error"), wxOK | wxICON_WARNING);
    return false;
  }
  
  if (newval) {
    *newval = value;
  }
  
  return value != oldval;
}

void DataCellEditor::ApplyEdit(int row, int col, wxGrid* grid) {
  if (text_ctrl_) {
    grid->SetCellValue(row, col, text_ctrl_->GetValue());
  }
}

void DataCellEditor::Reset() {
  if (text_ctrl_) {
    text_ctrl_->SetValue(wxEmptyString);
  }
}

wxGridCellEditor* DataCellEditor::Clone() const {
  auto* editor = new DataCellEditor();
  editor->SetDataType(data_type_);
  editor->SetMaxLength(max_length_);
  editor->SetAllowNull(allow_null_);
  if (custom_validator_) {
    editor->SetValidator(custom_validator_);
  }
  return editor;
}

wxString DataCellEditor::GetValue() const {
  if (text_ctrl_) {
    return text_ctrl_->GetValue();
  }
  return wxEmptyString;
}

void DataCellEditor::SetDataType(const wxString& data_type) {
  data_type_ = data_type;
}

void DataCellEditor::SetMaxLength(int max_length) {
  max_length_ = max_length;
  if (text_ctrl_ && max_length_ > 0) {
    text_ctrl_->SetMaxLength(max_length_);
  }
}

void DataCellEditor::SetValidator(ValidatorFn validator) {
  custom_validator_ = validator;
}

bool DataCellEditor::ValidateValue(const wxString& value, wxString* error) const {
  if (data_type_.IsEmpty()) {
    return true;
  }
  
  wxString type_lower = data_type_.Lower();
  
  if (type_lower == "integer" || type_lower == "int" || 
      type_lower == "smallint" || type_lower == "bigint") {
    long val;
    if (!value.ToLong(&val) && !value.IsEmpty()) {
      *error = _("Value must be an integer.");
      return false;
    }
  }
  else if (type_lower == "float" || type_lower == "double" || 
           type_lower == "decimal" || type_lower == "numeric" ||
           type_lower == "real") {
    double val;
    if (!value.ToDouble(&val) && !value.IsEmpty()) {
      *error = _("Value must be a number.");
      return false;
    }
  }
  else if (type_lower == "date" || type_lower == "time" || 
           type_lower == "timestamp") {
    // Basic date/time validation could be added here
  }
  else if (type_lower == "boolean" || type_lower == "bool") {
    wxString val_lower = value.Lower();
    if (!value.IsEmpty() && 
        val_lower != "true" && val_lower != "false" &&
        val_lower != "1" && val_lower != "0" &&
        val_lower != "yes" && val_lower != "no") {
      *error = _("Value must be a boolean (true/false, yes/no, 1/0).");
      return false;
    }
  }
  
  return true;
}

// DataCellRenderer Implementation

DataCellRenderer::DataCellRenderer() = default;

DataCellRenderer::~DataCellRenderer() = default;

void DataCellRenderer::Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
                            const wxRect& rect, int row, int col, bool isSelected) {
  // Apply custom colors if set
  if (has_text_color_) {
    attr.SetTextColour(text_color_);
  }
  if (has_bg_color_) {
    attr.SetBackgroundColour(bg_color_);
  }
  
  // Apply alignment
  attr.SetAlignment(h_align_, v_align_);
  
  // Call base renderer
  wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
}

void DataCellRenderer::SetFormat(const wxString& format) {
  format_ = format;
}

void DataCellRenderer::SetAlignment(int h_align, int v_align) {
  h_align_ = h_align;
  v_align_ = v_align;
}

void DataCellRenderer::SetTextColor(const wxColor& color) {
  text_color_ = color;
  has_text_color_ = true;
}

void DataCellRenderer::SetBackgroundColor(const wxColor& color) {
  bg_color_ = color;
  has_bg_color_ = true;
}

}  // namespace scratchrobin::ui
