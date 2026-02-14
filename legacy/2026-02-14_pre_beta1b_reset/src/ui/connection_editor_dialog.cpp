/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "connection_editor_dialog.h"

#include "core/connection_manager.h"
#include "core/network_backend.h"

#include <algorithm>
#include <cctype>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {

namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

constexpr int kBackendScratchBird = 0;
constexpr int kBackendPostgreSQL = 1;
constexpr int kBackendMySQL = 2;
constexpr int kBackendFirebird = 3;
constexpr int kBackendMock = 4;

constexpr int kConnectionModeNetwork = 0;
constexpr int kConnectionModeIpc = 1;
constexpr int kConnectionModeEmbedded = 2;

constexpr int kSSLModePrefer = 0;
constexpr int kSSLModeRequire = 1;
constexpr int kSSLModeDisable = 2;
constexpr int kSSLModeVerifyCA = 3;
constexpr int kSSLModeVerifyFull = 4;

int GetDefaultPortForBackend(int backend) {
    switch (backend) {
        case kBackendScratchBird: return 3092;
        case kBackendPostgreSQL: return 5432;
        case kBackendMySQL: return 3306;
        case kBackendFirebird: return 3050;
        default: return 0;
    }
}

const char* GetBackendName(int backend) {
    switch (backend) {
        case kBackendScratchBird: return "native";
        case kBackendPostgreSQL: return "postgresql";
        case kBackendMySQL: return "mysql";
        case kBackendFirebird: return "firebird";
        case kBackendMock: return "mock";
        default: return "";
    }
}

int GetBackendFromName(const std::string& name) {
    if (name == "native" || name == "scratchbird" || name == "network") return kBackendScratchBird;
    if (name == "postgresql" || name == "postgres" || name == "pg") return kBackendPostgreSQL;
    if (name == "mysql" || name == "mariadb") return kBackendMySQL;
    if (name == "firebird" || name == "fb") return kBackendFirebird;
    if (name == "mock") return kBackendMock;
    return kBackendScratchBird;
}

std::string SSLModeToString(int mode) {
    switch (mode) {
        case kSSLModePrefer: return "prefer";
        case kSSLModeRequire: return "require";
        case kSSLModeDisable: return "disable";
        case kSSLModeVerifyCA: return "verify-ca";
        case kSSLModeVerifyFull: return "verify-full";
        default: return "prefer";
    }
}

int SSLModeFromString(const std::string& mode) {
    if (mode == "prefer") return kSSLModePrefer;
    if (mode == "require") return kSSLModeRequire;
    if (mode == "disable") return kSSLModeDisable;
    if (mode == "verify-ca") return kSSLModeVerifyCA;
    if (mode == "verify-full") return kSSLModeVerifyFull;
    return kSSLModePrefer;
}

} // namespace

// =============================================================================
// ConnectionEditorDialog
// =============================================================================

wxBEGIN_EVENT_TABLE(ConnectionEditorDialog, wxDialog)
    EVT_CHOICE(wxID_ANY, ConnectionEditorDialog::OnBackendChanged)
    EVT_BUTTON(wxID_HIGHEST + 100, ConnectionEditorDialog::OnTestConnection)
    EVT_BUTTON(wxID_HIGHEST + 101, ConnectionEditorDialog::OnBrowseSSLRootCert)
    EVT_BUTTON(wxID_HIGHEST + 102, ConnectionEditorDialog::OnBrowseSSLCert)
    EVT_BUTTON(wxID_HIGHEST + 103, ConnectionEditorDialog::OnBrowseSSLKey)
wxEND_EVENT_TABLE()

ConnectionEditorDialog::ConnectionEditorDialog(wxWindow* parent,
                                               ConnectionEditorMode mode,
                                               const ConnectionProfile* existingProfile)
    : wxDialog(parent, wxID_ANY, 
               mode == ConnectionEditorMode::Create ? "New Connection" : 
               mode == ConnectionEditorMode::Edit ? "Edit Connection" : "Duplicate Connection",
               wxDefaultPosition, wxSize(550, 550)),
      mode_(mode) {
    BuildLayout();
    
    if (existingProfile) {
        LoadProfile(*existingProfile);
        if (mode == ConnectionEditorMode::Duplicate) {
            // Append " (Copy)" to name
            if (name_ctrl_) {
                wxString name = name_ctrl_->GetValue();
                name_ctrl_->SetValue(name + " (Copy)");
            }
        }
    } else {
        // Set default port for ScratchBird
        if (port_ctrl_) {
            port_ctrl_->SetValue("3092");
        }
    }
    
    UpdateFieldStates();
}

void ConnectionEditorDialog::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    
    // Notebook for tabs
    auto* notebook = new wxNotebook(this, wxID_ANY);
    
    BuildGeneralTab(notebook);
    BuildSSLTab(notebook);
    BuildAdvancedTab(notebook);
    
    rootSizer->Add(notebook, 1, wxEXPAND | wxALL, 10);
    
    // Test connection button
    auto* testSizer = new wxBoxSizer(wxHORIZONTAL);
    test_button_ = new wxButton(this, wxID_HIGHEST + 100, "Test Connection");
    testSizer->Add(test_button_, 0, wxRIGHT, 10);
    test_result_label_ = new wxStaticText(this, wxID_ANY, "");
    testSizer->Add(test_result_label_, 0, wxALIGN_CENTER_VERTICAL);
    rootSizer->Add(testSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Standard dialog buttons
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    rootSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    SetSizer(rootSizer);
    
    // Set OK as default
    SetAffirmativeId(wxID_OK);
}

void ConnectionEditorDialog::BuildGeneralTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* sizer = new wxFlexGridSizer(2, 5, 10);
    sizer->AddGrowableCol(1);
    
    // Connection name
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Connection Name:"), 0, wxALIGN_CENTER_VERTICAL);
    name_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    name_ctrl_->SetHint("My Database Connection");
    sizer->Add(name_ctrl_, 1, wxEXPAND);
    
    // Backend type
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Backend:"), 0, wxALIGN_CENTER_VERTICAL);
    backend_choice_ = new wxChoice(panel, wxID_ANY);
    backend_choice_->Append("ScratchBird (Native)");
    backend_choice_->Append("PostgreSQL");
    backend_choice_->Append("MySQL / MariaDB");
    backend_choice_->Append("Firebird");
    backend_choice_->Append("Mock (Offline Testing)");
    backend_choice_->SetSelection(0);
    sizer->Add(backend_choice_, 1, wxEXPAND);
    
    // Connection Mode (only for ScratchBird)
    connection_mode_label_ = new wxStaticText(panel, wxID_ANY, "Connection Mode:");
    sizer->Add(connection_mode_label_, 0, wxALIGN_CENTER_VERTICAL);
    connection_mode_choice_ = new wxChoice(panel, wxID_ANY);
    connection_mode_choice_->Append("Network (TCP/IP)");
    connection_mode_choice_->Append("IPC (Unix Socket/Pipe)");
    connection_mode_choice_->Append("Embedded (In-Process)");
    connection_mode_choice_->SetSelection(kConnectionModeNetwork);
    sizer->Add(connection_mode_choice_, 1, wxEXPAND);
    
    // IPC Path (only shown for IPC mode)
    ipc_path_label_ = new wxStaticText(panel, wxID_ANY, "Socket Path:");
    sizer->Add(ipc_path_label_, 0, wxALIGN_CENTER_VERTICAL);
    ipc_path_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    ipc_path_ctrl_->SetHint("/var/run/scratchbird/mydb.sock (optional)");
    sizer->Add(ipc_path_ctrl_, 1, wxEXPAND);
    
    // Host
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Host:"), 0, wxALIGN_CENTER_VERTICAL);
    host_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    host_ctrl_->SetHint("localhost");
    sizer->Add(host_ctrl_, 1, wxEXPAND);
    
    // Port
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Port:"), 0, wxALIGN_CENTER_VERTICAL);
    port_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    sizer->Add(port_ctrl_, 1, wxEXPAND);
    
    // Database
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Database:"), 0, wxALIGN_CENTER_VERTICAL);
    database_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    database_ctrl_->SetHint("Database name or path");
    sizer->Add(database_ctrl_, 1, wxEXPAND);
    
    // Username
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Username:"), 0, wxALIGN_CENTER_VERTICAL);
    username_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    sizer->Add(username_ctrl_, 1, wxEXPAND);
    
    // Password
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Password:"), 0, wxALIGN_CENTER_VERTICAL);
    password_ctrl_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, 
                                    wxDefaultSize, wxTE_PASSWORD);
    sizer->Add(password_ctrl_, 1, wxEXPAND);
    
    // Save password checkbox
    sizer->AddSpacer(0);
    save_password_ctrl_ = new wxCheckBox(panel, wxID_ANY, "Save password in keychain");
    save_password_ctrl_->SetValue(true);
    sizer->Add(save_password_ctrl_, 0, wxEXPAND);
    
    // Application name (optional)
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Application Name:"), 0, wxALIGN_CENTER_VERTICAL);
    application_name_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    application_name_ctrl_->SetHint("ScratchRobin (optional)");
    sizer->Add(application_name_ctrl_, 1, wxEXPAND);
    
    // Role (optional, mainly for Firebird/PostgreSQL)
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Role:"), 0, wxALIGN_CENTER_VERTICAL);
    role_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    role_ctrl_->SetHint("Database role (optional)");
    sizer->Add(role_ctrl_, 1, wxEXPAND);
    
    auto* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(sizer, 1, wxEXPAND | wxALL, 15);
    panel->SetSizer(outerSizer);
    
    notebook->AddPage(panel, "General");
}

void ConnectionEditorDialog::BuildSSLTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* sizer = new wxFlexGridSizer(3, 5, 10);
    sizer->AddGrowableCol(1);
    
    // SSL Mode
    sizer->Add(new wxStaticText(panel, wxID_ANY, "SSL Mode:"), 0, wxALIGN_CENTER_VERTICAL);
    ssl_mode_choice_ = new wxChoice(panel, wxID_ANY);
    ssl_mode_choice_->Append("Prefer (default)");
    ssl_mode_choice_->Append("Require");
    ssl_mode_choice_->Append("Disable");
    ssl_mode_choice_->Append("Verify CA");
    ssl_mode_choice_->Append("Verify Full");
    ssl_mode_choice_->SetSelection(0);
    sizer->Add(ssl_mode_choice_, 1, wxEXPAND);
    sizer->AddSpacer(0);
    
    // Root certificate
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Root CA Certificate:"), 0, wxALIGN_CENTER_VERTICAL);
    ssl_root_cert_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    ssl_root_cert_ctrl_->SetHint("/path/to/ca.crt (optional)");
    sizer->Add(ssl_root_cert_ctrl_, 1, wxEXPAND);
    ssl_root_cert_browse_btn_ = new wxButton(panel, wxID_HIGHEST + 101, "Browse...");
    sizer->Add(ssl_root_cert_browse_btn_, 0);
    
    // Client certificate
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Client Certificate:"), 0, wxALIGN_CENTER_VERTICAL);
    ssl_cert_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    ssl_cert_ctrl_->SetHint("/path/to/client.crt (optional)");
    sizer->Add(ssl_cert_ctrl_, 1, wxEXPAND);
    ssl_cert_browse_btn_ = new wxButton(panel, wxID_HIGHEST + 102, "Browse...");
    sizer->Add(ssl_cert_browse_btn_, 0);
    
    // Client key
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Client Key:"), 0, wxALIGN_CENTER_VERTICAL);
    ssl_key_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    ssl_key_ctrl_->SetHint("/path/to/client.key (optional)");
    sizer->Add(ssl_key_ctrl_, 1, wxEXPAND);
    ssl_key_browse_btn_ = new wxButton(panel, wxID_HIGHEST + 103, "Browse...");
    sizer->Add(ssl_key_browse_btn_, 0);
    
    // SSL Key password
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Key Password:"), 0, wxALIGN_CENTER_VERTICAL);
    ssl_password_ctrl_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                        wxDefaultSize, wxTE_PASSWORD);
    sizer->Add(ssl_password_ctrl_, 1, wxEXPAND);
    sizer->AddSpacer(0);
    
    auto* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(sizer, 1, wxEXPAND | wxALL, 15);
    
    // Help text
    auto* helpText = new wxStaticText(panel, wxID_ANY, 
        "SSL/TLS Options:\n"
        "• Prefer: Use SSL if available (default)\n"
        "• Require: Always use SSL\n"
        "• Disable: Never use SSL\n"
        "• Verify CA: Verify server certificate against CA\n"
        "• Verify Full: Verify CA and server hostname");
    outerSizer->Add(helpText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    panel->SetSizer(outerSizer);
    notebook->AddPage(panel, "SSL / TLS");
}

void ConnectionEditorDialog::BuildAdvancedTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* sizer = new wxFlexGridSizer(2, 5, 10);
    sizer->AddGrowableCol(1);
    
    // Connection options
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Connection Options:"), 0, wxALIGN_CENTER_VERTICAL);
    options_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    options_ctrl_->SetHint("key=value pairs, semicolon separated (optional)");
    sizer->Add(options_ctrl_, 1, wxEXPAND);
    
    // Connect timeout
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Connect Timeout (ms):"), 0, wxALIGN_CENTER_VERTICAL);
    connect_timeout_ctrl_ = new wxTextCtrl(panel, wxID_ANY, "5000");
    sizer->Add(connect_timeout_ctrl_, 1, wxEXPAND);
    
    // Query timeout
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Query Timeout (ms):"), 0, wxALIGN_CENTER_VERTICAL);
    query_timeout_ctrl_ = new wxTextCtrl(panel, wxID_ANY, "0");
    sizer->Add(query_timeout_ctrl_, 1, wxEXPAND);

    // Status auto-poll defaults
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Status Auto-Poll:"), 0, wxALIGN_CENTER_VERTICAL);
    status_auto_poll_check_ = new wxCheckBox(panel, wxID_ANY, "");
    sizer->Add(status_auto_poll_check_, 1, wxALIGN_CENTER_VERTICAL);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Status Poll Interval (ms):"), 0,
               wxALIGN_CENTER_VERTICAL);
    status_poll_interval_ctrl_ = new wxTextCtrl(panel, wxID_ANY, "2000");
    sizer->Add(status_poll_interval_ctrl_, 1, wxEXPAND);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Status Request Default:"), 0,
               wxALIGN_CENTER_VERTICAL);
    status_default_choice_ = new wxChoice(panel, wxID_ANY);
    status_default_choice_->Append("Server Info");
    status_default_choice_->Append("Connection Info");
    status_default_choice_->Append("Database Info");
    status_default_choice_->Append("Statistics");
    status_default_choice_->SetSelection(0);
    sizer->Add(status_default_choice_, 1, wxEXPAND);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Status Category Order:"), 0,
               wxALIGN_CENTER_VERTICAL);
    status_category_order_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    status_category_order_ctrl_->SetHint("Request, General, engine, db");
    sizer->Add(status_category_order_ctrl_, 1, wxEXPAND);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Status Category Filter:"), 0,
               wxALIGN_CENTER_VERTICAL);
    status_category_filter_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    status_category_filter_ctrl_->SetHint("All or a category name");
    sizer->Add(status_category_filter_ctrl_, 1, wxEXPAND);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Status Diff Enabled:"), 0,
               wxALIGN_CENTER_VERTICAL);
    status_diff_enabled_check_ = new wxCheckBox(panel, wxID_ANY, "");
    sizer->Add(status_diff_enabled_check_, 1, wxALIGN_CENTER_VERTICAL);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Status Diff Ignore Unchanged:"), 0,
               wxALIGN_CENTER_VERTICAL);
    status_diff_ignore_unchanged_check_ = new wxCheckBox(panel, wxID_ANY, "");
    status_diff_ignore_unchanged_check_->SetValue(true);
    sizer->Add(status_diff_ignore_unchanged_check_, 1, wxALIGN_CENTER_VERTICAL);

    sizer->Add(new wxStaticText(panel, wxID_ANY, "Status Diff Ignore Empty:"), 0,
               wxALIGN_CENTER_VERTICAL);
    status_diff_ignore_empty_check_ = new wxCheckBox(panel, wxID_ANY, "");
    status_diff_ignore_empty_check_->SetValue(true);
    sizer->Add(status_diff_ignore_empty_check_, 1, wxALIGN_CENTER_VERTICAL);
    
    auto* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(sizer, 1, wxEXPAND | wxALL, 15);
    
    // Help text
    auto* helpText = new wxStaticText(panel, wxID_ANY,
        "Advanced Options:\n"
        "• Connect Timeout: Time to wait for connection (0 = default)\n"
        "• Query Timeout: Time to wait for query results (0 = no timeout)\n"
        "• Connection Options: Backend-specific options\n"
        "• Status Auto-Poll: Default auto-poll behavior for status views\n"
        "• Status Poll Interval: Default status refresh interval (ms)\n"
        "• Status Request Default: Default status category for views\n"
        "• Status Category Order: Comma-separated card order (optional)\n"
        "• Status Category Filter: Default category filter (optional)\n"
        "• Status Diff: Default diff view options for status panels");
    outerSizer->Add(helpText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    panel->SetSizer(outerSizer);
    notebook->AddPage(panel, "Advanced");
}

void ConnectionEditorDialog::OnBackendChanged(wxCommandEvent& event) {
    int backend = backend_choice_->GetSelection();
    
    // Update default port
    int defaultPort = GetDefaultPortForBackend(backend);
    if (port_ctrl_ && (port_ctrl_->IsEmpty() || port_ctrl_->GetValue() == "0")) {
        port_ctrl_->SetValue(wxString::Format("%d", defaultPort));
    }
    
    UpdateFieldStates();
}

void ConnectionEditorDialog::OnConnectionModeChanged(wxCommandEvent& event) {
    UpdateFieldStates();
}

void ConnectionEditorDialog::UpdateFieldStates() {
    int backend = backend_choice_->GetSelection();
    bool isScratchBird = (backend == kBackendScratchBird);
    
    // Role field is mainly for Firebird/PostgreSQL
    if (role_ctrl_) {
        bool supportsRole = (backend == kBackendPostgreSQL || backend == kBackendFirebird);
        role_ctrl_->Enable(supportsRole);
    }
    
    // Connection mode controls only for ScratchBird
    if (connection_mode_label_) {
        connection_mode_label_->Show(isScratchBird);
    }
    if (connection_mode_choice_) {
        connection_mode_choice_->Show(isScratchBird);
    }
    
    // Show/hide fields based on connection mode
    if (isScratchBird && connection_mode_choice_) {
        int mode = connection_mode_choice_->GetSelection();
        
        // Network mode: show host/port, hide IPC path
        bool isNetwork = (mode == kConnectionModeNetwork);
        bool isIpc = (mode == kConnectionModeIpc);
        bool isEmbedded = (mode == kConnectionModeEmbedded);
        
        // Host/Port visibility
        // (Host might be shown for IPC to allow custom socket paths)
        
        // IPC path visibility
        if (ipc_path_label_) {
            ipc_path_label_->Show(isIpc);
        }
        if (ipc_path_ctrl_) {
            ipc_path_ctrl_->Show(isIpc);
        }
        
        // Port is not used for IPC/Embedded
        if (port_ctrl_) {
            port_ctrl_->Enable(isNetwork);
            if (!isNetwork && !port_ctrl_->IsEmpty()) {
                // Keep the value but disable
            }
        }
        
        // Update database hint based on mode
        if (database_ctrl_) {
            if (isEmbedded) {
                database_ctrl_->SetHint("/path/to/database.sbd");
            } else if (isIpc) {
                database_ctrl_->SetHint("Database name");
            } else {
                database_ctrl_->SetHint("Database name or path");
            }
        }
    } else {
        // Hide IPC path for non-ScratchBird backends
        if (ipc_path_label_) ipc_path_label_->Hide();
        if (ipc_path_ctrl_) ipc_path_ctrl_->Hide();
    }
    
    Layout();
}

void ConnectionEditorDialog::LoadProfile(const ConnectionProfile& profile) {
    if (name_ctrl_) name_ctrl_->SetValue(profile.name);
    
    if (backend_choice_) {
        int backend = GetBackendFromName(profile.backend);
        backend_choice_->SetSelection(backend);
    }
    
    // Load connection mode
    if (connection_mode_choice_) {
        int mode = kConnectionModeNetwork;  // Default
        switch (profile.mode) {
            case ConnectionMode::Network: mode = kConnectionModeNetwork; break;
            case ConnectionMode::Ipc: mode = kConnectionModeIpc; break;
            case ConnectionMode::Embedded: mode = kConnectionModeEmbedded; break;
        }
        connection_mode_choice_->SetSelection(mode);
    }
    
    // Load IPC path if set
    if (ipc_path_ctrl_) {
        ipc_path_ctrl_->SetValue(profile.ipcPath);
    }
    
    if (host_ctrl_) host_ctrl_->SetValue(profile.host);
    if (port_ctrl_) port_ctrl_->SetValue(wxString::Format("%d", profile.port ? profile.port : 3092));
    if (database_ctrl_) database_ctrl_->SetValue(profile.database);
    if (username_ctrl_) username_ctrl_->SetValue(profile.username);
    if (application_name_ctrl_) application_name_ctrl_->SetValue(profile.applicationName);
    if (role_ctrl_) role_ctrl_->SetValue(profile.role);
    
    // SSL settings
    if (ssl_mode_choice_) {
        ssl_mode_choice_->SetSelection(SSLModeFromString(profile.sslMode));
    }
    if (ssl_root_cert_ctrl_) ssl_root_cert_ctrl_->SetValue(profile.sslRootCert);
    if (ssl_cert_ctrl_) ssl_cert_ctrl_->SetValue(profile.sslCert);
    if (ssl_key_ctrl_) ssl_key_ctrl_->SetValue(profile.sslKey);
    if (ssl_password_ctrl_) ssl_password_ctrl_->SetValue(profile.sslPassword);
    
    // Advanced
    if (options_ctrl_) options_ctrl_->SetValue(profile.options);
    if (status_auto_poll_check_) status_auto_poll_check_->SetValue(profile.statusAutoPollEnabled);
    if (status_poll_interval_ctrl_) {
        status_poll_interval_ctrl_->SetValue(wxString::Format("%d", profile.statusPollIntervalMs));
    }
    if (status_default_choice_) {
        int selection = 0;
        switch (profile.statusDefaultKind) {
            case StatusRequestKind::ConnectionInfo: selection = 1; break;
            case StatusRequestKind::DatabaseInfo: selection = 2; break;
            case StatusRequestKind::Statistics: selection = 3; break;
            case StatusRequestKind::ServerInfo:
            default: selection = 0; break;
        }
        status_default_choice_->SetSelection(selection);
    }
    if (status_category_order_ctrl_) {
        std::string joined;
        for (size_t i = 0; i < profile.statusCategoryOrder.size(); ++i) {
            if (i > 0) {
                joined += ", ";
            }
            joined += profile.statusCategoryOrder[i];
        }
        status_category_order_ctrl_->SetValue(wxString::FromUTF8(joined));
    }
    if (status_category_filter_ctrl_) {
        status_category_filter_ctrl_->SetValue(wxString::FromUTF8(profile.statusCategoryFilter));
    }
    if (status_diff_enabled_check_) {
        status_diff_enabled_check_->SetValue(profile.statusDiffEnabled);
    }
    if (status_diff_ignore_unchanged_check_) {
        status_diff_ignore_unchanged_check_->SetValue(profile.statusDiffIgnoreUnchanged);
    }
    if (status_diff_ignore_empty_check_) {
        status_diff_ignore_empty_check_->SetValue(profile.statusDiffIgnoreEmpty);
    }
}

ConnectionProfile ConnectionEditorDialog::GetProfile() const {
    ConnectionProfile profile;
    
    if (name_ctrl_) profile.name = name_ctrl_->GetValue().ToStdString();
    
    if (backend_choice_) {
        profile.backend = GetBackendName(backend_choice_->GetSelection());
    }
    
    // Get connection mode
    if (connection_mode_choice_) {
        int mode = connection_mode_choice_->GetSelection();
        switch (mode) {
            case kConnectionModeNetwork: profile.mode = ConnectionMode::Network; break;
            case kConnectionModeIpc: profile.mode = ConnectionMode::Ipc; break;
            case kConnectionModeEmbedded: profile.mode = ConnectionMode::Embedded; break;
        }
    }
    
    // Get IPC path
    if (ipc_path_ctrl_) {
        profile.ipcPath = ipc_path_ctrl_->GetValue().ToStdString();
    }
    
    if (host_ctrl_) profile.host = host_ctrl_->GetValue().ToStdString();
    
    if (port_ctrl_) {
        long port = 0;
        port_ctrl_->GetValue().ToLong(&port);
        profile.port = static_cast<int>(port);
    }
    
    if (database_ctrl_) profile.database = database_ctrl_->GetValue().ToStdString();
    if (username_ctrl_) profile.username = username_ctrl_->GetValue().ToStdString();
    if (password_ctrl_) {
        wxString password = password_ctrl_->GetValue();
        // Note: In a real implementation, this would use the credential store
        // For now, we store it in the profile (not secure, but functional)
        if (!password.IsEmpty()) {
            profile.credentialId = "password:" + password.ToStdString();
        }
    }
    
    if (application_name_ctrl_) profile.applicationName = application_name_ctrl_->GetValue().ToStdString();
    if (role_ctrl_) profile.role = role_ctrl_->GetValue().ToStdString();
    
    // SSL
    if (ssl_mode_choice_) {
        profile.sslMode = SSLModeToString(ssl_mode_choice_->GetSelection());
    }
    if (ssl_root_cert_ctrl_) profile.sslRootCert = ssl_root_cert_ctrl_->GetValue().ToStdString();
    if (ssl_cert_ctrl_) profile.sslCert = ssl_cert_ctrl_->GetValue().ToStdString();
    if (ssl_key_ctrl_) profile.sslKey = ssl_key_ctrl_->GetValue().ToStdString();
    if (ssl_password_ctrl_) profile.sslPassword = ssl_password_ctrl_->GetValue().ToStdString();
    
    // Advanced
    if (options_ctrl_) profile.options = options_ctrl_->GetValue().ToStdString();
    if (status_auto_poll_check_) {
        profile.statusAutoPollEnabled = status_auto_poll_check_->GetValue();
    }
    if (status_poll_interval_ctrl_) {
        long interval = 0;
        if (status_poll_interval_ctrl_->GetValue().ToLong(&interval)) {
            profile.statusPollIntervalMs = static_cast<int>(interval);
        }
    }
    if (status_default_choice_) {
        switch (status_default_choice_->GetSelection()) {
            case 1: profile.statusDefaultKind = StatusRequestKind::ConnectionInfo; break;
            case 2: profile.statusDefaultKind = StatusRequestKind::DatabaseInfo; break;
            case 3: profile.statusDefaultKind = StatusRequestKind::Statistics; break;
            default: profile.statusDefaultKind = StatusRequestKind::ServerInfo; break;
        }
    }
    if (status_category_order_ctrl_) {
        std::string raw = status_category_order_ctrl_->GetValue().ToStdString();
        profile.statusCategoryOrder.clear();
        std::string token;
        for (char c : raw) {
            if (c == ',') {
                token = Trim(token);
                if (!token.empty()) {
                    profile.statusCategoryOrder.push_back(token);
                }
                token.clear();
            } else {
                token.push_back(c);
            }
        }
        token = Trim(token);
        if (!token.empty()) {
            profile.statusCategoryOrder.push_back(token);
        }
    }
    if (status_category_filter_ctrl_) {
        profile.statusCategoryFilter = Trim(status_category_filter_ctrl_->GetValue().ToStdString());
    }
    if (status_diff_enabled_check_) {
        profile.statusDiffEnabled = status_diff_enabled_check_->GetValue();
    }
    if (status_diff_ignore_unchanged_check_) {
        profile.statusDiffIgnoreUnchanged = status_diff_ignore_unchanged_check_->GetValue();
    }
    if (status_diff_ignore_empty_check_) {
        profile.statusDiffIgnoreEmpty = status_diff_ignore_empty_check_->GetValue();
    }
    
    return profile;
}

bool ConnectionEditorDialog::ValidateForm() {
    // Check required fields
    if (name_ctrl_ && name_ctrl_->IsEmpty()) {
        wxMessageBox("Connection name is required.", "Validation Error", wxOK | wxICON_ERROR, this);
        name_ctrl_->SetFocus();
        return false;
    }
    
    // Validate based on connection mode
    int backend = backend_choice_ ? backend_choice_->GetSelection() : kBackendScratchBird;
    bool isScratchBird = (backend == kBackendScratchBird);
    
    if (isScratchBird && connection_mode_choice_) {
        int mode = connection_mode_choice_->GetSelection();
        
        if (mode == kConnectionModeEmbedded) {
            // Embedded mode requires database path
            if (database_ctrl_ && database_ctrl_->IsEmpty()) {
                wxMessageBox("Database path is required for Embedded mode.", 
                            "Validation Error", wxOK | wxICON_ERROR, this);
                database_ctrl_->SetFocus();
                return false;
            }
        } else if (mode == kConnectionModeIpc) {
            // IPC mode - host can be empty (will use default)
            // but if provided, should be a valid path
        }
    }
    
    // Host validation (not required for Embedded mode)
    bool hostRequired = true;
    if (isScratchBird && connection_mode_choice_) {
        int mode = connection_mode_choice_->GetSelection();
        if (mode == kConnectionModeEmbedded) {
            hostRequired = false;
        }
    }
    
    if (hostRequired && host_ctrl_ && host_ctrl_->IsEmpty()) {
        wxMessageBox("Host is required.", "Validation Error", wxOK | wxICON_ERROR, this);
        host_ctrl_->SetFocus();
        return false;
    }
    
    if (port_ctrl_) {
        long port = 0;
        if (!port_ctrl_->GetValue().ToLong(&port) || port <= 0 || port > 65535) {
            wxMessageBox("Port must be a number between 1 and 65535.", "Validation Error", 
                        wxOK | wxICON_ERROR, this);
            port_ctrl_->SetFocus();
            return false;
        }
    }

    if (status_poll_interval_ctrl_) {
        long interval = 0;
        if (!status_poll_interval_ctrl_->GetValue().ToLong(&interval) || interval < 250) {
            wxMessageBox("Status poll interval must be at least 250 ms.", "Validation Error",
                        wxOK | wxICON_ERROR, this);
            status_poll_interval_ctrl_->SetFocus();
            return false;
        }
    }
    
    return true;
}

void ConnectionEditorDialog::OnTestConnection(wxCommandEvent& event) {
    if (!ValidateForm()) {
        return;
    }
    
    ConnectionProfile profile = GetProfile();
    
    test_button_->Enable(false);
    test_result_label_->SetLabel("Testing...");
    test_result_label_->SetForegroundColour(wxColour(128, 128, 128));
    
    // Perform connection test
    std::string errorMessage;
    bool success = TestConnection(profile, &errorMessage);
    
    if (success) {
        test_result_label_->SetLabel("✓ Connection successful!");
        test_result_label_->SetForegroundColour(wxColour(0, 128, 0));
    } else {
        test_result_label_->SetLabel("✗ " + errorMessage);
        test_result_label_->SetForegroundColour(wxColour(255, 0, 0));
    }
    
    test_button_->Enable(true);
}

bool ConnectionEditorDialog::TestConnection(const ConnectionProfile& profile, std::string* errorMessage) {
    // Create a temporary connection manager for testing
    ConnectionManager testManager;
    
    // Set a shorter timeout for testing
    NetworkOptions options;
    options.connectTimeoutMs = 5000;
    testManager.SetNetworkOptions(options);
    
    if (!testManager.Connect(profile)) {
        if (errorMessage) {
            *errorMessage = testManager.LastError();
        }
        return false;
    }
    
    testManager.Disconnect();
    return true;
}

std::string ConnectionEditorDialog::BrowseForFile(const wxString& title, const wxString& wildcard) {
    wxFileDialog dialog(this, title, wxEmptyString, wxEmptyString, wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() == wxID_OK) {
        return dialog.GetPath().ToStdString();
    }
    return "";
}

void ConnectionEditorDialog::OnBrowseSSLRootCert(wxCommandEvent& event) {
    std::string path = BrowseForFile("Select CA Certificate", "Certificate files (*.crt;*.pem)|*.crt;*.pem|All files (*.*)|*.*");
    if (!path.empty() && ssl_root_cert_ctrl_) {
        ssl_root_cert_ctrl_->SetValue(path);
    }
}

void ConnectionEditorDialog::OnBrowseSSLCert(wxCommandEvent& event) {
    std::string path = BrowseForFile("Select Client Certificate", "Certificate files (*.crt;*.pem)|*.crt;*.pem|All files (*.*)|*.*");
    if (!path.empty() && ssl_cert_ctrl_) {
        ssl_cert_ctrl_->SetValue(path);
    }
}

void ConnectionEditorDialog::OnBrowseSSLKey(wxCommandEvent& event) {
    std::string path = BrowseForFile("Select Client Key", "Key files (*.key;*.pem)|*.key;*.pem|All files (*.*)|*.*");
    if (!path.empty() && ssl_key_ctrl_) {
        ssl_key_ctrl_->SetValue(path);
    }
}

// =============================================================================
// ConnectionManagerDialog
// =============================================================================

wxBEGIN_EVENT_TABLE(ConnectionManagerDialog, wxDialog)
    EVT_BUTTON(wxID_HIGHEST + 200, ConnectionManagerDialog::OnNew)
    EVT_BUTTON(wxID_HIGHEST + 201, ConnectionManagerDialog::OnEdit)
    EVT_BUTTON(wxID_HIGHEST + 202, ConnectionManagerDialog::OnDuplicate)
    EVT_BUTTON(wxID_HIGHEST + 203, ConnectionManagerDialog::OnDelete)
    EVT_BUTTON(wxID_HIGHEST + 204, ConnectionManagerDialog::OnMoveUp)
    EVT_BUTTON(wxID_HIGHEST + 205, ConnectionManagerDialog::OnMoveDown)
    EVT_LISTBOX(wxID_ANY, ConnectionManagerDialog::OnSelectionChanged)
    EVT_LISTBOX_DCLICK(wxID_ANY, ConnectionManagerDialog::OnItemActivated)
wxEND_EVENT_TABLE()

ConnectionManagerDialog::ConnectionManagerDialog(wxWindow* parent, std::vector<ConnectionProfile>* connections)
    : wxDialog(parent, wxID_ANY, "Connection Manager", wxDefaultPosition, wxSize(500, 450)),
      connections_(connections) {
    BuildLayout();
    RefreshList();
}

void ConnectionManagerDialog::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    
    // List of connections
    list_box_ = new wxListBox(this, wxID_ANY);
    rootSizer->Add(list_box_, 1, wxEXPAND | wxALL, 10);
    
    // Button row
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left side: Add/Edit/Delete
    buttonSizer->Add(new wxButton(this, wxID_HIGHEST + 200, "&New..."), 0, wxRIGHT, 5);
    edit_button_ = new wxButton(this, wxID_HIGHEST + 201, "&Edit...");
    buttonSizer->Add(edit_button_, 0, wxRIGHT, 5);
    duplicate_button_ = new wxButton(this, wxID_HIGHEST + 202, "&Duplicate");
    buttonSizer->Add(duplicate_button_, 0, wxRIGHT, 5);
    delete_button_ = new wxButton(this, wxID_HIGHEST + 203, "&Delete");
    buttonSizer->Add(delete_button_, 0, wxRIGHT, 20);
    
    // Middle: Reordering
    up_button_ = new wxButton(this, wxID_HIGHEST + 204, "Move &Up");
    buttonSizer->Add(up_button_, 0, wxRIGHT, 5);
    down_button_ = new wxButton(this, wxID_HIGHEST + 205, "Move D&own");
    buttonSizer->Add(down_button_, 0, wxRIGHT, 20);
    
    buttonSizer->AddStretchSpacer();
    
    // Right side: Standard buttons
    buttonSizer->Add(new wxButton(this, wxID_OK, "&Close"), 0);
    
    rootSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    SetSizer(rootSizer);
    
    // Initial button states
    UpdateButtonStates();
}

void ConnectionManagerDialog::RefreshList() {
    if (!list_box_ || !connections_) return;
    
    list_box_->Clear();
    for (const auto& conn : *connections_) {
        wxString label = conn.name.empty() ? "(Unnamed)" : wxString::FromUTF8(conn.name);
        
        // Add backend indicator
        std::string backend = conn.backend;
        if (backend.empty()) backend = "native";
        label += wxString::Format(" [%s]", backend);
        
        // Add host indicator
        if (!conn.host.empty()) {
            label += wxString::Format(" - %s", conn.host);
            if (conn.port > 0) {
                label += wxString::Format(":%d", conn.port);
            }
        }

        // Default status category
        const char* status_label = "Server";
        switch (conn.statusDefaultKind) {
            case StatusRequestKind::ConnectionInfo: status_label = "Connection"; break;
            case StatusRequestKind::DatabaseInfo: status_label = "Database"; break;
            case StatusRequestKind::Statistics: status_label = "Statistics"; break;
            case StatusRequestKind::ServerInfo:
            default: status_label = "Server"; break;
        }
        label += wxString::Format(" | Status: %s", status_label);
        
        list_box_->Append(label);
    }
}

void ConnectionManagerDialog::UpdateButtonStates() {
    bool hasSelection = list_box_ && list_box_->GetSelection() != wxNOT_FOUND;
    
    if (edit_button_) edit_button_->Enable(hasSelection);
    if (duplicate_button_) duplicate_button_->Enable(hasSelection);
    if (delete_button_) delete_button_->Enable(hasSelection);
    if (up_button_) up_button_->Enable(hasSelection && list_box_->GetSelection() > 0);
    if (down_button_) down_button_->Enable(hasSelection && 
        list_box_->GetSelection() < static_cast<int>(list_box_->GetCount()) - 1);
}

void ConnectionManagerDialog::OnSelectionChanged(wxCommandEvent& event) {
    UpdateButtonStates();
}

void ConnectionManagerDialog::OnItemActivated(wxCommandEvent& event) {
    OnEdit(event);
}

void ConnectionManagerDialog::OnNew(wxCommandEvent& event) {
    ConnectionEditorDialog dialog(this, ConnectionEditorMode::Create);
    if (dialog.ShowModal() == wxID_OK) {
        if (dialog.ValidateForm()) {
            connections_->push_back(dialog.GetProfile());
            RefreshList();
            list_box_->SetSelection(list_box_->GetCount() - 1);
            UpdateButtonStates();
        }
    }
}

void ConnectionManagerDialog::OnEdit(wxCommandEvent& event) {
    int selection = list_box_->GetSelection();
    if (selection == wxNOT_FOUND || !connections_) return;
    
    ConnectionEditorDialog dialog(this, ConnectionEditorMode::Edit, &(*connections_)[selection]);
    if (dialog.ShowModal() == wxID_OK) {
        if (dialog.ValidateForm()) {
            (*connections_)[selection] = dialog.GetProfile();
            RefreshList();
            list_box_->SetSelection(selection);
        }
    }
}

void ConnectionManagerDialog::OnDuplicate(wxCommandEvent& event) {
    int selection = list_box_->GetSelection();
    if (selection == wxNOT_FOUND || !connections_) return;
    
    ConnectionEditorDialog dialog(this, ConnectionEditorMode::Duplicate, &(*connections_)[selection]);
    if (dialog.ShowModal() == wxID_OK) {
        if (dialog.ValidateForm()) {
            connections_->push_back(dialog.GetProfile());
            RefreshList();
            list_box_->SetSelection(list_box_->GetCount() - 1);
            UpdateButtonStates();
        }
    }
}

void ConnectionManagerDialog::OnDelete(wxCommandEvent& event) {
    int selection = list_box_->GetSelection();
    if (selection == wxNOT_FOUND || !connections_) return;
    
    wxString name = list_box_->GetString(selection);
    wxString msg = wxString::Format("Are you sure you want to delete the connection '%s'?", name);
    
    if (wxMessageBox(msg, "Confirm Delete", wxYES_NO | wxICON_QUESTION, this) == wxYES) {
        connections_->erase(connections_->begin() + selection);
        RefreshList();
        UpdateButtonStates();
    }
}

void ConnectionManagerDialog::OnMoveUp(wxCommandEvent& event) {
    int selection = list_box_->GetSelection();
    if (selection <= 0 || !connections_) return;
    
    std::swap((*connections_)[selection], (*connections_)[selection - 1]);
    RefreshList();
    list_box_->SetSelection(selection - 1);
    UpdateButtonStates();
}

void ConnectionManagerDialog::OnMoveDown(wxCommandEvent& event) {
    int selection = list_box_->GetSelection();
    if (selection == wxNOT_FOUND || selection >= static_cast<int>(connections_->size()) - 1 || !connections_) return;
    
    std::swap((*connections_)[selection], (*connections_)[selection + 1]);
    RefreshList();
    list_box_->SetSelection(selection + 1);
    UpdateButtonStates();
}

} // namespace scratchrobin
