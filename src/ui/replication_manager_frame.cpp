/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/replication_manager_frame.h"
#include <wx/wx.h>

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/statbox.h>
#include <wx/scrolwin.h>
#include <wx/listctrl.h>
#include <wx/gauge.h>

#include "ui/window_manager.h"
#include "core/connection_manager.h"
#include "core/config.h"

namespace scratchrobin {

enum {
    ID_SHOW_DOCUMENTATION = wxID_HIGHEST + 1,
    ID_JOIN_BETA
};

wxBEGIN_EVENT_TABLE(ReplicationManagerFrame, wxFrame)
    EVT_CLOSE(ReplicationManagerFrame::OnClose)
    EVT_BUTTON(ID_SHOW_DOCUMENTATION, ReplicationManagerFrame::OnShowDocumentation)
    EVT_BUTTON(ID_JOIN_BETA, ReplicationManagerFrame::OnJoinBeta)
wxEND_EVENT_TABLE()

ReplicationManagerFrame::ReplicationManagerFrame(WindowManager* windowManager,
                                                 ConnectionManager* connectionManager,
                                                 const std::vector<ConnectionProfile>* connections,
                                                 const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, _("Replication Manager [Beta Preview]"),
              wxDefaultPosition, wxSize(950, 700),
              wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , window_manager_(windowManager)
    , connection_manager_(connectionManager)
    , connections_(connections)
    , app_config_(appConfig) {
    
    SetBackgroundColour(wxColour(255, 250, 250));
    
    BuildMenu();
    BuildLayout();
    
    CentreOnScreen();
}

ReplicationManagerFrame::~ReplicationManagerFrame() = default;

void ReplicationManagerFrame::BuildMenu() {
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

void ReplicationManagerFrame::BuildLayout() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Beta banner
    auto* banner_panel = new wxPanel(this, wxID_ANY);
    banner_panel->SetBackgroundColour(wxColour(180, 100, 100));  // Dusty rose
    auto* banner_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    auto* banner_text = new wxStaticText(banner_panel, wxID_ANY,
        _("BETA FEATURE PREVIEW - Replication Management capabilities coming in Beta release"));
    banner_text->SetForegroundColour(*wxWHITE);
    banner_text->SetFont(wxFont(wxFontInfo(11).Bold()));
    banner_sizer->Add(banner_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    banner_panel->SetSizer(banner_sizer);
    
    main_sizer->Add(banner_panel, 0, wxEXPAND);
    
    // Content with notebook tabs
    auto* content_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left: Info panel
    auto* left_panel = new wxPanel(this, wxID_ANY);
    auto* left_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* title = new wxStaticText(left_panel, wxID_ANY, _("Replication Manager"));
    title->SetFont(wxFont(wxFontInfo(16).Bold()));
    left_sizer->Add(title, 0, wxALL, 15);
    
    auto* desc = new wxStaticText(left_panel, wxID_ANY,
        _("Monitor and manage database replication across your infrastructure. "
          "Track replication lag, manage slots, and configure logical replication "
          "publications and subscriptions."));
    desc->Wrap(350);
    left_sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Features
    auto* features_box = new wxStaticBox(left_panel, wxID_ANY, _("Planned Features"));
    auto* features_sizer = new wxStaticBoxSizer(features_box, wxVERTICAL);
    
    wxArrayString features;
    features.Add(_("• Real-time replication lag monitoring"));
    features.Add(_("• Physical and logical replication support"));
    features.Add(_("• Replication slot management"));
    features.Add(_("• Publication/subscription management"));
    features.Add(_("• Conflict detection and resolution"));
    features.Add(_("• Replication topology visualization"));
    features.Add(_("• Historical lag trend analysis"));
    features.Add(_("• Alerting on replication issues"));
    
    for (size_t i = 0; i < features.size(); ++i) {
        features_sizer->Add(new wxStaticText(left_panel, wxID_ANY, features[i]),
                           0, wxALL, 5);
    }
    
    left_sizer->Add(features_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Buttons
    auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    docs_button_ = new wxButton(left_panel, ID_SHOW_DOCUMENTATION, _("View Documentation"));
    beta_signup_button_ = new wxButton(left_panel, ID_JOIN_BETA, _("Join Beta Program"));
    beta_signup_button_->SetDefault();
    
    button_sizer->Add(docs_button_, 0, wxRIGHT, 10);
    button_sizer->Add(beta_signup_button_, 0);
    left_sizer->Add(button_sizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    left_panel->SetSizer(left_sizer);
    content_sizer->Add(left_panel, 0, wxEXPAND);
    
    // Right: Tabbed mockup
    auto* right_panel = new wxPanel(this, wxID_ANY);
    right_panel->SetBackgroundColour(wxColour(250, 245, 245));
    auto* right_sizer = new wxBoxSizer(wxVERTICAL);
    
    notebook_ = new wxNotebook(right_panel, wxID_ANY);
    
    // Tab 1: Topology
    auto* topo_panel = new wxPanel(notebook_, wxID_ANY);
    auto* topo_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* topo_title = new wxStaticText(topo_panel, wxID_ANY, _("Replication Topology"));
    topo_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    topo_sizer->Add(topo_title, 0, wxALL, 10);
    
    auto* topo_mockup = new wxStaticText(topo_panel, wxID_ANY, wxT(R"(
Master (primary-db-01)
├── Replica 1 (replica-db-01) - [Sync: 0.2s lag]
├── Replica 2 (replica-db-02) - [Sync: 0.5s lag]
└── Cascading Replica (replica-db-03)
    └── Leaf Replica (replica-db-04) - [Sync: 1.2s lag]

Replication Mode: Asynchronous Streaming
WAL Shipping: Enabled
Slot Status: Active (4 slots)
)"));
    topo_mockup->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    topo_sizer->Add(topo_mockup, 1, wxEXPAND | wxALL, 10);
    
    topo_panel->SetSizer(topo_sizer);
    notebook_->AddPage(topo_panel, _("Topology"));
    
    // Tab 2: Lag Monitor
    auto* lag_panel = new wxPanel(notebook_, wxID_ANY);
    auto* lag_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* lag_title = new wxStaticText(lag_panel, wxID_ANY, _("Replication Lag"));
    lag_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    lag_sizer->Add(lag_title, 0, wxALL, 10);
    
    auto* lag_mockup = new wxStaticText(lag_panel, wxID_ANY, wxT(R"(
Node                | Lag (bytes) | Lag (time) | Apply Rate | Status
--------------------+-------------+------------+------------+--------
replica-db-01       |    1.2 MB   |   0.2s     | 45 MB/s    | [OK]
replica-db-02       |    2.8 MB   |   0.5s     | 42 MB/s    | [OK]
replica-db-03       |    5.1 MB   |   0.9s     | 38 MB/s    | [OK]
replica-db-04       |   12.5 MB   |   2.4s     | 22 MB/s    | [WARN]

Last Updated: 2026-02-03 14:32:15
)"));
    lag_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    lag_sizer->Add(lag_mockup, 1, wxEXPAND | wxALL, 10);
    
    // Simulated progress bars for lag visualization
    auto* lag_viz_box = new wxStaticBox(lag_panel, wxID_ANY, _("Lag Visualization"));
    auto* lag_viz_sizer = new wxStaticBoxSizer(lag_viz_box, wxVERTICAL);
    
    lag_viz_sizer->Add(new wxStaticText(lag_panel, wxID_ANY, _("replica-db-01:")));
    auto* gauge1 = new wxGauge(lag_panel, wxID_ANY, 100, wxDefaultPosition, wxSize(200, 20));
    gauge1->SetValue(10);
    lag_viz_sizer->Add(gauge1, 0, wxEXPAND | wxBOTTOM, 5);
    
    lag_viz_sizer->Add(new wxStaticText(lag_panel, wxID_ANY, _("replica-db-02:")));
    auto* gauge2 = new wxGauge(lag_panel, wxID_ANY, 100, wxDefaultPosition, wxSize(200, 20));
    gauge2->SetValue(25);
    lag_viz_sizer->Add(gauge2, 0, wxEXPAND | wxBOTTOM, 5);
    
    lag_viz_sizer->Add(new wxStaticText(lag_panel, wxID_ANY, _("replica-db-03:")));
    auto* gauge3 = new wxGauge(lag_panel, wxID_ANY, 100, wxDefaultPosition, wxSize(200, 20));
    gauge3->SetValue(45);
    lag_viz_sizer->Add(gauge3, 0, wxEXPAND | wxBOTTOM, 5);
    
    lag_viz_sizer->Add(new wxStaticText(lag_panel, wxID_ANY, _("replica-db-04:")));
    auto* gauge4 = new wxGauge(lag_panel, wxID_ANY, 100, wxDefaultPosition, wxSize(200, 20));
    gauge4->SetValue(75);
    lag_viz_sizer->Add(gauge4, 0, wxEXPAND | wxBOTTOM, 5);
    
    lag_sizer->Add(lag_viz_sizer, 0, wxEXPAND | wxALL, 10);
    
    lag_panel->SetSizer(lag_sizer);
    notebook_->AddPage(lag_panel, _("Lag Monitor"));
    
    // Tab 3: Slots
    auto* slot_panel = new wxPanel(notebook_, wxID_ANY);
    auto* slot_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* slot_title = new wxStaticText(slot_panel, wxID_ANY, _("Replication Slots"));
    slot_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    slot_sizer->Add(slot_title, 0, wxALL, 10);
    
    auto* slot_mockup = new wxStaticText(slot_panel, wxID_ANY, wxT(R"(
Slot Name       | Plugin  | Database | State    | Confirmed LSN | Restart LSN
----------------+---------+----------+----------+---------------+------------
replica_01_slot | -       | -        | Active   | 0/12A4F800    | 0/12A4F000
replica_02_slot | -       | -        | Active   | 0/12A4F600    | 0/12A4F000
logical_01_slot | pgoutput| mydb     | Inactive | 0/12800000    | 0/12800000
replica_03_slot | -       | -        | Active   | 0/12A4F200    | 0/12A4F000

Total Slots: 4
Active: 3
Inactive: 1
)"));
    slot_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    slot_sizer->Add(slot_mockup, 1, wxEXPAND | wxALL, 10);
    
    slot_panel->SetSizer(slot_sizer);
    notebook_->AddPage(slot_panel, _("Slots"));
    
    // Tab 4: Publications/Subscriptions
    auto* pubsub_panel = new wxPanel(notebook_, wxID_ANY);
    auto* pubsub_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* pubsub_title = new wxStaticText(pubsub_panel, wxID_ANY,
                                          _("Publications & Subscriptions"));
    pubsub_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    pubsub_sizer->Add(pubsub_title, 0, wxALL, 10);
    
    auto* pub_mockup = new wxStaticText(pubsub_panel, wxID_ANY, wxT(R"(
[Publications]
Name           | Tables      | Operations           | Filter
---------------+-------------+----------------------+--------
users_pub      | users       | INSERT,UPDATE,DELETE | -
orders_pub     | orders      | INSERT,UPDATE        | active=true
analytics_pub  | ALL TABLES  | INSERT,UPDATE,DELETE | -

[Subscriptions]
Name           | Publications  | Connection                | Enabled
---------------+---------------+---------------------------+--------
sub_replica_01 | users_pub     | host=replica01,...        | Yes
sub_analytics  | analytics_pub | host=analytics-db,...     | Yes
sub_warehouse  | orders_pub    | host=warehouse,...        | Yes
)"));
    pub_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    pubsub_sizer->Add(pub_mockup, 1, wxEXPAND | wxALL, 10);
    
    pubsub_panel->SetSizer(pubsub_sizer);
    notebook_->AddPage(pubsub_panel, _("Pub/Sub"));
    
    right_sizer->Add(notebook_, 1, wxEXPAND | wxALL, 10);
    right_panel->SetSizer(right_sizer);
    content_sizer->Add(right_panel, 1, wxEXPAND);
    
    main_sizer->Add(content_sizer, 1, wxEXPAND);
    SetSizer(main_sizer);
}

void ReplicationManagerFrame::OnClose(wxCloseEvent& event) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

void ReplicationManagerFrame::OnShowDocumentation(wxCommandEvent& /*event*/) {
    wxMessageBox(
        _("Full documentation for the Replication Manager will be available "
          "when the Beta release is launched.\n\n"
          "Planned topics include:\n"
          "• Setting up streaming replication\n"
          "• Managing replication slots\n"
          "• Configuring logical replication\n"
          "• Monitoring replication lag\n"
          "• Handling replication conflicts\n"
          "• Failover and switchover procedures"),
        _("Replication Manager Documentation"),
        wxOK | wxICON_INFORMATION,
        this
    );
}

void ReplicationManagerFrame::OnJoinBeta(wxCommandEvent& /*event*/) {
    wxMessageBox(
        _("Thank you for your interest in the ScratchRobin Beta Program!\n\n"
          "To join the Beta and get early access to Replication Management:\n\n"
          "1. Visit: https://scratchbird.dev/beta\n"
          "2. Sign up with your email\n"
          "3. We'll notify you when Beta access is available"),
        _("Join Beta Program"),
        wxOK | wxICON_INFORMATION,
        this
    );
}

} // namespace scratchrobin
