/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "backup_schedule_dialog.h"

#include <algorithm>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/datectrl.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timectrl.h>

#include "connection_editor_dialog.h"

// Include full definition for ConnectionProfile
#include "core/connection_manager.h"

namespace scratchrobin {

namespace {
    constexpr int ID_ADD = wxID_HIGHEST + 1;
    constexpr int ID_EDIT = wxID_HIGHEST + 2;
    constexpr int ID_DELETE = wxID_HIGHEST + 3;
    constexpr int ID_ENABLE = wxID_HIGHEST + 4;
    constexpr int ID_RUN_NOW = wxID_HIGHEST + 5;
}

// Main dialog event table
wxBEGIN_EVENT_TABLE(BackupScheduleDialog, wxDialog)
    EVT_BUTTON(ID_ADD, BackupScheduleDialog::OnAdd)
    EVT_BUTTON(ID_EDIT, BackupScheduleDialog::OnEdit)
    EVT_BUTTON(ID_DELETE, BackupScheduleDialog::OnDelete)
    EVT_BUTTON(ID_ENABLE, BackupScheduleDialog::OnEnableDisable)
    EVT_BUTTON(ID_RUN_NOW, BackupScheduleDialog::OnRunNow)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, BackupScheduleDialog::OnScheduleSelected)
    EVT_LIST_ITEM_ACTIVATED(wxID_ANY, BackupScheduleDialog::OnScheduleActivated)
wxEND_EVENT_TABLE()

BackupScheduleDialog::BackupScheduleDialog(wxWindow* parent,
                                           const std::vector<ConnectionProfile>* connections)
    : wxDialog(parent, wxID_ANY, "Backup Schedule", wxDefaultPosition, wxSize(800, 500),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , connections_(connections) {
    BuildLayout();
    LoadSchedules();
}

void BackupScheduleDialog::BuildLayout() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Schedule list
    schedule_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxLC_REPORT | wxLC_SINGLE_SEL);
    schedule_list_->AppendColumn("ID", wxLIST_FORMAT_LEFT, 50);
    schedule_list_->AppendColumn("Name", wxLIST_FORMAT_LEFT, 150);
    schedule_list_->AppendColumn("Database", wxLIST_FORMAT_LEFT, 120);
    schedule_list_->AppendColumn("Type", wxLIST_FORMAT_LEFT, 80);
    schedule_list_->AppendColumn("Next Run", wxLIST_FORMAT_LEFT, 120);
    schedule_list_->AppendColumn("Status", wxLIST_FORMAT_LEFT, 80);
    mainSizer->Add(schedule_list_, 1, wxEXPAND | wxALL, 12);

    // Details
    details_text_ = new wxStaticText(this, wxID_ANY, "Select a schedule to view details");
    mainSizer->Add(details_text_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(new wxButton(this, ID_ADD, "Add..."), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, ID_EDIT, "Edit..."), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, ID_DELETE, "Delete"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, ID_ENABLE, "Enable/Disable"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, ID_RUN_NOW, "Run Now"), 0, wxRIGHT, 8);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CLOSE, "Close"), 0);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 12);

    SetSizer(mainSizer);
}

void BackupScheduleDialog::LoadSchedules() {
    // Mock data
    schedules_.clear();
    
    BackupSchedule s1;
    s1.schedule_id = 1;
    s1.name = "Daily Full Backup";
    s1.database = "scratchbird_prod";
    s1.schedule_type = "Recurring";
    s1.next_run = "2026-01-17 02:00:00";
    s1.status = "Active";
    s1.backup_type = "Full";
    schedules_.push_back(s1);
    
    BackupSchedule s2;
    s2.schedule_id = 2;
    s2.name = "Weekly Archive";
    s2.database = "scratchbird_archive";
    s2.schedule_type = "Recurring";
    s2.next_run = "2026-01-21 03:00:00";
    s2.status = "Active";
    s2.backup_type = "Full";
    schedules_.push_back(s2);
    
    BackupSchedule s3;
    s3.schedule_id = 3;
    s3.name = "Test Backup";
    s3.database = "test_db";
    s3.schedule_type = "One-time";
    s3.next_run = "2026-01-20 10:00:00";
    s3.status = "Disabled";
    s3.backup_type = "Incremental";
    schedules_.push_back(s3);
    
    UpdateList();
}

void BackupScheduleDialog::UpdateList() {
    schedule_list_->DeleteAllItems();
    
    for (size_t i = 0; i < schedules_.size(); ++i) {
        const auto& s = schedules_[i];
        long idx = schedule_list_->InsertItem(i, std::to_string(s.schedule_id));
        schedule_list_->SetItem(idx, 1, s.name);
        schedule_list_->SetItem(idx, 2, s.database);
        schedule_list_->SetItem(idx, 3, s.backup_type);
        schedule_list_->SetItem(idx, 4, s.next_run);
        schedule_list_->SetItem(idx, 5, s.status);
    }
}

void BackupScheduleDialog::OnAdd(wxCommandEvent&) {
    BackupSchedule newSchedule;
    BackupScheduleEditDialog dialog(this, connections_, &newSchedule);
    if (dialog.ShowModal() == wxID_OK) {
        newSchedule.schedule_id = schedules_.empty() ? 1 : schedules_.back().schedule_id + 1;
        schedules_.push_back(newSchedule);
        UpdateList();
    }
}

void BackupScheduleDialog::OnEdit(wxCommandEvent&) {
    if (selected_schedule_ < 0 || selected_schedule_ >= static_cast<int>(schedules_.size())) {
        wxMessageBox("Please select a schedule to edit", "Edit", wxOK | wxICON_WARNING, this);
        return;
    }
    BackupScheduleEditDialog dialog(this, connections_, &schedules_[selected_schedule_]);
    if (dialog.ShowModal() == wxID_OK) {
        UpdateList();
    }
}

void BackupScheduleDialog::OnDelete(wxCommandEvent&) {
    if (selected_schedule_ < 0 || selected_schedule_ >= static_cast<int>(schedules_.size())) {
        wxMessageBox("Please select a schedule to delete", "Delete", wxOK | wxICON_WARNING, this);
        return;
    }
    int ret = wxMessageBox("Delete this schedule?", "Confirm", wxYES_NO | wxICON_QUESTION, this);
    if (ret == wxYES) {
        schedules_.erase(schedules_.begin() + selected_schedule_);
        selected_schedule_ = -1;
        UpdateList();
    }
}

void BackupScheduleDialog::OnEnableDisable(wxCommandEvent&) {
    if (selected_schedule_ < 0 || selected_schedule_ >= static_cast<int>(schedules_.size())) {
        return;
    }
    auto& s = schedules_[selected_schedule_];
    s.status = (s.status == "Active") ? "Disabled" : "Active";
    UpdateList();
}

void BackupScheduleDialog::OnRunNow(wxCommandEvent&) {
    if (selected_schedule_ < 0 || selected_schedule_ >= static_cast<int>(schedules_.size())) {
        return;
    }
    wxMessageBox("Backup job queued for immediate execution", "Run Now", wxOK, this);
}

void BackupScheduleDialog::OnScheduleSelected(wxListEvent& event) {
    selected_schedule_ = event.GetIndex();
    if (selected_schedule_ >= 0 && selected_schedule_ < static_cast<int>(schedules_.size())) {
        const auto& s = schedules_[selected_schedule_];
        details_text_->SetLabel("Schedule: " + s.name + " | Database: " + s.database +
                               " | Type: " + s.schedule_type + " | Status: " + s.status);
    }
}

void BackupScheduleDialog::OnScheduleActivated(wxListEvent& event) {
    selected_schedule_ = event.GetIndex();
    wxCommandEvent dummy;
    OnEdit(dummy);
}

// Edit dialog event table
wxBEGIN_EVENT_TABLE(BackupScheduleEditDialog, wxDialog)
    EVT_RADIOBUTTON(wxID_ANY, BackupScheduleEditDialog::OnScheduleTypeChanged)
    EVT_CHOICE(wxID_ANY, BackupScheduleEditDialog::OnFrequencyChanged)
    EVT_BUTTON(wxID_OK, BackupScheduleEditDialog::OnOk)
wxEND_EVENT_TABLE()

BackupScheduleEditDialog::BackupScheduleEditDialog(wxWindow* parent,
                                                  const std::vector<ConnectionProfile>* connections,
                                                  BackupSchedule* schedule)
    : wxDialog(parent, wxID_ANY, schedule->schedule_id ? "Edit Schedule" : "New Schedule",
               wxDefaultPosition, wxSize(500, 600), wxDEFAULT_DIALOG_STYLE)
    , connections_(connections)
    , schedule_(schedule) {
    BuildLayout();
    PopulateDatabases();
    
    // Populate if editing
    if (schedule_->schedule_id) {
        name_ctrl_->SetValue(schedule_->name);
        database_choice_->SetStringSelection(schedule_->database);
        backup_type_choice_->SetStringSelection(schedule_->backup_type);
    }
}

void BackupScheduleEditDialog::BuildLayout() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Basic settings
    auto* basicSizer = new wxBoxSizer(wxVERTICAL);
    basicSizer->Add(new wxStaticText(this, wxID_ANY, "Schedule Name:"), 0, wxBOTTOM, 4);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    basicSizer->Add(name_ctrl_, 0, wxEXPAND | wxBOTTOM, 12);
    
    basicSizer->Add(new wxStaticText(this, wxID_ANY, "Database:"), 0, wxBOTTOM, 4);
    database_choice_ = new wxChoice(this, wxID_ANY);
    basicSizer->Add(database_choice_, 0, wxEXPAND | wxBOTTOM, 12);
    
    basicSizer->Add(new wxStaticText(this, wxID_ANY, "Backup Type:"), 0, wxBOTTOM, 4);
    backup_type_choice_ = new wxChoice(this, wxID_ANY);
    backup_type_choice_->Append("Full");
    backup_type_choice_->Append("Incremental");
    backup_type_choice_->Append("Differential");
    backup_type_choice_->SetSelection(0);
    basicSizer->Add(backup_type_choice_, 0, wxEXPAND | wxBOTTOM, 12);
    
    mainSizer->Add(basicSizer, 0, wxEXPAND | wxALL, 12);

    // Schedule type
    auto* typeSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Schedule Type");
    auto* radioSizer = new wxBoxSizer(wxHORIZONTAL);
    one_time_radio_ = new wxRadioButton(this, wxID_ANY, "One-time", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    recurring_radio_ = new wxRadioButton(this, wxID_ANY, "Recurring");
    radioSizer->Add(one_time_radio_, 0, wxRIGHT, 16);
    radioSizer->Add(recurring_radio_, 0);
    typeSizer->Add(radioSizer, 0, wxALL, 8);
    
    // One-time panel
    one_time_panel_ = new wxPanel(this, wxID_ANY);
    auto* oneSizer = new wxBoxSizer(wxVERTICAL);
    oneSizer->Add(new wxStaticText(one_time_panel_, wxID_ANY, "Date:"), 0, wxBOTTOM, 4);
    date_picker_ = new wxDatePickerCtrl(one_time_panel_, wxID_ANY);
    oneSizer->Add(date_picker_, 0, wxEXPAND | wxBOTTOM, 8);
    oneSizer->Add(new wxStaticText(one_time_panel_, wxID_ANY, "Time:"), 0, wxBOTTOM, 4);
    time_picker_ = new wxTimePickerCtrl(one_time_panel_, wxID_ANY);
    oneSizer->Add(time_picker_, 0, wxEXPAND, 0);
    one_time_panel_->SetSizer(oneSizer);
    typeSizer->Add(one_time_panel_, 0, wxEXPAND | wxALL, 8);
    
    // Recurring panel
    recurring_panel_ = new wxPanel(this, wxID_ANY);
    auto* recurSizer = new wxBoxSizer(wxVERTICAL);
    
    recurSizer->Add(new wxStaticText(recurring_panel_, wxID_ANY, "Frequency:"), 0, wxBOTTOM, 4);
    frequency_choice_ = new wxChoice(recurring_panel_, wxID_ANY);
    frequency_choice_->Append("Daily");
    frequency_choice_->Append("Weekly");
    frequency_choice_->Append("Monthly");
    frequency_choice_->SetSelection(0);
    recurSizer->Add(frequency_choice_, 0, wxEXPAND | wxBOTTOM, 8);
    
    // Weekly options
    weekly_panel_ = new wxPanel(recurring_panel_, wxID_ANY);
    auto* weekSizer = new wxBoxSizer(wxHORIZONTAL);
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int i = 0; i < 7; ++i) {
        day_checkboxes_[i] = new wxCheckBox(weekly_panel_, wxID_ANY, days[i]);
        weekSizer->Add(day_checkboxes_[i], 0, wxRIGHT, 8);
    }
    weekly_panel_->SetSizer(weekSizer);
    recurSizer->Add(weekly_panel_, 0, wxEXPAND | wxBOTTOM, 8);
    
    // Monthly options
    monthly_panel_ = new wxPanel(recurring_panel_, wxID_ANY);
    auto* monthSizer = new wxBoxSizer(wxHORIZONTAL);
    monthSizer->Add(new wxStaticText(monthly_panel_, wxID_ANY, "Day of month:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    monthly_day_spin_ = new wxSpinCtrl(monthly_panel_, wxID_ANY, "1");
    monthly_day_spin_->SetRange(1, 31);
    monthSizer->Add(monthly_day_spin_, 0);
    monthly_panel_->SetSizer(monthSizer);
    recurSizer->Add(monthly_panel_, 0, wxEXPAND | wxBOTTOM, 8);
    
    recurSizer->Add(new wxStaticText(recurring_panel_, wxID_ANY, "Time:"), 0, wxBOTTOM, 4);
    recurring_time_picker_ = new wxTimePickerCtrl(recurring_panel_, wxID_ANY);
    recurSizer->Add(recurring_time_picker_, 0, wxEXPAND, 0);
    
    recurring_panel_->SetSizer(recurSizer);
    typeSizer->Add(recurring_panel_, 0, wxEXPAND | wxALL, 8);
    
    mainSizer->Add(typeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Options
    auto* optsSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Options");
    auto* destSizer = new wxBoxSizer(wxHORIZONTAL);
    destSizer->Add(new wxStaticText(this, wxID_ANY, "Destination:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    destination_ctrl_ = new wxTextCtrl(this, wxID_ANY, "/backups/");
    destSizer->Add(destination_ctrl_, 1);
    optsSizer->Add(destSizer, 0, wxEXPAND | wxALL, 8);
    
    auto* retSizer = new wxBoxSizer(wxHORIZONTAL);
    retSizer->Add(new wxStaticText(this, wxID_ANY, "Retention (backups to keep):"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    retention_spin_ = new wxSpinCtrl(this, wxID_ANY, "7");
    retention_spin_->SetRange(1, 365);
    retSizer->Add(retention_spin_, 0);
    optsSizer->Add(retSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    mainSizer->Add(optsSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 12);

    SetSizer(mainSizer);
    
    // Initialize visibility
    UpdateScheduleTypeVisibility();
    UpdateFrequencyVisibility();
}

void BackupScheduleEditDialog::PopulateDatabases() {
    database_choice_->Append("");
    if (connections_) {
        for (const auto& profile : *connections_) {
            if (!profile.database.empty()) {
                database_choice_->Append(profile.database);
            }
        }
    }
    if (database_choice_->GetCount() > 0) {
        database_choice_->SetSelection(0);
    }
}

void BackupScheduleEditDialog::OnScheduleTypeChanged(wxCommandEvent&) {
    UpdateScheduleTypeVisibility();
}

void BackupScheduleEditDialog::OnFrequencyChanged(wxCommandEvent&) {
    UpdateFrequencyVisibility();
}

void BackupScheduleEditDialog::UpdateScheduleTypeVisibility() {
    if (!one_time_panel_ || !recurring_panel_) return;
    bool is_one_time = one_time_radio_->GetValue();
    one_time_panel_->Show(is_one_time);
    recurring_panel_->Show(!is_one_time);
    Layout();
}

void BackupScheduleEditDialog::UpdateFrequencyVisibility() {
    if (!frequency_choice_ || !weekly_panel_ || !monthly_panel_) return;
    wxString freq = frequency_choice_->GetStringSelection();
    weekly_panel_->Show(freq == "Weekly");
    monthly_panel_->Show(freq == "Monthly");
    Layout();
}

void BackupScheduleEditDialog::OnOk(wxCommandEvent& event) {
    if (name_ctrl_->GetValue().IsEmpty()) {
        wxMessageBox("Please enter a schedule name", "Validation", wxOK | wxICON_WARNING, this);
        return;
    }
    if (database_choice_->GetStringSelection().IsEmpty()) {
        wxMessageBox("Please select a database", "Validation", wxOK | wxICON_WARNING, this);
        return;
    }
    
    schedule_->name = name_ctrl_->GetValue().ToStdString();
    schedule_->database = database_choice_->GetStringSelection().ToStdString();
    schedule_->backup_type = backup_type_choice_->GetStringSelection().ToStdString();
    schedule_->schedule_type = one_time_radio_->GetValue() ? "One-time" : "Recurring";
    schedule_->status = "Active";
    
    event.Skip();
}

} // namespace scratchrobin
