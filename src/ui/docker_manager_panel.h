/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DOCKER_MANAGER_PANEL_H
#define SCRATCHROBIN_DOCKER_MANAGER_PANEL_H

#include <wx/panel.h>
#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/wizard.h>

#include "core/docker_config.h"

class wxNotebook;
class wxListCtrl;
class wxTextCtrl;
class wxGauge;
class wxButton;
class wxChoice;
class wxSpinCtrl;
class wxCheckBox;

namespace scratchrobin {

// ============================================================================
// Docker Manager Panel - Main Docker control interface
// ============================================================================
class DockerManagerPanel : public wxPanel {
public:
    DockerManagerPanel(wxWindow* parent);
    ~DockerManagerPanel();
    
    // Update display
    void RefreshStatus();
    void RefreshLogs();
    
private:
    void BuildLayout();
    void BuildToolbar();
    void BuildStatusPanel();
    void BuildDetailsPanel(wxSizer* parent);
    void BuildLogsPanel();
    void BuildConfigurationPanel();
    
    // Event handlers
    void OnStart(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnRestart(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnRemove(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnConfigure(wxCommandEvent& event);
    void OnSaveConfig(wxCommandEvent& event);
    void OnLoadConfig(wxCommandEvent& event);
    void OnTemplateSelect(wxCommandEvent& event);
    void OnBrowseDirectory(wxCommandEvent& event);
    void OnServiceToggle(wxCommandEvent& event);
    void OnPortChange(wxSpinEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnContainerSelect(wxListEvent& event);
    void OnButton(wxCommandEvent& event);
    
    // Actions
    void StartContainer();
    void StopContainer();
    void RestartContainer();
    void UpdateStatusDisplay(const DockerContainerStatus& status);
    void UpdateResourceDisplay(const DockerContainerStatus& status);
    void AppendLog(const std::string& message);
    void LoadConfiguration(const DockerContainerConfig& config);
    DockerContainerConfig GetConfigurationFromUI();
    
    // Members
    DockerContainerManager manager_;
    DockerContainerConfig current_config_;
    wxTimer status_timer_;
    std::string selected_container_;
    bool is_monitoring_ = false;
    
    // UI Components - Toolbar
    wxPanel* toolbar_ = nullptr;
    wxButton* start_button_ = nullptr;
    wxButton* stop_button_ = nullptr;
    wxButton* restart_button_ = nullptr;
    wxButton* pause_button_ = nullptr;
    wxButton* remove_button_ = nullptr;
    wxButton* refresh_button_ = nullptr;
    wxButton* configure_button_ = nullptr;
    
    // UI Components - Status Panel
    wxListCtrl* container_list_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxStaticText* health_label_ = nullptr;
    wxStaticText* id_label_ = nullptr;
    wxStaticText* image_label_ = nullptr;
    wxStaticText* ip_label_ = nullptr;
    wxGauge* cpu_gauge_ = nullptr;
    wxGauge* memory_gauge_ = nullptr;
    wxStaticText* cpu_text_ = nullptr;
    wxStaticText* memory_text_ = nullptr;
    
    // UI Components - Logs Panel
    wxTextCtrl* log_text_ = nullptr;
    wxButton* clear_logs_button_ = nullptr;
    wxButton* export_logs_button_ = nullptr;
    wxCheckBox* auto_scroll_checkbox_ = nullptr;
    
    // UI Components - Configuration Panel
    wxNotebook* config_notebook_ = nullptr;
    
    // General tab
    wxTextCtrl* container_name_ctrl_ = nullptr;
    wxChoice* template_choice_ = nullptr;
    wxTextCtrl* data_dir_ctrl_ = nullptr;
    wxTextCtrl* config_dir_ctrl_ = nullptr;
    wxTextCtrl* log_dir_ctrl_ = nullptr;
    wxTextCtrl* backup_dir_ctrl_ = nullptr;
    
    // Services tab
    wxCheckBox* native_checkbox_ = nullptr;
    wxSpinCtrl* native_port_ctrl_ = nullptr;
    wxCheckBox* postgres_checkbox_ = nullptr;
    wxSpinCtrl* postgres_port_ctrl_ = nullptr;
    wxCheckBox* mysql_checkbox_ = nullptr;
    wxSpinCtrl* mysql_port_ctrl_ = nullptr;
    wxCheckBox* firebird_checkbox_ = nullptr;
    wxSpinCtrl* firebird_port_ctrl_ = nullptr;
    
    // Resources tab
    wxSpinCtrl* memory_ctrl_ = nullptr;
    wxSpinCtrl* cpu_shares_ctrl_ = nullptr;
    wxSpinCtrl* max_connections_ctrl_ = nullptr;
    wxSpinCtrl* shared_buffers_ctrl_ = nullptr;
    
    // Security tab
    wxCheckBox* ssl_checkbox_ = nullptr;
    wxTextCtrl* ssl_cert_ctrl_ = nullptr;
    wxTextCtrl* ssl_key_ctrl_ = nullptr;
    wxCheckBox* auth_checkbox_ = nullptr;
    
    // Backup tab
    wxCheckBox* backup_checkbox_ = nullptr;
    wxTextCtrl* backup_schedule_ctrl_ = nullptr;
    wxSpinCtrl* backup_retention_ctrl_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// Docker Configuration Dialog
// ============================================================================
class DockerConfigDialog : public wxDialog {
public:
    DockerConfigDialog(wxWindow* parent, DockerContainerConfig* config);
    
    bool ValidateConfiguration();
    
private:
    void BuildLayout();
    void OnSave(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnTestPorts(wxCommandEvent& event);
    void OnGenerateCompose(wxCommandEvent& event);
    
    DockerContainerConfig* config_;
    bool config_valid_ = false;
};

// ============================================================================
// Docker Wizard - Step-by-step container setup
// ============================================================================
class DockerWizard : public wxWizard {
public:
    DockerWizard(wxWindow* parent);
    
    DockerContainerConfig GetConfiguration() const { return config_; }
    
private:
    DockerContainerConfig config_;
    
    // Wizard pages
    class WelcomePage;
    class TemplatePage;
    class ServicesPage;
    class ResourcesPage;
    class DirectoriesPage;
    class SummaryPage;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DOCKER_MANAGER_PANEL_H
