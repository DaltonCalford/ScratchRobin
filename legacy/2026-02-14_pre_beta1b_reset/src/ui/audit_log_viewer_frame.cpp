/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "audit_log_viewer_frame.h"

#include "core/config.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

constexpr int kMenuConnect = wxID_HIGHEST + 2560;
constexpr int kMenuDisconnect = wxID_HIGHEST + 2561;
constexpr int kMenuRefresh = wxID_HIGHEST + 2562;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 2563;

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string NormalizeBackendName(const std::string& raw) {
    std::string value = ToLowerCopy(Trim(raw));
    if (value.empty() || value == "network" || value == "scratchbird") {
        return "native";
    }
    if (value == "postgres" || value == "pg") {
        return "postgresql";
    }
    if (value == "mariadb") {
        return "mysql";
    }
    if (value == "fb") {
        return "firebird";
    }
    return value;
}

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

std::string FormatResult(const QueryResult& result) {
    if (result.rows.empty()) {
        return "No audit log rows returned.";
    }
    std::ostringstream out;
    for (const auto& col : result.columns) {
        out << col.name << "\t";
    }
    out << "\n";
    for (const auto& row : result.rows) {
        for (const auto& cell : row) {
            out << cell.text << "\t";
        }
        out << "\n";
    }
    return out.str();
}

} // namespace

wxBEGIN_EVENT_TABLE(AuditLogViewerFrame, wxFrame)
    EVT_BUTTON(kMenuConnect, AuditLogViewerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, AuditLogViewerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, AuditLogViewerFrame::OnRefresh)
    EVT_CLOSE(AuditLogViewerFrame::OnClose)
wxEND_EVENT_TABLE()

AuditLogViewerFrame::AuditLogViewerFrame(WindowManager* windowManager,
                                         ConnectionManager* connectionManager,
                                         const std::vector<ConnectionProfile>* connections,
                                         const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Audit Log Viewer",
              wxDefaultPosition, wxSize(980, 680)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
    BuildMenu();
    BuildLayout();
    PopulateConnections();
    UpdateStatus("Idle");
    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
}

AuditLogViewerFrame::~AuditLogViewerFrame() {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
}

void AuditLogViewerFrame::BuildMenu() {
    auto* menu_bar = new wxMenuBar();
    auto* file_menu = new wxMenu();
    file_menu->Append(wxID_CLOSE, "&Close\tCtrl+W");
    menu_bar->Append(file_menu, "&File");
    SetMenuBar(menu_bar);
}

void AuditLogViewerFrame::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* topPanel = new wxPanel(this, wxID_ANY);
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Connection:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connection_choice_ = new wxChoice(topPanel, kConnectionChoiceId);
    topSizer->Add(connection_choice_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connect_button_ = new wxButton(topPanel, kMenuConnect, "Connect");
    topSizer->Add(connect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    disconnect_button_ = new wxButton(topPanel, kMenuDisconnect, "Disconnect");
    topSizer->Add(disconnect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    refresh_button_ = new wxButton(topPanel, kMenuRefresh, "Refresh");
    topSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL);
    topPanel->SetSizer(topSizer);
    root->Add(topPanel, 0, wxEXPAND | wxALL, 8);

    output_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxTE_READONLY);
    root->Add(output_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* statusPanel = new wxPanel(this, wxID_ANY);
    auto* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    status_label_ = new wxStaticText(statusPanel, wxID_ANY, "Status: Idle");
    message_label_ = new wxStaticText(statusPanel, wxID_ANY, "");
    statusSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    statusSizer->Add(message_label_, 1, wxALIGN_CENTER_VERTICAL);
    statusPanel->SetSizer(statusSizer);
    root->Add(statusPanel, 0, wxEXPAND | wxALL, 6);

    SetSizer(root);
}

void AuditLogViewerFrame::PopulateConnections() {
    if (!connection_choice_ || !connections_) {
        return;
    }
    connection_choice_->Clear();
    for (const auto& profile : *connections_) {
        connection_choice_->Append(ProfileLabel(profile));
    }
    if (!connections_->empty()) {
        connection_choice_->SetSelection(0);
    }
}

void AuditLogViewerFrame::UpdateStatus(const wxString& status) {
    if (status_label_) {
        status_label_->SetLabel("Status: " + status);
    }
}

void AuditLogViewerFrame::SetMessage(const std::string& message) {
    if (message_label_) {
        message_label_->SetLabel(message);
    }
}

const ConnectionProfile* AuditLogViewerFrame::GetSelectedProfile() const {
    if (!connections_ || !connection_choice_) {
        return nullptr;
    }
    int index = connection_choice_->GetSelection();
    if (index < 0 || static_cast<size_t>(index) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(index)];
}

bool AuditLogViewerFrame::EnsureConnected(const ConnectionProfile& profile) {
    if (!connection_manager_) {
        return false;
    }
    if (connection_manager_->IsConnected()) {
        return true;
    }
    return connection_manager_->Connect(profile);
}

std::string AuditLogViewerFrame::BuildQuery() const {
    return "SELECT event_time, category, event_code, actor, detail\n"
           "FROM sys.audit_log\n"
           "ORDER BY event_time DESC\n"
           "LIMIT 200;";
}

void AuditLogViewerFrame::RefreshLog() {
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        SetMessage("Select a connection profile first.");
        return;
    }
    if (!EnsureConnected(*profile)) {
        SetMessage(connection_manager_ ? connection_manager_->LastError() : "Connection failed.");
        return;
    }
    if (NormalizeBackendName(profile->backend) != "native") {
        SetMessage("Audit log queries are supported for ScratchBird connections.");
        return;
    }
    UpdateStatus("Loading...");
    std::string sql = BuildQuery();
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                if (!ok) {
                    UpdateStatus("Load failed");
                    SetMessage(error.empty() ? "Failed to load audit log." : error);
                    return;
                }
                if (output_ctrl_) {
                    output_ctrl_->SetValue(FormatResult(result));
                }
                UpdateStatus("Updated");
                SetMessage("");
            });
        });
}

void AuditLogViewerFrame::OnConnect(wxCommandEvent&) {
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        SetMessage("Select a connection profile first.");
        return;
    }
    if (!EnsureConnected(*profile)) {
        SetMessage(connection_manager_ ? connection_manager_->LastError() : "Connection failed.");
        return;
    }
    UpdateStatus("Connected");
    RefreshLog();
}

void AuditLogViewerFrame::OnDisconnect(wxCommandEvent&) {
    if (connection_manager_) {
        connection_manager_->Disconnect();
    }
    UpdateStatus("Disconnected");
}

void AuditLogViewerFrame::OnRefresh(wxCommandEvent&) {
    RefreshLog();
}

void AuditLogViewerFrame::OnClose(wxCloseEvent& event) {
    Destroy();
    event.Skip(false);
}

} // namespace scratchrobin
