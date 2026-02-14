/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "help_browser.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/html/htmlwin.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>
#include <wx/statbmp.h>
#include <wx/textdlg.h>
#include <wx/stattext.h>
#include <wx/toolbar.h>
#include <wx/treectrl.h>

namespace scratchrobin {

namespace {

// Help topic IDs for toolbar buttons
constexpr int ID_HELP_BACK = wxID_HIGHEST + 1000;
constexpr int ID_HELP_FORWARD = wxID_HIGHEST + 1001;
constexpr int ID_HELP_HOME = wxID_HIGHEST + 1002;
constexpr int ID_HELP_SEARCH = wxID_HIGHEST + 1003;
constexpr int ID_HELP_FIND = wxID_HIGHEST + 1004;

// Tree item data to store topic ID
class TopicTreeItemData : public wxTreeItemData {
public:
    explicit TopicTreeItemData(HelpTopicId id) : topic_id_(id) {}
    HelpTopicId GetTopicId() const { return topic_id_; }
private:
    HelpTopicId topic_id_;
};

// Utility functions
std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

std::vector<std::string> SplitWords(const std::string& text) {
    std::vector<std::string> words;
    std::string current;
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            current.push_back(c);
        } else if (!current.empty()) {
            words.push_back(ToLower(current));
            current.clear();
        }
    }
    if (!current.empty()) {
        words.push_back(ToLower(current));
    }
    return words;
}

bool ContainsAllWords(const std::string& text, const std::vector<std::string>& words) {
    std::string lowerText = ToLower(text);
    for (const auto& word : words) {
        if (lowerText.find(word) == std::string::npos) {
            return false;
        }
    }
    return true;
}

} // namespace
} // namespace scratchrobin

// Static member initialization
namespace scratchrobin {
HelpBrowser* HelpBrowser::instance_ = nullptr;

// Event table
wxBEGIN_EVENT_TABLE(HelpBrowser, wxFrame)
    EVT_TOOL(ID_HELP_BACK, HelpBrowser::OnBack)
    EVT_TOOL(ID_HELP_FORWARD, HelpBrowser::OnForward)
    EVT_TOOL(ID_HELP_HOME, HelpBrowser::OnHome)
    EVT_TOOL(ID_HELP_FIND, HelpBrowser::OnFind)
    EVT_SEARCHCTRL_SEARCH_BTN(wxID_ANY, HelpBrowser::OnSearch)
    EVT_TREE_SEL_CHANGED(wxID_ANY, HelpBrowser::OnTreeSelection)
    EVT_CLOSE(HelpBrowser::OnClose)
wxEND_EVENT_TABLE()

// =============================================================================
// Static Public Methods
// =============================================================================

void HelpBrowser::ShowBrowser(wxWindow* parent) {
    if (!instance_) {
        instance_ = new HelpBrowser(parent);
    }
    instance_->Show(true);
    instance_->Raise();
}

void HelpBrowser::ShowHelp(HelpTopicId topicId) {
    ShowBrowser();
    if (instance_) {
        instance_->NavigateToTopic(topicId);
    }
}

void HelpBrowser::ShowHelpForWindow(const std::string& windowClassName) {
    HelpTopicId topicId = HelpTopicMapper::MapWindowClass(windowClassName);
    ShowHelp(topicId);
}

bool HelpBrowser::IsOpen() {
    return instance_ != nullptr;
}

void HelpBrowser::CloseBrowser() {
    if (instance_) {
        instance_->Close(true);
    }
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

HelpBrowser::HelpBrowser(wxWindow* parent)
    : wxFrame(parent, wxID_ANY, "ScratchRobin Help", 
              wxDefaultPosition, wxSize(1000, 700),
              wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT) {
    SetMinSize(wxSize(600, 400));
    
    PopulateHelpContent();
    BuildLayout();
    NavigateHome();
}

HelpBrowser::~HelpBrowser() {
    instance_ = nullptr;
}

// =============================================================================
// Layout Building
// =============================================================================

void HelpBrowser::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    
    // Build toolbar
    BuildToolbar();
    
    // Create splitter
    splitter_ = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxSP_LIVE_UPDATE | wxSP_3D);
    
    // Left panel: Topic tree
    auto* treePanel = new wxPanel(splitter_, wxID_ANY);
    auto* treeSizer = new wxBoxSizer(wxVERTICAL);
    
    treeSizer->Add(new wxStaticText(treePanel, wxID_ANY, "Topics"), 0, wxALL, 4);
    topic_tree_ = new wxTreeCtrl(treePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT);
    treeSizer->Add(topic_tree_, 1, wxEXPAND | wxALL, 4);
    treePanel->SetSizer(treeSizer);
    
    // Right panel: Content view
    auto* contentPanel = new wxPanel(splitter_, wxID_ANY);
    auto* contentSizer = new wxBoxSizer(wxVERTICAL);
    
    content_view_ = new wxHtmlWindow(contentPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxHW_SCROLLBAR_AUTO);
    contentSizer->Add(content_view_, 1, wxEXPAND | wxALL, 8);
    contentPanel->SetSizer(contentSizer);
    
    // Split the window
    splitter_->SplitVertically(treePanel, contentPanel, 280);
    splitter_->SetMinimumPaneSize(200);
    
    rootSizer->Add(splitter_, 1, wxEXPAND);
    SetSizer(rootSizer);
    
    // Build the topic tree
    BuildTopicTree();
}

void HelpBrowser::BuildToolbar() {
    toolbar_ = CreateToolBar(wxTB_FLAT | wxTB_HORIZONTAL);
    
    toolbar_->AddTool(ID_HELP_BACK, "Back", wxNullBitmap, "Go back");
    toolbar_->AddTool(ID_HELP_FORWARD, "Forward", wxNullBitmap, "Go forward");
    toolbar_->AddTool(ID_HELP_HOME, "Home", wxNullBitmap, "Go to home page");
    toolbar_->AddSeparator();
    
    // Search control
    search_ctrl_ = new wxSearchCtrl(toolbar_, ID_HELP_SEARCH, wxEmptyString, 
                                    wxDefaultPosition, wxSize(250, -1));
    search_ctrl_->SetDescriptiveText("Search help...");
    search_ctrl_->ShowSearchButton(true);
    search_ctrl_->ShowCancelButton(true);
    toolbar_->AddControl(search_ctrl_, "Search");
    
    toolbar_->AddSeparator();
    toolbar_->AddTool(ID_HELP_FIND, "Find in Page", wxNullBitmap, "Find in current page");
    
    toolbar_->Realize();
    
    // Initial state
    toolbar_->EnableTool(ID_HELP_BACK, false);
    toolbar_->EnableTool(ID_HELP_FORWARD, false);
}

void HelpBrowser::BuildTopicTree() {
    if (!topic_tree_) return;
    
    topic_tree_->DeleteAllItems();
    tree_item_to_topic_.clear();
    
    wxTreeItemId root = topic_tree_->AddRoot("Help Topics");
    
    // Getting Started
    wxTreeItemId gettingStarted = topic_tree_->AppendItem(root, "Getting Started", -1, -1,
        new TopicTreeItemData(HelpTopicId::GettingStarted));
    tree_item_to_topic_[gettingStarted] = HelpTopicId::GettingStarted;
    
    wxTreeItemId intro = topic_tree_->AppendItem(gettingStarted, "Introduction", -1, -1,
        new TopicTreeItemData(HelpTopicId::Introduction));
    tree_item_to_topic_[intro] = HelpTopicId::Introduction;
    
    wxTreeItemId quickStart = topic_tree_->AppendItem(gettingStarted, "Quick Start Guide", -1, -1,
        new TopicTreeItemData(HelpTopicId::QuickStartGuide));
    tree_item_to_topic_[quickStart] = HelpTopicId::QuickStartGuide;
    
    wxTreeItemId connectDb = topic_tree_->AppendItem(gettingStarted, "Connecting to a Database", -1, -1,
        new TopicTreeItemData(HelpTopicId::ConnectingToDatabase));
    tree_item_to_topic_[connectDb] = HelpTopicId::ConnectingToDatabase;
    
    // SQL Editor
    wxTreeItemId sqlEditor = topic_tree_->AppendItem(root, "SQL Editor", -1, -1,
        new TopicTreeItemData(HelpTopicId::SqlEditor));
    tree_item_to_topic_[sqlEditor] = HelpTopicId::SqlEditor;
    
    wxTreeItemId writingQueries = topic_tree_->AppendItem(sqlEditor, "Writing Queries", -1, -1,
        new TopicTreeItemData(HelpTopicId::WritingQueries));
    tree_item_to_topic_[writingQueries] = HelpTopicId::WritingQueries;
    
    wxTreeItemId executingSql = topic_tree_->AppendItem(sqlEditor, "Executing SQL", -1, -1,
        new TopicTreeItemData(HelpTopicId::ExecutingSql));
    tree_item_to_topic_[executingSql] = HelpTopicId::ExecutingSql;
    
    wxTreeItemId resultGrid = topic_tree_->AppendItem(sqlEditor, "Result Grid", -1, -1,
        new TopicTreeItemData(HelpTopicId::ResultGrid));
    tree_item_to_topic_[resultGrid] = HelpTopicId::ResultGrid;
    
    // Database Objects
    wxTreeItemId dbObjects = topic_tree_->AppendItem(root, "Database Objects", -1, -1,
        new TopicTreeItemData(HelpTopicId::DatabaseObjects));
    tree_item_to_topic_[dbObjects] = HelpTopicId::DatabaseObjects;
    
    wxTreeItemId tables = topic_tree_->AppendItem(dbObjects, "Tables", -1, -1,
        new TopicTreeItemData(HelpTopicId::Tables));
    tree_item_to_topic_[tables] = HelpTopicId::Tables;
    
    wxTreeItemId indexes = topic_tree_->AppendItem(dbObjects, "Indexes", -1, -1,
        new TopicTreeItemData(HelpTopicId::Indexes));
    tree_item_to_topic_[indexes] = HelpTopicId::Indexes;
    
    wxTreeItemId views = topic_tree_->AppendItem(dbObjects, "Views", -1, -1,
        new TopicTreeItemData(HelpTopicId::Views));
    tree_item_to_topic_[views] = HelpTopicId::Views;
    
    wxTreeItemId triggers = topic_tree_->AppendItem(dbObjects, "Triggers", -1, -1,
        new TopicTreeItemData(HelpTopicId::Triggers));
    tree_item_to_topic_[triggers] = HelpTopicId::Triggers;
    
    wxTreeItemId procedures = topic_tree_->AppendItem(dbObjects, "Procedures", -1, -1,
        new TopicTreeItemData(HelpTopicId::Procedures));
    tree_item_to_topic_[procedures] = HelpTopicId::Procedures;
    
    wxTreeItemId sequences = topic_tree_->AppendItem(dbObjects, "Sequences", -1, -1,
        new TopicTreeItemData(HelpTopicId::Sequences));
    tree_item_to_topic_[sequences] = HelpTopicId::Sequences;
    
    wxTreeItemId domains = topic_tree_->AppendItem(dbObjects, "Domains", -1, -1,
        new TopicTreeItemData(HelpTopicId::Domains));
    tree_item_to_topic_[domains] = HelpTopicId::Domains;
    
    // ERD / Diagramming
    wxTreeItemId erd = topic_tree_->AppendItem(root, "ERD / Diagramming", -1, -1,
        new TopicTreeItemData(HelpTopicId::ErdDiagramming));
    tree_item_to_topic_[erd] = HelpTopicId::ErdDiagramming;
    
    wxTreeItemId creatingDiagrams = topic_tree_->AppendItem(erd, "Creating Diagrams", -1, -1,
        new TopicTreeItemData(HelpTopicId::CreatingDiagrams));
    tree_item_to_topic_[creatingDiagrams] = HelpTopicId::CreatingDiagrams;
    
    wxTreeItemId reverseEng = topic_tree_->AppendItem(erd, "Reverse Engineering", -1, -1,
        new TopicTreeItemData(HelpTopicId::ReverseEngineering));
    tree_item_to_topic_[reverseEng] = HelpTopicId::ReverseEngineering;
    
    wxTreeItemId forwardEng = topic_tree_->AppendItem(erd, "Forward Engineering", -1, -1,
        new TopicTreeItemData(HelpTopicId::ForwardEngineering));
    tree_item_to_topic_[forwardEng] = HelpTopicId::ForwardEngineering;
    
    wxTreeItemId notations = topic_tree_->AppendItem(erd, "Notations", -1, -1,
        new TopicTreeItemData(HelpTopicId::Notations));
    tree_item_to_topic_[notations] = HelpTopicId::Notations;
    
    // Administration
    wxTreeItemId admin = topic_tree_->AppendItem(root, "Administration", -1, -1,
        new TopicTreeItemData(HelpTopicId::Administration));
    tree_item_to_topic_[admin] = HelpTopicId::Administration;
    
    wxTreeItemId backupRestore = topic_tree_->AppendItem(admin, "Backup and Restore", -1, -1,
        new TopicTreeItemData(HelpTopicId::BackupRestore));
    tree_item_to_topic_[backupRestore] = HelpTopicId::BackupRestore;
    
    wxTreeItemId userMgmt = topic_tree_->AppendItem(admin, "User Management", -1, -1,
        new TopicTreeItemData(HelpTopicId::UserManagement));
    tree_item_to_topic_[userMgmt] = HelpTopicId::UserManagement;
    
    wxTreeItemId jobSched = topic_tree_->AppendItem(admin, "Job Scheduling", -1, -1,
        new TopicTreeItemData(HelpTopicId::JobScheduling));
    tree_item_to_topic_[jobSched] = HelpTopicId::JobScheduling;
    
    wxTreeItemId storageMgmt = topic_tree_->AppendItem(admin, "Storage Management", -1, -1,
        new TopicTreeItemData(HelpTopicId::StorageManagement));
    tree_item_to_topic_[storageMgmt] = HelpTopicId::StorageManagement;
    
    // SQL Reference
    wxTreeItemId sqlRef = topic_tree_->AppendItem(root, "SQL Reference", -1, -1,
        new TopicTreeItemData(HelpTopicId::SqlReference));
    tree_item_to_topic_[sqlRef] = HelpTopicId::SqlReference;
    
    wxTreeItemId dataTypes = topic_tree_->AppendItem(sqlRef, "Data Types", -1, -1,
        new TopicTreeItemData(HelpTopicId::DataTypes));
    tree_item_to_topic_[dataTypes] = HelpTopicId::DataTypes;
    
    wxTreeItemId functions = topic_tree_->AppendItem(sqlRef, "Functions", -1, -1,
        new TopicTreeItemData(HelpTopicId::Functions));
    tree_item_to_topic_[functions] = HelpTopicId::Functions;
    
    wxTreeItemId operators = topic_tree_->AppendItem(sqlRef, "Operators", -1, -1,
        new TopicTreeItemData(HelpTopicId::Operators));
    tree_item_to_topic_[operators] = HelpTopicId::Operators;
    
    // Expand main categories
    topic_tree_->Expand(gettingStarted);
    topic_tree_->Expand(sqlEditor);
    topic_tree_->Expand(dbObjects);
    topic_tree_->Expand(erd);
    topic_tree_->Expand(admin);
    topic_tree_->Expand(sqlRef);
}

// =============================================================================
// Help Content Population
// =============================================================================

void HelpBrowser::PopulateHelpContent() {
    // Getting Started topics
    auto addTopic = [this](HelpTopicId id, const std::string& title, 
                          const std::string& description, HelpTopicId parent) {
        auto topic = std::make_unique<HelpTopic>();
        topic->id = id;
        topic->title = title;
        topic->description = description;
        topic->parentId = parent;
        topics_[id] = std::move(topic);
    };
    
    // Home page
    addTopic(HelpTopicId::Home, "ScratchRobin Help", 
        "Welcome to ScratchRobin - Your Database Management Tool", HelpTopicId::None);
    
    // Getting Started
    addTopic(HelpTopicId::GettingStarted, "Getting Started",
        "Learn the basics of using ScratchRobin.", HelpTopicId::Home);
    addTopic(HelpTopicId::Introduction, "Introduction",
        "Introduction to ScratchRobin and its features.", HelpTopicId::GettingStarted);
    addTopic(HelpTopicId::QuickStartGuide, "Quick Start Guide",
        "Get up and running quickly with ScratchRobin.", HelpTopicId::GettingStarted);
    addTopic(HelpTopicId::ConnectingToDatabase, "Connecting to a Database",
        "How to connect to your database servers.", HelpTopicId::GettingStarted);
    
    // SQL Editor
    addTopic(HelpTopicId::SqlEditor, "SQL Editor",
        "Write and execute SQL queries.", HelpTopicId::Home);
    addTopic(HelpTopicId::WritingQueries, "Writing Queries",
        "Tips for writing effective SQL queries.", HelpTopicId::SqlEditor);
    addTopic(HelpTopicId::ExecutingSql, "Executing SQL",
        "Run queries and view results.", HelpTopicId::SqlEditor);
    addTopic(HelpTopicId::ResultGrid, "Result Grid",
        "Work with query results in the data grid.", HelpTopicId::SqlEditor);
    
    // Database Objects
    addTopic(HelpTopicId::DatabaseObjects, "Database Objects",
        "Manage database schema objects.", HelpTopicId::Home);
    addTopic(HelpTopicId::Tables, "Tables",
        "Create and manage database tables.", HelpTopicId::DatabaseObjects);
    addTopic(HelpTopicId::Indexes, "Indexes",
        "Optimize queries with indexes.", HelpTopicId::DatabaseObjects);
    addTopic(HelpTopicId::Views, "Views",
        "Create views to simplify queries.", HelpTopicId::DatabaseObjects);
    addTopic(HelpTopicId::Triggers, "Triggers",
        "Automate actions with triggers.", HelpTopicId::DatabaseObjects);
    addTopic(HelpTopicId::Procedures, "Procedures",
        "Create stored procedures.", HelpTopicId::DatabaseObjects);
    addTopic(HelpTopicId::Sequences, "Sequences",
        "Generate unique identifiers.", HelpTopicId::DatabaseObjects);
    addTopic(HelpTopicId::Domains, "Domains",
        "Define reusable data types.", HelpTopicId::DatabaseObjects);
    
    // ERD / Diagramming
    addTopic(HelpTopicId::ErdDiagramming, "ERD / Diagramming",
        "Visual database design tools.", HelpTopicId::Home);
    addTopic(HelpTopicId::CreatingDiagrams, "Creating Diagrams",
        "Create entity-relationship diagrams.", HelpTopicId::ErdDiagramming);
    addTopic(HelpTopicId::ReverseEngineering, "Reverse Engineering",
        "Generate diagrams from existing databases.", HelpTopicId::ErdDiagramming);
    addTopic(HelpTopicId::ForwardEngineering, "Forward Engineering",
        "Generate SQL from diagrams.", HelpTopicId::ErdDiagramming);
    addTopic(HelpTopicId::Notations, "Notations",
        "Choose diagram notation styles.", HelpTopicId::ErdDiagramming);
    
    // Administration
    addTopic(HelpTopicId::Administration, "Administration",
        "Database administration tasks.", HelpTopicId::Home);
    addTopic(HelpTopicId::BackupRestore, "Backup and Restore",
        "Protect your data with backups.", HelpTopicId::Administration);
    addTopic(HelpTopicId::UserManagement, "User Management",
        "Manage database users and roles.", HelpTopicId::Administration);
    addTopic(HelpTopicId::JobScheduling, "Job Scheduling",
        "Schedule recurring tasks.", HelpTopicId::Administration);
    addTopic(HelpTopicId::StorageManagement, "Storage Management",
        "Monitor and optimize storage.", HelpTopicId::Administration);
    
    // SQL Reference
    addTopic(HelpTopicId::SqlReference, "SQL Reference",
        "SQL language reference.", HelpTopicId::Home);
    addTopic(HelpTopicId::DataTypes, "Data Types",
        "Supported data types.", HelpTopicId::SqlReference);
    addTopic(HelpTopicId::Functions, "Functions",
        "Built-in SQL functions.", HelpTopicId::SqlReference);
    addTopic(HelpTopicId::Operators, "Operators",
        "SQL operators and expressions.", HelpTopicId::SqlReference);
    
    // Window topics
    addTopic(HelpTopicId::CatalogBrowser, "Catalog Browser",
        "Browse database objects and metadata.", HelpTopicId::Home);
    addTopic(HelpTopicId::Monitoring, "Monitoring",
        "Monitor database performance and activity.", HelpTopicId::Home);
    addTopic(HelpTopicId::UsersRoles, "Users and Roles",
        "Manage database security.", HelpTopicId::Home);
    addTopic(HelpTopicId::Diagram, "Diagram",
        "Visual database designer.", HelpTopicId::Home);
    addTopic(HelpTopicId::JobScheduler, "Job Scheduler",
        "Schedule and manage jobs.", HelpTopicId::Home);
}

// =============================================================================
// Navigation
// =============================================================================

void HelpBrowser::NavigateToTopic(HelpTopicId id) {
    if (id == HelpTopicId::None) return;
    
    // Add to history if not navigating through history
    if (!navigating_history_) {
        // Remove any forward history
        if (history_position_ < history_.size()) {
            history_.erase(history_.begin() + history_position_ + 1, history_.end());
        }
        
        // Add new entry
        const HelpTopic* topic = GetTopic(id);
        if (topic) {
            HistoryEntry entry;
            entry.topicId = id;
            entry.topicTitle = topic->title;
            history_.push_back(entry);
            
            // Limit history size
            if (history_.size() > kMaxHistorySize) {
                history_.erase(history_.begin());
            }
            history_position_ = history_.size() - 1;
        }
    }
    
    navigating_history_ = false;
    current_topic_ = id;
    
    // Update UI
    LoadTopic(id);
    SelectTreeItemByTopicId(id);
    
    if (toolbar_) {
        toolbar_->EnableTool(ID_HELP_BACK, CanGoBack());
        toolbar_->EnableTool(ID_HELP_FORWARD, CanGoForward());
    }
}

void HelpBrowser::LoadTopic(HelpTopicId id) {
    if (!content_view_) return;
    
    wxString html = GetTopicHtml(id);
    content_view_->SetPage(html);
}

void HelpBrowser::NavigateBack() {
    if (!CanGoBack()) return;
    
    navigating_history_ = true;
    history_position_--;
    current_topic_ = history_[history_position_].topicId;
    LoadTopic(current_topic_);
    SelectTreeItemByTopicId(current_topic_);
    
    if (toolbar_) {
        toolbar_->EnableTool(ID_HELP_BACK, CanGoBack());
        toolbar_->EnableTool(ID_HELP_FORWARD, CanGoForward());
    }
}

void HelpBrowser::NavigateForward() {
    if (!CanGoForward()) return;
    
    navigating_history_ = true;
    history_position_++;
    current_topic_ = history_[history_position_].topicId;
    LoadTopic(current_topic_);
    SelectTreeItemByTopicId(current_topic_);
    
    if (toolbar_) {
        toolbar_->EnableTool(ID_HELP_BACK, CanGoBack());
        toolbar_->EnableTool(ID_HELP_FORWARD, CanGoForward());
    }
}

void HelpBrowser::NavigateHome() {
    NavigateToTopic(HelpTopicId::Home);
}

bool HelpBrowser::CanGoBack() const {
    return history_position_ > 0;
}

bool HelpBrowser::CanGoForward() const {
    return history_position_ + 1 < history_.size();
}

// =============================================================================
// Event Handlers
// =============================================================================

void HelpBrowser::OnTreeSelection(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk()) return;
    
    HelpTopicId topicId = GetTopicIdFromTreeItem(item);
    if (topicId != HelpTopicId::None && topicId != current_topic_) {
        NavigateToTopic(topicId);
    }
}

void HelpBrowser::OnBack(wxCommandEvent&) {
    NavigateBack();
}

void HelpBrowser::OnForward(wxCommandEvent&) {
    NavigateForward();
}

void HelpBrowser::OnHome(wxCommandEvent&) {
    NavigateHome();
}

void HelpBrowser::OnSearch(wxCommandEvent&) {
    if (!search_ctrl_) return;
    
    wxString query = search_ctrl_->GetValue();
    if (!query.IsEmpty()) {
        PerformSearch(query.ToStdString());
    }
}

void HelpBrowser::OnFind(wxCommandEvent&) {
    if (!content_view_) return;
    
    wxTextEntryDialog dialog(this, "Enter text to find:", "Find in Page", find_text_);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    find_text_ = dialog.GetValue();
    if (find_text_.IsEmpty()) {
        return;
    }
    
    // Get current topic's HTML
    if (current_topic_ == HelpTopicId::None) {
        return;
    }
    
    const HelpTopic* topic = GetTopic(current_topic_);
    if (!topic) return;
    
    wxString html = GetTopicHtml(current_topic_);
    wxString search_text = find_text_.Lower();
    wxString html_lower = html.Lower();
    
    // Count matches before highlighting
    int match_count = 0;
    int pos = 0;
    while ((pos = html_lower.find(search_text, pos)) != wxString::npos) {
        match_count++;
        pos += search_text.Length();
    }
    
    if (match_count == 0) {
        SetStatusText(wxString::Format("No matches found for '%s'", find_text_));
        wxBell();
        return;
    }
    
    // Highlight matches by inserting <mark> tags
    // We need to be careful not to replace inside HTML tags
    wxString result_html;
    pos = 0;
    size_t html_len = html.Length();
    bool in_tag = false;
    
    for (size_t i = 0; i < html_len; ) {
        if (html[i] == '<') {
            in_tag = true;
            result_html += html[i];
            i++;
        } else if (html[i] == '>') {
            in_tag = false;
            result_html += html[i];
            i++;
        } else if (!in_tag) {
            // Check if we have a match here
            wxString remaining = html_lower.Mid(i);
            if (remaining.StartsWith(search_text)) {
                // Found a match - add highlighted version
                wxString original_text = html.Mid(i, search_text.Length());
                result_html += wxString::Format("<mark style='background-color: yellow; color: black;'>%s</mark>", 
                                                original_text);
                i += search_text.Length();
            } else {
                result_html += html[i];
                i++;
            }
        } else {
            result_html += html[i];
            i++;
        }
    }
    
    // Update the page with highlighted content
    content_view_->SetPage(result_html);
    
    SetStatusText(wxString::Format("Found %d match(es) for '%s'", 
                                   match_count, find_text_));
}

void HelpBrowser::OnClose(wxCloseEvent& event) {
    // Just hide instead of destroy to preserve state
    Hide();
    // Don't destroy - we'll keep it around for reuse
    // event.Skip();
}

void HelpBrowser::OnKeyDown(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_F1) {
        NavigateHome();
    } else if (event.GetKeyCode() == WXK_ESCAPE) {
        Close();
    } else {
        event.Skip();
    }
}

// =============================================================================
// Search
// =============================================================================

void HelpBrowser::PerformSearch(const std::string& query) {
    std::vector<HelpTopicId> results = SearchTopics(query);
    ShowSearchResults(results, query);
}

std::vector<HelpTopicId> HelpBrowser::SearchTopics(const std::string& query) {
    std::vector<HelpTopicId> results;
    std::vector<std::string> queryWords = SplitWords(query);
    
    for (const auto& [id, topic] : topics_) {
        std::string searchableText = topic->title + " " + topic->description;
        
        // Add keywords and tags to searchable text
        for (const auto& kw : topic->keywords) {
            searchableText += " " + kw;
        }
        for (const auto& tag : topic->tags) {
            searchableText += " " + tag;
        }
        
        if (ContainsAllWords(searchableText, queryWords)) {
            results.push_back(id);
        }
    }
    
    return results;
}

void HelpBrowser::ShowSearchResults(const std::vector<HelpTopicId>& results, 
                                     const std::string& query) {
    if (!content_view_) return;
    
    wxString html = GenerateSearchResultsHtml(results, query);
    content_view_->SetPage(html);
}

std::string HelpBrowser::HighlightMatches(const std::string& html, 
                                           const std::string& query) {
    // Simple highlighting - replace query words with marked versions
    std::string result = html;
    std::vector<std::string> words = SplitWords(query);
    
    for (const auto& word : words) {
        size_t pos = 0;
        std::string lowerResult = ToLower(result);
        std::string lowerWord = word;
        
        while ((pos = lowerResult.find(lowerWord, pos)) != std::string::npos) {
            // Check if we're not inside a tag
            size_t tagStart = result.rfind('<', pos);
            size_t tagEnd = result.rfind('>', pos);
            if (tagStart == std::string::npos || tagEnd > tagStart) {
                // We're not inside a tag, safe to highlight
                std::string original = result.substr(pos, word.length());
                std::string replacement = "<span style='background-color: yellow;'>" + 
                                         original + "</span>";
                result.replace(pos, word.length(), replacement);
                lowerResult = ToLower(result);
                pos += replacement.length();
            } else {
                pos += word.length();
            }
        }
    }
    
    return result;
}

// =============================================================================
// Utility Methods
// =============================================================================

const HelpTopic* HelpBrowser::GetTopic(HelpTopicId id) const {
    auto it = topics_.find(id);
    if (it != topics_.end()) {
        return it->second.get();
    }
    return nullptr;
}

HelpTopicId HelpBrowser::GetTopicIdFromTreeItem(wxTreeItemId item) const {
    auto it = tree_item_to_topic_.find(item);
    if (it != tree_item_to_topic_.end()) {
        return it->second;
    }
    return HelpTopicId::None;
}

wxTreeItemId HelpBrowser::FindTreeItemByTopicId(wxTreeItemId parent, HelpTopicId id) {
    if (!topic_tree_) return wxTreeItemId();
    
    wxTreeItemIdValue cookie;
    wxTreeItemId child = topic_tree_->GetFirstChild(parent, cookie);
    
    while (child.IsOk()) {
        if (GetTopicIdFromTreeItem(child) == id) {
            return child;
        }
        
        // Recursively search children
        wxTreeItemId found = FindTreeItemByTopicId(child, id);
        if (found.IsOk()) {
            return found;
        }
        
        child = topic_tree_->GetNextChild(parent, cookie);
    }
    
    return wxTreeItemId();
}

void HelpBrowser::SelectTreeItemByTopicId(HelpTopicId id) {
    if (!topic_tree_) return;
    
    wxTreeItemId root = topic_tree_->GetRootItem();
    if (!root.IsOk()) return;
    
    wxTreeItemId item = FindTreeItemByTopicId(root, id);
    if (item.IsOk()) {
        topic_tree_->SelectItem(item);
        topic_tree_->EnsureVisible(item);
    }
}

// =============================================================================
// HTML Content Generation
// =============================================================================

std::string HelpBrowser::GetBaseHtmlTemplate() const {
    return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            font-size: 14px;
            line-height: 1.6;
            color: #333;
            margin: 0;
            padding: 20px;
            background-color: #fff;
        }
        h1 {
            color: #2c3e50;
            border-bottom: 2px solid #3498db;
            padding-bottom: 10px;
            margin-top: 0;
        }
        h2 {
            color: #34495e;
            margin-top: 25px;
            border-bottom: 1px solid #bdc3c7;
            padding-bottom: 5px;
        }
        h3 {
            color: #555;
            margin-top: 20px;
        }
        p {
            margin: 10px 0;
        }
        ul, ol {
            margin: 10px 0;
            padding-left: 25px;
        }
        li {
            margin: 5px 0;
        }
        code {
            background-color: #f4f4f4;
            padding: 2px 5px;
            border-radius: 3px;
            font-family: "Consolas", "Monaco", monospace;
            font-size: 0.9em;
        }
        pre {
            background-color: #f4f4f4;
            padding: 15px;
            border-radius: 5px;
            overflow-x: auto;
            border-left: 4px solid #3498db;
        }
        pre code {
            background-color: transparent;
            padding: 0;
        }
        .note {
            background-color: #e8f4fd;
            border-left: 4px solid #3498db;
            padding: 12px 15px;
            margin: 15px 0;
            border-radius: 0 4px 4px 0;
        }
        .warning {
            background-color: #fff3cd;
            border-left: 4px solid #ffc107;
            padding: 12px 15px;
            margin: 15px 0;
            border-radius: 0 4px 4px 0;
        }
        .tip {
            background-color: #d4edda;
            border-left: 4px solid #28a745;
            padding: 12px 15px;
            margin: 15px 0;
            border-radius: 0 4px 4px 0;
        }
        a {
            color: #3498db;
            text-decoration: none;
        }
        a:hover {
            text-decoration: underline;
        }
        table {
            border-collapse: collapse;
            width: 100%;
            margin: 15px 0;
        }
        th, td {
            border: 1px solid #ddd;
            padding: 8px 12px;
            text-align: left;
        }
        th {
            background-color: #f5f5f5;
            font-weight: 600;
        }
        tr:nth-child(even) {
            background-color: #fafafa;
        }
        .search-result {
            padding: 15px;
            margin: 10px 0;
            border: 1px solid #ddd;
            border-radius: 4px;
            background-color: #fafafa;
        }
        .search-result h3 {
            margin-top: 0;
            margin-bottom: 8px;
        }
        .search-result p {
            margin: 0;
            color: #666;
        }
        .breadcrumb {
            color: #666;
            font-size: 0.9em;
            margin-bottom: 15px;
        }
    </style>
</head>
<body>
{{CONTENT}}
</body>
</html>)";
}

wxString HelpBrowser::GetTopicHtml(HelpTopicId id) const {
    const HelpTopic* topic = GetTopic(id);
    if (!topic) {
        return wxString::FromUTF8(GetBaseHtmlTemplate().replace(
            GetBaseHtmlTemplate().find("{{CONTENT}}"), 11,
            "<h1>Topic Not Found</h1><p>The requested help topic could not be found.</p>"));
    }
    
    return wxString::FromUTF8(GenerateTopicHtml(*topic));
}

std::string HelpBrowser::GenerateTopicHtml(const HelpTopic& topic) const {
    std::string html = GetBaseHtmlTemplate();
    
    std::string content;
    
    // Breadcrumb
    content += "<div class=\"breadcrumb\">";
    content += "<a href=\"#\">Help</a>";
    
    if (topic.parentId != HelpTopicId::None && topic.parentId != HelpTopicId::Home) {
        const HelpTopic* parent = GetTopic(topic.parentId);
        if (parent) {
            content += " &gt; " + parent->title;
        }
    }
    content += " &gt; " + topic.title;
    content += "</div>";
    
    // Title
    content += "<h1>" + topic.title + "</h1>";
    
    // Topic-specific content
    switch (topic.id) {
        case HelpTopicId::Home:
            content += R"(
                <p>Welcome to <strong>ScratchRobin</strong>, a powerful database management and development tool.</p>
                
                <h2>Getting Started</h2>
                <p>If you're new to ScratchRobin, check out these topics:</p>
                <ul>
                    <li><a href="#" onclick="return false;">Introduction</a> - Learn about ScratchRobin's features</li>
                    <li><a href="#" onclick="return false;">Quick Start Guide</a> - Get up and running quickly</li>
                    <li><a href="#" onclick="return false;">Connecting to a Database</a> - Set up your first connection</li>
                </ul>
                
                <h2>Main Features</h2>
                <ul>
                    <li><strong>SQL Editor</strong> - Write and execute SQL with syntax highlighting</li>
                    <li><strong>Visual Designer</strong> - Create ERD diagrams and generate SQL</li>
                    <li><strong>Database Administration</strong> - Manage users, backups, and jobs</li>
                    <li><strong>Multi-Backend Support</strong> - Works with ScratchBird, PostgreSQL, MySQL, and Firebird</li>
                </ul>
                
                <div class="tip">
                    <strong>Tip:</strong> Press F1 in any window to get context-sensitive help for that feature.
                </div>
                
                <h2>Keyboard Shortcuts</h2>
                <table>
                    <tr><th>Shortcut</th><th>Action</th></tr>
                    <tr><td>F1</td><td>Open Help</td></tr>
                    <tr><td>Ctrl+N</td><td>New SQL Editor</td></tr>
                    <tr><td>Ctrl+R</td><td>Run Query</td></tr>
                    <tr><td>Ctrl+S</td><td>Save</td></tr>
                </table>
            )";
            break;
            
        case HelpTopicId::Introduction:
            content += R"(
                <p>ScratchRobin is a comprehensive database management tool designed for developers 
                and database administrators. It provides an intuitive interface for working with 
                multiple database backends.</p>
                
                <h2>Key Features</h2>
                <ul>
                    <li>Modern, tabbed interface</li>
                    <li>Advanced SQL editor with auto-completion</li>
                    <li>Visual entity-relationship diagramming</li>
                    <li>Database administration tools</li>
                    <li>Job scheduling and automation</li>
                    <li>Performance monitoring</li>
                </ul>
                
                <h2>Supported Databases</h2>
                <ul>
                    <li>ScratchBird (Native)</li>
                    <li>PostgreSQL</li>
                    <li>MySQL / MariaDB</li>
                    <li>Firebird</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::QuickStartGuide:
            content += R"(
                <p>This guide will help you get started with ScratchRobin in just a few minutes.</p>
                
                <h2>1. Create a Connection</h2>
                <p>First, you need to set up a connection to your database:</p>
                <ol>
                    <li>Click <strong>Connections</strong> in the menu bar</li>
                    <li>Select <strong>Manage Connections</strong></li>
                    <li>Click <strong>New</strong> and fill in your database details</li>
                    <li>Click <strong>Test Connection</strong> to verify</li>
                    <li>Click <strong>OK</strong> to save</li>
                </ol>
                
                <h2>2. Open a SQL Editor</h2>
                <p>Once connected, open a SQL editor:</p>
                <ol>
                    <li>Select your connection from the dropdown</li>
                    <li>Click the <strong>Connect</strong> button</li>
                    <li>Click <strong>New SQL Editor</strong> or press Ctrl+N</li>
                </ol>
                
                <h2>3. Run Your First Query</h2>
                <p>Type a SQL query and click <strong>Run</strong> or press Ctrl+R:</p>
                <pre><code>SELECT * FROM your_table LIMIT 10;</code></pre>
                
                <div class="note">
                    <strong>Note:</strong> ScratchRobin supports multiple result sets and paginated results for large queries.
                </div>
            )";
            break;
            
        case HelpTopicId::ConnectingToDatabase:
            content += R"(
                <p>ScratchRobin supports connections to multiple database backends. Each connection 
                is stored as a profile that you can quickly select and connect to.</p>
                
                <h2>Connection Settings</h2>
                <table>
                    <tr><th>Setting</th><th>Description</th></tr>
                    <tr><td>Name</td><td>A friendly name for this connection</td></tr>
                    <tr><td>Backend</td><td>The database type (PostgreSQL, MySQL, etc.)</td></tr>
                    <tr><td>Host</td><td>Server hostname or IP address</td></tr>
                    <tr><td>Port</td><td>Database server port</td></tr>
                    <tr><td>Database</td><td>Database name or path</td></tr>
                    <tr><td>Username</td><td>Login username</td></tr>
                    <tr><td>Password</td><td>Login password (stored securely)</td></tr>
                </table>
                
                <h2>SSL/TLS Options</h2>
                <p>For secure connections, you can configure SSL/TLS in the connection settings:</p>
                <ul>
                    <li><strong>Prefer</strong> - Use SSL if available (default)</li>
                    <li><strong>Require</strong> - Always use SSL</li>
                    <li><strong>Disable</strong> - Never use SSL</li>
                    <li><strong>Verify CA</strong> - Verify server certificate</li>
                    <li><strong>Verify Full</strong> - Verify CA and hostname</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::SqlEditor:
            content += R"(
                <p>The SQL Editor is the primary interface for writing and executing SQL queries. 
                It provides syntax highlighting, auto-completion, and result visualization.</p>
                
                <h2>Features</h2>
                <ul>
                    <li>Syntax highlighting for SQL</li>
                    <li>Multiple result sets</li>
                    <li>Query history</li>
                    <li>Export to CSV and JSON</li>
                    <li>Transaction management</li>
                    <li>Query plan visualization</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::WritingQueries:
            content += R"(
                <p>Write SQL queries using standard syntax for your database backend.</p>
                
                <h2>Query Tips</h2>
                <ul>
                    <li>Use semicolons to separate multiple statements</li>
                    <li>Comments with <code>--</code> or <code>/* */</code> are supported</li>
                    <li>Parameter placeholders vary by backend</li>
                </ul>
                
                <h2>Example Queries</h2>
                <pre><code>-- Simple SELECT
SELECT id, name, email 
FROM users 
WHERE active = true 
ORDER BY name;</code></pre>
            )";
            break;
            
        case HelpTopicId::ExecutingSql:
            content += R"(
                <p>Execute SQL queries using the Run button or keyboard shortcut.</p>
                
                <h2>Execution Options</h2>
                <ul>
                    <li><strong>Run</strong> - Execute the current query or selection</li>
                    <li><strong>Paging</strong> - Return results in pages for large datasets</li>
                    <li><strong>Stream</strong> - Append results as they arrive</li>
                </ul>
                
                <h2>Transaction Control</h2>
                <p>Manage transactions with the Begin, Commit, and Rollback buttons. 
                Enable Auto-commit for automatic transaction handling.</p>
            )";
            break;
            
        case HelpTopicId::ResultGrid:
            content += R"(
                <p>Query results are displayed in a data grid with sorting and navigation features.</p>
                
                <h2>Grid Features</h2>
                <ul>
                    <li>Click column headers to sort</li>
                    <li>Drag columns to reorder</li>
                    <li>Right-click for context menu options</li>
                    <li>Copy cells or rows to clipboard</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::DatabaseObjects:
            content += R"(
                <p>Manage all database schema objects through the Database Objects section.</p>
                
                <h2>Object Types</h2>
                <ul>
                    <li><strong>Tables</strong> - Store your data</li>
                    <li><strong>Indexes</strong> - Optimize query performance</li>
                    <li><strong>Views</strong> - Virtual tables based on queries</li>
                    <li><strong>Triggers</strong> - Automated actions on data changes</li>
                    <li><strong>Procedures</strong> - Reusable SQL routines</li>
                    <li><strong>Sequences</strong> - Auto-incrementing number generators</li>
                    <li><strong>Domains</strong> - Custom data type definitions</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::Tables:
            content += R"(
                <p>Tables are the fundamental storage structure in relational databases.</p>
                
                <h2>Table Designer</h2>
                <p>Use the Table Designer to create and modify tables:</p>
                <ul>
                    <li>Add, remove, and reorder columns</li>
                    <li>Set data types and constraints</li>
                    <li>Define primary keys</li>
                    <li>Create foreign key relationships</li>
                    <li>Add indexes</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::Indexes:
            content += R"(
                <p>Indexes improve query performance by allowing faster data retrieval.</p>
                
                <h2>Index Types</h2>
                <ul>
                    <li><strong>B-Tree</strong> - Default index type for most queries</li>
                    <li><strong>Unique</strong> - Enforces uniqueness constraints</li>
                    <li><strong>Composite</strong> - Indexes multiple columns</li>
                    <li><strong>Partial</strong> - Indexes a subset of rows</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::ErdDiagramming:
            content += R"(
                <p>The ERD (Entity-Relationship Diagram) tool provides visual database design capabilities.</p>
                
                <h2>Features</h2>
                <ul>
                    <li>Drag-and-drop table design</li>
                    <li>Automatic relationship detection</li>
                    <li>Multiple notation styles (Crow's Foot, IE, UML)</li>
                    <li>Reverse engineering from existing databases</li>
                    <li>Forward engineering to generate SQL</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::CreatingDiagrams:
            content += R"(
                <p>Create new diagrams from scratch or from existing database objects.</p>
                
                <h2>Steps to Create a Diagram</h2>
                <ol>
                    <li>Click <strong>New Diagram</strong> or press the diagram button</li>
                    <li>Add tables by dragging from the catalog or creating new ones</li>
                    <li>Define relationships between tables</li>
                    <li>Arrange layout using auto-layout or manual positioning</li>
                    <li>Save the diagram for future editing</li>
                </ol>
            )";
            break;
            
        case HelpTopicId::ReverseEngineering:
            content += R"(
                <p>Reverse engineering creates a diagram from an existing database schema.</p>
                
                <h2>How to Reverse Engineer</h2>
                <ol>
                    <li>Connect to your database</li>
                    <li>Select <strong>Reverse Engineer</strong> from the Diagram menu</li>
                    <li>Choose the schemas and tables to include</li>
                    <li>Configure options (indexes, triggers, etc.)</li>
                    <li>Generate the diagram</li>
                </ol>
            )";
            break;
            
        case HelpTopicId::ForwardEngineering:
            content += R"(
                <p>Forward engineering generates SQL DDL from your diagram.</p>
                
                <h2>SQL Generation Options</h2>
                <ul>
                    <li>Generate CREATE TABLE statements</li>
                    <li>Include foreign key constraints</li>
                    <li>Add indexes and triggers</li>
                    <li>Preview before execution</li>
                    <li>Save to file or execute directly</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::Notations:
            content += R"(
                <p>ScratchRobin supports multiple diagram notation styles.</p>
                
                <h2>Supported Notations</h2>
                <ul>
                    <li><strong>Crow's Foot</strong> - Popular notation showing cardinality with crow's foot symbols</li>
                    <li><strong>Information Engineering (IE)</strong> - Uses crows foot with circle/bar notation</li>
                    <li><strong>UML</strong> - Standard UML class diagram notation</li>
                    <li><strong>Bachman</strong> - Simple arrow notation</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::Administration:
            content += R"(
                <p>Database administration tools help you manage and maintain your databases.</p>
                
                <h2>Administration Features</h2>
                <ul>
                    <li>Backup and Restore</li>
                    <li>User and Role Management</li>
                    <li>Job Scheduling</li>
                    <li>Storage Management</li>
                    <li>Performance Monitoring</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::BackupRestore:
            content += R"(
                <p>Protect your data with regular backups and restore when needed.</p>
                
                <h2>Backup Options</h2>
                <ul>
                    <li><strong>Full Backup</strong> - Complete database backup</li>
                    <li><strong>Incremental</strong> - Backup only changed data</li>
                    <li><strong>Scheduled</strong> - Automated backup jobs</li>
                </ul>
                
                <div class="warning">
                    <strong>Warning:</strong> Always test your backup restoration process periodically 
                    to ensure backups are valid.
                </div>
            )";
            break;
            
        case HelpTopicId::UserManagement:
            content += R"(
                <p>Manage database users, roles, and permissions.</p>
                
                <h2>User Management Tasks</h2>
                <ul>
                    <li>Create and delete users</li>
                    <li>Assign roles and permissions</li>
                    <li>Manage password policies</li>
                    <li>View user activity</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::JobScheduling:
            content += R"(
                <p>The Job Scheduler automates recurring database tasks.</p>
                
                <h2>Job Types</h2>
                <ul>
                    <li>SQL script execution</li>
                    <li>Backup operations</li>
                    <li>Data import/export</li>
                    <li>Maintenance tasks</li>
                </ul>
                
                <h2>Scheduling Options</h2>
                <ul>
                    <li>One-time execution</li>
                    <li>Recurring (daily, weekly, monthly)</li>
                    <li>Cron-style expressions</li>
                    <li>Event-based triggers</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::StorageManagement:
            content += R"(
                <p>Monitor and optimize database storage usage.</p>
                
                <h2>Storage Features</h2>
                <ul>
                    <li>Tablespace management</li>
                    <li>Storage usage statistics</li>
                    <li>Index size analysis</li>
                    <li>Bloat detection</li>
                    <li>Vacuum and maintenance</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::SqlReference:
            content += R"(
                <p>Reference documentation for SQL commands, data types, and functions.</p>
                
                <h2>Reference Sections</h2>
                <ul>
                    <li><strong>Data Types</strong> - Supported column types</li>
                    <li><strong>Functions</strong> - Built-in SQL functions</li>
                    <li><strong>Operators</strong> - Comparison and logical operators</li>
                </ul>
                
                <div class="note">
                    <strong>Note:</strong> SQL syntax may vary between database backends. 
                    This reference covers common standards; consult your backend documentation for specifics.
                </div>
            )";
            break;
            
        case HelpTopicId::DataTypes:
            content += R"(
                <p>Common SQL data types supported across database backends.</p>
                
                <h2>Numeric Types</h2>
                <table>
                    <tr><th>Type</th><th>Description</th></tr>
                    <tr><td>INTEGER</td><td>Whole numbers</td></tr>
                    <tr><td>BIGINT</td><td>Large whole numbers</td></tr>
                    <tr><td>DECIMAL(p,s)</td><td>Exact decimal numbers</td></tr>
                    <tr><td>NUMERIC(p,s)</td><td>Exact decimal numbers</td></tr>
                    <tr><td>REAL</td><td>Single-precision float</td></tr>
                    <tr><td>DOUBLE</td><td>Double-precision float</td></tr>
                </table>
                
                <h2>String Types</h2>
                <table>
                    <tr><th>Type</th><th>Description</th></tr>
                    <tr><td>CHAR(n)</td><td>Fixed-length character string</td></tr>
                    <tr><td>VARCHAR(n)</td><td>Variable-length character string</td></tr>
                    <tr><td>TEXT</td><td>Unlimited length text</td></tr>
                </table>
                
                <h2>Date/Time Types</h2>
                <table>
                    <tr><th>Type</th><th>Description</th></tr>
                    <tr><td>DATE</td><td>Calendar date</td></tr>
                    <tr><td>TIME</td><td>Time of day</td></tr>
                    <tr><td>TIMESTAMP</td><td>Date and time</td></tr>
                    <tr><td>INTERVAL</td><td>Time span</td></tr>
                </table>
            )";
            break;
            
        case HelpTopicId::Functions:
            content += R"(
                <p>Common SQL functions for data manipulation and analysis.</p>
                
                <h2>String Functions</h2>
                <ul>
                    <li><code>CONCAT(str1, str2)</code> - Concatenate strings</li>
                    <li><code>LENGTH(str)</code> - String length</li>
                    <li><code>SUBSTRING(str, start, len)</code> - Extract substring</li>
                    <li><code>UPPER(str)</code> / <code>LOWER(str)</code> - Case conversion</li>
                    <li><code>TRIM(str)</code> - Remove whitespace</li>
                </ul>
                
                <h2>Numeric Functions</h2>
                <ul>
                    <li><code>ABS(x)</code> - Absolute value</li>
                    <li><code>ROUND(x, d)</code> - Round to d decimal places</li>
                    <li><code>CEILING(x)</code> / <code>FLOOR(x)</code> - Round up/down</li>
                    <li><code>MOD(x, y)</code> - Modulo operation</li>
                </ul>
                
                <h2>Date Functions</h2>
                <ul>
                    <li><code>CURRENT_DATE</code> - Current date</li>
                    <li><code>CURRENT_TIME</code> - Current time</li>
                    <li><code>CURRENT_TIMESTAMP</code> - Current date and time</li>
                    <li><code>EXTRACT(field FROM date)</code> - Extract date part</li>
                </ul>
                
                <h2>Aggregate Functions</h2>
                <ul>
                    <li><code>COUNT(*)</code> - Count rows</li>
                    <li><code>SUM(column)</code> - Sum of values</li>
                    <li><code>AVG(column)</code> - Average value</li>
                    <li><code>MAX(column)</code> / <code>MIN(column)</code> - Extreme values</li>
                </ul>
            )";
            break;
            
        case HelpTopicId::Operators:
            content += R"(
                <p>SQL operators for comparisons, logic, and arithmetic.</p>
                
                <h2>Comparison Operators</h2>
                <table>
                    <tr><th>Operator</th><th>Description</th></tr>
                    <tr><td>=</td><td>Equal</td></tr>
                    <tr><td>&lt;&gt; or !=</td><td>Not equal</td></tr>
                    <tr><td>&lt;</td><td>Less than</td></tr>
                    <tr><td>&gt;</td><td>Greater than</td></tr>
                    <tr><td>&lt;=</td><td>Less than or equal</td></tr>
                    <tr><td>&gt;=</td><td>Greater than or equal</td></tr>
                    <tr><td>BETWEEN</td><td>Within range</td></tr>
                    <tr><td>IN</td><td>Match any in list</td></tr>
                    <tr><td>LIKE</td><td>Pattern match</td></tr>
                    <tr><td>IS NULL</td><td>Null check</td></tr>
                </table>
                
                <h2>Logical Operators</h2>
                <ul>
                    <li><code>AND</code> - Both conditions true</li>
                    <li><code>OR</code> - Either condition true</li>
                    <li><code>NOT</code> - Negate condition</li>
                </ul>
                
                <h2>Arithmetic Operators</h2>
                <ul>
                    <li><code>+</code> - Addition</li>
                    <li><code>-</code> - Subtraction</li>
                    <li><code>*</code> - Multiplication</li>
                    <li><code>/</code> - Division</li>
                    <li><code>%</code> - Modulo</li>
                </ul>
            )";
            break;
            
        default:
            // Generic topic content
            content += "<p>" + topic.description + "</p>";
            content += "<p>Detailed documentation for this topic is being prepared. ";
            content += "Please check back in a future update.</p>";
            break;
    }
    
    // Replace placeholder
    size_t pos = html.find("{{CONTENT}}");
    if (pos != std::string::npos) {
        html.replace(pos, 11, content);
    }
    
    return html;
}

std::string HelpBrowser::GenerateSearchResultsHtml(const std::vector<HelpTopicId>& results, 
                                                    const std::string& query) const {
    std::string html = GetBaseHtmlTemplate();
    
    std::string content;
    content += "<div class=\"breadcrumb\"><a href=\"#\">Help</a> &gt; Search Results</div>";
    content += "<h1>Search Results</h1>";
    content += "<p>Search query: <strong>" + query + "</strong></p>";
    
    if (results.empty()) {
        content += R"(
            <div class="note">
                <strong>No results found.</strong><br>
                Try different keywords or check your spelling.
            </div>
        )";
    } else {
        content += "<p>Found " + std::to_string(results.size()) + " result(s):</p>";
        
        for (HelpTopicId id : results) {
            const HelpTopic* topic = GetTopic(id);
            if (!topic) continue;
            
            content += "<div class=\"search-result\">";
            content += "<h3>" + topic->title + "</h3>";
            content += "<p>" + topic->description + "</p>";
            content += "</div>";
        }
    }
    
    content += R"(
        <div style="margin-top: 30px; padding-top: 15px; border-top: 1px solid #ddd;">
            <p><strong>Search Tips:</strong></p>
            <ul>
                <li>Use specific keywords related to what you're looking for</li>
                <li>Try searching for feature names or menu items</li>
                <li>Check the topic tree on the left for related topics</li>
            </ul>
        </div>
    )";
    
    size_t pos = html.find("{{CONTENT}}");
    if (pos != std::string::npos) {
        html.replace(pos, 11, content);
    }
    
    return html;
}

// =============================================================================
// HelpTopicMapper Implementation
// =============================================================================

HelpTopicId HelpTopicMapper::MapWindowClass(const std::string& className) {
    std::string lower = ToLower(className);
    
    if (lower.find("sqleditor") != std::string::npos || 
        lower.find("sql_editor") != std::string::npos) {
        return HelpTopicId::SqlEditor;
    }
    if (lower.find("catalog") != std::string::npos) {
        return HelpTopicId::CatalogBrowser;
    }
    if (lower.find("diagram") != std::string::npos || 
        lower.find("erd") != std::string::npos) {
        return HelpTopicId::ErdDiagramming;
    }
    if (lower.find("monitor") != std::string::npos) {
        return HelpTopicId::Monitoring;
    }
    if (lower.find("user") != std::string::npos || 
        lower.find("role") != std::string::npos) {
        return HelpTopicId::UsersRoles;
    }
    if (lower.find("job") != std::string::npos || 
        lower.find("scheduler") != std::string::npos) {
        return HelpTopicId::JobScheduling;
    }
    if (lower.find("backup") != std::string::npos || 
        lower.find("restore") != std::string::npos) {
        return HelpTopicId::BackupRestore;
    }
    if (lower.find("table") != std::string::npos) {
        return HelpTopicId::Tables;
    }
    if (lower.find("index") != std::string::npos) {
        return HelpTopicId::Indexes;
    }
    if (lower.find("view") != std::string::npos) {
        return HelpTopicId::Views;
    }
    if (lower.find("trigger") != std::string::npos) {
        return HelpTopicId::Triggers;
    }
    if (lower.find("procedure") != std::string::npos || 
        lower.find("routine") != std::string::npos) {
        return HelpTopicId::Procedures;
    }
    if (lower.find("sequence") != std::string::npos) {
        return HelpTopicId::Sequences;
    }
    if (lower.find("domain") != std::string::npos) {
        return HelpTopicId::Domains;
    }
    if (lower.find("preference") != std::string::npos || 
        lower.find("setting") != std::string::npos) {
        return HelpTopicId::Preferences;
    }
    if (lower.find("connection") != std::string::npos) {
        return HelpTopicId::ConnectingToDatabase;
    }
    
    return HelpTopicId::Home;
}

std::string HelpTopicMapper::GetHelpTopicTitle(HelpTopicId id) {
    switch (id) {
        case HelpTopicId::Home: return "ScratchRobin Help";
        case HelpTopicId::GettingStarted: return "Getting Started";
        case HelpTopicId::Introduction: return "Introduction";
        case HelpTopicId::QuickStartGuide: return "Quick Start Guide";
        case HelpTopicId::ConnectingToDatabase: return "Connecting to a Database";
        case HelpTopicId::SqlEditor: return "SQL Editor";
        case HelpTopicId::WritingQueries: return "Writing Queries";
        case HelpTopicId::ExecutingSql: return "Executing SQL";
        case HelpTopicId::ResultGrid: return "Result Grid";
        case HelpTopicId::DatabaseObjects: return "Database Objects";
        case HelpTopicId::Tables: return "Tables";
        case HelpTopicId::Indexes: return "Indexes";
        case HelpTopicId::Views: return "Views";
        case HelpTopicId::Triggers: return "Triggers";
        case HelpTopicId::Procedures: return "Procedures";
        case HelpTopicId::Sequences: return "Sequences";
        case HelpTopicId::Domains: return "Domains";
        case HelpTopicId::ErdDiagramming: return "ERD / Diagramming";
        case HelpTopicId::CreatingDiagrams: return "Creating Diagrams";
        case HelpTopicId::ReverseEngineering: return "Reverse Engineering";
        case HelpTopicId::ForwardEngineering: return "Forward Engineering";
        case HelpTopicId::Notations: return "Notations";
        case HelpTopicId::Administration: return "Administration";
        case HelpTopicId::BackupRestore: return "Backup and Restore";
        case HelpTopicId::UserManagement: return "User Management";
        case HelpTopicId::JobScheduling: return "Job Scheduling";
        case HelpTopicId::StorageManagement: return "Storage Management";
        case HelpTopicId::SqlReference: return "SQL Reference";
        case HelpTopicId::DataTypes: return "Data Types";
        case HelpTopicId::Functions: return "Functions";
        case HelpTopicId::Operators: return "Operators";
        case HelpTopicId::CatalogBrowser: return "Catalog Browser";
        case HelpTopicId::Monitoring: return "Monitoring";
        case HelpTopicId::UsersRoles: return "Users and Roles";
        case HelpTopicId::Diagram: return "Diagram";
        case HelpTopicId::JobScheduler: return "Job Scheduler";
        case HelpTopicId::Preferences: return "Preferences";
        default: return "Help";
    }
}

} // namespace scratchrobin
