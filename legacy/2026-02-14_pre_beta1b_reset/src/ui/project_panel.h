/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_PROJECT_PANEL_H
#define SCRATCHROBIN_PROJECT_PANEL_H

#include <wx/panel.h>
#include <wx/treectrl.h>
#include <wx/dialog.h>
#include <wx/wizard.h>

#include "core/project.h"

class wxNotebook;
class wxSplitterWindow;
class wxListCtrl;
class wxToolBar;
class wxListBox;

namespace scratchrobin {

// Tree item data for project objects
class ProjectTreeItemData : public wxTreeItemData {
public:
    explicit ProjectTreeItemData(const UUID& id) : object_id_(id) {}
    
    const UUID& GetObjectId() const { return object_id_; }
    bool IsValid() const { return object_id_.IsValid(); }
    
private:
    UUID object_id_;
};

// ============================================================================
// Project Panel - Main project management UI
// ============================================================================
class ProjectPanel : public wxPanel {
public:
    ProjectPanel(wxWindow* parent);
    ~ProjectPanel();
    
    // Project binding
    void SetProject(std::shared_ptr<Project> project);
    void ClearProject();
    
    // Refresh
    void RefreshTree();
    void RefreshStats();
    
    // Selection
    std::shared_ptr<ProjectObject> GetSelectedObject();
    void SelectObject(const UUID& id);
    
private:
    void BuildLayout();
    void BuildToolbar();
    void BuildTree();
    void BuildStatsPanel();
    void AppendSyncEvent(const Project::StatusEvent& evt);
    void PostStatus(const std::string& message, bool is_error);
    
    // Event handlers
    void OnTreeSelection(wxTreeEvent& event);
    void OnTreeItemMenu(wxTreeEvent& event);
    void OnTreeActivate(wxTreeEvent& event);
    
    void OnNewObject(wxCommandEvent& event);
    void OnDeleteObject(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnFilter(wxCommandEvent& event);
    void OnSyncToDb(wxCommandEvent& event);
    void OnSyncFromDb(wxCommandEvent& event);
    void OnApproveObject(wxCommandEvent& event);
    void OnRejectObject(wxCommandEvent& event);
    void OnGenerateDDL(wxCommandEvent& event);
    void OnViewDiff(wxCommandEvent& event);
    
    // Tree population
    void PopulateTree();
    wxTreeItemId AddObjectToTree(const std::shared_ptr<ProjectObject>& obj,
                                  wxTreeItemId parent);
    wxTreeItemId FindOrCreateCategory(const std::string& category);
    
    // State change handler
    void OnProjectObjectChanged(const UUID& id, const std::string& action);
    
    // UI helpers
    int GetIconForObject(const ProjectObject& obj);
    wxString GetObjectLabel(const ProjectObject& obj);
    void UpdateStatsDisplay();
    
    std::shared_ptr<Project> project_;
    Project::ObjectChangedCallback object_changed_callback_;
    
    // UI components
    wxToolBar* toolbar_ = nullptr;
    wxTreeCtrl* tree_ = nullptr;
    wxPanel* stats_panel_ = nullptr;
    wxListCtrl* stats_list_ = nullptr;
    wxListBox* sync_list_ = nullptr;
    wxImageList* tree_images_ = nullptr;
    
    // Tree organization
    wxTreeItemId root_item_;
    wxTreeItemId extracted_item_;
    wxTreeItemId new_item_;
    wxTreeItemId modified_item_;
    wxTreeItemId pending_item_;
    wxTreeItemId approved_item_;
    wxTreeItemId deleted_item_;
    wxTreeItemId implemented_item_;
    wxTreeItemId diagrams_item_;
    wxTreeItemId whiteboards_item_;
    wxTreeItemId mindmaps_item_;
    
    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// New Project Wizard
// ============================================================================
class NewProjectWizard : public wxDialog {
public:
    NewProjectWizard(wxWindow* parent);
    
    ProjectConfig GetConfig() const { return config_; }
    std::string GetProjectPath() const { return project_path_; }
    
private:
    void BuildPages();
    void OnFinish(wxWizardEvent& event);
    void OnPageChanging(wxWizardEvent& event);
    
    ProjectConfig config_;
    std::string project_path_;
    
    // Wizard pages
    class WelcomePage* welcome_page_ = nullptr;
    class TemplatePage* template_page_ = nullptr;
    class ConfigPage* config_page_ = nullptr;
    class DatabasePage* db_page_ = nullptr;
    class SummaryPage* summary_page_ = nullptr;
};

// ============================================================================
// Object Property Panel - Edit selected object
// ============================================================================
class ObjectPropertyPanel : public wxPanel {
public:
    ObjectPropertyPanel(wxWindow* parent);
    
    void SetObject(std::shared_ptr<ProjectObject> obj);
    void Clear();
    
private:
    void BuildLayout();
    void PopulateFields();
    void SaveChanges();
    
    std::shared_ptr<ProjectObject> object_;
    
    // UI fields
    class wxTextCtrl* name_ctrl_ = nullptr;
    class wxChoice* state_choice_ = nullptr;
    class wxTextCtrl* reason_ctrl_ = nullptr;
    class wxTextCtrl* comments_ctrl_ = nullptr;
    class wxListBox* history_list_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PROJECT_PANEL_H
