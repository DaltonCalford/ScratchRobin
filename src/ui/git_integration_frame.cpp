/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/git_integration_frame.h"
#include <wx/wx.h>

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/statbox.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>

#include "ui/window_manager.h"
#include "core/connection_manager.h"
#include "core/config.h"

namespace scratchrobin {

enum {
    ID_SHOW_DOCUMENTATION = wxID_HIGHEST + 1,
    ID_JOIN_BETA
};

wxBEGIN_EVENT_TABLE(GitIntegrationFrame, wxFrame)
    EVT_CLOSE(GitIntegrationFrame::OnClose)
    EVT_BUTTON(ID_SHOW_DOCUMENTATION, GitIntegrationFrame::OnShowDocumentation)
    EVT_BUTTON(ID_JOIN_BETA, GitIntegrationFrame::OnJoinBeta)
wxEND_EVENT_TABLE()

GitIntegrationFrame::GitIntegrationFrame(WindowManager* windowManager,
                                         ConnectionManager* connectionManager,
                                         const std::vector<ConnectionProfile>* connections,
                                         const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, _("Git Integration [Beta Preview]"),
              wxDefaultPosition, wxSize(1000, 700),
              wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , window_manager_(windowManager)
    , connection_manager_(connectionManager)
    , connections_(connections)
    , app_config_(appConfig) {
    
    SetBackgroundColour(wxColour(250, 250, 250));
    
    BuildMenu();
    BuildLayout();
    
    CentreOnScreen();
}

GitIntegrationFrame::~GitIntegrationFrame() = default;

void GitIntegrationFrame::BuildMenu() {
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

void GitIntegrationFrame::BuildLayout() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Beta banner
    auto* banner_panel = new wxPanel(this, wxID_ANY);
    banner_panel->SetBackgroundColour(wxColour(100, 100, 140));  // Purple-gray
    auto* banner_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    auto* banner_text = new wxStaticText(banner_panel, wxID_ANY,
        _("BETA FEATURE PREVIEW - Git Integration coming in Beta release"));
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
    
    auto* title = new wxStaticText(left_panel, wxID_ANY, _("Git Integration"));
    title->SetFont(wxFont(wxFontInfo(16).Bold()));
    left_sizer->Add(title, 0, wxALL, 15);
    
    auto* desc = new wxStaticText(left_panel, wxID_ANY,
        _("Version control for your database schema and migration scripts. "
          "Track changes, collaborate with team members, and deploy with "
          "confidence using Git-based workflows."));
    desc->Wrap(350);
    left_sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Features
    auto* features_box = new wxStaticBox(left_panel, wxID_ANY, _("Planned Features"));
    auto* features_sizer = new wxStaticBoxSizer(features_box, wxVERTICAL);
    
    wxArrayString features;
    features.Add(_("• Schema versioning with Git"));
    features.Add(_("• Automatic DDL generation"));
    features.Add(_("• Migration script management"));
    features.Add(_("• Visual diff for schema changes"));
    features.Add(_("• Branch-based workflows"));
    features.Add(_("• Pull request integration"));
    features.Add(_("• CI/CD pipeline hooks"));
    features.Add(_("• Code review for DB changes"));
    features.Add(_("• Deployment tracking"));
    
    for (size_t i = 0; i < features.size(); ++i) {
        features_sizer->Add(new wxStaticText(left_panel, wxID_ANY, features[i]),
                           0, wxALL, 5);
    }
    
    left_sizer->Add(features_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    // Workflow
    auto* workflow_box = new wxStaticBox(left_panel, wxID_ANY, _("Supported Workflows"));
    auto* workflow_sizer = new wxStaticBoxSizer(workflow_box, wxVERTICAL);
    
    workflow_sizer->Add(new wxStaticText(left_panel, wxID_ANY,
        _("• GitFlow for database projects\n"
          "• Trunk-based development\n"
          "• Environment promotion\n"
          "• Feature branch deployments\n"
          "• Hotfix management")), 0, wxALL, 10);
    
    left_sizer->Add(workflow_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
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
    
    // Right panel: Git mockup
    auto* right_panel = new wxPanel(this, wxID_ANY);
    right_panel->SetBackgroundColour(wxColour(245, 245, 245));
    auto* right_sizer = new wxBoxSizer(wxVERTICAL);
    
    notebook_ = new wxNotebook(right_panel, wxID_ANY);
    
    // Tab 1: Repository Status
    auto* status_panel = new wxPanel(notebook_, wxID_ANY);
    auto* status_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* status_title = new wxStaticText(status_panel, wxID_ANY, _("Repository Status"));
    status_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    status_sizer->Add(status_title, 0, wxALL, 10);
    
    auto* status_mockup = new wxStaticText(status_panel, wxID_ANY, wxT(R"(
Repository: myproject-db
Branch: feature/add-user-table
Remote: origin (github.com:acme/myproject-db.git)

Status:
  Modified:  schema/tables/public/users.sql
  Modified:  migrations/V003__add_user_table.sql
  Staged:    (none)
  Untracked: (none)

Ahead: 3 commits | Behind: 0 commits

Last Commit:
  abc1234 Add users table schema
  Author: Jane Developer <jane@example.com>
  Date:   2026-02-03 10:45:23
)"));
    status_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    status_sizer->Add(status_mockup, 1, wxEXPAND | wxALL, 10);
    
    status_panel->SetSizer(status_sizer);
    notebook_->AddPage(status_panel, _("Status"));
    
    // Tab 2: Commit History
    auto* history_panel = new wxPanel(notebook_, wxID_ANY);
    auto* history_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* history_title = new wxStaticText(history_panel, wxID_ANY, _("Commit History"));
    history_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    history_sizer->Add(history_title, 0, wxALL, 10);
    
    auto* history_mockup = new wxStaticText(history_panel, wxID_ANY, wxT(R"(
abc1234 Add users table schema                        (HEAD -> feature/add-user-table)
        Jane Developer, 2026-02-03 10:45
        
def5678 Update indexes for performance
        John Smith, 2026-02-02 16:30
        
789abcd Add foreign key constraints
        Jane Developer, 2026-02-02 14:15
        
ef0123a Initial schema import
        John Smith, 2026-02-01 09:00

[main --------------------------------------------------------------------------]
        
456cdef Create orders table
        Jane Developer, 2026-02-03 11:20
        
1234abc Add customer table
        John Smith, 2026-02-03 09:30
)"));
    history_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    history_sizer->Add(history_mockup, 1, wxEXPAND | wxALL, 10);
    
    history_panel->SetSizer(history_sizer);
    notebook_->AddPage(history_panel, _("History"));
    
    // Tab 3: Schema Diff
    auto* diff_panel = new wxPanel(notebook_, wxID_ANY);
    auto* diff_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* diff_title = new wxStaticText(diff_panel, wxID_ANY, _("Schema Changes"));
    diff_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    diff_sizer->Add(diff_title, 0, wxALL, 10);
    
    auto* diff_mockup = new wxStaticText(diff_panel, wxID_ANY, wxT(R"(
File: schema/tables/public/users.sql
=====================================

@@ -10,6 +10,8 @@ CREATE TABLE users (
     id          SERIAL PRIMARY KEY,
     username    VARCHAR(50) NOT NULL UNIQUE,
     email       VARCHAR(255) NOT NULL,
+    created_at  TIMESTAMP DEFAULT NOW(),
+    status      VARCHAR(20) DEFAULT 'active',
     password_hash VARCHAR(255) NOT NULL
 );
 
@@ -18,5 +20,6 @@ CREATE TABLE users (
 CREATE INDEX idx_users_username ON users(username);
 CREATE INDEX idx_users_email ON users(email);
+CREATE INDEX idx_users_status ON users(status);
 
 )"));
    diff_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    diff_sizer->Add(diff_mockup, 1, wxEXPAND | wxALL, 10);
    
    diff_panel->SetSizer(diff_sizer);
    notebook_->AddPage(diff_panel, _("Diff"));
    
    // Tab 4: Migrations
    auto* migration_panel = new wxPanel(notebook_, wxID_ANY);
    auto* migration_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* migration_title = new wxStaticText(migration_panel, wxID_ANY,
                                             _("Migration Scripts"));
    migration_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    migration_sizer->Add(migration_title, 0, wxALL, 10);
    
    auto* migration_mockup = new wxStaticText(migration_panel, wxID_ANY, wxT(R"(
Version | Description                    | Type     | Status    | Applied
--------+--------------------------------+----------+-----------+------------------
1.0.0   | Initial schema                 | Baseline | Applied   | 2026-01-15 09:00
1.0.1   | Add indexes on foreign keys    | Upgrade  | Applied   | 2026-01-16 14:30
1.0.2   | Create audit log table         | Upgrade  | Applied   | 2026-01-20 11:45
1.0.3   | Add users table                | Upgrade  | Pending   | -
1.0.4   | Add orders table               | Upgrade  | Pending   | -

[Pending Migrations: 2]  [Apply All]  [Apply Selected]  [Generate Rollback]
)"));
    migration_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    migration_sizer->Add(migration_mockup, 1, wxEXPAND | wxALL, 10);
    
    migration_panel->SetSizer(migration_sizer);
    notebook_->AddPage(migration_panel, _("Migrations"));
    
    // Tab 5: Branches
    auto* branch_panel = new wxPanel(notebook_, wxID_ANY);
    auto* branch_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* branch_title = new wxStaticText(branch_panel, wxID_ANY, _("Branches"));
    branch_title->SetFont(wxFont(wxFontInfo(12).Bold()));
    branch_sizer->Add(branch_title, 0, wxALL, 10);
    
    auto* branch_mockup = new wxStaticText(branch_panel, wxID_ANY, wxT(R"(
Local Branches:
* feature/add-user-table    abc1234  [ahead 3]  Add users table schema
  feature/orders-module     456cdef  [ahead 2]  Create orders table
  main                      ef0123a  [origin/main]  Initial schema import
  develop                   7890abc  [ahead 5, behind 2]  Merge branch 'feature/...

Remote Branches:
  origin/main               ef0123a  Last fetched: 2 hours ago
  origin/develop            7890abc  
  origin/release/v1.0       abcde12  

[Create Branch]  [Checkout]  [Merge]  [Delete]  [Push]  [Pull]
)"));
    branch_mockup->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    branch_sizer->Add(branch_mockup, 1, wxEXPAND | wxALL, 10);
    
    branch_panel->SetSizer(branch_sizer);
    notebook_->AddPage(branch_panel, _("Branches"));
    
    right_sizer->Add(notebook_, 1, wxEXPAND | wxALL, 10);
    right_panel->SetSizer(right_sizer);
    content_sizer->Add(right_panel, 1, wxEXPAND);
    
    main_sizer->Add(content_sizer, 1, wxEXPAND);
    SetSizer(main_sizer);
}

void GitIntegrationFrame::OnClose(wxCloseEvent& event) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

void GitIntegrationFrame::OnShowDocumentation(wxCommandEvent& /*event*/) {
    wxMessageBox(
        _("Full documentation for Git Integration will be available "
          "when the Beta release is launched.\n\n"
          "Planned topics include:\n"
          "• Setting up Git for database projects\n"
          "• Schema versioning workflows\n"
          "• Migration script management\n"
          "• Branch-based database development\n"
          "• CI/CD integration for DB changes\n"
          "• Code review practices for DDL\n"
          "• Deployment automation"),
        _("Git Integration Documentation"),
        wxOK | wxICON_INFORMATION,
        this
    );
}

void GitIntegrationFrame::OnJoinBeta(wxCommandEvent& /*event*/) {
    wxMessageBox(
        _("Thank you for your interest in the ScratchRobin Beta Program!\n\n"
          "To join the Beta and get early access to Git Integration:\n\n"
          "1. Visit: https://scratchbird.dev/beta\n"
          "2. Sign up with your email\n"
          "3. We'll notify you when Beta access is available"),
        _("Join Beta Program"),
        wxOK | wxICON_INFORMATION,
        this
    );
}

} // namespace scratchrobin
