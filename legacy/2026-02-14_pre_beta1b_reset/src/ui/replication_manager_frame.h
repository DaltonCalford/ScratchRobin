/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#ifndef SCRATCHROBIN_REPLICATION_MANAGER_FRAME_H
#define SCRATCHROBIN_REPLICATION_MANAGER_FRAME_H

#include <string>
#include <vector>

#include <wx/frame.h>

#include "core/replication_model.h"

class wxStaticText;
class wxButton;
class wxPanel;
class wxNotebook;

namespace scratchrobin {

class WindowManager;
class ConnectionManager;
struct ConnectionProfile;
struct AppConfig;

/**
 * @brief Replication Manager frame (Beta Placeholder)
 * 
 * This frame provides a placeholder UI for the Replication Manager feature,
 * planned for the Beta release. It displays replication topology,
 * lag metrics, and slot management as non-functional previews.
 */
class ReplicationManagerFrame : public wxFrame {
public:
    ReplicationManagerFrame(WindowManager* windowManager,
                            ConnectionManager* connectionManager,
                            const std::vector<ConnectionProfile>* connections,
                            const AppConfig* appConfig);
    
    ~ReplicationManagerFrame();

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

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_REPLICATION_MANAGER_FRAME_H
