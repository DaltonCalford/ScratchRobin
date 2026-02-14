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

#include "diagram_mini_view.h"
#include "diagram_page.h"
#include "diagram/diagram_serialization.h"
#include "diagram/trace_util.h"
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
#include "core/project.h"

#include <wx/notebook.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <filesystem>

namespace scratchrobin {
namespace {

std::filesystem::path GetDiagramStorageRoot(const Project& project) {
    std::filesystem::path base = project.project_root_path;
    base /= project.config.designs_path;
    base /= project.config.diagrams_path;
    return base;
}

std::string MakeRelativePath(const Project& project, const std::filesystem::path& full_path) {
    std::error_code ec;
    std::filesystem::path root = project.project_root_path;
    std::filesystem::path rel = std::filesystem::relative(full_path, root, ec);
    if (ec) {
        return full_path.string();
    }
    return rel.generic_string();
}

} // namespace

wxBEGIN_EVENT_TABLE(DiagramFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, DiagramFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_DIAGRAM_OPEN, DiagramFrame::OnOpenDiagram)
    EVT_MENU(ID_MENU_DIAGRAM_SAVE, DiagramFrame::OnSaveDiagram)
    EVT_MENU(ID_MENU_DIAGRAM_SAVE_AS, DiagramFrame::OnSaveDiagramAs)
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

// Static member initialization
int DiagramFrame::nextFormId_ = 1;

DiagramFrame::DiagramFrame(WindowManager* windowManager, const AppConfig* config)
    : wxFrame(nullptr, wxID_ANY, "Diagrams", wxDefaultPosition, wxSize(1100, 700)),
      window_manager_(windowManager),
      app_config_(config),
      formId_("diagram_" + std::to_string(nextFormId_++)) {
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
        if (menu_bar) {
            auto* diagram_menu = new wxMenu();
            diagram_menu->Append(ID_MENU_DIAGRAM_OPEN, "Open Diagram...");
            diagram_menu->Append(ID_MENU_DIAGRAM_SAVE, "Save Diagram");
            diagram_menu->Append(ID_MENU_DIAGRAM_SAVE_AS, "Save Diagram As...");
            menu_bar->Append(diagram_menu, "Diagram");
        }
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
    AddDiagramTabOfType(DiagramType::Erd, title);
}

void DiagramFrame::AddDiagramTabOfType(DiagramType type, const wxString& title) {
    if (!notebook_) {
        return;
    }
    diagram_counter_++;
    wxString label = title;
    if (label.empty()) {
        // Use type-specific default name
        switch (type) {
            case DiagramType::Erd:
                label = wxString::Format("ERD %d", diagram_counter_);
                break;
            case DiagramType::DataFlow:
                label = wxString::Format("DFD %d", diagram_counter_);
                break;
            case DiagramType::MindMap:
                label = wxString::Format("Mind Map %d", diagram_counter_);
                break;
            case DiagramType::Whiteboard:
                label = wxString::Format("Whiteboard %d", diagram_counter_);
                break;
            case DiagramType::Silverston:
                label = wxString::Format("Model %d", diagram_counter_);
                break;
            default:
                label = wxString::Format("Diagram %d", diagram_counter_);
                break;
        }
    }
    
    auto* page = new DiagramPage(notebook_);
    page->SetDiagramType(type);
    notebook_->AddPage(page, label, true);
}

DiagramPage* DiagramFrame::GetActiveDiagramPage() const {
    if (!notebook_) return nullptr;
    auto* page = notebook_->GetCurrentPage();
    return dynamic_cast<DiagramPage*>(page);
}

bool DiagramFrame::FocusNodeInOpenDiagrams(const std::string& ref, DiagramType preferred_type) {
    if (!notebook_) return false;
    TraceTarget target = ParseTraceRef(ref);
    std::string diagram_path = target.diagram_path;
    std::string node_name = target.node_name;
    if (node_name.empty()) return false;
    for (size_t i = 0; i < notebook_->GetPageCount(); ++i) {
        auto* page = dynamic_cast<DiagramPage*>(notebook_->GetPage(i));
        if (!page) continue;
        if (page->diagram_type() != preferred_type) continue;
        if (!diagram_path.empty() && !page->file_path().empty()) {
            std::filesystem::path page_path = page->file_path();
            if (page_path.filename().string() != diagram_path &&
                page->file_path().find(diagram_path) == std::string::npos) {
                continue;
            }
        }
        if (page->FocusNodeByName(node_name)) {
            notebook_->SetSelection(static_cast<int>(i));
            return true;
        }
    }
    return false;
}

bool DiagramFrame::OpenDiagramFile(const std::string& path, const std::string& focus_node) {
    if (!notebook_) return false;
    AddDiagramTab(std::filesystem::path(path).filename().string());
    size_t index = notebook_->GetPageCount() - 1;
    auto* page = dynamic_cast<DiagramPage*>(notebook_->GetPage(index));
    if (!page) return false;
    std::string error;
    if (!page->LoadFromFile(path, &error)) {
        wxMessageBox(error.empty() ? "Failed to open diagram" : error,
                     "Open Diagram", wxOK | wxICON_ERROR, this);
        return false;
    }
    page->set_file_path(path);
    notebook_->SetSelection(static_cast<int>(index));
    if (!focus_node.empty()) {
        page->FocusNodeByName(focus_node);
    }
    return true;
}

bool DiagramFrame::OpenDiagramFromTrace(const std::string& ref) {
    TraceTarget target = ParseTraceRef(ref);
    std::string diagram_path = target.diagram_path;
    std::string node_name = target.node_name;
    if (diagram_path.empty() && node_name.empty()) {
        return false;
    }
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (!project) return false;

    std::filesystem::path full_path;
    if (!diagram_path.empty()) {
        std::filesystem::path candidate = diagram_path;
        if (candidate.is_relative()) {
            full_path = std::filesystem::path(project->project_root_path) / candidate;
        } else {
            full_path = candidate;
        }
        if (std::filesystem::exists(full_path)) {
            return OpenDiagramFile(full_path.string(), node_name);
        }
    }

    std::filesystem::path diagram_root = GetDiagramStorageRoot(*project);
    if (!std::filesystem::exists(diagram_root)) return false;
    for (const auto& entry : std::filesystem::directory_iterator(diagram_root)) {
        if (!entry.is_regular_file()) continue;
        std::filesystem::path file = entry.path();
        if (file.extension() != ".sberd") continue;
        DiagramModel probe(DiagramType::Erd);
        diagram::DiagramDocument doc;
        std::string error;
        if (!diagram::DiagramSerializer::LoadFromFile(&probe, &doc, file.string(), &error)) {
            continue;
        }
        for (const auto& node : probe.nodes()) {
            if (node.name == node_name || node.id == node_name) {
                return OpenDiagramFile(file.string(), node_name);
            }
        }
    }
    return false;
}

void DiagramFrame::OnNewDiagram(wxCommandEvent&) {
    AddDiagramTab(wxString::Format("Diagram %d", diagram_counter_ + 1));
}

void DiagramFrame::OnOpenDiagram(wxCommandEvent&) {
    DiagramPage* page = GetActiveDiagramPage();
    if (!page) return;
    wxFileDialog dialog(this, "Open Diagram", "", "",
                        "Diagram Files (*.sbdgm;*.sberd)|*.sbdgm;*.sberd|All Files|*.*",
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_OK) return;
    std::string error;
    if (!page->LoadFromFile(dialog.GetPath().ToStdString(), &error)) {
        wxMessageBox(error.empty() ? "Failed to open diagram" : error,
                     "Open Diagram", wxOK | wxICON_ERROR, this);
        return;
    }
    page->set_file_path(dialog.GetPath().ToStdString());
    notebook_->SetPageText(notebook_->GetSelection(), dialog.GetFilename());
}

void DiagramFrame::OnSaveDiagram(wxCommandEvent&) {
    DiagramPage* page = GetActiveDiagramPage();
    if (!page) return;
    if (page->file_path().empty()) {
        auto project = ProjectManager::Instance().GetCurrentProject();
        if (project) {
            std::filesystem::path dir = GetDiagramStorageRoot(*project);
            std::filesystem::create_directories(dir);
            std::string ext = page->diagram_type() == DiagramType::Erd ? ".sberd" : ".sbdgm";
            wxString title = notebook_ ? notebook_->GetPageText(notebook_->GetSelection()) : "diagram";
            std::string filename = title.ToStdString();
            if (filename.empty()) filename = "diagram";
            std::filesystem::path path = dir / (filename + ext);
            std::string error;
            if (!page->SaveToFile(path.string(), &error)) {
                wxMessageBox(error.empty() ? "Failed to save diagram" : error,
                             "Save Diagram", wxOK | wxICON_ERROR, this);
                return;
            }
            page->set_file_path(path.string());
            std::string rel = MakeRelativePath(*project, path);
            project->RegisterDiagramObject(path.stem().string(),
                                           rel,
                                           DiagramTypeKey(page->diagram_type()));
            if (notebook_) {
                notebook_->SetPageText(notebook_->GetSelection(), path.filename().string());
            }
            return;
        }
        wxCommandEvent evt;
        OnSaveDiagramAs(evt);
        return;
    }
    std::string error;
    if (!page->SaveToFile(page->file_path(), &error)) {
        wxMessageBox(error.empty() ? "Failed to save diagram" : error,
                     "Save Diagram", wxOK | wxICON_ERROR, this);
        return;
    }
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        std::filesystem::path path = page->file_path();
        std::string rel = MakeRelativePath(*project, path);
        project->RegisterDiagramObject(path.stem().string(),
                                       rel,
                                       DiagramTypeKey(page->diagram_type()));
    }
}

void DiagramFrame::OnSaveDiagramAs(wxCommandEvent&) {
    DiagramPage* page = GetActiveDiagramPage();
    if (!page) return;
    wxString wildcard = "Diagram Files (*.sbdgm;*.sberd)|*.sbdgm;*.sberd|All Files|*.*";
    wxString default_dir;
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        std::filesystem::path dir = GetDiagramStorageRoot(*project);
        std::filesystem::create_directories(dir);
        default_dir = dir.string();
    }
    wxFileDialog dialog(this, "Save Diagram As", default_dir, "", wildcard,
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) return;
    std::string error;
    if (!page->SaveToFile(dialog.GetPath().ToStdString(), &error)) {
        wxMessageBox(error.empty() ? "Failed to save diagram" : error,
                     "Save Diagram", wxOK | wxICON_ERROR, this);
        return;
    }
    page->set_file_path(dialog.GetPath().ToStdString());
    if (project) {
        std::filesystem::path path = dialog.GetPath().ToStdString();
        std::string rel = MakeRelativePath(*project, path);
        project->RegisterDiagramObject(path.stem().string(),
                                       rel,
                                       DiagramTypeKey(page->diagram_type()));
    }
    notebook_->SetPageText(notebook_->GetSelection(), dialog.GetFilename());
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

//=============================================================================
// IFormWindow Implementation
//=============================================================================

std::string DiagramFrame::GetFormId() const {
    return formId_;
}

wxString DiagramFrame::GetFormTitle() const {
    return GetTitle();
}

void DiagramFrame::OnFormActivated() {
    // Could refresh thumbnails or update state
}

void DiagramFrame::OnFormDeactivated() {
    // Could pause rendering or save state
}

bool DiagramFrame::CanAcceptChild(ui::IFormWindow* child) const {
    if (!child) {
        return false;
    }
    // Only accept other diagrams
    return child->GetFormCategory() == ui::FormCategory::Diagram;
}

void DiagramFrame::AddChildForm(ui::IFormWindow* child) {
    if (!child || !CanAcceptChild(child)) {
        return;
    }
    
    // Add as embedded diagram
    DiagramFrame* childDiagram = dynamic_cast<DiagramFrame*>(child->GetWindow());
    if (childDiagram) {
        AddEmbeddedDiagram(childDiagram, 50, 50);  // Default position
    }
}

void DiagramFrame::RemoveChildForm(ui::IFormWindow* child) {
    if (!child) {
        return;
    }
    RemoveEmbeddedDiagram(child->GetFormId());
}

void DiagramFrame::AddEmbeddedDiagram(DiagramFrame* childDiagram, int x, int y) {
    if (!childDiagram || !notebook_) {
        return;
    }
    
    // Get the active page to add the mini-view to
    DiagramPage* activePage = GetActiveDiagramPage();
    if (!activePage) {
        return;
    }
    
    EmbeddedDiagram embedded;
    embedded.diagramId = childDiagram->GetFormId();
    embedded.title = childDiagram->GetFormTitle();
    embedded.x = x;
    embedded.y = y;
    embedded.miniView = nullptr;  // Created by the diagram page
    
    childDiagrams_.push_back(embedded);
    
    // Notify the page to add the mini-view
    // This would require DiagramPage to support mini-views
}

void DiagramFrame::RemoveEmbeddedDiagram(const std::string& diagramId) {
    auto it = std::remove_if(childDiagrams_.begin(), childDiagrams_.end(),
        [&diagramId](const EmbeddedDiagram& ed) {
            return ed.diagramId == diagramId;
        });
    childDiagrams_.erase(it, childDiagrams_.end());
}

} // namespace scratchrobin
