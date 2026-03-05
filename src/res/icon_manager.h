/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <wx/bitmap.h>
#include <wx/bmpbndl.h>
#include <wx/string.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace scratchrobin::res {

// Icon information structure
struct IconInfo {
    std::string id;                    // Unique identifier (e.g., "document-new")
    std::string display_name;          // Human readable name
    std::string category;              // Category (actions, database, navigation, etc.)
    std::vector<std::string> tags;     // Search tags
    wxBitmapBundle bitmap;             // The icon bitmap bundle
    const char* svg_data{nullptr};     // Raw SVG data if available
    bool is_builtin{true};             // Built-in vs custom
    std::string file_path;             // Path to SVG file for custom icons
};

// Icon manager - centralized icon registry and loading
class IconManager {
public:
    static IconManager* get();
    
    // Initialize and load all icons
    void initialize();
    
    // Icon registration
    void registerIcon(const IconInfo& info);
    void registerBuiltinIcons();
    
    // Icon retrieval
    IconInfo* getIcon(const std::string& id);
    wxBitmapBundle getBitmapBundle(const std::string& id, const wxSize& size = wxSize(16, 16));
    wxBitmap getBitmap(const std::string& id, const wxSize& size = wxSize(16, 16));
    
    // Search icons
    std::vector<IconInfo*> searchIcons(const std::string& query);
    std::vector<IconInfo*> getIconsByCategory(const std::string& category);
    std::vector<IconInfo*> getAllIcons();
    std::vector<std::string> getCategories();
    
    // Custom icon management
    bool loadCustomIcon(const std::string& file_path, const std::string& id, 
                        const std::string& display_name);
    void unloadCustomIcon(const std::string& id);
    bool reloadIcon(const std::string& id);
    
    // Icon modification (for toolbar customization)
    bool updateIconBitmap(const std::string& id, const wxBitmapBundle& bitmap);
    
    // Persistence
    void saveCustomIconsToConfig();
    void loadCustomIconsFromConfig();
    
    // Get SVG data for an icon
    const char* getSVGData(const std::string& id);
    
    // Check if icon exists
    bool hasIcon(const std::string& id) const;

private:
    IconManager() = default;
    
    std::map<std::string, IconInfo> icons_;
    std::map<std::string, wxBitmapBundle> cache_;  // Size-specific cache
    
    void registerActionIcons();
    void registerDatabaseIcons();
    void registerNavigationIcons();
    void registerToolbarIcons();
    
    wxBitmapBundle loadSVGFromData(const char* svg_data);
    wxBitmapBundle loadSVGFromFile(const std::string& file_path);
};

// Helper to convert between wxArtID and our icon IDs
wxString IconIdToArtId(const std::string& icon_id);
std::string ArtIdToIconId(const wxString& art_id);

// Common icon IDs (for use in toolbar editor)
namespace IconIds {
    // File operations
    constexpr const char* DOCUMENT_NEW = "document-new";
    constexpr const char* DOCUMENT_OPEN = "document-open";
    constexpr const char* DOCUMENT_SAVE = "document-save";
    constexpr const char* DOCUMENT_SAVE_AS = "document-save-as";
    constexpr const char* DOCUMENT_IMPORT = "document-import";
    constexpr const char* DOCUMENT_EXPORT = "document-export";
    
    // Edit operations
    constexpr const char* EDIT_UNDO = "edit-undo";
    constexpr const char* EDIT_REDO = "edit-redo";
    constexpr const char* EDIT_CUT = "edit-cut";
    constexpr const char* EDIT_COPY = "edit-copy";
    constexpr const char* EDIT_PASTE = "edit-paste";
    constexpr const char* EDIT_FIND = "edit-find";
    constexpr const char* EDIT_FIND_REPLACE = "edit-find-replace";
    
    // View operations
    constexpr const char* VIEW_REFRESH = "view-refresh";
    constexpr const char* VIEW_ZOOM_IN = "view-zoom-in";
    constexpr const char* VIEW_ZOOM_OUT = "view-zoom-out";
    constexpr const char* VIEW_FULLSCREEN = "view-fullscreen";
    
    // Database operations
    constexpr const char* DB_CONNECT = "db-connect";
    constexpr const char* DB_DISCONNECT = "db-disconnect";
    constexpr const char* DB_NEW = "db-new";
    constexpr const char* DB_BACKUP = "db-backup";
    constexpr const char* DB_RESTORE = "db-restore";
    constexpr const char* TABLE_NEW = "table-new";
    constexpr const char* TABLE_EDIT = "table-edit";
    constexpr const char* TABLE_DELETE = "table-delete";
    constexpr const char* INDEX_NEW = "index-new";
    constexpr const char* VIEW_NEW = "view-new";
    constexpr const char* PROCEDURE_NEW = "procedure-new";
    constexpr const char* FUNCTION_NEW = "function-new";
    constexpr const char* TRIGGER_NEW = "trigger-new";
    
    // Transaction operations
    constexpr const char* TX_START = "transaction-start";
    constexpr const char* TX_COMMIT = "transaction-commit";
    constexpr const char* TX_ROLLBACK = "transaction-rollback";
    constexpr const char* TX_AUTOCOMMIT = "transaction-autocommit";
    
    // Query operations
    constexpr const char* QUERY_EXECUTE = "query-execute";
    constexpr const char* QUERY_EXECUTE_SELECTION = "query-execute-selection";
    constexpr const char* QUERY_EXPLAIN = "query-explain";
    constexpr const char* QUERY_STOP = "query-stop";
    constexpr const char* QUERY_FORMAT = "query-format";
    
    // Navigation
    constexpr const char* NAV_BACK = "nav-back";
    constexpr const char* NAV_FORWARD = "nav-forward";
    constexpr const char* NAV_UP = "nav-up";
    constexpr const char* NAV_HOME = "nav-home";
    constexpr const char* NAV_REFRESH = "nav-refresh";
    
    // Window operations
    constexpr const char* WIN_NEW = "window-new";
    constexpr const char* WIN_CLOSE = "window-close";
    constexpr const char* WIN_PIN = "window-pin";
    constexpr const char* WIN_UNPIN = "window-unpin";
    
    // Tools
    constexpr const char* TOOLS_PREFERENCES = "tools-preferences";
    constexpr const char* TOOLS_MONITOR = "tools-monitor";
    constexpr const char* TOOLS_USERS = "tools-users";
    constexpr const char* TOOLS_GENERATE_DDL = "tools-generate-ddl";
    
    // Help
    constexpr const char* HELP_CONTENTS = "help-contents";
    constexpr const char* HELP_CONTEXT = "help-context";
    constexpr const char* HELP_ABOUT = "help-about";
    
    // Miscellaneous
    constexpr const char* SEPARATOR = "separator";
    constexpr const char* EXPANDER = "expander";
    constexpr const char* COLLAPSER = "collapser";
    constexpr const char* SETTINGS = "settings";
    constexpr const char* FILTER = "filter";
    constexpr const char* SORT_ASC = "sort-ascending";
    constexpr const char* SORT_DESC = "sort-descending";
}

}  // namespace scratchrobin::res
