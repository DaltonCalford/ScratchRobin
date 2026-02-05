/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */

#ifndef SCRATCHROBIN_GIT_INTEGRATION_FRAME_H
#define SCRATCHROBIN_GIT_INTEGRATION_FRAME_H

#include <string>
#include <vector>
#include <memory>

#include <wx/frame.h>
#include <wx/timer.h>

#include "core/git_model.h"

class wxStaticText;
class wxButton;
class wxPanel;
class wxNotebook;
class wxListBox;
class wxTreeCtrl;
class wxListCtrl;
class wxTextCtrl;
class wxComboBox;
class wxSplitterWindow;
class wxListEvent;

namespace scratchrobin {

class WindowManager;
class ConnectionManager;
class GitClient;
struct ConnectionProfile;
struct AppConfig;

/**
 * @brief Git Integration frame - Fully Functional
 * 
 * Provides complete Git version control integration for database projects
 * including repository management, commits, branches, and visual diffs.
 */
class GitIntegrationFrame : public wxFrame {
public:
    GitIntegrationFrame(WindowManager* windowManager,
                        ConnectionManager* connectionManager,
                        const std::vector<ConnectionProfile>* connections,
                        const AppConfig* appConfig);
    
    ~GitIntegrationFrame();
    
    // Initialize with a project path
    void SetProjectPath(const std::string& path);

private:
    void BuildMenu();
    void BuildLayout();
    void BuildToolbar();
    void BuildStatusPanel();
    void BuildHistoryPanel();
    void BuildDiffPanel();
    void BuildBranchesPanel();
    void BuildRemotesPanel();
    
    // Git operations
    void OnInitRepository(wxCommandEvent& event);
    void OnCloneRepository(wxCommandEvent& event);
    void OnOpenRepository(wxCommandEvent& event);
    void OnCommit(wxCommandEvent& event);
    void OnPush(wxCommandEvent& event);
    void OnPull(wxCommandEvent& event);
    void OnFetch(wxCommandEvent& event);
    void OnStash(wxCommandEvent& event);
    void OnStashPop(wxCommandEvent& event);
    
    // Branch operations
    void OnCreateBranch(wxCommandEvent& event);
    void OnCheckoutBranch(wxCommandEvent& event);
    void OnMergeBranch(wxCommandEvent& event);
    void OnDeleteBranch(wxCommandEvent& event);
    
    // UI updates
    void RefreshStatus();
    void RefreshHistory();
    void RefreshBranches();
    void RefreshDiff();
    void OnTimer(wxTimerEvent& event);
    void OnRefresh(wxCommandEvent& event);
    
    // Selection handlers
    void OnFileSelected(wxListEvent& event);
    void OnCommitSelected(wxListEvent& event);
    void OnBranchSelected(wxListEvent& event);
    
    // Dialogs
    void ShowCommitDialog();
    void ShowCloneDialog();
    void ShowBranchDialog();
    void ShowMergeDialog();
    
    void OnClose(wxCloseEvent& event);
    void OnShowDocumentation(wxCommandEvent& event);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;
    
    // Git client
    std::unique_ptr<GitClient> git_;
    std::string project_path_;
    bool has_repository_ = false;
    
    // Auto-refresh timer
    wxTimer refresh_timer_;
    
    // UI Elements - Toolbar
    wxButton* btn_init_ = nullptr;
    wxButton* btn_clone_ = nullptr;
    wxButton* btn_open_ = nullptr;
    wxButton* btn_commit_ = nullptr;
    wxButton* btn_push_ = nullptr;
    wxButton* btn_pull_ = nullptr;
    wxButton* btn_fetch_ = nullptr;
    wxButton* btn_refresh_ = nullptr;
    
    // Status panel
    wxStaticText* lbl_repo_name_ = nullptr;
    wxStaticText* lbl_branch_ = nullptr;
    wxStaticText* lbl_remote_ = nullptr;
    wxStaticText* lbl_ahead_behind_ = nullptr;
    wxListCtrl* list_changed_files_ = nullptr;
    wxTextCtrl* txt_commit_message_ = nullptr;
    
    // History panel
    wxListCtrl* list_commits_ = nullptr;
    wxTextCtrl* txt_commit_details_ = nullptr;
    
    // Diff panel
    wxListCtrl* list_diff_files_ = nullptr;
    wxTextCtrl* txt_diff_content_ = nullptr;
    
    // Branches panel
    wxListCtrl* list_branches_ = nullptr;
    wxButton* btn_new_branch_ = nullptr;
    wxButton* btn_checkout_ = nullptr;
    wxButton* btn_merge_ = nullptr;
    wxButton* btn_delete_branch_ = nullptr;
    
    // Remotes panel
    wxListCtrl* list_remotes_ = nullptr;
    wxButton* btn_add_remote_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_GIT_INTEGRATION_FRAME_H
