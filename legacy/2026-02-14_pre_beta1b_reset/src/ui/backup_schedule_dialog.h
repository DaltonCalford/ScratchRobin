/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_BACKUP_SCHEDULE_DIALOG_H
#define SCRATCHROBIN_BACKUP_SCHEDULE_DIALOG_H

#include <memory>
#include <string>
#include <vector>

#include <wx/dialog.h>

class wxButton;
class wxCheckBox;
class wxChoice;
class wxDatePickerCtrl;
class wxListCtrl;
class wxListEvent;
class wxPanel;
class wxRadioButton;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;
class wxTimePickerCtrl;

namespace scratchrobin {

struct ConnectionProfile;
class WindowManager;

// Backup schedule data structure
struct BackupSchedule {
    int schedule_id = 0;
    std::string name;
    std::string database;
    std::string schedule_type;
    std::string next_run;
    std::string last_run;
    std::string status;
    std::string backup_type;
};

class BackupScheduleDialog : public wxDialog {
public:
    BackupScheduleDialog(wxWindow* parent,
                        const std::vector<ConnectionProfile>* connections);

private:
    void BuildLayout();
    void LoadSchedules();
    void UpdateList();
    
    void OnAdd(wxCommandEvent& event);
    void OnEdit(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnEnableDisable(wxCommandEvent& event);
    void OnRunNow(wxCommandEvent& event);
    void OnScheduleSelected(wxListEvent& event);
    void OnScheduleActivated(wxListEvent& event);

    const std::vector<ConnectionProfile>* connections_ = nullptr;

    wxListCtrl* schedule_list_ = nullptr;
    wxStaticText* details_text_ = nullptr;

    std::vector<BackupSchedule> schedules_;
    int selected_schedule_ = -1;

    wxDECLARE_EVENT_TABLE();
};

// Sub-dialog for editing schedules
class BackupScheduleEditDialog : public wxDialog {
public:
    BackupScheduleEditDialog(wxWindow* parent,
                            const std::vector<ConnectionProfile>* connections,
                            BackupSchedule* schedule);

private:
    void BuildLayout();
    void PopulateDatabases();
    void OnScheduleTypeChanged(wxCommandEvent& event);
    void OnFrequencyChanged(wxCommandEvent& event);
    void OnOk(wxCommandEvent& event);
    void UpdateScheduleTypeVisibility();
    void UpdateFrequencyVisibility();

    const std::vector<ConnectionProfile>* connections_ = nullptr;
    BackupSchedule* schedule_ = nullptr;

    // Basic settings
    wxTextCtrl* name_ctrl_ = nullptr;
    wxChoice* database_choice_ = nullptr;
    wxChoice* backup_type_choice_ = nullptr;

    // Schedule type
    wxRadioButton* one_time_radio_ = nullptr;
    wxRadioButton* recurring_radio_ = nullptr;
    wxPanel* one_time_panel_ = nullptr;
    wxPanel* recurring_panel_ = nullptr;

    // One-time settings
    wxDatePickerCtrl* date_picker_ = nullptr;
    wxTimePickerCtrl* time_picker_ = nullptr;

    // Recurring settings
    wxChoice* frequency_choice_ = nullptr;
    wxPanel* weekly_panel_ = nullptr;
    wxPanel* monthly_panel_ = nullptr;
    wxCheckBox* day_checkboxes_[7] = {};
    wxSpinCtrl* monthly_day_spin_ = nullptr;
    wxTimePickerCtrl* recurring_time_picker_ = nullptr;

    // Options
    wxTextCtrl* destination_ctrl_ = nullptr;
    wxSpinCtrl* retention_spin_ = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_BACKUP_SCHEDULE_DIALOG_H
