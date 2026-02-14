/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "status_monitor_frame.h"

#include "icon_bar.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "window_manager.h"

#include <fstream>
#include <map>
#include <sstream>
#include <chrono>
#include <ctime>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/timer.h>
#include <wx/listbox.h>

namespace scratchrobin {

namespace {
constexpr int kConnectionChoiceId = wxID_HIGHEST + 4200;
constexpr int kStatusTypeChoiceId = wxID_HIGHEST + 4201;
} // namespace

StatusMonitorFrame::StatusMonitorFrame(WindowManager* windowManager,
                                       ConnectionManager* connectionManager,
                                       const std::vector<ConnectionProfile>* connections,
                                       const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Status Monitor", wxDefaultPosition, wxSize(800, 600)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
    BuildMenu();
    if (app_config_ && app_config_->chrome.monitoring.showIconBar) {
        IconBarType type = app_config_->chrome.monitoring.replicateIconBar
                               ? IconBarType::Main
                               : IconBarType::Monitoring;
        BuildIconBar(this, type, 24);
    }
    BuildLayout();
    PopulateConnections();
    UpdateControls();
    UpdateStatus("Ready");
    ApplyStatusDefaults(GetSelectedProfile(), false);

    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
}

void StatusMonitorFrame::BuildMenu() {
    // Child windows use minimal menu bar (File + Help only)
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void StatusMonitorFrame::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* topPanel = new wxPanel(this, wxID_ANY);
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Connection:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
    connection_choice_ = new wxChoice(topPanel, kConnectionChoiceId);
    topSizer->Add(connection_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connect_button_ = new wxButton(topPanel, wxID_ANY, "Connect");
    disconnect_button_ = new wxButton(topPanel, wxID_ANY, "Disconnect");
    topSizer->Add(connect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    topSizer->Add(disconnect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Type:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    status_type_choice_ = new wxChoice(topPanel, kStatusTypeChoiceId);
    status_type_choice_->Append("Server Info");
    status_type_choice_->Append("Connection Info");
    status_type_choice_->Append("Database Info");
    status_type_choice_->Append("Statistics");
    status_type_choice_->SetSelection(0);
    topSizer->Add(status_type_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Category:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    status_category_choice_ = new wxChoice(topPanel, wxID_ANY);
    status_category_choice_->Append("All");
    status_category_choice_->SetSelection(0);
    topSizer->Add(status_category_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    status_diff_check_ = new wxCheckBox(topPanel, wxID_ANY, "Diff");
    topSizer->Add(status_diff_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    fetch_button_ = new wxButton(topPanel, wxID_ANY, "Fetch");
    clear_button_ = new wxButton(topPanel, wxID_ANY, "Clear");
    copy_button_ = new wxButton(topPanel, wxID_ANY, "Copy JSON");
    save_button_ = new wxButton(topPanel, wxID_ANY, "Save JSON");
    topSizer->Add(fetch_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    topSizer->Add(clear_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    topSizer->Add(copy_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    topSizer->Add(save_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    auto* diffOptionsSizer = new wxBoxSizer(wxHORIZONTAL);
    diffOptionsSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Diff options:"), 0,
                          wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    status_diff_ignore_unchanged_check_ =
        new wxCheckBox(topPanel, wxID_ANY, "Ignore unchanged");
    status_diff_ignore_empty_check_ = new wxCheckBox(topPanel, wxID_ANY, "Ignore empty");
    status_diff_ignore_unchanged_check_->SetValue(true);
    status_diff_ignore_empty_check_->SetValue(true);
    diffOptionsSizer->Add(status_diff_ignore_unchanged_check_, 0,
                          wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    diffOptionsSizer->Add(status_diff_ignore_empty_check_, 0, wxALIGN_CENTER_VERTICAL);
    topSizer->Add(diffOptionsSizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    poll_check_ = new wxCheckBox(topPanel, wxID_ANY, "Auto-poll");
    poll_interval_ctrl_ = new wxSpinCtrl(topPanel, wxID_ANY);
    poll_interval_ctrl_->SetRange(250, 60000);
    poll_interval_ctrl_->SetValue(2000);
    topSizer->Add(poll_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Interval (ms):"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    topSizer->Add(poll_interval_ctrl_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    topSizer->AddStretchSpacer(1);
    status_label_ = new wxStaticText(topPanel, wxID_ANY, "Ready");
    topSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    topPanel->SetSizer(topSizer);
    root->Add(topPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    status_message_label_ = new wxStaticText(this, wxID_ANY, "Ready");
    root->Add(status_message_label_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    auto* statusBody = new wxBoxSizer(wxHORIZONTAL);
    auto* historyBox = new wxStaticBox(this, wxID_ANY, "History");
    auto* historySizer = new wxStaticBoxSizer(historyBox, wxVERTICAL);
    status_history_list_ = new wxListBox(historyBox, wxID_ANY);
    status_history_list_->SetMinSize(wxSize(180, -1));
    historySizer->Add(status_history_list_, 1, wxEXPAND | wxALL, 6);
    statusBody->Add(historySizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    status_cards_panel_ = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                               wxDefaultSize, wxVSCROLL);
    status_cards_panel_->SetScrollRate(0, 10);
    status_cards_sizer_ = new wxBoxSizer(wxVERTICAL);
    status_cards_panel_->SetSizer(status_cards_sizer_);
    statusBody->Add(status_cards_panel_, 1, wxEXPAND | wxALL, 6);
    root->Add(statusBody, 1, wxEXPAND);

    SetSizer(root);

    connect_button_->Bind(wxEVT_BUTTON, &StatusMonitorFrame::OnConnect, this);
    disconnect_button_->Bind(wxEVT_BUTTON, &StatusMonitorFrame::OnDisconnect, this);
    fetch_button_->Bind(wxEVT_BUTTON, &StatusMonitorFrame::OnFetch, this);
    clear_button_->Bind(wxEVT_BUTTON, &StatusMonitorFrame::OnClear, this);
    copy_button_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (!has_status_) {
            SetStatusMessage("No status data to copy");
            return;
        }
        std::string json = BuildStatusJson(last_status_, SelectedStatusCategory(),
                                           status_diff_check_ && status_diff_check_->GetValue());
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(json)));
            wxTheClipboard->Close();
            SetStatusMessage("Status JSON copied to clipboard");
        } else {
            SetStatusMessage("Unable to access clipboard");
        }
    });
    save_button_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (!has_status_) {
            SetStatusMessage("No status data to save");
            return;
        }
        wxFileDialog dialog(this, "Save Status JSON", "", "status.json",
                            "JSON files (*.json)|*.json|All files (*.*)|*.*",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dialog.ShowModal() != wxID_OK) {
            return;
        }
        std::ofstream out(dialog.GetPath().ToStdString());
        if (!out.is_open()) {
            SetStatusMessage("Failed to save status JSON");
            return;
        }
        out << BuildStatusJson(last_status_, SelectedStatusCategory(),
                               status_diff_check_ && status_diff_check_->GetValue());
        SetStatusMessage("Status JSON saved");
    });

    if (status_type_choice_) {
        status_type_choice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
            PersistStatusPreferences();
        });
    }
    if (status_category_choice_) {
        status_category_choice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
            if (has_status_) {
                BuildStatusCards(last_status_);
            }
            PersistStatusPreferences();
        });
    }
    if (status_diff_check_) {
        status_diff_check_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            UpdateDiffOptionControls();
            if (has_status_) {
                BuildStatusCards(last_status_);
            }
            PersistStatusPreferences();
        });
    }
    if (status_diff_ignore_unchanged_check_) {
        status_diff_ignore_unchanged_check_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            if (has_status_) {
                BuildStatusCards(last_status_);
            }
            PersistStatusPreferences();
        });
    }
    if (status_diff_ignore_empty_check_) {
        status_diff_ignore_empty_check_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            if (has_status_) {
                BuildStatusCards(last_status_);
            }
            PersistStatusPreferences();
        });
    }
    if (status_history_list_) {
        status_history_list_->Bind(wxEVT_LISTBOX, &StatusMonitorFrame::OnHistorySelection, this);
    }
    poll_check_->Bind(wxEVT_CHECKBOX, &StatusMonitorFrame::OnTogglePolling, this);
    connection_choice_->Bind(wxEVT_CHOICE, &StatusMonitorFrame::OnConnectionChanged, this);
    if (poll_interval_ctrl_) {
        poll_interval_ctrl_->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent&) {
            if (poll_timer_ && poll_check_ && poll_check_->GetValue()) {
                int interval = poll_interval_ctrl_->GetValue();
                if (interval <= 0) {
                    interval = 2000;
                }
                poll_timer_->Start(interval);
            }
            PersistStatusPreferences();
        });
    }

    poll_timer_ = new wxTimer(this);
    Bind(wxEVT_TIMER, &StatusMonitorFrame::OnPollTimer, this, poll_timer_->GetId());
    Bind(wxEVT_CLOSE_WINDOW, &StatusMonitorFrame::OnClose, this);
}

void StatusMonitorFrame::PopulateConnections() {
    if (!connection_choice_) {
        return;
    }
    connection_choice_->Clear();
    if (!connections_) {
        return;
    }
    for (const auto& profile : *connections_) {
        std::string label = profile.name.empty() ? profile.database : profile.name;
        if (label.empty()) {
            label = "(unnamed)";
        }
        connection_choice_->Append(label);
    }
    if (!connections_->empty()) {
        connection_choice_->SetSelection(0);
    }
}

const ConnectionProfile* StatusMonitorFrame::GetSelectedProfile() const {
    if (!connections_ || !connection_choice_) {
        return nullptr;
    }
    int selection = connection_choice_->GetSelection();
    if (selection < 0 || static_cast<size_t>(selection) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(selection)];
}

StatusRequestKind StatusMonitorFrame::SelectedRequestKind() const {
    if (!status_type_choice_) {
        return StatusRequestKind::ServerInfo;
    }
    switch (status_type_choice_->GetSelection()) {
        case 1: return StatusRequestKind::ConnectionInfo;
        case 2: return StatusRequestKind::DatabaseInfo;
        case 3: return StatusRequestKind::Statistics;
        default: return StatusRequestKind::ServerInfo;
    }
}

void StatusMonitorFrame::UpdateControls() {
    bool has_connections = connections_ && !connections_->empty();
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    BackendCapabilities caps = connection_manager_ ? connection_manager_->Capabilities()
                                                   : BackendCapabilities{};

    if (connection_choice_) {
        connection_choice_->Enable(has_connections && !connect_running_ && !fetch_pending_);
    }
    if (connect_button_) {
        connect_button_->Enable(has_connections && !connected && !connect_running_ && !fetch_pending_);
    }
    if (disconnect_button_) {
        disconnect_button_->Enable(connected && !connect_running_ && !fetch_pending_);
    }
    if (status_type_choice_) {
        status_type_choice_->Enable(connected && caps.supportsStatus && !fetch_pending_);
    }
    if (status_category_choice_) {
        status_category_choice_->Enable(connected && caps.supportsStatus);
    }
    if (fetch_button_) {
        fetch_button_->Enable(connected && caps.supportsStatus && !fetch_pending_);
    }
    if (clear_button_) {
        clear_button_->Enable(true);
    }
    if (copy_button_) {
        copy_button_->Enable(has_status_);
    }
    if (save_button_) {
        save_button_->Enable(has_status_);
    }
    if (status_diff_check_) {
        status_diff_check_->Enable(has_status_);
    }
    if (poll_check_) {
        poll_check_->Enable(connected && caps.supportsStatus);
    }
    if (poll_interval_ctrl_) {
        poll_interval_ctrl_->Enable(connected && caps.supportsStatus && !fetch_pending_);
    }
    if (poll_check_ && !connected) {
        poll_check_->SetValue(false);
        if (poll_timer_) {
            poll_timer_->Stop();
        }
    }
}

void StatusMonitorFrame::UpdateStatus(const std::string& message) {
    if (status_label_) {
        status_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void StatusMonitorFrame::DisplayStatusSnapshot(const StatusSnapshot& snapshot) {
    UpdateStatusCategoryChoices(snapshot);
    BuildStatusCards(snapshot);
}

void StatusMonitorFrame::SetStatusMessage(const std::string& message) {
    if (status_message_label_) {
        status_message_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void StatusMonitorFrame::ApplyStatusDefaults(const ConnectionProfile* profile, bool restartTimer) {
    if (poll_timer_) {
        poll_timer_->Stop();
    }
    if (!profile) {
        if (poll_check_) {
            poll_check_->SetValue(false);
        }
        if (poll_interval_ctrl_) {
            poll_interval_ctrl_->SetValue(2000);
        }
        if (status_type_choice_) {
            status_type_choice_->SetSelection(0);
        }
        if (status_category_choice_) {
            status_category_choice_->SetSelection(0);
        }
        if (status_diff_check_) {
            status_diff_check_->SetValue(false);
        }
        if (status_diff_ignore_unchanged_check_) {
            status_diff_ignore_unchanged_check_->SetValue(true);
        }
        if (status_diff_ignore_empty_check_) {
            status_diff_ignore_empty_check_->SetValue(true);
        }
        status_category_preference_ = "All";
        UpdateDiffOptionControls();
        return;
    }
    if (poll_check_) {
        poll_check_->SetValue(profile->statusAutoPollEnabled);
    }
    if (poll_interval_ctrl_) {
        int interval = profile->statusPollIntervalMs > 0 ? profile->statusPollIntervalMs : 2000;
        if (interval < 250) {
            interval = 250;
        }
        poll_interval_ctrl_->SetValue(interval);
    }
    if (status_type_choice_) {
        int selection = 0;
        switch (profile->statusDefaultKind) {
            case StatusRequestKind::ConnectionInfo: selection = 1; break;
            case StatusRequestKind::DatabaseInfo: selection = 2; break;
            case StatusRequestKind::Statistics: selection = 3; break;
            case StatusRequestKind::ServerInfo:
            default: selection = 0; break;
        }
        status_type_choice_->SetSelection(selection);
    }
    status_category_order_ = profile->statusCategoryOrder;
    status_category_preference_ = profile->statusCategoryFilter.empty()
                                      ? "All"
                                      : profile->statusCategoryFilter;
    if (status_category_choice_) {
        int restore = status_category_choice_->FindString(
            wxString::FromUTF8(status_category_preference_));
        status_category_choice_->SetSelection(restore == wxNOT_FOUND ? 0 : restore);
    }
    if (status_diff_check_) {
        status_diff_check_->SetValue(profile->statusDiffEnabled);
    }
    if (status_diff_ignore_unchanged_check_) {
        status_diff_ignore_unchanged_check_->SetValue(profile->statusDiffIgnoreUnchanged);
    }
    if (status_diff_ignore_empty_check_) {
        status_diff_ignore_empty_check_->SetValue(profile->statusDiffIgnoreEmpty);
    }
    UpdateDiffOptionControls();
    if (restartTimer && poll_check_ && poll_check_->GetValue()) {
        poll_timer_->Start(poll_interval_ctrl_ ? poll_interval_ctrl_->GetValue() : 2000);
    }
}

void StatusMonitorFrame::OnConnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        UpdateStatus("No connection profile selected");
        return;
    }
    if (connect_running_) {
        return;
    }
    connect_running_ = true;
    UpdateControls();
    UpdateStatus("Connecting...");

    connect_job_ = connection_manager_->ConnectAsync(*profile,
        [this](bool ok, const std::string& error) {
            CallAfter([this, ok, error]() {
                connect_running_ = false;
                if (ok) {
                    UpdateStatus("Connected");
                    ApplyStatusDefaults(GetSelectedProfile(), true);
                } else {
                    UpdateStatus("Connect failed");
                    SetStatusMessage(error.empty() ? "Connect failed" : error);
                }
                UpdateControls();
            });
        });
}

void StatusMonitorFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    ApplyStatusDefaults(nullptr, false);
    ClearStatusCards();
    has_status_ = false;
    previous_status_ = StatusSnapshot{};
    status_history_.clear();
    RefreshStatusHistory();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void StatusMonitorFrame::OnFetch(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    if (!connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    BackendCapabilities caps = connection_manager_->Capabilities();
    if (!caps.supportsStatus) {
        SetStatusMessage("Status not supported by backend");
        return;
    }
    fetch_pending_ = true;
    UpdateControls();
    auto kind = SelectedRequestKind();
    connection_manager_->FetchStatusAsync(
        kind,
        [this](bool ok, StatusSnapshot snapshot, const std::string& error) {
            CallAfter([this, ok, snapshot, error]() {
                fetch_pending_ = false;
                if (!ok) {
                    SetStatusMessage("Status error: " + error);
                    UpdateControls();
                    return;
                }
                if (has_status_) {
                    previous_status_ = last_status_;
                }
                last_status_ = snapshot;
                has_status_ = true;
                AddStatusHistory(snapshot);
                DisplayStatusSnapshot(snapshot);
                SetStatusMessage("Status updated");
                UpdateControls();
            });
        });
}

void StatusMonitorFrame::OnClear(wxCommandEvent&) {
    ClearStatusCards();
    has_status_ = false;
    previous_status_ = StatusSnapshot{};
    status_history_.clear();
    RefreshStatusHistory();
    SetStatusMessage("Status cleared");
    UpdateControls();
}

void StatusMonitorFrame::OnTogglePolling(wxCommandEvent&) {
    if (!poll_timer_ || !poll_check_ || !poll_interval_ctrl_) {
        return;
    }
    if (poll_check_->GetValue()) {
        int interval = poll_interval_ctrl_->GetValue();
        if (interval <= 0) {
            interval = 2000;
        }
        poll_interval_ctrl_->SetValue(interval);
        poll_timer_->Start(interval);
        SetStatusMessage("Auto-poll enabled (" + std::to_string(interval) + " ms)");
    } else {
        poll_timer_->Stop();
        SetStatusMessage("Auto-poll disabled");
    }
    PersistStatusPreferences();
}

void StatusMonitorFrame::OnPollTimer(wxTimerEvent&) {
    if (fetch_pending_) {
        return;
    }
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        return;
    }
    auto caps = connection_manager_->Capabilities();
    if (!caps.supportsStatus) {
        return;
    }
    fetch_pending_ = true;
    auto kind = SelectedRequestKind();
    connection_manager_->FetchStatusAsync(
        kind,
        [this](bool ok, StatusSnapshot snapshot, const std::string& error) {
            CallAfter([this, ok, snapshot, error]() {
                fetch_pending_ = false;
                if (!ok) {
                    SetStatusMessage("Status error: " + error);
                    return;
                }
                if (has_status_) {
                    previous_status_ = last_status_;
                }
                last_status_ = snapshot;
                has_status_ = true;
                AddStatusHistory(snapshot);
                DisplayStatusSnapshot(snapshot);
            });
        });
}

void StatusMonitorFrame::OnConnectionChanged(wxCommandEvent&) {
    ApplyStatusDefaults(GetSelectedProfile(), false);
    ClearStatusCards();
    has_status_ = false;
    previous_status_ = StatusSnapshot{};
    status_history_.clear();
    RefreshStatusHistory();
    UpdateControls();
}

void StatusMonitorFrame::OnHistorySelection(wxCommandEvent&) {
    if (!status_history_list_) {
        return;
    }
    int selection = status_history_list_->GetSelection();
    if (selection < 0) {
        return;
    }
    ShowHistorySnapshot(static_cast<size_t>(selection));
    SetStatusMessage("Status history selected");
}

void StatusMonitorFrame::OnClose(wxCloseEvent& event) {
    if (poll_timer_) {
        poll_timer_->Stop();
    }
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
    event.Skip();
}

std::string StatusMonitorFrame::SelectedStatusCategory() const {
    if (!status_category_choice_) {
        return "All";
    }
    return status_category_choice_->GetStringSelection().ToStdString();
}

void StatusMonitorFrame::UpdateStatusCategoryChoices(const StatusSnapshot& snapshot) {
    if (!status_category_choice_) {
        return;
    }
    std::string previous = status_category_preference_.empty()
                               ? status_category_choice_->GetStringSelection().ToStdString()
                               : status_category_preference_;
    status_category_choice_->Clear();
    status_category_choice_->Append("All");

    std::map<std::string, bool> seen;
    auto add_category = [&](const std::string& name) {
        if (name.empty() || seen[name]) {
            return;
        }
        seen[name] = true;
        status_category_choice_->Append(wxString::FromUTF8(name));
    };

    for (const auto& category : status_category_order_) {
        if (category == "Request") {
            continue;
        }
        add_category(category);
    }
    for (const auto& entry : snapshot.entries) {
        std::string category = "General";
        size_t delim = entry.key.find_first_of(".:");
        if (delim != std::string::npos) {
            category = entry.key.substr(0, delim);
        }
        add_category(category);
    }

    int restore = status_category_choice_->FindString(wxString::FromUTF8(previous));
    if (restore == wxNOT_FOUND) {
        status_category_choice_->SetSelection(0);
    } else {
        status_category_choice_->SetSelection(restore);
    }
}

void StatusMonitorFrame::AddStatusHistory(const StatusSnapshot& snapshot) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char time_buf[32] = {0};
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
    std::string label = std::string(time_buf) + " | " + StatusRequestKindToString(snapshot.kind);
    status_history_.push_back({label, snapshot});
    if (status_history_.size() > status_history_limit_) {
        status_history_.erase(status_history_.begin(),
                              status_history_.begin() +
                              static_cast<long>(status_history_.size() - status_history_limit_));
    }
    RefreshStatusHistory();
    if (status_history_list_ && !status_history_.empty()) {
        status_history_list_->SetSelection(static_cast<int>(status_history_.size() - 1));
    }
}

void StatusMonitorFrame::RefreshStatusHistory() {
    if (!status_history_list_) {
        return;
    }
    status_history_list_->Clear();
    for (const auto& entry : status_history_) {
        status_history_list_->Append(wxString::FromUTF8(entry.label));
    }
}

void StatusMonitorFrame::ShowHistorySnapshot(size_t index) {
    if (index >= status_history_.size()) {
        return;
    }
    if (index > 0) {
        previous_status_ = status_history_[index - 1].snapshot;
    } else {
        previous_status_ = StatusSnapshot{};
    }
    last_status_ = status_history_[index].snapshot;
    has_status_ = true;
    DisplayStatusSnapshot(last_status_);
}

void StatusMonitorFrame::PersistStatusPreferences() {
    if (!connections_ || !connection_choice_) {
        return;
    }
    auto* editable_connections = const_cast<std::vector<ConnectionProfile>*>(connections_);
    if (!editable_connections) {
        return;
    }
    int selection = connection_choice_->GetSelection();
    if (selection < 0 || static_cast<size_t>(selection) >= editable_connections->size()) {
        return;
    }
    ConnectionProfile& profile = (*editable_connections)[static_cast<size_t>(selection)];
    status_category_preference_ = SelectedStatusCategory();
    profile.statusCategoryFilter =
        (status_category_preference_ == "All") ? "" : status_category_preference_;
    profile.statusDiffEnabled = status_diff_check_ && status_diff_check_->GetValue();
    profile.statusDiffIgnoreUnchanged = status_diff_ignore_unchanged_check_
                                            ? status_diff_ignore_unchanged_check_->GetValue()
                                            : true;
    profile.statusDiffIgnoreEmpty = status_diff_ignore_empty_check_
                                        ? status_diff_ignore_empty_check_->GetValue()
                                        : true;
    if (poll_check_) {
        profile.statusAutoPollEnabled = poll_check_->GetValue();
    }
    if (poll_interval_ctrl_) {
        profile.statusPollIntervalMs = poll_interval_ctrl_->GetValue();
    }
    if (status_type_choice_) {
        switch (status_type_choice_->GetSelection()) {
            case 1: profile.statusDefaultKind = StatusRequestKind::ConnectionInfo; break;
            case 2: profile.statusDefaultKind = StatusRequestKind::DatabaseInfo; break;
            case 3: profile.statusDefaultKind = StatusRequestKind::Statistics; break;
            default: profile.statusDefaultKind = StatusRequestKind::ServerInfo; break;
        }
    }

    ConfigStore store;
    wxFileName config_root(wxStandardPaths::Get().GetUserConfigDir(), "");
    config_root.AppendDir("scratchrobin");
    if (!config_root.DirExists()) {
        config_root.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    wxFileName connection_path(config_root);
    connection_path.SetFullName("connections.toml");
    store.SaveConnections(connection_path.GetFullPath().ToStdString(), *editable_connections);
}

void StatusMonitorFrame::UpdateDiffOptionControls() {
    bool enabled = status_diff_check_ && status_diff_check_->GetValue();
    if (status_diff_ignore_unchanged_check_) {
        status_diff_ignore_unchanged_check_->Enable(enabled);
    }
    if (status_diff_ignore_empty_check_) {
        status_diff_ignore_empty_check_->Enable(enabled);
    }
}

std::string StatusMonitorFrame::BuildStatusJson(const StatusSnapshot& snapshot,
                                                const std::string& category,
                                                bool diff_only) const {
    auto escape = [](const std::string& input) {
        std::string out;
        out.reserve(input.size() + 8);
        for (char c : input) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"': out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += c; break;
            }
        }
        return out;
    };

    auto category_of = [](const std::string& key) {
        size_t delim = key.find_first_of(".:");
        if (delim == std::string::npos) {
            return std::string("General");
        }
        return key.substr(0, delim);
    };

    std::map<std::string, std::string> prev_map;
    if (diff_only) {
        for (const auto& entry : previous_status_.entries) {
            prev_map[entry.key] = entry.value;
        }
    }
    bool ignore_unchanged = status_diff_ignore_unchanged_check_
                                ? status_diff_ignore_unchanged_check_->GetValue()
                                : true;
    bool ignore_empty = status_diff_ignore_empty_check_
                            ? status_diff_ignore_empty_check_->GetValue()
                            : true;

    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"request_type\": \"" << escape(StatusRequestKindToString(snapshot.kind)) << "\",\n";
    if (diff_only) {
        oss << "  \"diff\": [\n";
        bool first = true;
        for (const auto& entry : snapshot.entries) {
            if (!category.empty() && category != "All" && category_of(entry.key) != category) {
                continue;
            }
            auto it = prev_map.find(entry.key);
            std::string old_value = it == prev_map.end() ? "" : it->second;
            if (ignore_empty && entry.value.empty() && old_value.empty()) {
                continue;
            }
            if (ignore_unchanged && it != prev_map.end() && old_value == entry.value) {
                continue;
            }
            if (!first) {
                oss << ",\n";
            }
            first = false;
            oss << "    {\"key\": \"" << escape(entry.key) << "\", \"old\": \""
                << escape(old_value) << "\", \"new\": \"" << escape(entry.value) << "\"}";
        }
        oss << "\n  ]\n";
    } else {
        oss << "  \"entries\": [\n";
        bool first = true;
        for (const auto& entry : snapshot.entries) {
            if (!category.empty() && category != "All" && category_of(entry.key) != category) {
                continue;
            }
            if (!first) {
                oss << ",\n";
            }
            first = false;
            oss << "    {\"key\": \"" << escape(entry.key) << "\", \"value\": \""
                << escape(entry.value) << "\"}";
        }
        oss << "\n  ]\n";
    }
    oss << "}\n";
    return oss.str();
}

void StatusMonitorFrame::ClearStatusCards() {
    if (!status_cards_panel_ || !status_cards_sizer_) {
        return;
    }
    status_cards_panel_->Freeze();
    status_cards_sizer_->Clear(true);
    status_cards_panel_->Layout();
    status_cards_panel_->FitInside();
    status_cards_panel_->Thaw();
}

void StatusMonitorFrame::BuildStatusCards(const StatusSnapshot& snapshot) {
    if (!status_cards_panel_ || !status_cards_sizer_) {
        return;
    }
    status_cards_panel_->Freeze();
    status_cards_sizer_->Clear(true);

    auto category_of = [](const std::string& key) {
        size_t delim = key.find_first_of(".:");
        if (delim == std::string::npos) {
            return std::string("General");
        }
        return key.substr(0, delim);
    };

    bool diff_only = status_diff_check_ && status_diff_check_->GetValue();
    bool ignore_unchanged = status_diff_ignore_unchanged_check_
                                ? status_diff_ignore_unchanged_check_->GetValue()
                                : true;
    bool ignore_empty = status_diff_ignore_empty_check_
                            ? status_diff_ignore_empty_check_->GetValue()
                            : true;
    std::string filter_category = SelectedStatusCategory();
    std::map<std::string, std::string> prev_map;
    if (diff_only) {
        for (const auto& entry : previous_status_.entries) {
            prev_map[entry.key] = entry.value;
        }
    }

    std::map<std::string, std::vector<StatusEntry>> grouped;
    grouped["Request"].push_back({"Type", StatusRequestKindToString(snapshot.kind)});
    for (const auto& entry : snapshot.entries) {
        std::string category = category_of(entry.key);
        if (!filter_category.empty() && filter_category != "All" && filter_category != category) {
            continue;
        }
        std::string key = entry.key;
        size_t delim = key.find_first_of(".:");
        if (delim != std::string::npos) {
            key = key.substr(delim + 1);
        }
        if (diff_only) {
            auto it = prev_map.find(entry.key);
            std::string old_value = it == prev_map.end() ? "" : it->second;
            if (ignore_empty && entry.value.empty() && old_value.empty()) {
                continue;
            }
            if (ignore_unchanged && it != prev_map.end() && old_value == entry.value) {
                continue;
            }
            grouped["Changes"].push_back({key, old_value + " → " + entry.value});
        } else {
            grouped[category].push_back({key, entry.value});
        }
    }

    std::vector<std::string> ordered_categories;
    if (diff_only) {
        ordered_categories.push_back("Request");
        if (!grouped["Changes"].empty()) {
            ordered_categories.push_back("Changes");
        }
    } else {
        if (!status_category_order_.empty()) {
            ordered_categories = status_category_order_;
        }
        if (std::find(ordered_categories.begin(), ordered_categories.end(), "Request") ==
            ordered_categories.end()) {
            ordered_categories.insert(ordered_categories.begin(), "Request");
        }
        for (const auto& group : grouped) {
            if (std::find(ordered_categories.begin(), ordered_categories.end(), group.first) ==
                ordered_categories.end()) {
                ordered_categories.push_back(group.first);
            }
        }
    }

    for (const auto& category : ordered_categories) {
        auto it = grouped.find(category);
        if (it == grouped.end() || it->second.empty()) {
            continue;
        }
        const auto& group = *it;
        auto* box = new wxStaticBox(status_cards_panel_, wxID_ANY, wxString::FromUTF8(group.first));
        auto* boxSizer = new wxStaticBoxSizer(box, wxVERTICAL);
        auto* grid = new wxFlexGridSizer(2, 6, 12);
        grid->AddGrowableCol(1);
        for (const auto& entry : group.second) {
            grid->Add(new wxStaticText(box, wxID_ANY, wxString::FromUTF8(entry.key)), 0,
                      wxALIGN_CENTER_VERTICAL);
            grid->Add(new wxStaticText(box, wxID_ANY, wxString::FromUTF8(entry.value)), 0,
                      wxALIGN_CENTER_VERTICAL);
        }
        boxSizer->Add(grid, 1, wxEXPAND | wxALL, 8);
        status_cards_sizer_->Add(boxSizer, 0, wxEXPAND | wxALL, 6);
    }

    status_cards_panel_->Layout();
    status_cards_panel_->FitInside();
    status_cards_panel_->Thaw();
}

} // namespace scratchrobin
