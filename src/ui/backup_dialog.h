/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_BACKUP_DIALOG_H
#define SCRATCHROBIN_BACKUP_DIALOG_H

#include <memory>
#include <string>
#include <vector>

#include <wx/dialog.h>
#include <wx/process.h>

class wxChoice;
class wxTextCtrl;
class wxStaticText;
class wxButton;
class wxGauge;
class wxCheckBox;
class wxSlider;
class wxListBox;
class wxPanel;
class wxFileDialog;

namespace scratchrobin {

struct ConnectionProfile;

// Process handler for backup subprocess
class BackupProcessHandler : public wxProcess {
public:
    BackupProcessHandler(wxWindow* parent);

    void OnTerminate(int pid, int status) override;

private:
    wxWindow* parent_;
};

// Backup result status
struct BackupResult {
    bool success = false;
    std::string backupFile;
    int64_t bytesWritten = 0;
    std::string errorMessage;
    bool cancelled = false;
};

class BackupDialog : public wxDialog {
public:
    BackupDialog(wxWindow* parent,
                 const std::vector<ConnectionProfile>* connections,
                 const std::string& currentDatabase = "");

    const BackupResult& GetResult() const { return result_; }

private:
    void BuildLayout();
    void BuildSourceSection(wxSizer* parentSizer);
    void BuildDestinationSection(wxSizer* parentSizer);
    void BuildOptionsSection(wxSizer* parentSizer);
    void BuildProgressSection(wxSizer* parentSizer);
    void BuildButtonSizer(wxSizer* parentSizer);

    void OnConnectionChanged(wxCommandEvent& event);
    void OnBrowse(wxCommandEvent& event);
    void OnStartBackup(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);

    void StartBackupProcess();
    void StopBackupProcess();
    void ReadProcessOutput();
    void OnProcessTerminated(int exitCode);
    void UpdateProgress(const std::string& status, int percent);
    void AddLogMessage(const std::string& message, bool isError = false);

    bool ValidateInputs();
    std::string BuildBackupCommand();
    void EnableControls(bool enable);
    void ShowProgressSection(bool show);

    const std::vector<ConnectionProfile>* connections_;
    std::string current_database_;
    BackupResult result_;

    // Source section
    wxChoice* connection_choice_ = nullptr;
    wxStaticText* database_label_ = nullptr;

    // Destination section
    wxTextCtrl* backup_path_ctrl_ = nullptr;
    wxButton* browse_btn_ = nullptr;
    wxChoice* format_choice_ = nullptr;

    // Options section
    wxSlider* compression_slider_ = nullptr;
    wxStaticText* compression_value_label_ = nullptr;
    wxListBox* schema_list_ = nullptr;
    wxCheckBox* include_data_checkbox_ = nullptr;
    wxCheckBox* consistent_snapshot_checkbox_ = nullptr;

    // Progress section
    wxPanel* progress_panel_ = nullptr;
    wxGauge* progress_gauge_ = nullptr;
    wxStaticText* status_text_ = nullptr;
    wxListBox* log_list_ = nullptr;
    wxButton* cancel_btn_ = nullptr;

    // Main buttons
    wxButton* start_backup_btn_ = nullptr;
    wxButton* close_btn_ = nullptr;
    wxButton* help_btn_ = nullptr;

    // Process handling
    std::unique_ptr<BackupProcessHandler> process_handler_;
    long process_pid_ = 0;
    bool is_running_ = false;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_BACKUP_DIALOG_H
