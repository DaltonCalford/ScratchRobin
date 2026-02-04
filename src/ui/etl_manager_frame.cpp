/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/etl_manager_frame.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/statbox.h>
#include <wx/listctrl.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/splitter.h>

#include "app/window_manager.h"
#include "core/connection_manager.h"
#include "core/config.h"

namespace scratchrobin {

enum {
    ID_SHOW_DOCUMENTATION = wxID_HIGHEST + 1,
    ID_JOIN_BETA
};

wxBEGIN_EVENT_TABLE(EtlManagerFrame, wxFrame)
    EVT_CLOSE(EtlManagerFrame::OnClose)
    EVT_BUTTON(ID_SHOW_DOCUMENTATION, EtlManagerFrame::OnShowDocumentation)
    EVT_BUTTON(ID_JOIN_BETA, EtlManagerFrame::OnJoinBeta)
wxEND_EVENT_TABLE()

EtlManagerFrame::EtlManagerFrame(WindowManager* windowManager,
                                 ConnectionManager* connectionManager,
                                 const std::vector<ConnectionProfile>* connections,
                                 const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, _("ETL Manager [Beta Preview]"),
              wxDefaultPosition, wxSize(1000, 750),
              wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , window_manager_(windowManager)
    , connection_manager_(connectionManager)
    , connections_(connections)
    , app_config_(appConfig) {
    
    SetBackgroundColour(wxColour(255, 255, 245));
    
    BuildMenu();
    BuildLayout();
    
    CentreOnScreen();
}

EtlManagerFrame::~EtlManagerFrame() = default;

void EtlManagerFrame::BuildMenu() {
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

void EtlManagerFrame::BuildLayout() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Beta banner
    auto* banner_panel = new wxPanel(this, wxID_ANY);
    banner_panel->SetBackgroundColour(wxColour(100, 140, 100));  // Sage green
    auto* banner_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    auto* banner_text = new wxStaticText(banner_panel, wxID_ANY,
        _("BETA FEATURE PREVIEW - ETL Tools coming in Beta release"));
    banner_text->SetForegroundColour(*wxWHITE);
    banner_text->SetFont(wxFont(wxFontInfo(11).Bold()));
    banner_sizer->Add(banner_text, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    banner_panel->SetSizer(banner_sizer);
    
    main_sizer->Add(banner_panel, 0, wxEXPAND);
    
    // Content area
    auto* content_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left panel: Info
    auto* left_panel = new wxPanel(this, wxID_ANY);
    auto* left_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* title = new wxStaticText(left_panel, wxID_ANY, _("ETL Manager"));
    title->SetFont(wxFont(wxFontInfo(16).Bold()));
    left_sizer->Add(title, 0, wxALL, 15);
    
    auto* desc = new wxStaticText(left_panel, wxID_ANY,
        _("Design, schedule, and monitor data integration workflows. "
          "Extract data from multiple sources, apply transformations, "
          "and load into target databases with full data quality validation."));
    desc->Wrap(350);
    left_sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Features
    auto* features_box = new wxStaticBox(left_panel, wxID_ANY, _("Planned Features"));
    auto* features_sizer = new wxStaticBoxSizer(features_box, wxVERTICAL);
    
    wxArrayString features;
    features.Add(_("• Visual job designer"));
    features.Add(_("• 20+ data source connectors"));
    features.Add(_("• Drag-and-drop transformation"));
    features.Add(_("• Data quality rules engine"));
    features.Add(_("• Workflow orchestration"));
    features.Add(_("• Incremental loading"));
    features.Add(_("• Change data capture (CDC)"));
    features.Add(_("• Schedule and monitor jobs"));
    features.Add(_("• Error handling and retry"));
    features.Add(_("• Data lineage tracking"));
    
    for (size_t i = 0; i < features.size(); ++i) {
        features_sizer->Add(new wxStaticText(left_panel, wxID_ANY, features[i]),
                           0, wxALL, 5);
    }
    
    left_sizer->Add(features_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Source types
    auto* source_box = new wxStaticBox(left_panel, wxID_ANY, _("Supported Sources"));
    auto* source_sizer = new wxStaticBoxSizer(source_box, wxVERTICAL);
    
    source_sizer->Add(new wxStaticText(left_panel, wxID_ANY,
        _("• PostgreSQL, MySQL, Firebird, ScratchBird\n"
          "• CSV, JSON, XML, Excel, Parquet\n"
          "• REST APIs, Message Queues\n"
          "• Cloud storage (S3, GCS, Azure)")), 0, wxALL, 10);
    
    left_sizer->Add(source_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
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
    
    // Right panel: Mockup designer
    auto* right_panel = new wxPanel(this, wxID_ANY);
    right_panel->SetBackgroundColour(wxColour(250, 250, 240));
    auto* right_sizer = new wxBoxSizer(wxVERTICAL);
    
    notebook_ = new wxNotebook(right_panel, wxID_ANY);
    
    // Tab 1: Job Designer Mockup
    auto* designer_panel = new wxPanel(notebook_, wxID_ANY);
    auto* designer_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* designer_title = new wxStaticText(designer_panel, wxID_ANY,
                                            _("Visual Job Designer"));
    designer_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    designer_sizer->Add(designer_title, 0, wxALL, 10);
    
    auto* designer_mockup = new wxStaticText(designer_panel, wxID_ANY, wxT(R"(
+-------------------------------------------------------------+
|  Job: Daily Sales ETL                                       |
+-------------------------------------------------------------+
|                                                             |
|  [Source: PostgreSQL]      [Source: CSV Files]             |
|       |                           |                         |
|       v                           v                         |
|  [Extract: orders]         [Extract: returns]              |
|       |                           |                         |
|       +------------+--------------+                         |
|                    |                                        |
|                    v                                        |
|            [Transform: Join]                               |
|                    |                                        |
|                    v                                        |
|            [Filter: status='completed']                    |
|                    |                                        |
|                    v                                        |
|            [Calculate: net_amount]                         |
|                    |                                        |
|                    v                                        |
|            [Target: Data Warehouse]                        |
|                    |                                        |
|                    v                                        |
|            [Load: fact_sales]                              |
|                                                             |
+-------------------------------------------------------------+

Status: IDLE | Last Run: 2026-02-02 02:00:00 | Duration: 4m 32s
)"));
    designer_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    designer_sizer->Add(designer_mockup, 1, wxEXPAND | wxALL, 10);
    
    designer_panel->SetSizer(designer_sizer);
    notebook_->AddPage(designer_panel, _("Job Designer"));
    
    // Tab 2: Job List
    auto* jobs_panel = new wxPanel(notebook_, wxID_ANY);
    auto* jobs_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* jobs_title = new wxStaticText(jobs_panel, wxID_ANY, _("ETL Jobs"));
    jobs_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    jobs_sizer->Add(jobs_title, 0, wxALL, 10);
    
    auto* jobs_mockup = new wxStaticText(jobs_panel, wxID_ANY, wxT(R"(
Name                 | Source           | Target           | Schedule   | Status
---------------------+------------------+------------------+------------+--------
Daily Sales ETL      | orders_db        | warehouse        | Daily 2AM  | [OK]
Customer Sync        | CRM_API          | customer_db      | Hourly     | [RUN]
Product Import       | products.csv     | catalog_db       | On Demand  | [IDLE]
Analytics Export     | warehouse        | S3/parquet       | Weekly Sun | [OK]
Full Backup          | All DBs          | Backup Storage   | Daily 12AM | [OK]
CDC Stream           | prod_db          | replica          | Continuous | [RUN]
)"));
    jobs_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    jobs_sizer->Add(jobs_mockup, 1, wxEXPAND | wxALL, 10);
    
    jobs_panel->SetSizer(jobs_sizer);
    notebook_->AddPage(jobs_panel, _("Jobs"));
    
    // Tab 3: Transform Library
    auto* transform_panel = new wxPanel(notebook_, wxID_ANY);
    auto* transform_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* transform_title = new wxStaticText(transform_panel, wxID_ANY,
                                             _("Transformation Library"));
    transform_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    transform_sizer->Add(transform_title, 0, wxALL, 10);
    
    auto* transform_mockup = new wxStaticText(transform_panel, wxID_ANY, wxT(R"(
[Column Operations]
├── Map           - Direct column mapping
├── Rename        - Rename columns
├── Cast          - Type conversion
├── Default       - Set default values
└── Calculated    - Formula/expression columns

[Row Operations]
├── Filter        - Row filtering (WHERE conditions)
├── Sort          - Order by columns
└── Deduplicate   - Remove duplicate rows

[Data Cleansing]
├── Trim          - Remove whitespace
├── Normalize     - Standardize values
├── Validate      - Data validation rules
├── Replace       - Find and replace
└── Null Handling - NULL value strategies

[Advanced]
├── Aggregate     - Group by with aggregates
├── Pivot         - Pivot tables
├── Lookup        - Reference data joins
├── Custom SQL    - SQL transformations
└── Script        - Python/JS transformations
)"));
    transform_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    transform_sizer->Add(transform_mockup, 1, wxEXPAND | wxALL, 10);
    
    transform_panel->SetSizer(transform_sizer);
    notebook_->AddPage(transform_panel, _("Transforms"));
    
    // Tab 4: Run History
    auto* history_panel = new wxPanel(notebook_, wxID_ANY);
    auto* history_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* history_title = new wxStaticText(history_panel, wxID_ANY, _("Run History"));
    history_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    history_sizer->Add(history_title, 0, wxALL, 10);
    
    auto* history_mockup = new wxStaticText(history_panel, wxID_ANY, wxT(R"(
Job              | Started             | Duration | Rows In | Rows Out | Status
-----------------+---------------------+----------+---------+----------+--------
Daily Sales ETL  | 2026-02-02 02:00:00 | 4m 32s   | 45,231  | 44,892   | SUCCESS
Customer Sync    | 2026-02-03 14:00:00 | 1m 12s   | 1,245   | 1,245    | RUNNING
Product Import   | 2026-02-03 10:30:00 | 2m 45s   | 5,600   | 5,598    | SUCCESS
Analytics Export | 2026-02-02 03:00:00 | 8m 15s   | 892K    | 892K     | SUCCESS
Daily Sales ETL  | 2026-02-01 02:00:00 | 5m 10s   | 42,890  | 42,567   | SUCCESS
Customer Sync    | 2026-02-03 13:00:00 | 1m 08s   | 980     | 980      | SUCCESS
)"));
    history_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    history_sizer->Add(history_mockup, 1, wxEXPAND | wxALL, 10);
    
    history_panel->SetSizer(history_sizer);
    notebook_->AddPage(history_panel, _("History"));
    
    right_sizer->Add(notebook_, 1, wxEXPAND | wxALL, 10);
    right_panel->SetSizer(right_sizer);
    content_sizer->Add(right_panel, 1, wxEXPAND);
    
    main_sizer->Add(content_sizer, 1, wxEXPAND);
    SetSizer(main_sizer);
}

void EtlManagerFrame::OnClose(wxCloseEvent& event) {
    if (window_manager_) {
        window_manager_->OnChildWindowClosing(this);
    }
    Destroy();
}

void EtlManagerFrame::OnShowDocumentation(wxCommandEvent& /*event*/) {
    wxMessageBox(
        _("Full documentation for the ETL Manager will be available "
          "when the Beta release is launched.\n\n"
          "Planned topics include:\n"
          "• Creating ETL jobs with the visual designer\n"
          "• Configuring data sources and targets\n"
          "• Building transformation pipelines\n"
          "• Setting up data quality rules\n"
          "• Scheduling and monitoring jobs\n"
          "• Workflow orchestration\n"
          "• Change data capture (CDC) setup"),
        _("ETL Manager Documentation"),
        wxOK | wxICON_INFORMATION,
        this
    );
}

void EtlManagerFrame::OnJoinBeta(wxCommandEvent& /*event*/) {
    wxMessageBox(
        _("Thank you for your interest in the ScratchRobin Beta Program!\n\n"
          "To join the Beta and get early access to ETL Tools:\n\n"
          "1. Visit: https://scratchbird.dev/beta\n"
          "2. Sign up with your email\n"
          "3. We'll notify you when Beta access is available"),
        _("Join Beta Program"),
        wxOK | wxICON_INFORMATION,
        this
    );
}

} // namespace scratchrobin
