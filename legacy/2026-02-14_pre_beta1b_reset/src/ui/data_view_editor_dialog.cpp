/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "data_view_editor_dialog.h"

#include "core/data_view_validation.h"

#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

DataViewEditorDialog::DataViewEditorDialog(wxWindow* parent, const std::string& json_payload)
    : wxDialog(parent, wxID_ANY, "Edit Data View",
               wxDefaultPosition, wxSize(720, 520),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    BuildLayout();
    if (json_ctrl_) {
        json_ctrl_->SetValue(json_payload);
    }
    CentreOnParent();
}

std::string DataViewEditorDialog::json_payload() const {
    if (!json_ctrl_) {
        return {};
    }
    return json_ctrl_->GetValue().ToStdString();
}

void DataViewEditorDialog::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* label = new wxStaticText(this, wxID_ANY, "Data View JSON");
    rootSizer->Add(label, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    json_ctrl_ = new wxTextCtrl(this, wxID_ANY, "",
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_RICH2);
    rootSizer->Add(json_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* buttons = CreateButtonSizer(wxOK | wxCANCEL);
    rootSizer->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    SetSizer(rootSizer);

    Bind(wxEVT_BUTTON, &DataViewEditorDialog::OnOk, this, wxID_OK);
}

void DataViewEditorDialog::OnOk(wxCommandEvent&) {
    std::string payload = json_payload();
    std::string error;
    if (!ValidateDataViewJson(payload, &error)) {
        wxMessageBox("Invalid JSON: " + error, "Data View",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    EndModal(wxID_OK);
}

} // namespace scratchrobin
