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

#include <functional>
#include <string>

#include <wx/grid.h>

namespace scratchrobin::ui {

/**
 * DataCellEditor - Custom cell editor for data grid with validation
 */
class DataCellEditor : public wxGridCellEditor {
 public:
  DataCellEditor();
  ~DataCellEditor() override;

  // wxGridCellEditor overrides
  void Create(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler) override;
  void BeginEdit(int row, int col, wxGrid* grid) override;
  bool EndEdit(int row, int col, const wxGrid* grid,
               const wxString& oldval, wxString* newval) override;
  void ApplyEdit(int row, int col, wxGrid* grid) override;
  void Reset() override;
  wxGridCellEditor* Clone() const override;
  wxString GetValue() const override;

  // Set data type for validation
  void SetDataType(const wxString& data_type);

  // Set maximum length
  void SetMaxLength(int max_length);

  // Set whether null is allowed
  void SetAllowNull(bool allow) { allow_null_ = allow; }

  // Set custom validator
  using ValidatorFn = std::function<bool(const wxString& value, wxString* error)>;
  void SetValidator(ValidatorFn validator);

 private:
  wxTextCtrl* text_ctrl_{nullptr};
  wxString data_type_;
  int max_length_{-1};
  bool allow_null_{true};
  ValidatorFn custom_validator_;

  bool ValidateValue(const wxString& value, wxString* error) const;
};

/**
 * DataCellRenderer - Custom cell renderer with formatting
 */
class DataCellRenderer : public wxGridCellStringRenderer {
 public:
  DataCellRenderer();
  ~DataCellRenderer() override;

  // wxGridCellRenderer overrides
  void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
            const wxRect& rect, int row, int col, bool isSelected) override;

  // Set format for numbers/dates
  void SetFormat(const wxString& format);

  // Set alignment
  void SetAlignment(int h_align, int v_align);

  // Set text color for specific values
  void SetTextColor(const wxColor& color);

  // Set background color for specific values
  void SetBackgroundColor(const wxColor& color);

 private:
  wxString format_;
  int h_align_{wxALIGN_LEFT};
  int v_align_{wxALIGN_CENTRE_VERTICAL};
  wxColor text_color_;
  wxColor bg_color_;
  bool has_text_color_{false};
  bool has_bg_color_{false};
};

}  // namespace scratchrobin::ui
