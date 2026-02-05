/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_HELP_BROWSER_H
#define SCRATCHROBIN_HELP_BROWSER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <wx/frame.h>
#include <wx/treectrl.h>

class wxButton;
class wxHtmlWindow;
class wxSearchCtrl;
class wxSplitterWindow;
class wxTextCtrl;
class wxToolBar;
class wxTreeCtrl;
class wxTreeEvent;

namespace scratchrobin {

// Help topic identifiers for context-sensitive help
enum class HelpTopicId {
    // Getting Started
    GettingStarted,
    Introduction,
    QuickStartGuide,
    ConnectingToDatabase,
    
    // SQL Editor
    SqlEditor,
    WritingQueries,
    ExecutingSql,
    ResultGrid,
    
    // Database Objects
    DatabaseObjects,
    Tables,
    Indexes,
    Views,
    Triggers,
    Procedures,
    Sequences,
    Domains,
    
    // ERD / Diagramming
    ErdDiagramming,
    CreatingDiagrams,
    ReverseEngineering,
    ForwardEngineering,
    Notations,
    
    // Administration
    Administration,
    BackupRestore,
    UserManagement,
    JobScheduling,
    StorageManagement,
    
    // SQL Reference
    SqlReference,
    DataTypes,
    Functions,
    Operators,
    
    // Window-specific topics
    CatalogBrowser,
    Monitoring,
    UsersRoles,
    Diagram,
    JobScheduler,
    ProjectWorkspace,
    ServerManager,
    ClusterManager,
    DatabaseManager,
    GitIntegration,
    Preferences,
    ActivityLog,
    StartupBranding,
    
    // Special
    Home,
    SearchResults,
    None
};

// Help topic data structure
struct HelpTopic {
    HelpTopicId id;
    std::string title;
    std::string description;
    std::string htmlContent;
    HelpTopicId parentId;
    std::vector<std::string> keywords;
    std::vector<std::string> tags;
};

// Navigation history entry
struct HistoryEntry {
    HelpTopicId topicId;
    std::string topicTitle;
};

class HelpBrowser : public wxFrame {
public:
    // Show the help browser (creates if needed, brings to front if exists)
    static void ShowBrowser(wxWindow* parent = nullptr);
    
    // Show help for a specific topic
    static void ShowHelp(HelpTopicId topicId);
    static void ShowHelpForWindow(const std::string& windowClassName);
    
    // Check if browser is open
    static bool IsOpen();
    
    // Close the browser
    static void CloseBrowser();

private:
    // Singleton instance
    static HelpBrowser* instance_;
    
    HelpBrowser(wxWindow* parent);
    ~HelpBrowser();
    
    // Disable copy/move
    HelpBrowser(const HelpBrowser&) = delete;
    HelpBrowser& operator=(const HelpBrowser&) = delete;

    void BuildLayout();
    void BuildToolbar();
    void BuildTopicTree();
    void PopulateHelpContent();
    
    // Topic management
    void LoadTopic(HelpTopicId id);
    void NavigateToTopic(HelpTopicId id);
    void NavigateBack();
    void NavigateForward();
    void NavigateHome();
    bool CanGoBack() const;
    bool CanGoForward() const;
    
    // Search
    void OnSearch(wxCommandEvent& event);
    void PerformSearch(const std::string& query);
    void ShowSearchResults(const std::vector<HelpTopicId>& results, const std::string& query);
    std::vector<HelpTopicId> SearchTopics(const std::string& query);
    std::string HighlightMatches(const std::string& html, const std::string& query);
    
    // Tree navigation
    void OnTreeSelection(wxTreeEvent& event);
    wxTreeItemId FindTreeItemByTopicId(wxTreeItemId parent, HelpTopicId id);
    void SelectTreeItemByTopicId(HelpTopicId id);
    
    // Toolbar actions
    void OnBack(wxCommandEvent& event);
    void OnForward(wxCommandEvent& event);
    void OnHome(wxCommandEvent& event);
    void OnFind(wxCommandEvent& event);
    
    // Window events
    void OnClose(wxCloseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    
    // Topic data access
    const HelpTopic* GetTopic(HelpTopicId id) const;
    HelpTopicId GetTopicIdFromTreeItem(wxTreeItemId item) const;
    wxString GetTopicHtml(HelpTopicId id) const;
    
    // Content generation
    std::string GenerateTopicHtml(const HelpTopic& topic) const;
    std::string GetBaseHtmlTemplate() const;
    std::string GenerateSearchResultsHtml(const std::vector<HelpTopicId>& results, 
                                          const std::string& query) const;

    // UI components
    wxToolBar* toolbar_ = nullptr;
    wxSplitterWindow* splitter_ = nullptr;
    wxTreeCtrl* topic_tree_ = nullptr;
    wxHtmlWindow* content_view_ = nullptr;
    wxSearchCtrl* search_ctrl_ = nullptr;
    
    // Topic data storage
    std::map<HelpTopicId, std::unique_ptr<HelpTopic>> topics_;
    std::map<wxTreeItemId, HelpTopicId, std::less<wxTreeItemId>> tree_item_to_topic_;
    
    // Navigation history
    std::vector<HistoryEntry> history_;
    size_t history_position_ = 0;
    static constexpr size_t kMaxHistorySize = 50;
    
    // Current state
    HelpTopicId current_topic_ = HelpTopicId::None;
    bool navigating_history_ = false;
    wxString find_text_;  // Last search text for find-in-page
    
    wxDECLARE_EVENT_TABLE();
};

// Help topic ID mapping for window classes
class HelpTopicMapper {
public:
    static HelpTopicId MapWindowClass(const std::string& className);
    static std::string GetHelpTopicTitle(HelpTopicId id);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_HELP_BROWSER_H
