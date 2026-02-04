/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "backup_history_dialog.h"

#include <iomanip>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/datectrl.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "core/connection_manager.h"
#include "ui/connection_editor_dialog.h"

namespace scratchrobin {

namespace {
    constexpr int ID_REFRESH = wxID_HIGHEST + 1;
    constexpr int ID_EXPORT = wxID_HIGHEST + 2;
    constexpr int ID_RESTORE = wxID_HIGHEST + 3;
    constexpr int ID_DELETE = wxID_HIGHEST + 4;
}

wxBEGIN_EVENT_TABLE(BackupHistoryDialog, wxDialog)
    EVT_BUTTON(ID_REFRESH, BackupHistoryDialog::OnRefresh)
    EVT_BUTTON(ID_EXPORT, BackupHistoryDialog::OnExport)
    EVT_BUTTON(ID_RESTORE, BackupHistoryDialog::OnRestore)
    EVT_BUTTON(ID_DELETE, BackupHistoryDialog::OnDelete)
    EVT_GRID_SELECT_CELL(BackupHistoryDialog::OnGridSelect)
wxEND_EVENT_TABLE()

BackupHistoryDialog::BackupHistoryDialog(wxWindow* parent,
                                         ConnectionManager* connectionManager,
                                         const std::vector<ConnectionProfile>* connections)
    : wxDialog(parent, wxID_ANY, "Backup History", wxDefaultPosition, wxSize(900, 600),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , connection_manager_(connectionManager)
    , connections_(connections) {
    BuildLayout();
    LoadHistory();
}

void BackupHistoryDialog::BuildLayout() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Filter section
    auto* filterSizer = new wxBoxSizer(wxHORIZONTAL);
    
    filterSizer->Add(new wxStaticText(this, wxID_ANY, "Database:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    database_filter_ = new wxChoice(this, wxID_ANY);
    database_filter_->Append("All");
    filterSizer->Add(database_filter_, 1, wxRIGHT, 16);
    
    filterSizer->Add(new wxStaticText(this, wxID_ANY, "Status:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    status_filter_ = new wxChoice(this, wxID_ANY);
    status_filter_->Append("All");
    status_filter_->Append("Success");
    status_filter_->Append("Failed");
    status_filter_->SetSelection(0);
    filterSizer->Add(status_filter_, 1, wxRIGHT, 16);
    
    filterSizer->Add(new wxStaticText(this, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    type_filter_ = new wxChoice(this, wxID_ANY);
    type_filter_->Append("All");
    type_filter_->Append("Full");
    type_filter_->Append("Incremental");
    type_filter_->Append("Differential");
    type_filter_->SetSelection(0);
    filterSizer->Add(type_filter_, 1);

    mainSizer->Add(filterSizer, 0, wxEXPAND | wxALL, 12);

    // Grid
    history_grid_ = new wxGrid(this, wxID_ANY);
    history_grid_->CreateGrid(0, 8);
    history_grid_->SetColLabelValue(0, "ID");
    history_grid_->SetColLabelValue(1, "Time");
    history_grid_->SetColLabelValue(2, "Database");
    history_grid_->SetColLabelValue(3, "Type");
    history_grid_->SetColLabelValue(4, "Size");
    history_grid_->SetColLabelValue(5, "Duration");
    history_grid_->SetColLabelValue(6, "Status");
    history_grid_->SetColLabelValue(7, "Created By");
    history_grid_->SetDefaultColSize(100);
    history_grid_->SetColSize(1, 150);
    history_grid_->SetSelectionMode(wxGrid::wxGridSelectRows);
    mainSizer->Add(history_grid_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Details
    details_text_ = new wxStaticText(this, wxID_ANY, "Select a backup to view details");
    mainSizer->Add(details_text_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Summary
    summary_text_ = new wxStaticText(this, wxID_ANY, "Total: 0 backups");
    mainSizer->Add(summary_text_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(new wxButton(this, ID_REFRESH, "Refresh"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, ID_EXPORT, "Export"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, ID_RESTORE, "Restore..."), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, ID_DELETE, "Delete"), 0, wxRIGHT, 8);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CLOSE, "Close"), 0);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 12);

    SetSizer(mainSizer);
}

void BackupHistoryDialog::LoadHistory() {
    // Mock data for demonstration
    all_records_.clear();
    
    BackupHistoryRecord r1;
    r1.backup_id = 1;
    r1.backup_time = "2026-01-15 10:30:00";
    r1.database_name = "scratchbird_prod";
    r1.type = BackupType::Full;
    r1.size_bytes = 1024 * 1024 * 500; // 500 MB
    r1.compression_ratio = 0.3;
    r1.duration_seconds = 120;
    r1.status = BackupStatus::Success;
    r1.file_path = "/backups/scratchbird_prod_20260115_103000.sbbak";
    r1.created_by = "admin";
    all_records_.push_back(r1);
    
    BackupHistoryRecord r2;
    r2.backup_id = 2;
    r2.backup_time = "2026-01-16 02:00:00";
    r2.database_name = "scratchbird_prod";
    r2.type = BackupType::Incremental;
    r2.size_bytes = 1024 * 1024 * 50; // 50 MB
    r2.compression_ratio = 0.25;
    r2.duration_seconds = 30;
    r2.status = BackupStatus::Success;
    r2.file_path = "/backups/scratchbird_prod_20260116_020000_inc.sbbak";
    r2.created_by = "scheduler";
    all_records_.push_back(r2);
    
    BackupHistoryRecord r3;
    r3.backup_id = 3;
    r3.backup_time = "2026-01-16 14:15:00";
    r3.database_name = "test_db";
    r3.type = BackupType::Full;
    r3.size_bytes = 1024 * 1024 * 100; // 100 MB
    r3.duration_seconds = 0;
    r3.status = BackupStatus::Failed;
    r3.error_message = "Disk full";
    r3.created_by = "developer";
    all_records_.push_back(r3);
    
    ApplyFilters();
}

void BackupHistoryDialog::ApplyFilters() {
    filtered_records_ = all_records_;
    
    // Apply status filter
    int statusSel = status_filter_->GetSelection();
    if (statusSel > 0) {
        BackupStatus target = (statusSel == 1) ? BackupStatus::Success : BackupStatus::Failed;
        filtered_records_.erase(
            std::remove_if(filtered_records_.begin(), filtered_records_.end(),
                [target](const auto& r) { return r.status != target; }),
            filtered_records_.end());
    }
    
    // Apply type filter
    int typeSel = type_filter_->GetSelection();
    if (typeSel > 0) {
        BackupType target = static_cast<BackupType>(typeSel - 1);
        filtered_records_.erase(
            std::remove_if(filtered_records_.begin(), filtered_records_.end(),
                [target](const auto& r) { return r.type != target; }),
            filtered_records_.end());
    }
    
    UpdateGrid();
    UpdateSummary();
}

void BackupHistoryDialog::UpdateGrid() {
    if (history_grid_->GetNumberRows() > 0) {
        history_grid_->DeleteRows(0, history_grid_->GetNumberRows());
    }
    
    history_grid_->AppendRows(filtered_records_.size());
    
    for (size_t i = 0; i < filtered_records_.size(); ++i) {
        const auto& r = filtered_records_[i];
        history_grid_->SetCellValue(i, 0, std::to_string(r.backup_id));
        history_grid_->SetCellValue(i, 1, r.backup_time);
        history_grid_->SetCellValue(i, 2, r.database_name);
        
        const char* typeStr = (r.type == BackupType::Full) ? "Full" :
                              (r.type == BackupType::Incremental) ? "Incremental" : "Differential";
        history_grid_->SetCellValue(i, 3, typeStr);
        
        history_grid_->SetCellValue(i, 4, FormatSize(r.size_bytes));
        history_grid_->SetCellValue(i, 5, FormatDuration(r.duration_seconds));
        history_grid_->SetCellValue(i, 6, GetStatusText(r.status));
        history_grid_->SetCellValue(i, 7, r.created_by);
        
        // Color status
        if (r.status == BackupStatus::Success) {
            history_grid_->SetCellBackgroundColour(i, 6, wxColour(200, 255, 200));
        } else if (r.status == BackupStatus::Failed) {
            history_grid_->SetCellBackgroundColour(i, 6, wxColour(255, 200, 200));
        }
    }
    
    history_grid_->AutoSizeColumns();
}

void BackupHistoryDialog::UpdateSummary() {
    int total = filtered_records_.size();
    int success = 0;
    int64_t totalSize = 0;
    
    for (const auto& r : filtered_records_) {
        if (r.status == BackupStatus::Success) ++success;
        totalSize += r.size_bytes;
    }
    
    std::ostringstream oss;
    oss << "Total: " << total << " backups | Successful: " << success
        << " | Failed: " << (total - success)
        << " | Total Size: " << FormatSize(totalSize);
    summary_text_->SetLabel(oss.str());
}

void BackupHistoryDialog::OnRefresh(wxCommandEvent&) {
    LoadHistory();
}

void BackupHistoryDialog::OnExport(wxCommandEvent&) {
    // Export to clipboard
    if (wxTheClipboard->Open()) {
        std::ostringstream oss;
        oss << "ID,Time,Database,Type,Size,Duration,Status,Created By\n";
        for (const auto& r : filtered_records_) {
            const char* typeStr = (r.type == BackupType::Full) ? "Full" :
                                  (r.type == BackupType::Incremental) ? "Incremental" : "Differential";
            oss << r.backup_id << ","
                << r.backup_time << ","
                << r.database_name << ","
                << typeStr << ","
                << FormatSize(r.size_bytes) << ","
                << FormatDuration(r.duration_seconds) << ","
                << GetStatusText(r.status) << ","
                << r.created_by << "\n";
        }
        wxTheClipboard->SetData(new wxTextDataObject(oss.str()));
        wxTheClipboard->Close();
        wxMessageBox("History exported to clipboard", "Export", wxOK | wxICON_INFORMATION, this);
    }
}

void BackupHistoryDialog::OnRestore(wxCommandEvent&) {
    if (selected_record_ < 0 || selected_record_ >= static_cast<int>(filtered_records_.size())) {
        wxMessageBox("Please select a backup to restore", "Restore", wxOK | wxICON_WARNING, this);
        return;
    }
    const auto& r = filtered_records_[selected_record_];
    wxMessageBox("Restore dialog would open for: " + r.file_path, "Restore", wxOK, this);
}

void BackupHistoryDialog::OnDelete(wxCommandEvent&) {
    if (selected_record_ < 0 || selected_record_ >= static_cast<int>(filtered_records_.size())) {
        wxMessageBox("Please select a backup to delete", "Delete", wxOK | wxICON_WARNING, this);
        return;
    }
    const auto& r = filtered_records_[selected_record_];
    int ret = wxMessageBox("Delete backup " + std::to_string(r.backup_id) + "?", "Confirm Delete",
                           wxYES_NO | wxICON_QUESTION, this);
    if (ret == wxYES) {
        // Remove from all_records_
        all_records_.erase(
            std::remove_if(all_records_.begin(), all_records_.end(),
                [&r](const auto& rec) { return rec.backup_id == r.backup_id; }),
            all_records_.end());
        ApplyFilters();
    }
}

void BackupHistoryDialog::OnGridSelect(wxGridEvent& event) {
    selected_record_ = event.GetRow();
    if (selected_record_ >= 0 && selected_record_ < static_cast<int>(filtered_records_.size())) {
        const auto& r = filtered_records_[selected_record_];
        std::ostringstream oss;
        oss << "Backup ID: " << r.backup_id << " | File: " << r.file_path;
        if (!r.error_message.empty()) {
            oss << " | Error: " << r.error_message;
        }
        details_text_->SetLabel(oss.str());
    }
}

void BackupHistoryDialog::OnFilterChanged(wxCommandEvent&) {
    ApplyFilters();
}

std::string BackupHistoryDialog::FormatSize(int64_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        ++unit;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

std::string BackupHistoryDialog::FormatDuration(int seconds) const {
    if (seconds < 60) {
        return std::to_string(seconds) + "s";
    } else if (seconds < 3600) {
        return std::to_string(seconds / 60) + "m " + std::to_string(seconds % 60) + "s";
    } else {
        return std::to_string(seconds / 3600) + "h " + std::to_string((seconds % 3600) / 60) + "m";
    }
}

wxString BackupHistoryDialog::GetStatusText(BackupStatus status) const {
    switch (status) {
        case BackupStatus::Success: return "Success";
        case BackupStatus::Failed: return "Failed";
        case BackupStatus::InProgress: return "In Progress";
    }
    return "Unknown";
}

} // namespace scratchrobin
