/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_STATUS_MONITOR_FRAME_H
#define SCRATCHROBIN_STATUS_MONITOR_FRAME_H

#include <vector>

#include <wx/frame.h>

#include "core/connection_manager.h"
#include "core/config.h"
#include "core/status_types.h"

class wxChoice;
class wxButton;
class wxCheckBox;
class wxBoxSizer;
class wxSpinCtrl;
class wxStaticText;
class wxScrolledWindow;
class wxTimer;
class wxListBox;
class wxTimerEvent;

namespace scratchrobin {

class WindowManager;
struct ConnectionProfile;
struct AppConfig;

class StatusMonitorFrame : public wxFrame {
public:
    StatusMonitorFrame(WindowManager* windowManager,
                       ConnectionManager* connectionManager,
                       const std::vector<ConnectionProfile>* connections,
                       const AppConfig* appConfig);

private:
    void BuildMenu();
    void BuildLayout();
    void PopulateConnections();
    const ConnectionProfile* GetSelectedProfile() const;
    void UpdateControls();
    void UpdateStatus(const std::string& message);
    void DisplayStatusSnapshot(const StatusSnapshot& snapshot);
    void SetStatusMessage(const std::string& message);
    void ApplyStatusDefaults(const ConnectionProfile* profile, bool restartTimer);
    std::string BuildStatusJson(const StatusSnapshot& snapshot,
                                const std::string& category,
                                bool diff_only) const;
    void ClearStatusCards();
    void BuildStatusCards(const StatusSnapshot& snapshot);
    std::string SelectedStatusCategory() const;
    void UpdateStatusCategoryChoices(const StatusSnapshot& snapshot);
    StatusRequestKind SelectedRequestKind() const;
    void AddStatusHistory(const StatusSnapshot& snapshot);
    void RefreshStatusHistory();
    void ShowHistorySnapshot(size_t index);
    void PersistStatusPreferences();
    void UpdateDiffOptionControls();

    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnFetch(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);
    void OnTogglePolling(wxCommandEvent& event);
    void OnPollTimer(wxTimerEvent& event);
    void OnConnectionChanged(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnHistorySelection(wxCommandEvent& event);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;

    wxChoice* connection_choice_ = nullptr;
    wxButton* connect_button_ = nullptr;
    wxButton* disconnect_button_ = nullptr;
    wxChoice* status_type_choice_ = nullptr;
    wxChoice* status_category_choice_ = nullptr;
    wxCheckBox* status_diff_check_ = nullptr;
    wxCheckBox* status_diff_ignore_unchanged_check_ = nullptr;
    wxCheckBox* status_diff_ignore_empty_check_ = nullptr;
    wxButton* fetch_button_ = nullptr;
    wxButton* clear_button_ = nullptr;
    wxButton* copy_button_ = nullptr;
    wxButton* save_button_ = nullptr;
    wxCheckBox* poll_check_ = nullptr;
    wxSpinCtrl* poll_interval_ctrl_ = nullptr;
    wxStaticText* status_message_label_ = nullptr;
    wxScrolledWindow* status_cards_panel_ = nullptr;
    wxBoxSizer* status_cards_sizer_ = nullptr;
    wxListBox* status_history_list_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxTimer* poll_timer_ = nullptr;

    bool connect_running_ = false;
    bool fetch_pending_ = false;
    JobHandle connect_job_;
    StatusSnapshot last_status_;
    StatusSnapshot previous_status_;
    bool has_status_ = false;
    std::vector<std::string> status_category_order_;
    std::string status_category_preference_;
    size_t status_history_limit_ = 50;
    struct StatusHistoryEntry {
        std::string label;
        StatusSnapshot snapshot;
    };
    std::vector<StatusHistoryEntry> status_history_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_STATUS_MONITOR_FRAME_H
