/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <string>

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * Connection configuration structure
 */
struct ConnectionConfig {
  std::string host = "localhost";
  int port = 2055;
  std::string database;
  std::string username;
  std::string password;
  std::string charset = "UTF8";
  std::string role;
  int timeout = 30;
  bool use_compression = true;
  bool use_ssl = false;
};

/**
 * Connection Dialog
 * Dialog for configuring and establishing database connections
 */
class ConnectionDialog : public wxDialog {
 public:
  ConnectionDialog(wxWindow* parent, scratchrobin::backend::SessionClient* session_client);
  ~ConnectionDialog() override;

  // Get the configured connection settings
  ConnectionConfig GetConnectionConfig() const;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnTestConnection(wxCommandEvent& event);
  void OnConnect(wxCommandEvent& event);
  void OnCancel(wxCommandEvent& event);
  void OnBrowseDatabase(wxCommandEvent& event);
  void OnSaveProfile(wxCommandEvent& event);
  void OnLoadProfile(wxCommandEvent& event);

  // Backend
  scratchrobin::backend::SessionClient* session_client_;

  // UI Components
  wxTextCtrl* host_ctrl_{nullptr};
  wxTextCtrl* port_ctrl_{nullptr};
  wxTextCtrl* database_ctrl_{nullptr};
  wxTextCtrl* username_ctrl_{nullptr};
  wxTextCtrl* password_ctrl_{nullptr};
  wxChoice* charset_ctrl_{nullptr};
  wxTextCtrl* role_ctrl_{nullptr};
  wxCheckBox* compression_check_{nullptr};
  wxCheckBox* ssl_check_{nullptr};
  wxChoice* saved_profiles_ctrl_{nullptr};

  // State
  ConnectionConfig config_;

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
