/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "restore_dialog.h"

#include "core/connection_manager.h"

#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/gauge.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>

namespace scratchrobin {

namespace {

std::string ProfileLabel(const ConnectionProfile& profile) {
    if (!profile.name.empty()) {
        return profile.name;
    }
    if (!profile.database.empty()) {
        return profile.database;
    }
    std::string label = profile.host.empty() ? "localhost" : profile.host;
    if (profile.port != 0) {
        label += ":" + std::to_string(profile.port);
    }
    return label;
}

} // namespace

// RestoreProcessHandler implementation
RestoreProcessHandler::RestoreProcessHandler(wxWindow* parent)
    : wxProcess(parent), parent_(parent) {
    Redirect();
}

void RestoreProcessHandler::OnTerminate(int pid, int status) {
    wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, wxID_ANY);
    event.SetInt(status);
    event.SetExtraLong(1); // Signal termination
    wxPostEvent(parent_, event);
}

// Event table
wxBEGIN_EVENT_TABLE(RestoreDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, RestoreDialog::OnStartRestore)
wxEND_EVENT_TABLE()

RestoreDialog::RestoreDialog(wxWindow* parent,
                             const std::vector<ConnectionProfile>* connections)
    : wxDialog(parent, wxID_ANY, "Restore Database",
               wxDefaultPosition, wxSize(700, 650),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      connections_(connections) {
    BuildLayout();
}

void RestoreDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    // Source section
    BuildSourceSection(root);

    // Target section
    BuildTargetSection(root);

    // Options section
    BuildOptionsSection(root);

    // Progress section (initially hidden)
    BuildProgressSection(root);

    // Button sizer
    BuildButtonSizer(root);

    SetSizer(root);
    CentreOnParent();

    ShowProgressSection(false);
}

void RestoreDialog::BuildSourceSection(wxSizer* parentSizer) {
    auto* sourceBox = new wxStaticBoxSizer(wxVERTICAL, this, "Source");

    // Backup file path
    auto* pathSizer = new wxBoxSizer(wxHORIZONTAL);
    pathSizer->Add(new wxStaticText(this, wxID_ANY, "Backup File:"),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    backup_path_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    pathSizer->Add(backup_path_ctrl_, 1, wxEXPAND | wxRIGHT, 8);

    browse_btn_ = new wxButton(this, wxID_ANY, "Browse...");
    browse_btn_->Bind(wxEVT_BUTTON, &RestoreDialog::OnBrowse, this);
    pathSizer->Add(browse_btn_, 0);

    sourceBox->Add(pathSizer, 0, wxEXPAND | wxALL, 8);

    // Verify backup checkbox
    verify_backup_checkbox_ = new wxCheckBox(this, wxID_ANY, "Verify backup file before restore");
    verify_backup_checkbox_->SetValue(true);
    verify_backup_checkbox_->SetToolTip("Check the integrity of the backup file before starting restore");
    verify_backup_checkbox_->Bind(wxEVT_CHECKBOX, &RestoreDialog::OnVerifyBackup, this);
    sourceBox->Add(verify_backup_checkbox_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    parentSizer->Add(sourceBox, 0, wxEXPAND | wxALL, 12);
}

void RestoreDialog::BuildTargetSection(wxSizer* parentSizer) {
    auto* targetBox = new wxStaticBoxSizer(wxVERTICAL, this, "Target");

    // Connection profile dropdown
    auto* connSizer = new wxBoxSizer(wxHORIZONTAL);
    connSizer->Add(new wxStaticText(this, wxID_ANY, "Connection Profile:"),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    connection_choice_ = new wxChoice(this, wxID_ANY);
    if (connections_) {
        for (const auto& profile : *connections_) {
            connection_choice_->Append(ProfileLabel(profile));
        }
        if (!connections_->empty()) {
            connection_choice_->SetSelection(0);
        }
    }
    connSizer->Add(connection_choice_, 1, wxEXPAND);
    targetBox->Add(connSizer, 0, wxEXPAND | wxALL, 8);

    // Target database
    auto* dbSizer = new wxBoxSizer(wxHORIZONTAL);
    dbSizer->Add(new wxStaticText(this, wxID_ANY, "Target Database:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    target_database_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    target_database_ctrl_->SetToolTip("Enter the target database name for restore");
    dbSizer->Add(target_database_ctrl_, 1, wxEXPAND);

    targetBox->Add(dbSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Create database checkbox
    create_database_checkbox_ = new wxCheckBox(this, wxID_ANY, "Create database if it does not exist");
    create_database_checkbox_->SetValue(true);
    targetBox->Add(create_database_checkbox_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    parentSizer->Add(targetBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
}

void RestoreDialog::BuildOptionsSection(wxSizer* parentSizer) {
    auto* optionsBox = new wxStaticBoxSizer(wxVERTICAL, this, "Restore Options");

    // Restore mode radio buttons
    wxArrayString modes;
    modes.Add("Full Restore (Schema + Data)");
    modes.Add("Schema Only");
    modes.Add("Data Only");

    restore_mode_radio_ = new wxRadioBox(this, wxID_ANY, "Restore Mode",
                                          wxDefaultPosition, wxDefaultSize,
                                          modes, 1, wxRA_SPECIFY_COLS);
    restore_mode_radio_->SetSelection(0);
    optionsBox->Add(restore_mode_radio_, 0, wxEXPAND | wxALL, 8);

    // Clean before restore checkbox
    clean_restore_checkbox_ = new wxCheckBox(this, wxID_ANY,
        "Clean (drop) existing objects before restore");
    clean_restore_checkbox_->SetValue(false);
    clean_restore_checkbox_->SetToolTip(
        "WARNING: This will drop existing objects before recreating them");
    optionsBox->Add(clean_restore_checkbox_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Disable triggers checkbox
    disable_triggers_checkbox_ = new wxCheckBox(this, wxID_ANY,
        "Disable triggers during restore");
    disable_triggers_checkbox_->SetValue(true);
    disable_triggers_checkbox_->SetToolTip(
        "Temporarily disable triggers to improve restore performance and avoid trigger errors");
    optionsBox->Add(disable_triggers_checkbox_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    parentSizer->Add(optionsBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
}

void RestoreDialog::BuildProgressSection(wxSizer* parentSizer) {
    progress_panel_ = new wxPanel(this, wxID_ANY);
    auto* progressSizer = new wxBoxSizer(wxVERTICAL);

    // Phase and status text
    phase_text_ = new wxStaticText(progress_panel_, wxID_ANY, "Ready to restore");
    phase_text_->SetFont(wxFont(wxFontInfo().Bold()));
    progressSizer->Add(phase_text_, 0, wxBOTTOM, 4);

    status_text_ = new wxStaticText(progress_panel_, wxID_ANY, "Waiting to start...");
    progressSizer->Add(status_text_, 0, wxBOTTOM, 8);

    // Progress gauge
    auto* gaugeSizer = new wxBoxSizer(wxHORIZONTAL);
    progress_gauge_ = new wxGauge(progress_panel_, wxID_ANY, 100,
                                   wxDefaultPosition, wxSize(-1, 20));
    gaugeSizer->Add(progress_gauge_, 1, wxEXPAND | wxRIGHT, 8);

    cancel_btn_ = new wxButton(progress_panel_, wxID_ANY, "Cancel");
    cancel_btn_->Bind(wxEVT_BUTTON, &RestoreDialog::OnCancel, this);
    gaugeSizer->Add(cancel_btn_, 0);

    progressSizer->Add(gaugeSizer, 0, wxEXPAND | wxBOTTOM, 8);

    // Log list
    progressSizer->Add(new wxStaticText(progress_panel_, wxID_ANY, "Progress Log:"),
                       0, wxBOTTOM, 4);
    log_list_ = new wxListBox(progress_panel_, wxID_ANY, wxDefaultPosition,
                               wxSize(-1, 120));
    progressSizer->Add(log_list_, 1, wxEXPAND);

    progress_panel_->SetSizer(progressSizer);
    parentSizer->Add(progress_panel_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
}

void RestoreDialog::BuildButtonSizer(wxSizer* parentSizer) {
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    start_restore_btn_ = new wxButton(this, wxID_ANY, "Start Restore");
    start_restore_btn_->SetDefault();
    start_restore_btn_->Bind(wxEVT_BUTTON, &RestoreDialog::OnStartRestore, this);
    btnSizer->Add(start_restore_btn_, 0, wxRIGHT, 8);

    btnSizer->AddStretchSpacer(1);

    close_btn_ = new wxButton(this, wxID_CLOSE, "Close");
    close_btn_->Bind(wxEVT_BUTTON, &RestoreDialog::OnClose, this);
    btnSizer->Add(close_btn_, 0, wxRIGHT, 8);

    help_btn_ = new wxButton(this, wxID_HELP, "Help");
    help_btn_->Bind(wxEVT_BUTTON, &RestoreDialog::OnHelp, this);
    btnSizer->Add(help_btn_, 0);

    parentSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
}

void RestoreDialog::OnBrowse(wxCommandEvent& /*event*/) {
    wxString defaultPath = backup_path_ctrl_->GetValue();
    if (defaultPath.IsEmpty()) {
        defaultPath = wxFileName::GetHomeDir();
    }

    wxFileDialog dialog(this, "Select Backup File", defaultPath, "",
                        "Backup files (*.sbbak;*.sql;*.custom)|*.sbbak;*.sql;*.custom|"
                        "All files (*.*)|*.*",
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dialog.ShowModal() == wxID_OK) {
        backup_path_ctrl_->SetValue(dialog.GetPath());
    }
}

void RestoreDialog::OnStartRestore(wxCommandEvent& event) {
    // Check if this is a process termination event
    if (event.GetExtraLong() == 1) {
        int exitCode = event.GetInt();
        OnProcessTerminated(exitCode);
        return;
    }

    if (is_running_) {
        return;
    }

    if (!ValidateInputs()) {
        return;
    }

    // Show warning if clean restore is selected
    if (clean_restore_checkbox_ && clean_restore_checkbox_->IsChecked()) {
        wxMessageDialog confirm(this,
            "WARNING: Clean restore is enabled. This will DROP existing objects before restoring.\n\n"
            "Are you sure you want to continue?",
            "Confirm Clean Restore", wxYES_NO | wxICON_WARNING);
        if (confirm.ShowModal() == wxID_NO) {
            return;
        }
    }

    // Show progress section and start restore
    ShowProgressSection(true);
    EnableControls(false);
    StartRestoreProcess();
}

void RestoreDialog::OnCancel(wxCommandEvent& /*event*/) {
    if (is_running_) {
        StopRestoreProcess();
        result_.cancelled = true;
        AddLogMessage("Restore cancelled by user", true);
    }
}

void RestoreDialog::OnClose(wxCommandEvent& /*event*/) {
    if (is_running_) {
        wxMessageDialog confirm(this,
            "A restore is currently in progress. Are you sure you want to cancel and close?",
            "Confirm Close", wxYES_NO | wxICON_QUESTION);
        if (confirm.ShowModal() == wxID_NO) {
            return;
        }
        StopRestoreProcess();
    }
    EndModal(wxID_CLOSE);
}

void RestoreDialog::OnHelp(wxCommandEvent& /*event*/) {
    wxMessageBox(
        "Restore Database Help:\n\n"
        "Source: Select the backup file to restore from.\n\n"
        "Verify Backup: Check the integrity of the backup file before restore.\n\n"
        "Target: Select the connection profile and specify the target database name.\n\n"
        "Restore Options:\n"
        "  - Full Restore: Restores both schema and data\n"
        "  - Schema Only: Restores only database structure\n"
        "  - Data Only: Restores only data (schema must exist)\n"
        "  - Clean Restore: Drops existing objects before restoring\n"
        "  - Disable Triggers: Improves performance during restore\n\n"
        "Note: The restore process runs in the background. You can cancel it at any time.",
        "Restore Help", wxOK | wxICON_INFORMATION);
}

void RestoreDialog::OnVerifyBackup(wxCommandEvent& /*event*/) {
    // Could add logic to enable/disable verify-specific options here
}

void RestoreDialog::StartRestoreProcess() {
    log_list_->Clear();
    progress_gauge_->SetValue(0);
    AddLogMessage("Starting restore process...");

    // First verify backup if requested
    if (verify_backup_checkbox_ && verify_backup_checkbox_->IsChecked()) {
        AddLogMessage("Verifying backup file...");
        // In a real implementation, run sb_backup --verify first
    }

    std::string cmd = BuildRestoreCommand();
    if (cmd.empty()) {
        AddLogMessage("Failed to build restore command", true);
        EnableControls(true);
        return;
    }

    process_handler_ = std::make_unique<RestoreProcessHandler>(this);
    process_pid_ = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, process_handler_.get());

    if (process_pid_ == 0) {
        AddLogMessage("Failed to start restore process", true);
        process_handler_.reset();
        EnableControls(true);
        return;
    }

    is_running_ = true;
    UpdateProgress("Initializing", 0);
}

void RestoreDialog::StopRestoreProcess() {
    if (is_running_ && process_pid_ != 0) {
        wxKill(process_pid_, wxSIGTERM);
        process_pid_ = 0;
    }
    is_running_ = false;
}

void RestoreDialog::ReadProcessOutput() {
    if (!process_handler_) return;

    wxInputStream* stdoutStream = process_handler_->GetInputStream();
    wxInputStream* stderrStream = process_handler_->GetErrorStream();

    if (stdoutStream && stdoutStream->CanRead()) {
        wxString output;
        while (stdoutStream->CanRead()) {
            char buffer[1024];
            stdoutStream->Read(buffer, sizeof(buffer) - 1);
            size_t count = stdoutStream->LastRead();
            if (count > 0) {
                buffer[count] = '\0';
                output += wxString::FromUTF8(buffer, count);
            }
        }

        // Parse progress output
        // Expected format: "PHASE:<phase_name>:<percent>:<status_message>"
        if (!output.IsEmpty()) {
            // Parse phase and progress from output
            wxString phase = "Restoring";
            int percent = 0;

            // Simple parsing - in real implementation would be more robust
            if (output.Contains("PHASE:")) {
                wxArrayString parts = wxSplit(output, ':');
                if (parts.GetCount() >= 2) {
                    phase = parts[1];
                }
                if (parts.GetCount() >= 3) {
                    long val;
                    if (parts[2].ToLong(&val)) {
                        percent = static_cast<int>(val);
                    }
                }
            }

            UpdateProgress(phase.ToStdString(), percent);
            AddLogMessage(output.ToStdString());
        }
    }

    if (stderrStream && stderrStream->CanRead()) {
        wxString errors;
        while (stderrStream->CanRead()) {
            char buffer[1024];
            stderrStream->Read(buffer, sizeof(buffer) - 1);
            size_t count = stderrStream->LastRead();
            if (count > 0) {
                buffer[count] = '\0';
                errors += wxString::FromUTF8(buffer, count);
            }
        }
        if (!errors.IsEmpty()) {
            AddLogMessage(errors.ToStdString(), true);
        }
    }
}

void RestoreDialog::OnProcessTerminated(int exitCode) {
    is_running_ = false;
    process_pid_ = 0;

    if (exitCode == 0) {
        result_.success = true;
        result_.backupFile = backup_path_ctrl_->GetValue().ToStdString();
        result_.targetDatabase = target_database_ctrl_->GetValue().ToStdString();
        UpdateProgress("Completed", 100);
        status_text_->SetLabel("Restore completed successfully");
        AddLogMessage("Restore completed successfully");
        wxMessageBox("Restore completed successfully!", "Success", wxOK | wxICON_INFORMATION);
    } else {
        result_.success = false;
        result_.errorMessage = "Restore process exited with code " + std::to_string(exitCode);
        UpdateProgress("Failed", 0);
        status_text_->SetLabel("Restore failed");
        AddLogMessage(result_.errorMessage, true);
    }

    EnableControls(true);
}

void RestoreDialog::UpdateProgress(const std::string& phase, int percent) {
    CallAfter([this, phase, percent]() {
        phase_text_->SetLabel(phase);
        progress_gauge_->SetValue(percent);
        status_text_->SetLabel(wxString::Format("Progress: %d%%", percent));
    });
}

void RestoreDialog::AddLogMessage(const std::string& message, bool isError) {
    CallAfter([this, message, isError]() {
        wxString prefix = isError ? wxString::FromUTF8("[ERROR] ") : wxString::FromUTF8("[INFO] ");
        log_list_->Append(prefix + wxString::FromUTF8(message));
        // Scroll to bottom
        if (log_list_->GetCount() > 0) {
            log_list_->SetSelection(log_list_->GetCount() - 1);
        }
    });
}

bool RestoreDialog::ValidateInputs() {
    wxString backupPath = backup_path_ctrl_->GetValue();
    if (backupPath.IsEmpty()) {
        wxMessageBox("Please select a backup file.", "Validation Error", wxOK | wxICON_ERROR);
        return false;
    }

    if (!wxFileName::Exists(backupPath)) {
        wxMessageBox("The specified backup file does not exist.", "Validation Error",
                     wxOK | wxICON_ERROR);
        return false;
    }

    if (!connection_choice_ || connection_choice_->GetSelection() < 0) {
        wxMessageBox("Please select a connection profile.", "Validation Error",
                     wxOK | wxICON_ERROR);
        return false;
    }

    wxString targetDb = target_database_ctrl_->GetValue();
    if (targetDb.IsEmpty()) {
        wxMessageBox("Please specify a target database name.", "Validation Error",
                     wxOK | wxICON_ERROR);
        return false;
    }

    return true;
}

std::string RestoreDialog::BuildRestoreCommand() {
    // Build command for sb_restore CLI tool
    // Format: sb_restore --input <file> --host <host> --database <db> [options]

    int sel = connection_choice_->GetSelection();
    if (sel < 0 || !connections_ || sel >= static_cast<int>(connections_->size())) {
        return "";
    }

    const auto& profile = (*connections_)[sel];
    wxString cmd = "sb_restore";

    // Input file
    cmd += " --input " + backup_path_ctrl_->GetValue();

    // Connection parameters
    if (!profile.host.empty()) {
        cmd += " --host " + wxString::FromUTF8(profile.host);
    }
    if (profile.port != 0) {
        cmd += " --port " + wxString::Format("%d", profile.port);
    }

    // Target database
    cmd += " --database " + target_database_ctrl_->GetValue();

    // Create database if not exists
    if (create_database_checkbox_ && create_database_checkbox_->IsChecked()) {
        cmd += " --create-database";
    }

    // Restore mode
    if (restore_mode_radio_) {
        int mode = restore_mode_radio_->GetSelection();
        if (mode == 1) {
            cmd += " --schema-only";
        } else if (mode == 2) {
            cmd += " --data-only";
        }
    }

    // Clean restore
    if (clean_restore_checkbox_ && clean_restore_checkbox_->IsChecked()) {
        cmd += " --clean";
    }

    // Disable triggers
    if (disable_triggers_checkbox_ && disable_triggers_checkbox_->IsChecked()) {
        cmd += " --disable-triggers";
    }

    // Verify backup (separate step in real implementation)
    if (verify_backup_checkbox_ && verify_backup_checkbox_->IsChecked()) {
        cmd += " --verify";
    }

    return cmd.ToStdString();
}

void RestoreDialog::EnableControls(bool enable) {
    if (backup_path_ctrl_) backup_path_ctrl_->Enable(enable);
    if (browse_btn_) browse_btn_->Enable(enable);
    if (verify_backup_checkbox_) verify_backup_checkbox_->Enable(enable);
    if (connection_choice_) connection_choice_->Enable(enable);
    if (target_database_ctrl_) target_database_ctrl_->Enable(enable);
    if (create_database_checkbox_) create_database_checkbox_->Enable(enable);
    if (restore_mode_radio_) restore_mode_radio_->Enable(enable);
    if (clean_restore_checkbox_) clean_restore_checkbox_->Enable(enable);
    if (disable_triggers_checkbox_) disable_triggers_checkbox_->Enable(enable);

    if (start_restore_btn_) start_restore_btn_->Enable(enable);
    if (close_btn_) close_btn_->Enable(enable);

    // Cancel button is enabled during restore
    if (cancel_btn_) cancel_btn_->Enable(!enable);
}

void RestoreDialog::ShowProgressSection(bool show) {
    if (progress_panel_) {
        progress_panel_->Show(show);
        Layout();
    }
}

} // namespace scratchrobin
