/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "migration_dialog.h"

#include "diagram/migration_generator.h"
#include "ui/diagram_model.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

wxBEGIN_EVENT_TABLE(MigrationDialog, wxDialog)
wxEND_EVENT_TABLE()

MigrationDialog::MigrationDialog(wxWindow* parent, const DiagramModel* model)
    : wxDialog(parent, wxID_ANY, "Generate Migration Script",
               wxDefaultPosition, wxSize(800, 600)),
      model_(model),
      generator_(std::make_unique<diagram::MigrationGenerator>()) {
    BuildLayout();
}

MigrationDialog::~MigrationDialog() = default;

void MigrationDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Title
    root->Add(new wxStaticText(this, wxID_ANY, 
        "Generate SQL migration scripts from diagram changes"),
        0, wxALL, 12);
    
    // Options
    auto* optionsSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Dialect selection
    optionsSizer->Add(new wxStaticText(this, wxID_ANY, "Dialect:"), 0,
                      wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    dialect_choice_ = new wxChoice(this, wxID_ANY);
    dialect_choice_->Append("ScratchBird");
    dialect_choice_->Append("PostgreSQL");
    dialect_choice_->Append("MySQL");
    dialect_choice_->Append("Firebird");
    dialect_choice_->SetSelection(0);
    optionsSizer->Add(dialect_choice_, 0, wxRIGHT, 16);
    
    // Direction
    wxArrayString directions;
    directions.Add("Upgrade (apply diagram to database)");
    directions.Add("Downgrade (revert to previous state)");
    direction_radio_ = new wxRadioBox(this, wxID_ANY, "Direction",
                                       wxDefaultPosition, wxDefaultSize, directions, 1);
    direction_radio_->SetSelection(0);
    optionsSizer->Add(direction_radio_, 0);
    
    root->Add(optionsSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Generate button
    auto* generateBtn = new wxButton(this, wxID_ANY, "Generate Migration");
    root->Add(generateBtn, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Notebook for script and warnings
    notebook_ = new wxNotebook(this, wxID_ANY);
    
    // Script tab
    auto* scriptPanel = new wxPanel(notebook_);
    auto* scriptSizer = new wxBoxSizer(wxVERTICAL);
    script_text_ = new wxTextCtrl(scriptPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
    script_text_->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    scriptSizer->Add(script_text_, 1, wxEXPAND);
    
    auto* scriptBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* copyBtn = new wxButton(scriptPanel, wxID_ANY, "Copy to Clipboard");
    auto* saveBtn = new wxButton(scriptPanel, wxID_ANY, "Save to File...");
    scriptBtnSizer->Add(copyBtn, 0, wxRIGHT, 8);
    scriptBtnSizer->Add(saveBtn, 0);
    scriptSizer->Add(scriptBtnSizer, 0, wxTOP, 8);
    
    scriptPanel->SetSizer(scriptSizer);
    notebook_->AddPage(scriptPanel, "Migration Script");
    
    // Warnings tab
    auto* warningsPanel = new wxPanel(notebook_);
    auto* warningsSizer = new wxBoxSizer(wxVERTICAL);
    warnings_list_ = new wxListBox(warningsPanel, wxID_ANY);
    warningsSizer->Add(warnings_list_, 1, wxEXPAND);
    warningsPanel->SetSizer(warningsSizer);
    notebook_->AddPage(warningsPanel, "Warnings");
    
    root->Add(notebook_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Close button
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CLOSE, "Close"), 0);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
    
    // Bind events
    generateBtn->Bind(wxEVT_BUTTON, &MigrationDialog::OnGenerate, this);
    copyBtn->Bind(wxEVT_BUTTON, &MigrationDialog::OnCopy, this);
    saveBtn->Bind(wxEVT_BUTTON, &MigrationDialog::OnSaveScript, this);
}

void MigrationDialog::OnGenerate(wxCommandEvent&) {
    if (!model_) {
        wxMessageBox("No diagram model available", "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    std::string dialect;
    switch (dialect_choice_->GetSelection()) {
        case 0: dialect = "scratchbird"; break;
        case 1: dialect = "postgresql"; break;
        case 2: dialect = "mysql"; break;
        case 3: dialect = "firebird"; break;
        default: dialect = "scratchbird";
    }
    
    // For demo purposes, we'll generate a migration from an empty model
    // In practice, you'd compare with the actual database state
    DiagramModel empty_model(DiagramType::Erd);
    
    bool is_upgrade = (direction_radio_->GetSelection() == 0);
    
    diagram::MigrationGenerator::MigrationScripts scripts;
    if (is_upgrade) {
        scripts = generator_->GenerateFullMigration(empty_model, *model_, dialect);
        script_text_->SetValue(scripts.upgrade_script);
    } else {
        scripts = generator_->GenerateFullMigration(*model_, empty_model, dialect);
        script_text_->SetValue(scripts.downgrade_script);
    }
    
    // Populate warnings
    warnings_list_->Clear();
    for (const auto& warning : scripts.warnings) {
        warnings_list_->Append(warning);
    }
    
    if (!scripts.warnings.empty()) {
        notebook_->SetSelection(1);  // Switch to warnings tab
    }
}

void MigrationDialog::OnSaveScript(wxCommandEvent&) {
    wxString script = script_text_->GetValue();
    if (script.IsEmpty()) {
        wxMessageBox("No script to save. Generate a migration first.", 
                     "Info", wxOK | wxICON_INFORMATION);
        return;
    }
    
    wxFileDialog dialog(this, "Save Migration Script", "", "migration.sql",
                        "SQL files (*.sql)|*.sql|All files (*.*)|*.*",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxFile file(dialog.GetPath(), wxFile::write);
        if (file.IsOpened()) {
            file.Write(script);
            file.Close();
            wxMessageBox("Script saved successfully", "Success", wxOK | wxICON_INFORMATION);
        }
    }
}

void MigrationDialog::OnCopy(wxCommandEvent&) {
    wxString script = script_text_->GetValue();
    if (script.IsEmpty()) {
        return;
    }
    
    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(script));
        wxTheClipboard->Close();
        wxMessageBox("Script copied to clipboard", "Success", wxOK | wxICON_INFORMATION);
    }
}

void MigrationDialog::OnDialectChanged(wxCommandEvent& event) {
    // Regenerate if we have content
    if (!script_text_->GetValue().IsEmpty()) {
        OnGenerate(event);
    }
}

} // namespace scratchrobin
