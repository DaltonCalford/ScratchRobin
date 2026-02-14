/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "lockout_policy_frame.h"

#include "core/config.h"
#include "sql_editor_frame.h"
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

constexpr int kMenuConnect = wxID_HIGHEST + 2500;
constexpr int kMenuDisconnect = wxID_HIGHEST + 2501;
constexpr int kMenuRefresh = wxID_HIGHEST + 2502;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 2503;

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
        return "No lockout policy rows returned.";
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

wxBEGIN_EVENT_TABLE(LockoutPolicyFrame, wxFrame)
    EVT_BUTTON(kMenuConnect, LockoutPolicyFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, LockoutPolicyFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, LockoutPolicyFrame::OnRefresh)
    EVT_CLOSE(LockoutPolicyFrame::OnClose)
wxEND_EVENT_TABLE()

LockoutPolicyFrame::LockoutPolicyFrame(WindowManager* windowManager,
                                       ConnectionManager* connectionManager,
                                       const std::vector<ConnectionProfile>* connections,
                                       const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Login Lockout Policy",
              wxDefaultPosition, wxSize(900, 640)),
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

LockoutPolicyFrame::~LockoutPolicyFrame() {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
}

void LockoutPolicyFrame::BuildMenu() {
    auto* menu_bar = new wxMenuBar();
    auto* file_menu = new wxMenu();
    file_menu->Append(wxID_CLOSE, "&Close\tCtrl+W");
    menu_bar->Append(file_menu, "&File");
    SetMenuBar(menu_bar);
}

void LockoutPolicyFrame::BuildLayout() {
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

void LockoutPolicyFrame::PopulateConnections() {
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

void LockoutPolicyFrame::UpdateStatus(const wxString& status) {
    if (status_label_) {
        status_label_->SetLabel("Status: " + status);
    }
}

void LockoutPolicyFrame::SetMessage(const std::string& message) {
    if (message_label_) {
        message_label_->SetLabel(message);
    }
}

const ConnectionProfile* LockoutPolicyFrame::GetSelectedProfile() const {
    if (!connections_ || !connection_choice_) {
        return nullptr;
    }
    int index = connection_choice_->GetSelection();
    if (index < 0 || static_cast<size_t>(index) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(index)];
}

bool LockoutPolicyFrame::EnsureConnected(const ConnectionProfile& profile) {
    if (!connection_manager_) {
        return false;
    }
    if (connection_manager_->IsConnected()) {
        return true;
    }
    return connection_manager_->Connect(profile);
}

std::string LockoutPolicyFrame::BuildQuery() const {
    return "SELECT key, value\n"
           "FROM sys.security_settings\n"
           "WHERE key LIKE 'lockout.%'\n"
           "ORDER BY key;";
}

void LockoutPolicyFrame::RefreshPolicy() {
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
        SetMessage("Lockout policy queries are supported for ScratchBird connections.");
        return;
    }
    UpdateStatus("Loading...");
    std::string sql = BuildQuery();
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                if (!ok) {
                    UpdateStatus("Load failed");
                    SetMessage(error.empty() ? "Failed to load policy." : error);
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

void LockoutPolicyFrame::OnConnect(wxCommandEvent&) {
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
    RefreshPolicy();
}

void LockoutPolicyFrame::OnDisconnect(wxCommandEvent&) {
    if (connection_manager_) {
        connection_manager_->Disconnect();
    }
    UpdateStatus("Disconnected");
}

void LockoutPolicyFrame::OnRefresh(wxCommandEvent&) {
    RefreshPolicy();
}

void LockoutPolicyFrame::OnClose(wxCloseEvent& event) {
    Destroy();
    event.Skip(false);
}

} // namespace scratchrobin
