/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "project_panel.h"

#include <wx/artprov.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/toolbar.h>
#include <wx/treectrl.h>

#include "core/project.h"

namespace scratchrobin {

namespace {
    // Menu IDs
    constexpr int kNewObject = wxID_HIGHEST + 100;
    constexpr int kDeleteObject = wxID_HIGHEST + 101;
    constexpr int kRefreshTree = wxID_HIGHEST + 102;
    constexpr int kSyncToDb = wxID_HIGHEST + 103;
    constexpr int kSyncFromDb = wxID_HIGHEST + 104;
    constexpr int kApproveObject = wxID_HIGHEST + 105;
    constexpr int kRejectObject = wxID_HIGHEST + 106;
    constexpr int kGenerateDDL = wxID_HIGHEST + 107;
    constexpr int kViewDiff = wxID_HIGHEST + 108;
}

wxBEGIN_EVENT_TABLE(ProjectPanel, wxPanel)
    EVT_TREE_SEL_CHANGED(wxID_ANY, ProjectPanel::OnTreeSelection)
    EVT_TREE_ITEM_MENU(wxID_ANY, ProjectPanel::OnTreeItemMenu)
    EVT_TREE_ITEM_ACTIVATED(wxID_ANY, ProjectPanel::OnTreeActivate)
    EVT_TOOL(kNewObject, ProjectPanel::OnNewObject)
    EVT_TOOL(kDeleteObject, ProjectPanel::OnDeleteObject)
    EVT_TOOL(kRefreshTree, ProjectPanel::OnRefresh)
    EVT_TOOL(kSyncToDb, ProjectPanel::OnSyncToDb)
    EVT_TOOL(kSyncFromDb, ProjectPanel::OnSyncFromDb)
wxEND_EVENT_TABLE()

ProjectPanel::ProjectPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    BuildLayout();
    BuildToolbar();
    BuildTree();
    BuildStatsPanel();
}

ProjectPanel::~ProjectPanel() {
    if (project_ && object_changed_callback_) {
        // Would remove observer in real implementation
    }
}

void ProjectPanel::BuildLayout() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Toolbar
    toolbar_ = new wxToolBar(this, wxID_ANY);
    sizer->Add(toolbar_, 0, wxEXPAND);
    
    // Splitter for tree and stats
    auto* splitter = new wxSplitterWindow(this, wxID_ANY);
    
    // Tree panel
    tree_ = new wxTreeCtrl(splitter, wxID_ANY);
    
    // Stats panel
    stats_panel_ = new wxPanel(splitter, wxID_ANY);
    
    splitter->SplitVertically(tree_, stats_panel_, 300);
    sizer->Add(splitter, 1, wxEXPAND);
    
    SetSizer(sizer);
}

void ProjectPanel::BuildToolbar() {
    toolbar_->AddTool(kNewObject, "New", 
                      wxArtProvider::GetBitmap(wxART_PLUS, wxART_TOOLBAR, wxSize(16, 16)),
                      "Create new object");
    toolbar_->AddTool(kDeleteObject, "Delete",
                      wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR, wxSize(16, 16)),
                      "Delete selected object");
    toolbar_->AddSeparator();
    toolbar_->AddTool(kRefreshTree, "Refresh",
                      wxArtProvider::GetBitmap(wxART_REFRESH, wxART_TOOLBAR, wxSize(16, 16)),
                      "Refresh tree");
    toolbar_->AddSeparator();
    toolbar_->AddTool(kSyncToDb, "Sync to DB",
                      wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR, wxSize(16, 16)),
                      "Sync changes to database");
    toolbar_->AddTool(kSyncFromDb, "Sync from DB",
                      wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR, wxSize(16, 16)),
                      "Sync changes from database");
    
    toolbar_->Realize();
}

void ProjectPanel::BuildTree() {
    // Create image list for tree icons
    tree_images_ = new wxImageList(16, 16);
    
    // Add standard icons
    tree_images_->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));
    tree_images_->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16, 16)));
    tree_images_->Add(wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_OTHER, wxSize(16, 16)));
    tree_images_->Add(wxArtProvider::GetBitmap(wxART_WARNING, wxART_OTHER, wxSize(16, 16)));
    tree_images_->Add(wxArtProvider::GetBitmap(wxART_PLUS, wxART_OTHER, wxSize(16, 16)));
    tree_images_->Add(wxArtProvider::GetBitmap(wxART_DELETE, wxART_OTHER, wxSize(16, 16)));
    
    tree_->SetImageList(tree_images_);
}

void ProjectPanel::BuildStatsPanel() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* title = new wxStaticText(stats_panel_, wxID_ANY, "Project Statistics");
    title->SetFont(title->GetFont().Bold());
    sizer->Add(title, 0, wxALL, 8);
    
    stats_list_ = new wxListCtrl(stats_panel_, wxID_ANY, wxDefaultPosition, 
                                  wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    stats_list_->AppendColumn("Metric", wxLIST_FORMAT_LEFT, 120);
    stats_list_->AppendColumn("Count", wxLIST_FORMAT_RIGHT, 80);
    
    sizer->Add(stats_list_, 1, wxEXPAND | wxALL, 8);
    
    stats_panel_->SetSizer(sizer);
}

void ProjectPanel::SetProject(std::shared_ptr<Project> project) {
    if (project_) {
        // Remove old observer
    }
    
    project_ = project;
    
    if (project_) {
        // Add observer
        object_changed_callback_ = [this](const UUID& id, const std::string& action) {
            OnProjectObjectChanged(id, action);
        };
        project_->AddObserver(object_changed_callback_);
        
        PopulateTree();
        UpdateStatsDisplay();
    }
}

void ProjectPanel::ClearProject() {
    SetProject(nullptr);
    tree_->DeleteAllItems();
    root_item_ = wxTreeItemId();
    stats_list_->DeleteAllItems();
}

void ProjectPanel::PopulateTree() {
    tree_->DeleteAllItems();
    
    if (!project_) {
        return;
    }
    
    // Root
    root_item_ = tree_->AddRoot(project_->config.name, 0);
    
    // State-based organization
    extracted_item_ = tree_->AppendItem(root_item_, "Extracted", 0);
    new_item_ = tree_->AppendItem(root_item_, "New Objects", 4);
    modified_item_ = tree_->AppendItem(root_item_, "Modified", 3);
    pending_item_ = tree_->AppendItem(root_item_, "Pending Review", 3);
    approved_item_ = tree_->AppendItem(root_item_, "Approved", 2);
    deleted_item_ = tree_->AppendItem(root_item_, "Deleted", 5);
    implemented_item_ = tree_->AppendItem(root_item_, "Implemented", 2);
    
    tree_->AppendItem(root_item_, "", -1);  // Separator
    
    // Design artifacts
    diagrams_item_ = tree_->AppendItem(root_item_, "Diagrams", 0);
    whiteboards_item_ = tree_->AppendItem(root_item_, "Whiteboards", 0);
    mindmaps_item_ = tree_->AppendItem(root_item_, "Mind Maps", 0);
    
    // Add objects
    for (const auto& [id, obj] : project_->objects_by_id) {
        AddObjectToTree(obj, wxTreeItemId());
    }
    
    tree_->Expand(root_item_);
}

wxTreeItemId ProjectPanel::AddObjectToTree(const std::shared_ptr<ProjectObject>& obj,
                                            wxTreeItemId parent) {
    wxTreeItemId category;
    
    // Determine category based on state
    switch (obj->GetState()) {
        case ObjectState::EXTRACTED: category = extracted_item_; break;
        case ObjectState::NEW: category = new_item_; break;
        case ObjectState::MODIFIED: category = modified_item_; break;
        case ObjectState::PENDING: category = pending_item_; break;
        case ObjectState::APPROVED: category = approved_item_; break;
        case ObjectState::DELETED: category = deleted_item_; break;
        case ObjectState::IMPLEMENTED: category = implemented_item_; break;
        default: category = root_item_;
    }
    
    int icon = GetIconForObject(*obj);
    wxString label = GetObjectLabel(*obj);
    
    auto item = tree_->AppendItem(category, label, icon, icon,
                                   new ProjectTreeItemData(obj->id));
    return item;
}

int ProjectPanel::GetIconForObject(const ProjectObject& obj) {
    // Use the state-based icon mapping
    return GetObjectStateIconIndex(obj.GetState());
}

wxString ProjectPanel::GetObjectLabel(const ProjectObject& obj) {
    wxString label = obj.name;
    
    // Add state indicator
    switch (obj.GetState()) {
        case ObjectState::NEW:
            label += " [NEW]";
            break;
        case ObjectState::MODIFIED:
            label += " [MOD]";
            break;
        case ObjectState::DELETED:
            label += " [DEL]";
            break;
        case ObjectState::PENDING:
            label += " [PEND]";
            break;
        case ObjectState::APPROVED:
            label += " [APPR]";
            break;
        default:
            break;
    }
    
    return label;
}

void ProjectPanel::UpdateStatsDisplay() {
    stats_list_->DeleteAllItems();
    
    if (!project_) {
        return;
    }
    
    auto stats = project_->GetStats();
    
    auto add_stat = [this](const wxString& name, int value) {
        long idx = stats_list_->InsertItem(stats_list_->GetItemCount(), name);
        stats_list_->SetItem(idx, 1, wxString::Format("%d", value));
    };
    
    add_stat("Total Objects", stats.total_objects);
    add_stat("Extracted", stats.extracted);
    add_stat("New", stats.new_objects);
    add_stat("Modified", stats.modified);
    add_stat("Pending", stats.pending);
    add_stat("Approved", stats.approved);
    add_stat("Implemented", stats.implemented);
    add_stat("Deleted", stats.deleted);
}

void ProjectPanel::OnTreeSelection(wxTreeEvent& event) {
    auto* data = dynamic_cast<ProjectTreeItemData*>(tree_->GetItemData(event.GetItem()));
    if (data && data->IsValid() && project_) {
        // Would notify main window to show object details
    }
}

void ProjectPanel::OnTreeItemMenu(wxTreeEvent& event) {
    // Context menu for tree items
}

void ProjectPanel::OnTreeActivate(wxTreeEvent& event) {
    // Double-click - open object editor
}

void ProjectPanel::OnProjectObjectChanged(const UUID& id, const std::string& action) {
    // Refresh tree when objects change
    RefreshTree();
}

void ProjectPanel::RefreshTree() {
    PopulateTree();
}

void ProjectPanel::RefreshStats() {
    UpdateStatsDisplay();
}

std::shared_ptr<ProjectObject> ProjectPanel::GetSelectedObject() {
    auto item = tree_->GetSelection();
    if (!item.IsOk()) {
        return nullptr;
    }
    
    auto* data = dynamic_cast<ProjectTreeItemData*>(tree_->GetItemData(item));
    if (!data || !data->IsValid() || !project_) {
        return nullptr;
    }
    
    return project_->GetObject(data->GetObjectId());
}

void ProjectPanel::OnNewObject(wxCommandEvent&) {
    // Show new object dialog
}

void ProjectPanel::OnDeleteObject(wxCommandEvent&) {
    auto obj = GetSelectedObject();
    if (obj && project_) {
        project_->DeleteObject(obj->id);
    }
}

void ProjectPanel::OnRefresh(wxCommandEvent&) {
    RefreshTree();
    RefreshStats();
}

void ProjectPanel::OnSyncToDb(wxCommandEvent&) {
    if (project_) {
        project_->SyncToDatabase();
    }
}

void ProjectPanel::OnSyncFromDb(wxCommandEvent&) {
    if (project_) {
        project_->SyncFromDatabase();
    }
}

} // namespace scratchrobin
