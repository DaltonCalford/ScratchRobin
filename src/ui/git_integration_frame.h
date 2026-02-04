/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#ifndef SCRATCHROBIN_GIT_INTEGRATION_FRAME_H
#define SCRATCHROBIN_GIT_INTEGRATION_FRAME_H

#include <string>
#include <vector>

#include <wx/frame.h>

#include "core/git_model.h"

class wxStaticText;
class wxButton;
class wxPanel;
class wxNotebook;
class wxListBox;
class wxTreeCtrl;

namespace scratchrobin {

class WindowManager;
class ConnectionManager;
struct ConnectionProfile;
struct AppConfig;

/**
 * @brief Git Integration frame (Beta Placeholder)
 * 
 * This frame provides a placeholder UI for Git version control integration,
 * planned for the Beta release. It displays repository status, commit history,
 * and database object versioning as non-functional previews.
 */
class GitIntegrationFrame : public wxFrame {
public:
    GitIntegrationFrame(WindowManager* windowManager,
                        ConnectionManager* connectionManager,
                        const std::vector<ConnectionProfile>* connections,
                        const AppConfig* appConfig);
    
    ~GitIntegrationFrame();

private:
    void BuildMenu();
    void BuildLayout();
    
    void OnClose(wxCloseEvent& event);
    void OnShowDocumentation(wxCommandEvent& event);
    void OnJoinBeta(wxCommandEvent& event);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;
    
    // UI Elements
    wxNotebook* notebook_ = nullptr;
    wxButton* docs_button_ = nullptr;
    wxButton* beta_signup_button_ = nullptr;
    wxTreeCtrl* repo_tree_ = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_GIT_INTEGRATION_FRAME_H
