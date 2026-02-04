/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ISSUE_TRACKER_PANEL_H
#define SCRATCHROBIN_ISSUE_TRACKER_PANEL_H

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <memory>
#include <sstream>
#include <vector>

#include "../core/issue_tracker.h"

namespace scratchrobin {

// Issue Tracker panel for linking database objects to issues
class IssueTrackerPanel : public wxPanel {
public:
    IssueTrackerPanel(wxWindow* parent);
    ~IssueTrackerPanel();
    
    // Set the current database object context
    void SetCurrentObject(const ObjectReference& obj);
    void ClearCurrentObject();
    
    // Refresh linked issues
    void RefreshLinks();
    
private:
    void CreateControls();
    void BindEvents();
    void UpdateUI();
    
    void OnCreateIssue(wxCommandEvent& event);
    void OnLinkExisting(wxCommandEvent& event);
    void OnUnlink(wxCommandEvent& event);
    void OnSync(wxCommandEvent& event);
    void OnViewIssue(wxCommandEvent& event);
    void OnIssueSelected(wxListEvent& event);
    void OnSettings(wxCommandEvent& event);
    
    void LoadLinkedIssues();
    void ShowIssueDetails(const IssueLink& link);
    
    // UI Controls
    wxStaticText* object_label_;
    wxListCtrl* issues_list_;
    wxTextCtrl* details_text_;
    wxButton* create_button_;
    wxButton* link_button_;
    wxButton* unlink_button_;
    wxButton* sync_button_;
    wxButton* view_button_;
    wxButton* settings_button_;
    
    // Current context
    std::unique_ptr<ObjectReference> current_object_;
    std::vector<IssueLink> current_links_;
    
    enum {
        ID_CREATE_ISSUE = wxID_HIGHEST + 1,
        ID_LINK_EXISTING,
        ID_UNLINK,
        ID_SYNC,
        ID_VIEW_ISSUE,
        ID_SETTINGS,
        ID_ISSUES_LIST
    };
};

// Create Issue Dialog
class CreateIssueDialog : public wxDialog {
public:
    CreateIssueDialog(wxWindow* parent, const ObjectReference& obj);
    
    IssueReference GetCreatedIssue() const { return created_issue_; }
    
private:
    void CreateControls();
    void OnCreate(wxCommandEvent& event);
    void OnProviderChanged(wxCommandEvent& event);
    
    // UI Controls
    wxChoice* provider_choice_;
    wxTextCtrl* title_ctrl_;
    wxTextCtrl* description_ctrl_;
    wxChoice* type_choice_;
    wxChoice* priority_choice_;
    wxTextCtrl* labels_ctrl_;
    wxCheckBox* attach_schema_cb_;
    wxCheckBox* attach_diagram_cb_;
    
    ObjectReference object_;
    IssueReference created_issue_;
};

// Link Existing Issue Dialog
class LinkIssueDialog : public wxDialog {
public:
    LinkIssueDialog(wxWindow* parent, const ObjectReference& obj);
    
    std::string GetSelectedIssueId();
    
private:
    void CreateControls();
    void OnSearch(wxCommandEvent& event);
    void OnLink(wxCommandEvent& event);
    void LoadRecentIssues(wxCommandEvent& event);
    
    wxChoice* provider_choice_;
    wxTextCtrl* search_ctrl_;
    wxListCtrl* results_list_;
    wxButton* search_button_;
    
    ObjectReference object_;
    std::vector<IssueReference> search_results_;
};

// Issue Tracker Settings Dialog
class IssueTrackerSettingsDialog : public wxDialog {
public:
    IssueTrackerSettingsDialog(wxWindow* parent);
    
private:
    void CreateControls();
    void OnAddTracker(wxCommandEvent& event);
    void OnRemoveTracker(wxCommandEvent& event);
    void OnTestConnection(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    
    wxListCtrl* trackers_list_;
    wxButton* add_button_;
    wxButton* remove_button_;
    wxButton* test_button_;
    std::vector<IssueReference> search_results_;  // Reuse from LinkIssueDialog
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_ISSUE_TRACKER_PANEL_H
