/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "prepared_params_dialog.h"

#include <string>

#include <wx/button.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace scratchrobin {

PreparedParamsDialog::PreparedParamsDialog(wxWindow* parent,
                                           size_t parameterCount,
                                           const std::vector<PreparedParameter>& initial)
    : wxDialog(parent, wxID_ANY, "Prepared Parameters",
               wxDefaultPosition, wxSize(520, 360),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      params_(initial) {
    auto* root = new wxBoxSizer(wxVERTICAL);
    root->Add(new wxStaticText(this, wxID_ANY,
                               "Enter parameter values (1-based index). Type hints: null, bool, int64, double, string, bytes."),
              0, wxLEFT | wxRIGHT | wxTOP, 8);

    grid_ = new wxGrid(this, wxID_ANY);
    grid_->CreateGrid(static_cast<int>(parameterCount), 3);
    grid_->SetColLabelValue(0, "Index");
    grid_->SetColLabelValue(1, "Type");
    grid_->SetColLabelValue(2, "Value");
    grid_->SetColSize(0, 60);
    grid_->SetColSize(1, 120);
    grid_->SetColSize(2, 260);
    grid_->EnableEditing(true);

    wxArrayString types;
    types.Add("null");
    types.Add("bool");
    types.Add("int64");
    types.Add("double");
    types.Add("string");
    types.Add("bytes");

    for (int row = 0; row < static_cast<int>(parameterCount); ++row) {
        grid_->SetCellValue(row, 0, wxString::Format("%d", row + 1));
        grid_->SetReadOnly(row, 0, true);
        PreparedParamType type = PreparedParamType::Null;
        std::string value;
        if (row < static_cast<int>(params_.size())) {
            type = params_[static_cast<size_t>(row)].type;
            switch (type) {
                case PreparedParamType::Bool:
                    value = params_[static_cast<size_t>(row)].boolValue ? "true" : "false";
                    break;
                case PreparedParamType::Int64:
                    value = std::to_string(params_[static_cast<size_t>(row)].intValue);
                    break;
                case PreparedParamType::Double:
                    value = std::to_string(params_[static_cast<size_t>(row)].doubleValue);
                    break;
                case PreparedParamType::String:
                    value = params_[static_cast<size_t>(row)].stringValue;
                    break;
                case PreparedParamType::Bytes:
                    value = "<bytes>";
                    break;
                default:
                    value.clear();
                    break;
            }
        }
        grid_->SetCellEditor(row, 1, new wxGridCellChoiceEditor(types));
        grid_->SetCellValue(row, 1, TypeToString(type));
        grid_->SetCellValue(row, 2, value);
    }

    root->Add(grid_, 1, wxEXPAND | wxALL, 8);

    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    apply_button_ = new wxButton(this, wxID_OK, "Apply");
    auto* cancel_button = new wxButton(this, wxID_CANCEL, "Cancel");
    buttons->AddStretchSpacer(1);
    buttons->Add(apply_button_, 0, wxRIGHT, 8);
    buttons->Add(cancel_button, 0);
    root->Add(buttons, 0, wxEXPAND | wxALL, 8);

    SetSizer(root);

    apply_button_->Bind(wxEVT_BUTTON, &PreparedParamsDialog::OnApply, this);
}

void PreparedParamsDialog::OnApply(wxCommandEvent&) {
    params_.clear();
    int rows = grid_ ? grid_->GetNumberRows() : 0;
    params_.reserve(static_cast<size_t>(rows));
    for (int row = 0; row < rows; ++row) {
        PreparedParameter param;
        std::string error;
        if (!TryParseRow(static_cast<size_t>(row), &param, &error)) {
            wxMessageBox("Row " + std::to_string(row + 1) + ": " + error,
                         "Invalid Parameter", wxOK | wxICON_WARNING, this);
            return;
        }
        params_.push_back(param);
    }
    EndModal(wxID_OK);
}

PreparedParamType PreparedParamsDialog::ParseType(const wxString& value) const {
    wxString lower = value.Lower();
    if (lower == "bool" || lower == "boolean") {
        return PreparedParamType::Bool;
    }
    if (lower == "int" || lower == "int64" || lower == "integer") {
        return PreparedParamType::Int64;
    }
    if (lower == "double" || lower == "float") {
        return PreparedParamType::Double;
    }
    if (lower == "string" || lower == "text" || lower == "varchar") {
        return PreparedParamType::String;
    }
    if (lower == "bytes" || lower == "bytea") {
        return PreparedParamType::Bytes;
    }
    return PreparedParamType::Null;
}

PreparedParameter PreparedParamsDialog::ParseRow(size_t row) const {
    PreparedParameter param;
    if (!grid_) {
        return param;
    }
    wxString typeValue = grid_->GetCellValue(static_cast<int>(row), 1);
    wxString dataValue = grid_->GetCellValue(static_cast<int>(row), 2);
    param.type = ParseType(typeValue);
    switch (param.type) {
        case PreparedParamType::Bool:
            param.boolValue = dataValue.Lower() == "true" || dataValue == "1";
            break;
        case PreparedParamType::Int64:
            param.intValue = std::stoll(dataValue.ToStdString());
            break;
        case PreparedParamType::Double:
            param.doubleValue = std::stod(dataValue.ToStdString());
            break;
        case PreparedParamType::String:
            param.stringValue = dataValue.ToStdString();
            break;
        case PreparedParamType::Bytes:
            param.bytesValue.assign(dataValue.begin(), dataValue.end());
            break;
        case PreparedParamType::Null:
        default:
            break;
    }
    return param;
}

bool PreparedParamsDialog::TryParseRow(size_t row, PreparedParameter* outParam,
                                       std::string* error) const {
    if (!outParam) {
        return false;
    }
    try {
        *outParam = ParseRow(row);
        return true;
    } catch (const std::exception& ex) {
        if (error) {
            *error = ex.what();
        }
    } catch (...) {
        if (error) {
            *error = "Invalid value";
        }
    }
    return false;
}

wxString PreparedParamsDialog::TypeToString(PreparedParamType type) const {
    switch (type) {
        case PreparedParamType::Bool: return "bool";
        case PreparedParamType::Int64: return "int64";
        case PreparedParamType::Double: return "double";
        case PreparedParamType::String: return "string";
        case PreparedParamType::Bytes: return "bytes";
        case PreparedParamType::Null: default: return "null";
    }
}

} // namespace scratchrobin
