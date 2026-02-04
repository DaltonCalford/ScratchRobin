/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/cluster_manager_frame.h"

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/hyperlink.h>
#include <wx/statbox.h>
#include <wx/scrolwin.h>
#include <wx/menu.h>

#include "ui/window_manager.h"
#include "core/connection_manager.h"
#include "core/config.h"

namespace scratchrobin {

enum {
    ID_SHOW_DOCUMENTATION = wxID_HIGHEST + 1,
    ID_JOIN_BETA
};

wxBEGIN_EVENT_TABLE(ClusterManagerFrame, wxFrame)
    EVT_CLOSE(ClusterManagerFrame::OnClose)
    EVT_BUTTON(ID_SHOW_DOCUMENTATION, ClusterManagerFrame::OnShowDocumentation)
    EVT_BUTTON(ID_JOIN_BETA, ClusterManagerFrame::OnJoinBeta)
wxEND_EVENT_TABLE()

ClusterManagerFrame::ClusterManagerFrame(WindowManager* windowManager,
                                         ConnectionManager* connectionManager,
                                         const std::vector<ConnectionProfile>* connections,
                                         const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, _("Cluster Manager [Beta Preview]"),
              wxDefaultPosition, wxSize(900, 650),
              wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , window_manager_(windowManager)
    , connection_manager_(connectionManager)
    , connections_(connections)
    , app_config_(appConfig) {
    
    // Set a distinctive icon/color to indicate beta status
    SetBackgroundColour(wxColour(250, 250, 255));
    
    BuildMenu();
    BuildLayout();
    
    CentreOnScreen();
}

ClusterManagerFrame::~ClusterManagerFrame() = default;

void ClusterManagerFrame::BuildMenu() {
    auto* menu_bar = new wxMenuBar();
    
    auto* file_menu = new wxMenu();
    file_menu->Append(wxID_CLOSE, _("&Close") + wxT("\tCtrl+W"));
    menu_bar->Append(file_menu, _("&File"));
    
    auto* help_menu = new wxMenu();
    help_menu->Append(ID_SHOW_DOCUMENTATION, _("&Documentation..."));
    help_menu->AppendSeparator();
    help_menu->Append(ID_JOIN_BETA, _("&Join Beta Program..."));
    menu_bar->Append(help_menu, _("&Help"));
    
    SetMenuBar(menu_bar);
}

void ClusterManagerFrame::BuildLayout() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Beta banner at top
    auto* banner_panel = new wxPanel(this, wxID_ANY);
    banner_panel->SetBackgroundColour(wxColour(70, 130, 180));  // Steel blue
    auto* banner_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    auto* banner_text = new wxStaticText(banner_panel, wxID_ANY,
        _("BETA FEATURE PREVIEW - Cluster Management capabilities coming in Beta release"));
    banner_text->SetForegroundColour(*wxWHITE);
    banner_text->SetFont(wxFont(wxFontInfo(11).Bold()));
    banner_sizer->Add(banner_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    banner_panel->SetSizer(banner_sizer);
    
    main_sizer->Add(banner_panel, 0, wxEXPAND);
    
    // Main content area
    auto* content_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left panel: Feature overview
    auto* left_panel = new wxScrolledWindow(this, wxID_ANY);
    left_panel->SetScrollRate(5, 5);
    auto* left_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    auto* title = new wxStaticText(left_panel, wxID_ANY, _("Cluster Manager"));
    title->SetFont(wxFont(wxFontInfo(16).Bold()));
    left_sizer->Add(title, 0, wxALL, 15);
    
    // Description
    auto* desc = new wxStaticText(left_panel, wxID_ANY,
        _("The Cluster Manager provides comprehensive tools for managing "
          "high-availability database clusters, including node topology visualization, "
          "health monitoring, and automated failover management."));
    desc->Wrap(350);
    left_sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Feature list box
    auto* features_box = new wxStaticBox(left_panel, wxID_ANY, _("Planned Features"));
    auto* features_sizer = new wxStaticBoxSizer(features_box, wxVERTICAL);
    
    wxArrayString features;
    features.Add(_("• Visual cluster topology diagram"));
    features.Add(_("• Real-time node health monitoring"));
    features.Add(_("• Automatic failover configuration"));
    features.Add(_("• Load balancer integration"));
    features.Add(_("• Quorum and consensus management"));
    features.Add(_("• Rolling upgrade orchestration"));
    features.Add(_("• Performance metrics and alerting"));
    features.Add(_("• Multi-datacenter cluster support"));
    
    for (size_t i = 0; i < features.size(); ++i) {
        features_sizer->Add(new wxStaticText(left_panel, wxID_ANY, features[i]),
                           0, wxALL, 5);
    }
    
    left_sizer->Add(features_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Supported topologies
    auto* topo_box = new wxStaticBox(left_panel, wxID_ANY, _("Supported Topologies"));
    auto* topo_sizer = new wxStaticBoxSizer(topo_box, wxVERTICAL);
    
    topo_sizer->Add(new wxStaticText(left_panel, wxID_ANY,
        _("• Single-Primary (Primary-Replica)")), 0, wxALL, 5);
    topo_sizer->Add(new wxStaticText(left_panel, wxID_ANY,
        _("• Multi-Primary (Multi-Master)")), 0, wxALL, 5);
    topo_sizer->Add(new wxStaticText(left_panel, wxID_ANY,
        _("• Ring Replication")), 0, wxALL, 5);
    topo_sizer->Add(new wxStaticText(left_panel, wxID_ANY,
        _("• Sharded Clusters")), 0, wxALL, 5);
    topo_sizer->Add(new wxStaticText(left_panel, wxID_ANY,
        _("• Distributed Consensus (Raft/Paxos)")), 0, wxALL, 5);
    
    left_sizer->Add(topo_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Action buttons
    auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    docs_button_ = new wxButton(left_panel, ID_SHOW_DOCUMENTATION,
                                _("View Documentation"));
    beta_signup_button_ = new wxButton(left_panel, ID_JOIN_BETA,
                                       _("Join Beta Program"));
    beta_signup_button_->SetDefault();
    
    button_sizer->Add(docs_button_, 0, wxRIGHT, 10);
    button_sizer->Add(beta_signup_button_, 0);
    
    left_sizer->Add(button_sizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    left_panel->SetSizer(left_sizer);
    content_sizer->Add(left_panel, 0, wxEXPAND);
    
    // Right panel: Visual mockup
    auto* right_panel = new wxPanel(this, wxID_ANY);
    right_panel->SetBackgroundColour(wxColour(245, 245, 250));
    auto* right_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* mockup_title = new wxStaticText(right_panel, wxID_ANY,
                                          _("Cluster Topology Preview"));
    mockup_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    right_sizer->Add(mockup_title, 0, wxALL, 15);
    
    // ASCII-art style mockup
    auto* mockup_text = new wxStaticText(right_panel, wxID_ANY, wxT(R"(
    +------------------+
    |   Primary Node   |
    |   [HEALTHY]      |
    |   192.168.1.10   |
    +--------+---------+
             |
    +--------+---------+
    |                  |
+---v---+        +---v---+
|Replica |        |Replica |
|  #1    |        |  #2    |
|[HEALTHY]|       |[SYNC] |
+--------+        +--------+

Node Health Summary:
-------------------
Primary:   Healthy (0ms lag)
Replica 1: Healthy (12ms lag)
Replica 2: Syncing (450ms lag)

Cluster Status: OPERATIONAL
Quorum: 3/3 nodes available
Auto-failover: ENABLED
)"));
    mockup_text->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    right_sizer->Add(mockup_text, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Status bar simulation
    auto* status_box = new wxStaticBox(right_panel, wxID_ANY, _("Cluster Metrics"));
    auto* status_sizer = new wxStaticBoxSizer(status_box, wxHORIZONTAL);
    
    status_sizer->Add(new wxStaticText(right_panel, wxID_ANY,
        _("Connections: 247\n"
          "TPS: 1,245\n"
          "Replication Lag: 12ms")), 1, wxALL, 10);
    
    status_sizer->Add(new wxStaticText(right_panel, wxID_ANY,
        _("CPU: 34% avg\n"
          "Memory: 62% avg\n"
          "Disk: 78% avg")), 1, wxALL, 10);
    
    right_sizer->Add(status_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    right_panel->SetSizer(right_sizer);
    content_sizer->Add(right_panel, 1, wxEXPAND);
    
    main_sizer->Add(content_sizer, 1, wxEXPAND);
    
    SetSizer(main_sizer);
}

void ClusterManagerFrame::OnClose(wxCloseEvent& event) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    event.Skip();
}

void ClusterManagerFrame::OnShowDocumentation(wxCommandEvent& /*event*/) {
    // Open documentation URL or show help dialog
    wxMessageBox(
        _("Full documentation for the Cluster Manager will be available "
          "when the Beta release is launched.\n\n"
          "Planned topics include:\n"
          "• Cluster setup and configuration\n"
          "• Node management and failover\n"
          "• Monitoring and alerting\n"
          "• Best practices for HA deployments"),
        _("Cluster Manager Documentation"),
        wxOK | wxICON_INFORMATION,
        this
    );
}

void ClusterManagerFrame::OnJoinBeta(wxCommandEvent& /*event*/) {
    wxMessageBox(
        _("Thank you for your interest in the ScratchRobin Beta Program!\n\n"
          "To join the Beta and get early access to Cluster Management features:\n\n"
          "1. Visit: https://scratchbird.dev/beta\n"
          "2. Sign up with your email\n"
          "3. We'll notify you when Beta access is available\n\n"
          "Beta participants will receive:\n"
          "• Early access to new features\n"
          "• Direct input on feature development\n"
          "• Priority support during Beta"),
        _("Join Beta Program"),
        wxOK | wxICON_INFORMATION,
        this
    );
}

} // namespace scratchrobin
