/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_RESTORE_DIALOG_H
#define SCRATCHROBIN_RESTORE_DIALOG_H

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
class wxRadioBox;
class wxListBox;
class wxPanel;
class wxFileDialog;

namespace scratchrobin {

struct ConnectionProfile;

// Restore mode options
enum class RestoreMode {
    Full,        // Restore schema and data
    SchemaOnly,  // Restore schema only
    DataOnly     // Restore data only
};

// Process handler for restore subprocess
class RestoreProcessHandler : public wxProcess {
public:
    RestoreProcessHandler(wxWindow* parent);

    void OnTerminate(int pid, int status) override;

private:
    wxWindow* parent_;
};

// Restore result status
struct RestoreResult {
    bool success = false;
    std::string backupFile;
    std::string targetDatabase;
    int64_t objectsRestored = 0;
    int64_t rowsRestored = 0;
    std::string errorMessage;
    bool cancelled = false;
};

class RestoreDialog : public wxDialog {
public:
    RestoreDialog(wxWindow* parent,
                  const std::vector<ConnectionProfile>* connections);

    const RestoreResult& GetResult() const { return result_; }

private:
    void BuildLayout();
    void BuildSourceSection(wxSizer* parentSizer);
    void BuildTargetSection(wxSizer* parentSizer);
    void BuildOptionsSection(wxSizer* parentSizer);
    void BuildProgressSection(wxSizer* parentSizer);
    void BuildButtonSizer(wxSizer* parentSizer);

    void OnBrowse(wxCommandEvent& event);
    void OnStartRestore(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);
    void OnVerifyBackup(wxCommandEvent& event);

    void StartRestoreProcess();
    void StopRestoreProcess();
    void ReadProcessOutput();
    void OnProcessTerminated(int exitCode);
    void UpdateProgress(const std::string& phase, int percent);
    void AddLogMessage(const std::string& message, bool isError = false);

    bool ValidateInputs();
    std::string BuildRestoreCommand();
    void EnableControls(bool enable);
    void ShowProgressSection(bool show);

    const std::vector<ConnectionProfile>* connections_;
    RestoreResult result_;

    // Source section
    wxTextCtrl* backup_path_ctrl_ = nullptr;
    wxButton* browse_btn_ = nullptr;
    wxCheckBox* verify_backup_checkbox_ = nullptr;

    // Target section
    wxChoice* connection_choice_ = nullptr;
    wxTextCtrl* target_database_ctrl_ = nullptr;
    wxCheckBox* create_database_checkbox_ = nullptr;

    // Options section
    wxRadioBox* restore_mode_radio_ = nullptr;
    wxCheckBox* clean_restore_checkbox_ = nullptr;
    wxCheckBox* disable_triggers_checkbox_ = nullptr;

    // Progress section
    wxPanel* progress_panel_ = nullptr;
    wxGauge* progress_gauge_ = nullptr;
    wxStaticText* phase_text_ = nullptr;
    wxStaticText* status_text_ = nullptr;
    wxListBox* log_list_ = nullptr;
    wxButton* cancel_btn_ = nullptr;

    // Main buttons
    wxButton* start_restore_btn_ = nullptr;
    wxButton* close_btn_ = nullptr;
    wxButton* help_btn_ = nullptr;

    // Process handling
    std::unique_ptr<RestoreProcessHandler> process_handler_;
    long process_pid_ = 0;
    bool is_running_ = false;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_RESTORE_DIALOG_H
