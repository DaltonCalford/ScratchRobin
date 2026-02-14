/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "backup_dialog.h"

#include "core/connection_manager.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/gauge.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/slider.h>
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

wxString GetBackupWildcard(int formatIndex) {
    switch (formatIndex) {
        case 0: return "ScratchBird Backup files (*.sbbak)|*.sbbak";
        case 1: return "SQL files (*.sql)|*.sql";
        case 2: return "Custom format files (*.custom)|*.custom|All files (*.*)|*.*";
        default: return "All files (*.*)|*.*";
    }
}

} // namespace

// BackupProcessHandler implementation
BackupProcessHandler::BackupProcessHandler(wxWindow* parent)
    : wxProcess(parent), parent_(parent) {
    Redirect();
}

void BackupProcessHandler::OnTerminate(int pid, int status) {
    wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, wxID_ANY);
    event.SetInt(status);
    event.SetExtraLong(1); // Signal termination
    wxPostEvent(parent_, event);
}

// Event table
wxBEGIN_EVENT_TABLE(BackupDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, BackupDialog::OnStartBackup)
wxEND_EVENT_TABLE()

BackupDialog::BackupDialog(wxWindow* parent,
                           const std::vector<ConnectionProfile>* connections,
                           const std::string& currentDatabase)
    : wxDialog(parent, wxID_ANY, "Backup Database",
               wxDefaultPosition, wxSize(700, 650),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      connections_(connections),
      current_database_(currentDatabase) {
    BuildLayout();
}

void BackupDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    // Source section
    BuildSourceSection(root);

    // Destination section
    BuildDestinationSection(root);

    // Options section
    BuildOptionsSection(root);

    // Progress section (initially hidden)
    BuildProgressSection(root);

    // Button sizer
    BuildButtonSizer(root);

    SetSizer(root);
    CentreOnParent();

    // Initialize database label
    if (connection_choice_ && connection_choice_->GetCount() > 0) {
        wxCommandEvent dummy;
        OnConnectionChanged(dummy);
    }

    ShowProgressSection(false);
}

void BackupDialog::BuildSourceSection(wxSizer* parentSizer) {
    auto* sourceBox = new wxStaticBoxSizer(wxVERTICAL, this, "Source");

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
    connection_choice_->Bind(wxEVT_CHOICE, &BackupDialog::OnConnectionChanged, this);
    connSizer->Add(connection_choice_, 1, wxEXPAND);
    sourceBox->Add(connSizer, 0, wxEXPAND | wxALL, 8);

    // Database name display
    auto* dbSizer = new wxBoxSizer(wxHORIZONTAL);
    dbSizer->Add(new wxStaticText(this, wxID_ANY, "Database:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    database_label_ = new wxStaticText(this, wxID_ANY, current_database_);
    database_label_->SetFont(wxFont(wxFontInfo().Bold()));
    dbSizer->Add(database_label_, 1, wxEXPAND);
    sourceBox->Add(dbSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    parentSizer->Add(sourceBox, 0, wxEXPAND | wxALL, 12);
}

void BackupDialog::BuildDestinationSection(wxSizer* parentSizer) {
    auto* destBox = new wxStaticBoxSizer(wxVERTICAL, this, "Destination");

    // Backup file path
    auto* pathSizer = new wxBoxSizer(wxHORIZONTAL);
    pathSizer->Add(new wxStaticText(this, wxID_ANY, "Backup File:"),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    backup_path_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    pathSizer->Add(backup_path_ctrl_, 1, wxEXPAND | wxRIGHT, 8);

    browse_btn_ = new wxButton(this, wxID_ANY, "Browse...");
    browse_btn_->Bind(wxEVT_BUTTON, &BackupDialog::OnBrowse, this);
    pathSizer->Add(browse_btn_, 0);

    destBox->Add(pathSizer, 0, wxEXPAND | wxALL, 8);

    // Format dropdown
    auto* formatSizer = new wxBoxSizer(wxHORIZONTAL);
    formatSizer->Add(new wxStaticText(this, wxID_ANY, "Format:"),
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    format_choice_ = new wxChoice(this, wxID_ANY);
    format_choice_->Append("ScratchBird Backup (.sbbak)");
    format_choice_->Append("SQL Script (.sql)");
    format_choice_->Append("Custom Format (.custom)");
    format_choice_->SetSelection(0);
    formatSizer->Add(format_choice_, 1, wxEXPAND);

    destBox->Add(formatSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    parentSizer->Add(destBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
}

void BackupDialog::BuildOptionsSection(wxSizer* parentSizer) {
    auto* optionsBox = new wxStaticBoxSizer(wxVERTICAL, this, "Options");

    // Compression level slider
    auto* compressionSizer = new wxBoxSizer(wxHORIZONTAL);
    compressionSizer->Add(new wxStaticText(this, wxID_ANY, "Compression Level:"),
                          0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    compression_slider_ = new wxSlider(this, wxID_ANY, 6, 0, 9,
                                       wxDefaultPosition, wxSize(150, -1));
    compressionSizer->Add(compression_slider_, 0, wxRIGHT, 8);

    compression_value_label_ = new wxStaticText(this, wxID_ANY, "6");
    compressionSizer->Add(compression_value_label_, 0, wxALIGN_CENTER_VERTICAL);

    compression_slider_->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
        compression_value_label_->SetLabel(
            wxString::Format("%d", compression_slider_->GetValue()));
    });

    optionsBox->Add(compressionSizer, 0, wxEXPAND | wxALL, 8);

    // Schema filter
    auto* schemaLabel = new wxStaticText(this, wxID_ANY,
        "Schemas to Include/Exclude (one per line, prefix with - to exclude):");
    optionsBox->Add(schemaLabel, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    schema_list_ = new wxListBox(this, wxID_ANY, wxDefaultPosition,
                                  wxSize(-1, 80), 0, nullptr,
                                  wxLB_MULTIPLE | wxLB_HSCROLL);
    // Add some example schemas (would be populated from database in real implementation)
    schema_list_->Append("public");
    schema_list_->SetSelection(0);
    optionsBox->Add(schema_list_, 0, wxEXPAND | wxALL, 8);

    // Include data checkbox
    include_data_checkbox_ = new wxCheckBox(this, wxID_ANY, "Include Data (uncheck for schema only)");
    include_data_checkbox_->SetValue(true);
    optionsBox->Add(include_data_checkbox_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Consistent snapshot checkbox
    consistent_snapshot_checkbox_ = new wxCheckBox(this, wxID_ANY, "Use Consistent Snapshot");
    consistent_snapshot_checkbox_->SetValue(true);
    consistent_snapshot_checkbox_->SetToolTip(
        "Create a consistent snapshot of the database during backup");
    optionsBox->Add(consistent_snapshot_checkbox_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    parentSizer->Add(optionsBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
}

void BackupDialog::BuildProgressSection(wxSizer* parentSizer) {
    progress_panel_ = new wxPanel(this, wxID_ANY);
    auto* progressSizer = new wxBoxSizer(wxVERTICAL);

    // Status text
    status_text_ = new wxStaticText(progress_panel_, wxID_ANY, "Ready to backup");
    progressSizer->Add(status_text_, 0, wxBOTTOM, 8);

    // Progress gauge
    auto* gaugeSizer = new wxBoxSizer(wxHORIZONTAL);
    progress_gauge_ = new wxGauge(progress_panel_, wxID_ANY, 100,
                                   wxDefaultPosition, wxSize(-1, 20));
    gaugeSizer->Add(progress_gauge_, 1, wxEXPAND | wxRIGHT, 8);

    cancel_btn_ = new wxButton(progress_panel_, wxID_ANY, "Cancel");
    cancel_btn_->Bind(wxEVT_BUTTON, &BackupDialog::OnCancel, this);
    gaugeSizer->Add(cancel_btn_, 0);

    progressSizer->Add(gaugeSizer, 0, wxEXPAND | wxBOTTOM, 8);

    // Log list
    progressSizer->Add(new wxStaticText(progress_panel_, wxID_ANY, "Progress Log:"),
                       0, wxBOTTOM, 4);
    log_list_ = new wxListBox(progress_panel_, wxID_ANY, wxDefaultPosition,
                               wxSize(-1, 100));
    progressSizer->Add(log_list_, 1, wxEXPAND);

    progress_panel_->SetSizer(progressSizer);
    parentSizer->Add(progress_panel_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
}

void BackupDialog::BuildButtonSizer(wxSizer* parentSizer) {
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    start_backup_btn_ = new wxButton(this, wxID_ANY, "Start Backup");
    start_backup_btn_->SetDefault();
    start_backup_btn_->Bind(wxEVT_BUTTON, &BackupDialog::OnStartBackup, this);
    btnSizer->Add(start_backup_btn_, 0, wxRIGHT, 8);

    btnSizer->AddStretchSpacer(1);

    close_btn_ = new wxButton(this, wxID_CLOSE, "Close");
    close_btn_->Bind(wxEVT_BUTTON, &BackupDialog::OnClose, this);
    btnSizer->Add(close_btn_, 0, wxRIGHT, 8);

    help_btn_ = new wxButton(this, wxID_HELP, "Help");
    help_btn_->Bind(wxEVT_BUTTON, &BackupDialog::OnHelp, this);
    btnSizer->Add(help_btn_, 0);

    parentSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
}

void BackupDialog::OnConnectionChanged(wxCommandEvent& /*event*/) {
    if (!connection_choice_ || !database_label_) return;

    int sel = connection_choice_->GetSelection();
    if (sel >= 0 && connections_ && sel < static_cast<int>(connections_->size())) {
        const auto& profile = (*connections_)[sel];
        database_label_->SetLabel(profile.database.empty() ? "(not connected)" : profile.database);
    }
}

void BackupDialog::OnBrowse(wxCommandEvent& /*event*/) {
    wxString defaultPath = backup_path_ctrl_->GetValue();
    if (defaultPath.IsEmpty()) {
        defaultPath = wxFileName::GetHomeDir();
    }

    wxString wildcard = GetBackupWildcard(format_choice_->GetSelection());

    wxFileDialog dialog(this, "Save Backup File", defaultPath, "",
                        wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() == wxID_OK) {
        backup_path_ctrl_->SetValue(dialog.GetPath());
    }
}

void BackupDialog::OnStartBackup(wxCommandEvent& event) {
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

    // Show progress section and start backup
    ShowProgressSection(true);
    EnableControls(false);
    StartBackupProcess();
}

void BackupDialog::OnCancel(wxCommandEvent& /*event*/) {
    if (is_running_) {
        StopBackupProcess();
        result_.cancelled = true;
        AddLogMessage("Backup cancelled by user", true);
    }
}

void BackupDialog::OnClose(wxCommandEvent& /*event*/) {
    if (is_running_) {
        wxMessageDialog confirm(this,
            "A backup is currently in progress. Are you sure you want to cancel and close?",
            "Confirm Close", wxYES_NO | wxICON_QUESTION);
        if (confirm.ShowModal() == wxID_NO) {
            return;
        }
        StopBackupProcess();
    }
    EndModal(wxID_CLOSE);
}

void BackupDialog::OnHelp(wxCommandEvent& /*event*/) {
    wxMessageBox(
        "Backup Database Help:\n\n"
        "Source: Select the connection profile and database to backup.\n\n"
        "Destination: Choose where to save the backup file and select the format.\n\n"
        "Options:\n"
        "  - Compression Level: 0 (none) to 9 (maximum compression)\n"
        "  - Include Data: Uncheck to backup only the database schema\n"
        "  - Consistent Snapshot: Ensures a point-in-time consistent backup\n\n"
        "Note: The backup process runs in the background. You can cancel it at any time.",
        "Backup Help", wxOK | wxICON_INFORMATION);
}

void BackupDialog::StartBackupProcess() {
    log_list_->Clear();
    progress_gauge_->SetValue(0);
    AddLogMessage("Starting backup process...");

    std::string cmd = BuildBackupCommand();
    if (cmd.empty()) {
        AddLogMessage("Failed to build backup command", true);
        EnableControls(true);
        return;
    }

    process_handler_ = std::make_unique<BackupProcessHandler>(this);
    process_pid_ = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, process_handler_.get());

    if (process_pid_ == 0) {
        AddLogMessage("Failed to start backup process", true);
        process_handler_.reset();
        EnableControls(true);
        return;
    }

    is_running_ = true;
    UpdateProgress("Backup in progress...", 0);

    // Start a timer to read process output
    // In a real implementation, you might use a wxTimer
    // For this implementation, we'll poll in the idle handler or use events
}

void BackupDialog::StopBackupProcess() {
    if (is_running_ && process_pid_ != 0) {
        wxKill(process_pid_, wxSIGTERM);
        process_pid_ = 0;
    }
    is_running_ = false;
}

void BackupDialog::ReadProcessOutput() {
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

        // Parse progress output (format would depend on sb_backup output)
        // For example: "PROGRESS:50:Backing up tables..."
        if (!output.IsEmpty()) {
            AddLogMessage(output.ToStdString());
            // Parse progress percentage if available
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

void BackupDialog::OnProcessTerminated(int exitCode) {
    is_running_ = false;
    process_pid_ = 0;

    if (exitCode == 0) {
        result_.success = true;
        result_.backupFile = backup_path_ctrl_->GetValue().ToStdString();
        UpdateProgress("Backup completed successfully", 100);
        AddLogMessage("Backup completed successfully");
        wxMessageBox("Backup completed successfully!", "Success", wxOK | wxICON_INFORMATION);
    } else {
        result_.success = false;
        result_.errorMessage = "Backup process exited with code " + std::to_string(exitCode);
        UpdateProgress("Backup failed", 0);
        AddLogMessage(result_.errorMessage, true);
    }

    EnableControls(true);
}

void BackupDialog::UpdateProgress(const std::string& status, int percent) {
    CallAfter([this, status, percent]() {
        status_text_->SetLabel(status);
        progress_gauge_->SetValue(percent);
    });
}

void BackupDialog::AddLogMessage(const std::string& message, bool isError) {
    CallAfter([this, message, isError]() {
        wxString prefix = isError ? wxString::FromUTF8("[ERROR] ") : wxString::FromUTF8("[INFO] ");
        log_list_->Append(prefix + wxString::FromUTF8(message));
        // Scroll to bottom
        if (log_list_->GetCount() > 0) {
            log_list_->SetSelection(log_list_->GetCount() - 1);
        }
    });
}

bool BackupDialog::ValidateInputs() {
    if (!connection_choice_ || connection_choice_->GetSelection() < 0) {
        wxMessageBox("Please select a connection profile.", "Validation Error", wxOK | wxICON_ERROR);
        return false;
    }

    wxString backupPath = backup_path_ctrl_->GetValue();
    if (backupPath.IsEmpty()) {
        wxMessageBox("Please specify a backup file path.", "Validation Error", wxOK | wxICON_ERROR);
        return false;
    }

    return true;
}

std::string BackupDialog::BuildBackupCommand() {
    // Build command for sb_backup CLI tool
    // Format: sb_backup --host <host> --database <db> --output <file> [options]

    int sel = connection_choice_->GetSelection();
    if (sel < 0 || !connections_ || sel >= static_cast<int>(connections_->size())) {
        return "";
    }

    const auto& profile = (*connections_)[sel];
    wxString cmd = "sb_backup";

    // Connection parameters
    if (!profile.host.empty()) {
        cmd += " --host " + wxString::FromUTF8(profile.host);
    }
    if (profile.port != 0) {
        cmd += " --port " + wxString::Format("%d", profile.port);
    }
    if (!profile.database.empty()) {
        cmd += " --database " + wxString::FromUTF8(profile.database);
    }

    // Output file
    cmd += " --output " + backup_path_ctrl_->GetValue();

    // Format
    int formatIndex = format_choice_->GetSelection();
    if (formatIndex == 0) {
        cmd += " --format sbbak";
    } else if (formatIndex == 1) {
        cmd += " --format sql";
    } else if (formatIndex == 2) {
        cmd += " --format custom";
    }

    // Compression
    cmd += wxString::Format(" --compression %d", compression_slider_->GetValue());

    // Schema filter (if any selected)
    if (schema_list_) {
        wxArrayInt selections;
        int count = schema_list_->GetSelections(selections);
        for (int i = 0; i < count; ++i) {
            cmd += " --include-schema " + schema_list_->GetString(selections[i]);
        }
    }

    // Data options
    if (include_data_checkbox_ && !include_data_checkbox_->IsChecked()) {
        cmd += " --schema-only";
    }

    // Consistent snapshot
    if (consistent_snapshot_checkbox_ && consistent_snapshot_checkbox_->IsChecked()) {
        cmd += " --consistent";
    }

    return cmd.ToStdString();
}

void BackupDialog::EnableControls(bool enable) {
    if (connection_choice_) connection_choice_->Enable(enable);
    if (backup_path_ctrl_) backup_path_ctrl_->Enable(enable);
    if (browse_btn_) browse_btn_->Enable(enable);
    if (format_choice_) format_choice_->Enable(enable);
    if (compression_slider_) compression_slider_->Enable(enable);
    if (schema_list_) schema_list_->Enable(enable);
    if (include_data_checkbox_) include_data_checkbox_->Enable(enable);
    if (consistent_snapshot_checkbox_) consistent_snapshot_checkbox_->Enable(enable);

    if (start_backup_btn_) start_backup_btn_->Enable(enable);
    if (close_btn_) close_btn_->Enable(enable);

    // Cancel button is enabled during backup
    if (cancel_btn_) cancel_btn_->Enable(!enable);
}

void BackupDialog::ShowProgressSection(bool show) {
    if (progress_panel_) {
        progress_panel_->Show(show);
        Layout();
    }
}

} // namespace scratchrobin
