/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "copy_dialog.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

CopyDialog::CopyDialog(wxWindow* parent, ConnectionManager* connectionManager,
                       const std::string& initialSql)
    : wxDialog(parent, wxID_ANY, "COPY", wxDefaultPosition, wxSize(640, 520),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      connection_manager_(connectionManager) {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* mode_row = new wxBoxSizer(wxHORIZONTAL);
    mode_row->Add(new wxStaticText(this, wxID_ANY, "Mode:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    mode_choice_ = new wxChoice(this, wxID_ANY);
    mode_choice_->Append("COPY IN");
    mode_choice_->Append("COPY OUT");
    mode_choice_->Append("COPY BOTH");
    mode_choice_->SetSelection(1);
    mode_row->Add(mode_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    root->Add(mode_row, 0, wxEXPAND | wxALL, 8);

    root->Add(new wxStaticText(this, wxID_ANY, "SQL:"), 0, wxLEFT | wxRIGHT, 8);
    sql_ctrl_ = new wxTextCtrl(this, wxID_ANY, initialSql, wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_RICH2);
    root->Add(sql_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* input_row = new wxBoxSizer(wxHORIZONTAL);
    input_row->Add(new wxStaticText(this, wxID_ANY, "Input file:"), 0,
                   wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    input_path_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    input_row->Add(input_path_ctrl_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto* input_browse = new wxButton(this, wxID_ANY, "Browse");
    input_row->Add(input_browse, 0, wxALIGN_CENTER_VERTICAL);
    root->Add(input_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    input_clipboard_check_ = new wxCheckBox(this, wxID_ANY, "Use clipboard for input");
    root->Add(input_clipboard_check_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* output_row = new wxBoxSizer(wxHORIZONTAL);
    output_row->Add(new wxStaticText(this, wxID_ANY, "Output file:"), 0,
                    wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    output_path_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    output_row->Add(output_path_ctrl_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto* output_browse = new wxButton(this, wxID_ANY, "Browse");
    output_row->Add(output_browse, 0, wxALIGN_CENTER_VERTICAL);
    root->Add(output_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    output_clipboard_check_ = new wxCheckBox(this, wxID_ANY, "Use clipboard for output");
    root->Add(output_clipboard_check_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    root->Add(new wxStaticText(this, wxID_ANY, "Clipboard payload:"), 0,
              wxLEFT | wxRIGHT, 8);
    clipboard_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxTE_MULTILINE | wxTE_RICH2);
    root->Add(clipboard_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    status_label_ = new wxStaticText(this, wxID_ANY, "Ready");
    root->Add(status_label_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* button_row = new wxBoxSizer(wxHORIZONTAL);
    run_button_ = new wxButton(this, wxID_OK, "Run COPY");
    auto* close_button = new wxButton(this, wxID_CANCEL, "Close");
    button_row->Add(run_button_, 0, wxRIGHT, 8);
    button_row->Add(close_button, 0);
    root->Add(button_row, 0, wxALIGN_RIGHT | wxALL, 8);

    SetSizer(root);

    mode_choice_->Bind(wxEVT_CHOICE, &CopyDialog::OnModeChanged, this);
    input_browse->Bind(wxEVT_BUTTON, &CopyDialog::OnBrowseInput, this);
    output_browse->Bind(wxEVT_BUTTON, &CopyDialog::OnBrowseOutput, this);
    input_clipboard_check_->Bind(wxEVT_CHECKBOX, &CopyDialog::OnInputClipboardToggle, this);
    output_clipboard_check_->Bind(wxEVT_CHECKBOX, &CopyDialog::OnOutputClipboardToggle, this);
    run_button_->Bind(wxEVT_BUTTON, &CopyDialog::OnRun, this);

    UpdateControlStates();
}

void CopyDialog::OnBrowseInput(wxCommandEvent&) {
    wxFileDialog dialog(this, "Select COPY input", "", "",
                        "All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_CANCEL) {
        input_path_ctrl_->SetValue(dialog.GetPath());
    }
}

void CopyDialog::OnBrowseOutput(wxCommandEvent&) {
    wxFileDialog dialog(this, "Select COPY output", "", "",
                        "All files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_CANCEL) {
        output_path_ctrl_->SetValue(dialog.GetPath());
    }
}

void CopyDialog::OnModeChanged(wxCommandEvent&) {
    UpdateControlStates();
}

void CopyDialog::OnInputClipboardToggle(wxCommandEvent&) {
    if (input_clipboard_check_ && input_clipboard_check_->GetValue()) {
        if (wxTheClipboard->Open()) {
            if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
                wxTextDataObject data;
                wxTheClipboard->GetData(data);
                if (clipboard_ctrl_) {
                    clipboard_ctrl_->SetValue(data.GetText());
                }
            }
            wxTheClipboard->Close();
        }
    }
    UpdateControlStates();
}

void CopyDialog::OnOutputClipboardToggle(wxCommandEvent&) {
    UpdateControlStates();
}

CopyOptions CopyDialog::BuildOptions() const {
    CopyOptions options;
    int mode = mode_choice_ ? mode_choice_->GetSelection() : 1;
    if (mode == 0) {
        options.mode = CopyMode::In;
    } else if (mode == 2) {
        options.mode = CopyMode::Both;
    } else {
        options.mode = CopyMode::Out;
    }
    options.sql = sql_ctrl_ ? sql_ctrl_->GetValue().ToStdString() : "";
    options.inputPath = input_path_ctrl_ ? input_path_ctrl_->GetValue().ToStdString() : "";
    options.outputPath = output_path_ctrl_ ? output_path_ctrl_->GetValue().ToStdString() : "";
    options.clipboardPayload = clipboard_ctrl_ ? clipboard_ctrl_->GetValue().ToStdString() : "";

    bool input_clip = input_clipboard_check_ && input_clipboard_check_->GetValue();
    bool output_clip = output_clipboard_check_ && output_clipboard_check_->GetValue();
    options.inputSource = input_clip ? CopyDataSource::Clipboard
                                     : (options.inputPath.empty() ? CopyDataSource::None
                                                                  : CopyDataSource::File);
    options.outputSource = output_clip ? CopyDataSource::Clipboard
                                       : (options.outputPath.empty() ? CopyDataSource::None
                                                                     : CopyDataSource::File);
    return options;
}

void CopyDialog::OnRun(wxCommandEvent&) {
    if (!connection_manager_) {
        status_label_->SetLabel("No connection manager available");
        return;
    }
    status_label_->SetLabel("Running COPY...");
    CopyOptions options = BuildOptions();
    if (options.inputSource == CopyDataSource::Clipboard && options.clipboardPayload.empty()) {
        if (wxTheClipboard->Open()) {
            if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
                wxTextDataObject data;
                wxTheClipboard->GetData(data);
                options.clipboardPayload = data.GetText().ToStdString();
                if (clipboard_ctrl_) {
                    clipboard_ctrl_->SetValue(data.GetText());
                }
            }
            wxTheClipboard->Close();
        }
    }
    CopyResult result;
    if (!connection_manager_->ExecuteCopy(options, &result)) {
        status_label_->SetLabel(connection_manager_->LastError());
        return;
    }
    if (!result.outputPayload.empty() && clipboard_ctrl_) {
        clipboard_ctrl_->SetValue(result.outputPayload);
        if (output_clipboard_check_ && output_clipboard_check_->GetValue()) {
            if (wxTheClipboard->Open()) {
                wxTheClipboard->SetData(new wxTextDataObject(result.outputPayload));
                wxTheClipboard->Close();
            }
        }
    }
    std::string status = "COPY complete";
    if (!result.commandTag.empty()) {
        status += " [" + result.commandTag + "]";
    }
    if (result.rowsProcessed > 0) {
        status += " Rows: " + std::to_string(result.rowsProcessed);
    }
    if (result.elapsedMs > 0.0) {
        status += " Time: " + std::to_string(static_cast<int64_t>(result.elapsedMs)) + " ms";
    }
    status_label_->SetLabel(status);
}

void CopyDialog::UpdateControlStates() {
    int mode = mode_choice_ ? mode_choice_->GetSelection() : 1;
    bool needs_input = (mode == 0 || mode == 2);
    bool needs_output = (mode == 1 || mode == 2);
    bool input_clip = input_clipboard_check_ && input_clipboard_check_->GetValue();
    bool output_clip = output_clipboard_check_ && output_clipboard_check_->GetValue();

    if (input_path_ctrl_) {
        input_path_ctrl_->Enable(needs_input && !input_clip);
    }
    if (output_path_ctrl_) {
        output_path_ctrl_->Enable(needs_output && !output_clip);
    }
    if (clipboard_ctrl_) {
        clipboard_ctrl_->Enable(input_clip || output_clip);
    }
}

} // namespace scratchrobin
