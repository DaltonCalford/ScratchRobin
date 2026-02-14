/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#ifndef SCRATCHROBIN_CLUSTER_MANAGER_FRAME_H
#define SCRATCHROBIN_CLUSTER_MANAGER_FRAME_H

#include <string>
#include <memory>

#include <wx/frame.h>

#include "core/cluster_model.h"

class wxStaticText;
class wxButton;
class wxListBox;
class wxPanel;

namespace scratchrobin {

class WindowManager;
class ConnectionManager;
struct ConnectionProfile;
struct AppConfig;

/**
 * @brief Cluster Manager frame (Beta Placeholder)
 * 
 * This frame provides a placeholder UI for the Cluster Manager feature,
 * planned for the Beta release. It displays the cluster topology,
 * node status, and failover capabilities as non-functional previews.
 */
class ClusterManagerFrame : public wxFrame {
public:
    ClusterManagerFrame(WindowManager* windowManager,
                        ConnectionManager* connectionManager,
                        const std::vector<ConnectionProfile>* connections,
                        const AppConfig* appConfig);
    
    ~ClusterManagerFrame();

private:
    void BuildMenu();
    void BuildLayout();
    void CreateBetaPlaceholderUI();
    
    void OnClose(wxCloseEvent& event);
    void OnShowDocumentation(wxCommandEvent& event);
    void OnJoinBeta(wxCommandEvent& event);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;
    
    // UI Elements (placeholder visuals)
    wxPanel* placeholder_panel_ = nullptr;
    wxStaticText* beta_message_ = nullptr;
    wxStaticText* feature_list_ = nullptr;
    wxButton* docs_button_ = nullptr;
    wxButton* beta_signup_button_ = nullptr;
    
    // Demo data for visualization
    std::vector<ClusterNode> demo_nodes_;
    ClusterConfig demo_config_;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CLUSTER_MANAGER_FRAME_H
