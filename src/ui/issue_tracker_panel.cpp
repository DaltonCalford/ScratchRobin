/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "issue_tracker_panel.h"

#include "../core/issue_tracker.h"
#include "../core/issue_tracker_jira.h"
#include "../core/issue_tracker_github.h"
#include "../core/issue_tracker_gitlab.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/msgdlg.h>

#include <sstream>

namespace scratchrobin {

// ============================================================================
// Issue Tracker Panel
// ============================================================================
IssueTrackerPanel::IssueTrackerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    CreateControls();
    BindEvents();
    UpdateUI();
}

IssueTrackerPanel::~IssueTrackerPanel() = default;

void IssueTrackerPanel::CreateControls() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Header with object info
    auto* header_box = new wxStaticBoxSizer(wxHORIZONTAL, this, "Linked Issues");
    object_label_ = new wxStaticText(this, wxID_ANY, "No object selected");
    object_label_->SetFont(wxFont(wxFontInfo().Bold()));
    header_box->Add(object_label_, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    auto* refresh_btn = new wxButton(this, wxID_ANY, "Refresh", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    header_box->Add(refresh_btn, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    main_sizer->Add(header_box, 0, wxEXPAND | wxALL, 5);
    
    // Splitter for list and details
    auto* content_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Issues list
    auto* list_box = new wxStaticBoxSizer(wxVERTICAL, this, "Issues");
    issues_list_ = new wxListCtrl(this, ID_ISSUES_LIST, wxDefaultPosition, wxDefaultSize,
                                   wxLC_REPORT | wxLC_SINGLE_SEL);
    issues_list_->AppendColumn("Key", wxLIST_FORMAT_LEFT, 80);
    issues_list_->AppendColumn("Title", wxLIST_FORMAT_LEFT, 200);
    issues_list_->AppendColumn("Status", wxLIST_FORMAT_LEFT, 80);
    issues_list_->AppendColumn("Provider", wxLIST_FORMAT_LEFT, 80);
    list_box->Add(issues_list_, 1, wxEXPAND | wxALL, 5);
    
    content_sizer->Add(list_box, 1, wxEXPAND | wxALL, 5);
    
    // Issue details
    auto* details_box = new wxStaticBoxSizer(wxVERTICAL, this, "Details");
    details_text_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY);
    details_box->Add(details_text_, 1, wxEXPAND | wxALL, 5);
    
    content_sizer->Add(details_box, 1, wxEXPAND | wxALL, 5);
    
    main_sizer->Add(content_sizer, 1, wxEXPAND);
    
    // Button toolbar
    auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    create_button_ = new wxButton(this, ID_CREATE_ISSUE, "Create Issue");
    button_sizer->Add(create_button_, 0, wxALL, 5);
    
    link_button_ = new wxButton(this, ID_LINK_EXISTING, "Link Existing");
    button_sizer->Add(link_button_, 0, wxALL, 5);
    
    unlink_button_ = new wxButton(this, ID_UNLINK, "Unlink");
    unlink_button_->Disable();
    button_sizer->Add(unlink_button_, 0, wxALL, 5);
    
    sync_button_ = new wxButton(this, ID_SYNC, "Sync");
    sync_button_->Disable();
    button_sizer->Add(sync_button_, 0, wxALL, 5);
    
    view_button_ = new wxButton(this, ID_VIEW_ISSUE, "View in Browser");
    view_button_->Disable();
    button_sizer->Add(view_button_, 0, wxALL, 5);
    
    button_sizer->AddStretchSpacer(1);
    
    settings_button_ = new wxButton(this, ID_SETTINGS, "Settings");
    button_sizer->Add(settings_button_, 0, wxALL, 5);
    
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    SetSizer(main_sizer);
}

void IssueTrackerPanel::BindEvents() {
    create_button_->Bind(wxEVT_BUTTON, &IssueTrackerPanel::OnCreateIssue, this);
    link_button_->Bind(wxEVT_BUTTON, &IssueTrackerPanel::OnLinkExisting, this);
    unlink_button_->Bind(wxEVT_BUTTON, &IssueTrackerPanel::OnUnlink, this);
    sync_button_->Bind(wxEVT_BUTTON, &IssueTrackerPanel::OnSync, this);
    view_button_->Bind(wxEVT_BUTTON, &IssueTrackerPanel::OnViewIssue, this);
    settings_button_->Bind(wxEVT_BUTTON, &IssueTrackerPanel::OnSettings, this);
    
    issues_list_->Bind(wxEVT_LIST_ITEM_SELECTED, &IssueTrackerPanel::OnIssueSelected, this);
}

void IssueTrackerPanel::UpdateUI() {
    bool has_object = current_object_ != nullptr;
    create_button_->Enable(has_object);
    link_button_->Enable(has_object);
    
    if (has_object) {
        object_label_->SetLabel("Object: " + current_object_->qualified_name);
    } else {
        object_label_->SetLabel("No object selected");
    }
}

void IssueTrackerPanel::SetCurrentObject(const ObjectReference& obj) {
    current_object_ = std::make_unique<ObjectReference>(obj);
    UpdateUI();
    LoadLinkedIssues();
}

void IssueTrackerPanel::ClearCurrentObject() {
    current_object_.reset();
    current_links_.clear();
    issues_list_->DeleteAllItems();
    details_text_->Clear();
    UpdateUI();
}

void IssueTrackerPanel::RefreshLinks() {
    LoadLinkedIssues();
}

void IssueTrackerPanel::LoadLinkedIssues() {
    issues_list_->DeleteAllItems();
    current_links_.clear();
    
    if (!current_object_) return;
    
    auto& manager = IssueLinkManager::Instance();
    current_links_ = manager.GetLinkedIssues(*current_object_);
    
    for (size_t i = 0; i < current_links_.size(); ++i) {
        const auto& link = current_links_[i];
        long idx = issues_list_->InsertItem(static_cast<long>(i), link.issue.display_key);
        issues_list_->SetItem(idx, 1, link.issue.title);
        issues_list_->SetItem(idx, 2, IssueStatusToString(link.issue.status));
        issues_list_->SetItem(idx, 3, link.issue.provider);
    }
}

void IssueTrackerPanel::OnCreateIssue(wxCommandEvent& /*event*/) {
    if (!current_object_) return;
    
    CreateIssueDialog dlg(this, *current_object_);
    if (dlg.ShowModal() == wxID_OK) {
        LoadLinkedIssues();
    }
}

void IssueTrackerPanel::OnLinkExisting(wxCommandEvent& /*event*/) {
    if (!current_object_) return;
    
    LinkIssueDialog dlg(this, *current_object_);
    if (dlg.ShowModal() == wxID_OK) {
        LoadLinkedIssues();
    }
}

void IssueTrackerPanel::OnUnlink(wxCommandEvent& /*event*/) {
    long idx = issues_list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (idx < 0 || static_cast<size_t>(idx) >= current_links_.size()) return;
    
    auto& manager = IssueLinkManager::Instance();
    manager.UnlinkObject(*current_object_, current_links_[idx].issue.issue_id);
    
    LoadLinkedIssues();
}

void IssueTrackerPanel::OnSync(wxCommandEvent& /*event*/) {
    long idx = issues_list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (idx < 0 || static_cast<size_t>(idx) >= current_links_.size()) return;
    
    auto& manager = IssueLinkManager::Instance();
    manager.SyncLink(current_links_[idx].link_id);
    
    LoadLinkedIssues();
}

void IssueTrackerPanel::OnViewIssue(wxCommandEvent& /*event*/) {
    long idx = issues_list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (idx < 0 || static_cast<size_t>(idx) >= current_links_.size()) return;
    
    const auto& link = current_links_[idx];
    if (!link.issue.url.empty()) {
        wxLaunchDefaultBrowser(link.issue.url);
    }
}

void IssueTrackerPanel::OnIssueSelected(wxListEvent& event) {
    long idx = event.GetIndex();
    if (idx >= 0 && static_cast<size_t>(idx) < current_links_.size()) {
        unlink_button_->Enable();
        sync_button_->Enable();
        view_button_->Enable();
        ShowIssueDetails(current_links_[idx]);
    }
}

void IssueTrackerPanel::ShowIssueDetails(const IssueLink& link) {
    std::ostringstream oss;
    oss << "Issue: " << link.issue.display_key << "\n";
    oss << "Title: " << link.issue.title << "\n";
    oss << "Status: " << IssueStatusToString(link.issue.status) << "\n";
    oss << "Provider: " << link.issue.provider << "\n";
    oss << "URL: " << link.issue.url << "\n\n";
    oss << "Link Type: " << (link.type == LinkType::MANUAL ? "Manual" : "Auto") << "\n";
    oss << "Sync: " << (link.is_sync_enabled ? "Enabled" : "Disabled") << "\n";
    oss << "Linked: " << ctime(&link.created_at);
    
    details_text_->SetValue(oss.str());
}

void IssueTrackerPanel::OnSettings(wxCommandEvent& /*event*/) {
    IssueTrackerSettingsDialog dlg(this);
    dlg.ShowModal();
}

// ============================================================================
// Create Issue Dialog
// ============================================================================
CreateIssueDialog::CreateIssueDialog(wxWindow* parent, const ObjectReference& obj)
    : wxDialog(parent, wxID_ANY, "Create Issue", wxDefaultPosition, wxSize(500, 450))
    , object_(obj) {
    CreateControls();
}

void CreateIssueDialog::CreateControls() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Provider selection
    auto* provider_row = new wxBoxSizer(wxHORIZONTAL);
    provider_row->Add(new wxStaticText(this, wxID_ANY, "Provider:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    provider_choice_ = new wxChoice(this, wxID_ANY);
    provider_choice_->Append("Jira");
    provider_choice_->Append("GitHub");
    provider_choice_->Append("GitLab");
    provider_choice_->SetSelection(0);
    provider_row->Add(provider_choice_, 1);
    main_sizer->Add(provider_row, 0, wxEXPAND | wxALL, 10);
    
    // Title
    auto* title_row = new wxBoxSizer(wxHORIZONTAL);
    title_row->Add(new wxStaticText(this, wxID_ANY, "Title:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    title_ctrl_ = new wxTextCtrl(this, wxID_ANY, "Schema change: " + object_.name);
    title_row->Add(title_ctrl_, 1);
    main_sizer->Add(title_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Description
    main_sizer->Add(new wxStaticText(this, wxID_ANY, "Description:"), 0, wxLEFT | wxRIGHT, 10);
    description_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 100),
                                        wxTE_MULTILINE);
    main_sizer->Add(description_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Type and Priority
    auto* type_prio_row = new wxBoxSizer(wxHORIZONTAL);
    type_prio_row->Add(new wxStaticText(this, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    type_choice_ = new wxChoice(this, wxID_ANY);
    type_choice_->Append("Task");
    type_choice_->Append("Bug");
    type_choice_->Append("Story");
    type_choice_->SetSelection(0);
    type_prio_row->Add(type_choice_, 0, wxRIGHT, 15);
    
    type_prio_row->Add(new wxStaticText(this, wxID_ANY, "Priority:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    priority_choice_ = new wxChoice(this, wxID_ANY);
    priority_choice_->Append("Low");
    priority_choice_->Append("Medium");
    priority_choice_->Append("High");
    priority_choice_->SetSelection(1);
    type_prio_row->Add(priority_choice_, 0);
    type_prio_row->AddStretchSpacer(1);
    main_sizer->Add(type_prio_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Labels
    auto* labels_row = new wxBoxSizer(wxHORIZONTAL);
    labels_row->Add(new wxStaticText(this, wxID_ANY, "Labels:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    labels_ctrl_ = new wxTextCtrl(this, wxID_ANY, "database,schema-change");
    labels_row->Add(labels_ctrl_, 1);
    main_sizer->Add(labels_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Attachments
    auto* attach_box = new wxStaticBoxSizer(wxVERTICAL, this, "Attachments");
    attach_schema_cb_ = new wxCheckBox(this, wxID_ANY, "Attach schema definition");
    attach_schema_cb_->SetValue(true);
    attach_box->Add(attach_schema_cb_, 0, wxALL, 5);
    
    attach_diagram_cb_ = new wxCheckBox(this, wxID_ANY, "Attach ER diagram");
    attach_box->Add(attach_diagram_cb_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
    main_sizer->Add(attach_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Buttons
    auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddStretchSpacer(1);
    button_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 5);
    auto* create_btn = new wxButton(this, wxID_OK, "Create");
    create_btn->SetDefault();
    button_sizer->Add(create_btn, 0);
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
    
    Bind(wxEVT_BUTTON, &CreateIssueDialog::OnCreate, this, wxID_OK);
}

void CreateIssueDialog::OnCreate(wxCommandEvent& /*event*/) {
    // Get the issue tracker manager
    auto& manager = IssueLinkManager::Instance();
    
    // Get selected provider
    int provider_idx = provider_choice_->GetSelection();
    std::string provider;
    switch (provider_idx) {
        case 0: provider = JiraAdapter::PROVIDER_NAME; break;
        case 1: provider = GitHubAdapter::PROVIDER_NAME; break;
        case 2: provider = GitLabAdapter::PROVIDER_NAME; break;
        default: provider = JiraAdapter::PROVIDER_NAME;
    }
    
    // Find the adapter
    auto* adapter = manager.GetAdapter(provider);
    if (!adapter) {
        wxMessageBox("Provider not configured", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    
    // Build the create request
    IssueCreateRequest request;
    request.title = title_ctrl_->GetValue().ToStdString();
    request.description = description_ctrl_->GetValue().ToStdString();
    
    // Add object context if available
    if (!object_.database.empty()) {
        request.description += "\n\n---\n**Object Context:**\n";
        request.description += "- Database: " + object_.database + "\n";
        request.description += "- Schema: " + object_.schema + "\n";
        request.description += "- Object: " + object_.name + "\n";
        request.description += "- Type: " + std::to_string(static_cast<int>(object_.type)) + "\n";
    }
    
    // Get issue type
    wxString type_str = type_choice_->GetStringSelection();
    if (type_str == "Bug") {
        request.issue_type = IssueType::BUG;
    } else if (type_str == "Enhancement") {
        request.issue_type = IssueType::ENHANCEMENT;
    } else if (type_str == "Task") {
        request.issue_type = IssueType::TASK;
    } else {
        request.issue_type = IssueType::OTHER;
    }
    
    // Set priority
    int priority_idx = priority_choice_->GetSelection();
    switch (priority_idx) {
        case 0: request.priority = IssuePriority::HIGHEST; break;
        case 1: request.priority = IssuePriority::HIGH; break;
        case 2: request.priority = IssuePriority::MEDIUM; break;
        case 3: request.priority = IssuePriority::LOW; break;
        case 4: request.priority = IssuePriority::LOWEST; break;
        default: request.priority = IssuePriority::MEDIUM;
    }
    
    // Create the issue
    IssueReference issue = adapter->CreateIssue(request);
    
    if (issue.issue_id.empty()) {
        wxMessageBox("Failed to create issue. Check your connection and settings.", 
                     "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    
    // Create link to the object
    if (!object_.name.empty()) {
        manager.CreateLink(object_, issue);
    }
    
    // Store the created issue
    created_issue_ = issue;
    
    wxMessageBox("Issue created successfully: " + issue.display_key, 
                 "Success", wxOK | wxICON_INFORMATION, this);
    
    EndModal(wxID_OK);
}

void CreateIssueDialog::OnProviderChanged(wxCommandEvent& /*event*/) {
    // Update available types based on provider
}

// ============================================================================
// Link Issue Dialog
// ============================================================================
LinkIssueDialog::LinkIssueDialog(wxWindow* parent, const ObjectReference& obj)
    : wxDialog(parent, wxID_ANY, "Link Existing Issue", wxDefaultPosition, wxSize(500, 400))
    , object_(obj) {
    CreateControls();
}

void LinkIssueDialog::CreateControls() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Provider selection
    auto* provider_row = new wxBoxSizer(wxHORIZONTAL);
    provider_row->Add(new wxStaticText(this, wxID_ANY, "Provider:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    provider_choice_ = new wxChoice(this, wxID_ANY);
    provider_choice_->Append("Jira");
    provider_choice_->Append("GitHub");
    provider_choice_->Append("GitLab");
    provider_choice_->SetSelection(0);
    provider_row->Add(provider_choice_, 0, wxRIGHT, 15);
    
    // Search
    provider_row->Add(new wxStaticText(this, wxID_ANY, "Search:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    search_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    provider_row->Add(search_ctrl_, 1, wxRIGHT, 5);
    
    search_button_ = new wxButton(this, wxID_ANY, "Search", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    provider_row->Add(search_button_, 0);
    main_sizer->Add(provider_row, 0, wxEXPAND | wxALL, 10);
    
    // Results list
    results_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxLC_REPORT | wxLC_SINGLE_SEL);
    results_list_->AppendColumn("Key", wxLIST_FORMAT_LEFT, 80);
    results_list_->AppendColumn("Title", wxLIST_FORMAT_LEFT, 300);
    results_list_->AppendColumn("Status", wxLIST_FORMAT_LEFT, 80);
    main_sizer->Add(results_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Recent issues button
    auto* recent_btn = new wxButton(this, wxID_ANY, "Load Recent Issues");
    main_sizer->Add(recent_btn, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Buttons
    auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddStretchSpacer(1);
    button_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 5);
    auto* link_btn = new wxButton(this, wxID_OK, "Link");
    link_btn->SetDefault();
    button_sizer->Add(link_btn, 0);
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
    
    search_button_->Bind(wxEVT_BUTTON, &LinkIssueDialog::OnSearch, this);
    recent_btn->Bind(wxEVT_BUTTON, &LinkIssueDialog::LoadRecentIssues, this);
    Bind(wxEVT_BUTTON, &LinkIssueDialog::OnLink, this, wxID_OK);
}

void LinkIssueDialog::OnSearch(wxCommandEvent& /*event*/) {
    auto& manager = IssueLinkManager::Instance();
    
    // Get selected provider
    int provider_idx = provider_choice_->GetSelection();
    std::string provider;
    switch (provider_idx) {
        case 0: provider = JiraAdapter::PROVIDER_NAME; break;
        case 1: provider = GitHubAdapter::PROVIDER_NAME; break;
        case 2: provider = GitLabAdapter::PROVIDER_NAME; break;
        default: provider = JiraAdapter::PROVIDER_NAME;
    }
    
    auto* adapter = manager.GetAdapter(provider);
    if (!adapter) {
        wxMessageBox("Provider not configured", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    
    // Clear existing results
    results_list_->DeleteAllItems();
    search_results_.clear();
    
    // Build search query
    SearchQuery query;
    query.text = search_ctrl_->GetValue().ToStdString();
    query.limit = 20;
    
    // Execute search
    auto results = adapter->SearchIssues(query);
    
    // Populate list
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& issue = results[i];
        long idx = results_list_->InsertItem(i, issue.display_key);
        results_list_->SetItem(idx, 1, issue.title);
        results_list_->SetItem(idx, 2, IssueStatusToString(issue.status));
        search_results_.push_back(issue);
    }
}

void LinkIssueDialog::OnLink(wxCommandEvent& /*event*/) {
    long idx = results_list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (idx < 0 || static_cast<size_t>(idx) >= search_results_.size()) {
        wxMessageBox("Please select an issue to link", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    
    auto& manager = IssueLinkManager::Instance();
    
    // Create the link
    bool success = manager.CreateLink(object_, search_results_[idx]);
    
    if (!success) {
        wxMessageBox("Failed to create link", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    
    wxMessageBox("Issue linked successfully", "Success", wxOK | wxICON_INFORMATION, this);
    EndModal(wxID_OK);
}

void LinkIssueDialog::LoadRecentIssues(wxCommandEvent& /*event*/) {
    auto& manager = IssueLinkManager::Instance();
    
    // Get selected provider
    int provider_idx = provider_choice_->GetSelection();
    std::string provider;
    switch (provider_idx) {
        case 0: provider = JiraAdapter::PROVIDER_NAME; break;
        case 1: provider = GitHubAdapter::PROVIDER_NAME; break;
        case 2: provider = GitLabAdapter::PROVIDER_NAME; break;
        default: provider = JiraAdapter::PROVIDER_NAME;
    }
    
    auto* adapter = manager.GetAdapter(provider);
    if (!adapter) {
        wxMessageBox("Provider not configured", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    
    // Clear existing results
    results_list_->DeleteAllItems();
    search_results_.clear();
    
    // Get recent issues
    auto results = adapter->GetRecentIssues(20);
    
    // Populate list
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& issue = results[i];
        long idx = results_list_->InsertItem(i, issue.display_key);
        results_list_->SetItem(idx, 1, issue.title);
        results_list_->SetItem(idx, 2, IssueStatusToString(issue.status));
        search_results_.push_back(issue);
    }
}

std::string LinkIssueDialog::GetSelectedIssueId() {
    long idx = results_list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (idx >= 0 && static_cast<size_t>(idx) < search_results_.size()) {
        return search_results_[idx].issue_id;
    }
    return "";
}

// ============================================================================
// Settings Dialog
// ============================================================================
IssueTrackerSettingsDialog::IssueTrackerSettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Issue Tracker Settings", wxDefaultPosition, wxSize(500, 350)) {
    CreateControls();
}

void IssueTrackerSettingsDialog::CreateControls() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Trackers list
    main_sizer->Add(new wxStaticText(this, wxID_ANY, "Configured Trackers:"), 0, wxALL, 10);
    trackers_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
    trackers_list_->AppendColumn("Name", wxLIST_FORMAT_LEFT, 120);
    trackers_list_->AppendColumn("Provider", wxLIST_FORMAT_LEFT, 100);
    trackers_list_->AppendColumn("Status", wxLIST_FORMAT_LEFT, 100);
    trackers_list_->AppendColumn("Project", wxLIST_FORMAT_LEFT, 150);
    main_sizer->Add(trackers_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Buttons
    auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    add_button_ = new wxButton(this, wxID_ANY, "Add...");
    button_sizer->Add(add_button_, 0, wxRIGHT, 5);
    
    remove_button_ = new wxButton(this, wxID_ANY, "Remove");
    button_sizer->Add(remove_button_, 0, wxRIGHT, 5);
    
    test_button_ = new wxButton(this, wxID_ANY, "Test Connection");
    button_sizer->Add(test_button_, 0, wxRIGHT, 5);
    
    button_sizer->AddStretchSpacer(1);
    
    button_sizer->Add(new wxButton(this, wxID_CANCEL, "Close"), 0, wxRIGHT, 5);
    auto* save_btn = new wxButton(this, wxID_OK, "Save");
    save_btn->SetDefault();
    button_sizer->Add(save_btn, 0);
    
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
    
    add_button_->Bind(wxEVT_BUTTON, &IssueTrackerSettingsDialog::OnAddTracker, this);
    remove_button_->Bind(wxEVT_BUTTON, &IssueTrackerSettingsDialog::OnRemoveTracker, this);
    test_button_->Bind(wxEVT_BUTTON, &IssueTrackerSettingsDialog::OnTestConnection, this);
    Bind(wxEVT_BUTTON, &IssueTrackerSettingsDialog::OnSave, this, wxID_OK);
}

void IssueTrackerSettingsDialog::OnAddTracker(wxCommandEvent& /*event*/) {
    // TODO: Show add tracker dialog
}

void IssueTrackerSettingsDialog::OnRemoveTracker(wxCommandEvent& /*event*/) {
    // TODO: Remove selected tracker
}

void IssueTrackerSettingsDialog::OnTestConnection(wxCommandEvent& /*event*/) {
    // TODO: Test connection to selected tracker
}

void IssueTrackerSettingsDialog::OnSave(wxCommandEvent& /*event*/) {
    EndModal(wxID_OK);
}

} // namespace scratchrobin
