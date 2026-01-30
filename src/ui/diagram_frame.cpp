/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "diagram_frame.h"

#include "diagram_page.h"
#include "icon_bar.h"
#include "domain_manager_frame.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "schema_manager_frame.h"
#include "sql_editor_frame.h"
#include "table_designer_frame.h"
#include "users_roles_frame.h"
#include "window_manager.h"

#include <wx/notebook.h>
#include <wx/sizer.h>

namespace scratchrobin {

wxBEGIN_EVENT_TABLE(DiagramFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, DiagramFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, DiagramFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_MONITORING, DiagramFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, DiagramFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, DiagramFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, DiagramFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, DiagramFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, DiagramFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, DiagramFrame::OnOpenIndexDesigner)
    EVT_CLOSE(DiagramFrame::OnClose)
wxEND_EVENT_TABLE()

DiagramFrame::DiagramFrame(WindowManager* windowManager, const AppConfig* config)
    : wxFrame(nullptr, wxID_ANY, "Diagrams", wxDefaultPosition, wxSize(1100, 700)),
      window_manager_(windowManager),
      app_config_(config) {
    if (window_manager_) {
        window_manager_->RegisterWindow(this);
        window_manager_->RegisterDiagramHost(this);
    }

    WindowChromeConfig chrome;
    if (app_config_) {
        chrome = app_config_->chrome.diagram;
    }

    if (chrome.showMenu) {
        MenuBuildOptions options;
        options.includeConnections = chrome.replicateMenu;
        auto* menu_bar = BuildMenuBar(options, window_manager_, this);
        SetMenuBar(menu_bar);
    }

    if (chrome.showIconBar) {
        IconBarType type = chrome.replicateIconBar ? IconBarType::Main : IconBarType::Diagram;
        BuildIconBar(this, type, 24);
    }

    notebook_ = new wxNotebook(this, wxID_ANY);
    AddDiagramTab();

    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(notebook_, 1, wxEXPAND);
    SetSizer(rootSizer);
}

void DiagramFrame::AddDiagramTab(const wxString& title) {
    if (!notebook_) {
        return;
    }
    diagram_counter_++;
    wxString label = title;
    if (label.empty()) {
        label = wxString::Format("Diagram %d", diagram_counter_);
    }
    notebook_->AddPage(new DiagramPage(notebook_), label, true);
}

void DiagramFrame::OnNewDiagram(wxCommandEvent&) {
    AddDiagramTab(wxString::Format("Diagram %d", diagram_counter_ + 1));
}

void DiagramFrame::OnNewSqlEditor(wxCommandEvent&) {
    auto* editor = new SqlEditorFrame(window_manager_, nullptr, nullptr, app_config_, nullptr);
    editor->Show(true);
}

void DiagramFrame::OnOpenMonitoring(wxCommandEvent&) {
    auto* monitor = new MonitoringFrame(window_manager_, nullptr, nullptr, app_config_);
    monitor->Show(true);
}

void DiagramFrame::OnOpenUsersRoles(wxCommandEvent&) {
    auto* users = new UsersRolesFrame(window_manager_, nullptr, nullptr, app_config_);
    users->Show(true);
}

void DiagramFrame::OnOpenJobScheduler(wxCommandEvent&) {
    auto* scheduler = new JobSchedulerFrame(window_manager_, nullptr, nullptr, app_config_);
    scheduler->Show(true);
}

void DiagramFrame::OnOpenDomainManager(wxCommandEvent&) {
    auto* domains = new DomainManagerFrame(window_manager_, nullptr, nullptr, app_config_);
    domains->Show(true);
}

void DiagramFrame::OnOpenSchemaManager(wxCommandEvent&) {
    auto* schemas = new SchemaManagerFrame(window_manager_, nullptr, nullptr, app_config_);
    schemas->Show(true);
}

void DiagramFrame::OnOpenTableDesigner(wxCommandEvent&) {
    auto* tables = new TableDesignerFrame(window_manager_, nullptr, nullptr, app_config_);
    tables->Show(true);
}

void DiagramFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    auto* indexes = new IndexDesignerFrame(window_manager_, nullptr, nullptr, app_config_);
    indexes->Show(true);
}

void DiagramFrame::OnClose(wxCloseEvent& event) {
    if (window_manager_) {
        window_manager_->UnregisterDiagramHost(this);
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
    event.Skip();
}

} // namespace scratchrobin
