/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CONNECTION_EDITOR_DIALOG_H
#define SCRATCHROBIN_CONNECTION_EDITOR_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>
#include <wx/notebook.h>

class wxButton;
class wxCheckBox;
class wxChoice;
class wxListBox;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

struct ConnectionProfile;

enum class ConnectionEditorMode {
    Create,
    Edit,
    Duplicate
};

class ConnectionEditorDialog : public wxDialog {
public:
    ConnectionEditorDialog(wxWindow* parent,
                          ConnectionEditorMode mode,
                          const ConnectionProfile* existingProfile = nullptr);

    // Returns the edited/created profile
    ConnectionProfile GetProfile() const;
    
    // Validate the form
    bool ValidateForm();

private:
    void BuildLayout();
    void BuildGeneralTab(wxNotebook* notebook);
    void BuildSSLTab(wxNotebook* notebook);
    void BuildAdvancedTab(wxNotebook* notebook);
    
    void OnBackendChanged(wxCommandEvent& event);
    void OnTestConnection(wxCommandEvent& event);
    void OnBrowseSSLCert(wxCommandEvent& event);
    void OnBrowseSSLKey(wxCommandEvent& event);
    void OnBrowseSSLRootCert(wxCommandEvent& event);
    void UpdateFieldStates();
    
    // Helper methods
    void LoadProfile(const ConnectionProfile& profile);
    bool TestConnection(const ConnectionProfile& profile, std::string* errorMessage);
    std::string BrowseForFile(const wxString& title, const wxString& wildcard);

    ConnectionEditorMode mode_;
    
    // General tab controls
    wxTextCtrl* name_ctrl_ = nullptr;
    wxChoice* backend_choice_ = nullptr;
    wxTextCtrl* host_ctrl_ = nullptr;
    wxTextCtrl* port_ctrl_ = nullptr;
    wxTextCtrl* database_ctrl_ = nullptr;
    wxTextCtrl* username_ctrl_ = nullptr;
    wxTextCtrl* password_ctrl_ = nullptr;
    wxCheckBox* save_password_ctrl_ = nullptr;
    wxTextCtrl* application_name_ctrl_ = nullptr;
    wxTextCtrl* role_ctrl_ = nullptr;
    
    // SSL tab controls
    wxChoice* ssl_mode_choice_ = nullptr;
    wxTextCtrl* ssl_root_cert_ctrl_ = nullptr;
    wxButton* ssl_root_cert_browse_btn_ = nullptr;
    wxTextCtrl* ssl_cert_ctrl_ = nullptr;
    wxButton* ssl_cert_browse_btn_ = nullptr;
    wxTextCtrl* ssl_key_ctrl_ = nullptr;
    wxButton* ssl_key_browse_btn_ = nullptr;
    wxTextCtrl* ssl_password_ctrl_ = nullptr;
    
    // Advanced tab controls
    wxTextCtrl* options_ctrl_ = nullptr;
    wxTextCtrl* connect_timeout_ctrl_ = nullptr;
    wxTextCtrl* query_timeout_ctrl_ = nullptr;
    
    // Buttons
    wxButton* test_button_ = nullptr;
    wxStaticText* test_result_label_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

// Profile management dialog
class ConnectionManagerDialog : public wxDialog {
public:
    ConnectionManagerDialog(wxWindow* parent, std::vector<ConnectionProfile>* connections);
    
    const std::vector<ConnectionProfile>& GetConnections() const { return *connections_; }

private:
    void BuildLayout();
    void RefreshList();
    
    void OnNew(wxCommandEvent& event);
    void OnEdit(wxCommandEvent& event);
    void OnDuplicate(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnMoveUp(wxCommandEvent& event);
    void OnMoveDown(wxCommandEvent& event);
    void OnImport(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnSelectionChanged(wxCommandEvent& event);
    void OnItemActivated(wxCommandEvent& event);
    void UpdateButtonStates();

    std::vector<ConnectionProfile>* connections_;
    wxListBox* list_box_ = nullptr;
    wxButton* edit_button_ = nullptr;
    wxButton* duplicate_button_ = nullptr;
    wxButton* delete_button_ = nullptr;
    wxButton* up_button_ = nullptr;
    wxButton* down_button_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONNECTION_EDITOR_DIALOG_H
