/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_BACKUP_HISTORY_DIALOG_H
#define SCRATCHROBIN_BACKUP_HISTORY_DIALOG_H

#include <cstdint>
#include <string>
#include <vector>

#include <wx/dialog.h>

class wxButton;
class wxChoice;
class wxDatePickerCtrl;
class wxGrid;
class wxGridEvent;
class wxStaticText;

namespace scratchrobin {

class ConnectionManager;
struct ConnectionProfile;

enum class BackupType {
    Full,
    Incremental,
    Differential
};

enum class BackupStatus {
    Success,
    Failed,
    InProgress
};

struct BackupHistoryRecord {
    int backup_id = 0;
    std::string backup_time;
    std::string database_name;
    BackupType type = BackupType::Full;
    int64_t size_bytes = 0;
    double compression_ratio = 1.0;
    int duration_seconds = 0;
    BackupStatus status = BackupStatus::Success;
    std::string file_path;
    std::string created_by;
    std::string error_message;
};

class BackupHistoryDialog : public wxDialog {
public:
    BackupHistoryDialog(wxWindow* parent,
                       ConnectionManager* connectionManager,
                       const std::vector<ConnectionProfile>* connections);

private:
    void BuildLayout();
    void LoadHistory();
    void ApplyFilters();
    void UpdateGrid();
    void UpdateSummary();
    void OnRefresh(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnRestore(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnGridSelect(wxGridEvent& event);
    void OnFilterChanged(wxCommandEvent& event);

    std::string FormatSize(int64_t bytes) const;
    std::string FormatDuration(int seconds) const;
    wxString GetStatusText(BackupStatus status) const;

    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;

    // Filter controls
    wxChoice* database_filter_ = nullptr;
    wxDatePickerCtrl* date_from_ = nullptr;
    wxDatePickerCtrl* date_to_ = nullptr;
    wxChoice* status_filter_ = nullptr;
    wxChoice* type_filter_ = nullptr;

    // Grid
    wxGrid* history_grid_ = nullptr;

    // Details
    wxStaticText* details_text_ = nullptr;

    // Summary
    wxStaticText* summary_text_ = nullptr;

    // Data
    std::vector<BackupHistoryRecord> all_records_;
    std::vector<BackupHistoryRecord> filtered_records_;
    int selected_record_ = -1;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_BACKUP_HISTORY_DIALOG_H
