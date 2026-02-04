/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "export_options_dialog.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

wxBEGIN_EVENT_TABLE(ExportOptionsDialog, wxDialog)
    EVT_CHOICE(wxID_ANY, ExportOptionsDialog::OnFormatChanged)
    EVT_BUTTON(wxID_OK, ExportOptionsDialog::OnOk)
wxEND_EVENT_TABLE()

ExportOptionsDialog::ExportOptionsDialog(wxWindow* parent, ExportOptions& options)
    : wxDialog(parent, wxID_ANY, "Export Diagram", wxDefaultPosition, wxSize(500, 400)),
      options_(options) {
    BuildLayout();
}

void ExportOptionsDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Format selection
    auto* formatSizer = new wxBoxSizer(wxHORIZONTAL);
    formatSizer->Add(new wxStaticText(this, wxID_ANY, "Format:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    format_choice_ = new wxChoice(this, wxID_ANY);
    format_choice_->Append("PNG Image");
    format_choice_->Append("SVG Vector");
    format_choice_->Append("PDF Document");
    format_choice_->SetSelection(static_cast<int>(options_.format));
    formatSizer->Add(format_choice_, 1, wxEXPAND);
    root->Add(formatSizer, 0, wxEXPAND | wxALL, 12);
    
    // Export scope
    wxArrayString scopeChoices;
    scopeChoices.Add("All entities");
    scopeChoices.Add("Visible area only");
    scopeChoices.Add("Selected entities only");
    scope_radio_ = new wxRadioBox(this, wxID_ANY, "Export Scope", wxDefaultPosition, wxDefaultSize, scopeChoices, 1);
    scope_radio_->SetSelection(static_cast<int>(options_.scope));
    root->Add(scope_radio_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // DPI/Resolution
    auto* dpiSizer = new wxBoxSizer(wxHORIZONTAL);
    dpiSizer->Add(new wxStaticText(this, wxID_ANY, "DPI/Resolution:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    dpi_spin_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1));
    dpi_spin_->SetRange(72, 600);
    dpi_spin_->SetValue(options_.dpi);
    dpiSizer->Add(dpi_spin_, 0);
    dpiSizer->Add(new wxStaticText(this, wxID_ANY, " (72-600)"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    dpiSizer->AddStretchSpacer(1);
    root->Add(dpiSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Quality (for PNG)
    auto* qualitySizer = new wxBoxSizer(wxHORIZONTAL);
    qualitySizer->Add(new wxStaticText(this, wxID_ANY, "Quality:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    quality_spin_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1));
    quality_spin_->SetRange(1, 100);
    quality_spin_->SetValue(options_.quality);
    qualitySizer->Add(quality_spin_, 0);
    qualitySizer->Add(new wxStaticText(this, wxID_ANY, "% (PNG only)"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    qualitySizer->AddStretchSpacer(1);
    root->Add(qualitySizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Filename
    auto* fileSizer = new wxBoxSizer(wxHORIZONTAL);
    fileSizer->Add(new wxStaticText(this, wxID_ANY, "Filename:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    filename_ctrl_ = new wxTextCtrl(this, wxID_ANY, options_.filename);
    fileSizer->Add(filename_ctrl_, 1, wxEXPAND);
    auto* browseBtn = new wxButton(this, wxID_ANY, "Browse...");
    fileSizer->Add(browseBtn, 0, wxLEFT, 6);
    root->Add(fileSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    browseBtn->Bind(wxEVT_BUTTON, &ExportOptionsDialog::OnBrowse, this);
    
    // Buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, wxID_OK, "Export"), 0);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
}

void ExportOptionsDialog::OnFormatChanged(wxCommandEvent&) {
    UpdateFilenameExtension();
}

void ExportOptionsDialog::UpdateFilenameExtension() {
    std::string filename = filename_ctrl_->GetValue().ToStdString();
    if (filename.empty()) return;
    
    // Remove current extension
    size_t dot_pos = filename.rfind('.');
    if (dot_pos != std::string::npos) {
        filename = filename.substr(0, dot_pos);
    }
    
    // Add new extension based on format
    int format = format_choice_->GetSelection();
    switch (format) {
        case 0: filename += ".png"; break;
        case 1: filename += ".svg"; break;
        case 2: filename += ".pdf"; break;
    }
    
    filename_ctrl_->SetValue(filename);
}

void ExportOptionsDialog::OnBrowse(wxCommandEvent&) {
    std::string current = filename_ctrl_->GetValue().ToStdString();
    int format = format_choice_->GetSelection();
    
    wxString wildcard;
    wxString defaultExtension;
    switch (format) {
        case 0:
            wildcard = "PNG files (*.png)|*.png";
            defaultExtension = "png";
            break;
        case 1:
            wildcard = "SVG files (*.svg)|*.svg";
            defaultExtension = "svg";
            break;
        case 2:
            wildcard = "PDF files (*.pdf)|*.pdf";
            defaultExtension = "pdf";
            break;
        default:
            wildcard = "All files (*.*)|*.*";
            defaultExtension = "";
    }
    
    wxFileDialog dialog(this, "Export Diagram", "", current, wildcard,
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() == wxID_OK) {
        filename_ctrl_->SetValue(dialog.GetPath());
    }
}

void ExportOptionsDialog::OnOk(wxCommandEvent& event) {
    // Validate
    std::string filename = filename_ctrl_->GetValue().ToStdString();
    if (filename.empty()) {
        wxMessageBox("Please specify a filename.", "Validation Error", wxOK | wxICON_ERROR, this);
        return;
    }
    
    // Save options
    options_.format = static_cast<ExportFormat>(format_choice_->GetSelection());
    options_.scope = static_cast<ExportScope>(scope_radio_->GetSelection());
    options_.dpi = dpi_spin_->GetValue();
    options_.quality = quality_spin_->GetValue();
    options_.filename = filename;
    
    confirmed_ = true;
    event.Skip();
}

} // namespace scratchrobin
