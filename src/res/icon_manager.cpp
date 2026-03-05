/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "res/icon_manager.h"
#include "res/default_icons.h"
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <algorithm>
#include <cctype>

namespace scratchrobin::res {

IconManager* IconManager::get() {
    static IconManager instance;
    return &instance;
}

void IconManager::initialize() {
    registerBuiltinIcons();
    loadCustomIconsFromConfig();
}

void IconManager::registerBuiltinIcons() {
    registerActionIcons();
    registerDatabaseIcons();
    registerNavigationIcons();
    registerToolbarIcons();
}

void IconManager::registerActionIcons() {
    // Map icon IDs to available SVG constants
    // File operations - use generic icons as placeholders
    registerIcon({IconIds::DOCUMENT_NEW, "New Document", "file", {"new", "create", "file"}, 
                  loadSVGFromData(DEFAULT_OBJECT_SVG), DEFAULT_OBJECT_SVG, true});
    registerIcon({IconIds::DOCUMENT_OPEN, "Open Document", "file", {"open", "load", "file"}, 
                  loadSVGFromData(FOLDER_OPEN_SVG), FOLDER_OPEN_SVG, true});
    registerIcon({IconIds::DOCUMENT_SAVE, "Save", "file", {"save", "write", "file"}, 
                  loadSVGFromData(DATABASE_SVG), DATABASE_SVG, true});
    registerIcon({IconIds::DOCUMENT_SAVE_AS, "Save As", "file", {"save", "as", "file"}, 
                  loadSVGFromData(DATABASE_SVG), DATABASE_SVG, true});
    registerIcon({IconIds::DOCUMENT_IMPORT, "Import", "file", {"import", "load"}, 
                  loadSVGFromData(INPUT_SVG), INPUT_SVG, true});
    registerIcon({IconIds::DOCUMENT_EXPORT, "Export", "file", {"export", "save"}, 
                  loadSVGFromData(OUTPUT_SVG), OUTPUT_SVG, true});
    
    // Edit operations
    registerIcon({IconIds::EDIT_UNDO, "Undo", "edit", {"undo", "revert"}, 
                  loadSVGFromData(DEFAULT_OBJECT_SVG), DEFAULT_OBJECT_SVG, true});
    registerIcon({IconIds::EDIT_REDO, "Redo", "edit", {"redo", "restore"}, 
                  loadSVGFromData(DEFAULT_OBJECT_SVG), DEFAULT_OBJECT_SVG, true});
    registerIcon({IconIds::EDIT_CUT, "Cut", "edit", {"cut", "clipboard"}, 
                  loadSVGFromData(COLUMN_SVG), COLUMN_SVG, true});
    registerIcon({IconIds::EDIT_COPY, "Copy", "edit", {"copy", "duplicate", "clipboard"}, 
                  loadSVGFromData(COLUMN_SVG), COLUMN_SVG, true});
    registerIcon({IconIds::EDIT_PASTE, "Paste", "edit", {"paste", "clipboard"}, 
                  loadSVGFromData(COLUMN_SVG), COLUMN_SVG, true});
    registerIcon({IconIds::EDIT_FIND, "Find", "edit", {"find", "search"}, 
                  loadSVGFromData(VIEW_SVG), VIEW_SVG, true});
    registerIcon({IconIds::EDIT_FIND_REPLACE, "Find and Replace", "edit", {"find", "replace", "search"}, 
                  loadSVGFromData(VIEW_SVG), VIEW_SVG, true});
    
    // View operations
    registerIcon({IconIds::VIEW_REFRESH, "Refresh", "view", {"refresh", "reload", "update"}, 
                  loadSVGFromData(TRIGGER_SVG), TRIGGER_SVG, true});
    registerIcon({IconIds::VIEW_ZOOM_IN, "Zoom In", "view", {"zoom", "in", "magnify"}, 
                  loadSVGFromData(INDEX_SVG), INDEX_SVG, true});
    registerIcon({IconIds::VIEW_ZOOM_OUT, "Zoom Out", "view", {"zoom", "out", "magnify"}, 
                  loadSVGFromData(INDEX_SVG), INDEX_SVG, true});
    registerIcon({IconIds::VIEW_FULLSCREEN, "Fullscreen", "view", {"fullscreen", "maximize"}, 
                  loadSVGFromData(TABLE_SVG), TABLE_SVG, true});
}

void IconManager::registerDatabaseIcons() {
    // Database operations
    registerIcon({IconIds::DB_CONNECT, "Connect", "database", {"connect", "database", "network"}, 
                  loadSVGFromData(ROOT_SVG), ROOT_SVG, true});
    registerIcon({IconIds::DB_DISCONNECT, "Disconnect", "database", {"disconnect", "database", "network"}, 
                  loadSVGFromData(DEFAULT_OBJECT_SVG), DEFAULT_OBJECT_SVG, true});
    registerIcon({IconIds::DB_NEW, "New Database", "database", {"new", "database", "create"}, 
                  loadSVGFromData(DATABASE_SVG), DATABASE_SVG, true});
    registerIcon({IconIds::DB_BACKUP, "Backup Database", "database", {"backup", "database", "save"}, 
                  loadSVGFromData(DATABASE_SVG), DATABASE_SVG, true});
    registerIcon({IconIds::DB_RESTORE, "Restore Database", "database", {"restore", "database", "load"}, 
                  loadSVGFromData(DATABASE_SVG), DATABASE_SVG, true});
    
    // Table operations
    registerIcon({IconIds::TABLE_NEW, "New Table", "database", {"table", "new", "create"}, 
                  loadSVGFromData(TABLE_SVG), TABLE_SVG, true});
    registerIcon({IconIds::TABLE_EDIT, "Edit Table", "database", {"table", "edit", "modify"}, 
                  loadSVGFromData(TABLE_SVG), TABLE_SVG, true});
    registerIcon({IconIds::TABLE_DELETE, "Delete Table", "database", {"table", "delete", "remove"}, 
                  loadSVGFromData(TABLE_SVG), TABLE_SVG, true});
    
    // Other database objects
    registerIcon({IconIds::INDEX_NEW, "New Index", "database", {"index", "new", "create"}, 
                  loadSVGFromData(INDEX_SVG), INDEX_SVG, true});
    registerIcon({IconIds::VIEW_NEW, "New View", "database", {"view", "new", "create"}, 
                  loadSVGFromData(VIEW_SVG), VIEW_SVG, true});
    registerIcon({IconIds::PROCEDURE_NEW, "New Procedure", "database", {"procedure", "new", "create"}, 
                  loadSVGFromData(PROCEDURE_SVG), PROCEDURE_SVG, true});
    registerIcon({IconIds::FUNCTION_NEW, "New Function", "database", {"function", "new", "create"}, 
                  loadSVGFromData(FUNCTION_SVG), FUNCTION_SVG, true});
    registerIcon({IconIds::TRIGGER_NEW, "New Trigger", "database", {"trigger", "new", "create"}, 
                  loadSVGFromData(TRIGGER_SVG), TRIGGER_SVG, true});
    
    // Transaction operations
    registerIcon({IconIds::TX_START, "Start Transaction", "transaction", {"transaction", "start", "begin"}, 
                  loadSVGFromData(DML_TRIGGER_SVG), DML_TRIGGER_SVG, true});
    registerIcon({IconIds::TX_COMMIT, "Commit Transaction", "transaction", {"transaction", "commit", "save"}, 
                  loadSVGFromData(DB_TRIGGER_SVG), DB_TRIGGER_SVG, true});
    registerIcon({IconIds::TX_ROLLBACK, "Rollback Transaction", "transaction", {"transaction", "rollback", "undo"}, 
                  loadSVGFromData(DDL_TRIGGER_SVG), DDL_TRIGGER_SVG, true});
    registerIcon({IconIds::TX_AUTOCOMMIT, "Auto Commit", "transaction", {"transaction", "autocommit", "auto"}, 
                  loadSVGFromData(GTT_SVG), GTT_SVG, true});
    
    // Query operations
    registerIcon({IconIds::QUERY_EXECUTE, "Execute Query", "query", {"query", "execute", "run", "sql"}, 
                  loadSVGFromData(PROCEDURE_SVG), PROCEDURE_SVG, true});
    registerIcon({IconIds::QUERY_EXECUTE_SELECTION, "Execute Selection", "query", {"query", "execute", "selection", "sql"}, 
                  loadSVGFromData(FUNCTION_SVG), FUNCTION_SVG, true});
    registerIcon({IconIds::QUERY_EXPLAIN, "Explain Query", "query", {"query", "explain", "plan"}, 
                  loadSVGFromData(VIEW_SVG), VIEW_SVG, true});
    registerIcon({IconIds::QUERY_STOP, "Stop Query", "query", {"query", "stop", "cancel"}, 
                  loadSVGFromData(EXCEPTION_SVG), EXCEPTION_SVG, true});
    registerIcon({IconIds::QUERY_FORMAT, "Format SQL", "query", {"query", "format", "sql", "beautify"}, 
                  loadSVGFromData(CHARSET_SVG), CHARSET_SVG, true});
}

void IconManager::registerNavigationIcons() {
    registerIcon({IconIds::NAV_BACK, "Back", "navigation", {"back", "previous", "navigate"}, 
                  loadSVGFromData(INPUT_SVG), INPUT_SVG, true});
    registerIcon({IconIds::NAV_FORWARD, "Forward", "navigation", {"forward", "next", "navigate"}, 
                  loadSVGFromData(OUTPUT_SVG), OUTPUT_SVG, true});
    registerIcon({IconIds::NAV_UP, "Up", "navigation", {"up", "parent", "navigate"}, 
                  loadSVGFromData(INDEX_SVG), INDEX_SVG, true});
    registerIcon({IconIds::NAV_HOME, "Home", "navigation", {"home", "root", "navigate"}, 
                  loadSVGFromData(ROOT_SVG), ROOT_SVG, true});
    registerIcon({IconIds::NAV_REFRESH, "Refresh", "navigation", {"refresh", "reload"}, 
                  loadSVGFromData(TRIGGER_SVG), TRIGGER_SVG, true});
}

void IconManager::registerToolbarIcons() {
    registerIcon({IconIds::WIN_NEW, "New Window", "window", {"window", "new"}, 
                  loadSVGFromData(PACKAGE_SVG), PACKAGE_SVG, true});
    registerIcon({IconIds::WIN_CLOSE, "Close Window", "window", {"window", "close"}, 
                  loadSVGFromData(EXCEPTION_SVG), EXCEPTION_SVG, true});
    registerIcon({IconIds::WIN_PIN, "Pin Window", "window", {"window", "pin"}, 
                  loadSVGFromData(COLUMN_PK_SVG), COLUMN_PK_SVG, true});
    registerIcon({IconIds::WIN_UNPIN, "Unpin Window", "window", {"window", "unpin"}, 
                  loadSVGFromData(COLUMN_FK_SVG), COLUMN_FK_SVG, true});
    
    registerIcon({IconIds::TOOLS_PREFERENCES, "Preferences", "tools", {"tools", "preferences", "settings"}, 
                  loadSVGFromData(DOMAIN_SVG), DOMAIN_SVG, true});
    registerIcon({IconIds::TOOLS_MONITOR, "Monitor", "tools", {"tools", "monitor", "performance"}, 
                  loadSVGFromData(GENERATOR_SVG), GENERATOR_SVG, true});
    registerIcon({IconIds::TOOLS_USERS, "Users", "tools", {"tools", "users", "security"}, 
                  loadSVGFromData(USER_SVG), USER_SVG, true});
    registerIcon({IconIds::TOOLS_GENERATE_DDL, "Generate DDL", "tools", {"tools", "ddl", "generate", "code"}, 
                  loadSVGFromData(FUNCTION_SVG), FUNCTION_SVG, true});
    
    registerIcon({IconIds::HELP_CONTENTS, "Help Contents", "help", {"help", "contents", "documentation"}, 
                  loadSVGFromData(CHARSET_SVG), CHARSET_SVG, true});
    registerIcon({IconIds::HELP_CONTEXT, "Context Help", "help", {"help", "context"}, 
                  loadSVGFromData(CHARSET_SVG), CHARSET_SVG, true});
    registerIcon({IconIds::HELP_ABOUT, "About", "help", {"help", "about"}, 
                  loadSVGFromData(USER_SVG), USER_SVG, true});
    
    registerIcon({IconIds::SEPARATOR, "Separator", "misc", {"separator", "line"}, 
                  wxBitmapBundle(), nullptr, true});
    registerIcon({IconIds::EXPANDER, "Expand", "misc", {"expand", "arrow", "down"}, 
                  loadSVGFromData(FOLDER_OPEN_SVG), FOLDER_OPEN_SVG, true});
    registerIcon({IconIds::COLLAPSER, "Collapse", "misc", {"collapse", "arrow", "up"}, 
                  loadSVGFromData(FOLDER_SVG), FOLDER_SVG, true});
    registerIcon({IconIds::SETTINGS, "Settings", "misc", {"settings", "preferences", "options"}, 
                  loadSVGFromData(DOMAIN_SVG), DOMAIN_SVG, true});
    registerIcon({IconIds::FILTER, "Filter", "misc", {"filter", "search"}, 
                  loadSVGFromData(VIEW_SVG), VIEW_SVG, true});
    registerIcon({IconIds::SORT_ASC, "Sort Ascending", "misc", {"sort", "ascending", "order"}, 
                  loadSVGFromData(INDEX_SVG), INDEX_SVG, true});
    registerIcon({IconIds::SORT_DESC, "Sort Descending", "misc", {"sort", "descending", "order"}, 
                  loadSVGFromData(COLLATION_SVG), COLLATION_SVG, true});
}

void IconManager::registerIcon(const IconInfo& info) {
    icons_[info.id] = info;
}

IconInfo* IconManager::getIcon(const std::string& id) {
    auto it = icons_.find(id);
    if (it != icons_.end()) {
        return &it->second;
    }
    return nullptr;
}

wxBitmapBundle IconManager::getBitmapBundle(const std::string& id, const wxSize& size) {
    IconInfo* info = getIcon(id);
    if (info && info->bitmap.IsOk()) {
        return info->bitmap;
    }
    return wxBitmapBundle();
}

wxBitmap IconManager::getBitmap(const std::string& id, const wxSize& size) {
    wxBitmapBundle bundle = getBitmapBundle(id, size);
    if (bundle.IsOk()) {
        return bundle.GetBitmap(size);
    }
    return wxBitmap();
}

std::vector<IconInfo*> IconManager::searchIcons(const std::string& query) {
    std::vector<IconInfo*> result;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (auto& [id, info] : icons_) {
        // Search in ID
        std::string lower_id = id;
        std::transform(lower_id.begin(), lower_id.end(), lower_id.begin(), ::tolower);
        if (lower_id.find(lower_query) != std::string::npos) {
            result.push_back(&info);
            continue;
        }
        
        // Search in display name
        std::string lower_name = info.display_name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find(lower_query) != std::string::npos) {
            result.push_back(&info);
            continue;
        }
        
        // Search in tags
        for (const auto& tag : info.tags) {
            std::string lower_tag = tag;
            std::transform(lower_tag.begin(), lower_tag.end(), lower_tag.begin(), ::tolower);
            if (lower_tag.find(lower_query) != std::string::npos) {
                result.push_back(&info);
                break;
            }
        }
    }
    
    // Sort by display name
    std::sort(result.begin(), result.end(), [](IconInfo* a, IconInfo* b) {
        return a->display_name < b->display_name;
    });
    
    return result;
}

std::vector<IconInfo*> IconManager::getIconsByCategory(const std::string& category) {
    std::vector<IconInfo*> result;
    for (auto& [id, info] : icons_) {
        if (info.category == category) {
            result.push_back(&info);
        }
    }
    std::sort(result.begin(), result.end(), [](IconInfo* a, IconInfo* b) {
        return a->display_name < b->display_name;
    });
    return result;
}

std::vector<IconInfo*> IconManager::getAllIcons() {
    std::vector<IconInfo*> result;
    for (auto& [id, info] : icons_) {
        result.push_back(&info);
    }
    std::sort(result.begin(), result.end(), [](IconInfo* a, IconInfo* b) {
        if (a->category != b->category) {
            return a->category < b->category;
        }
        return a->display_name < b->display_name;
    });
    return result;
}

std::vector<std::string> IconManager::getCategories() {
    std::vector<std::string> result;
    for (const auto& [id, info] : icons_) {
        if (std::find(result.begin(), result.end(), info.category) == result.end()) {
            result.push_back(info.category);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool IconManager::loadCustomIcon(const std::string& file_path, const std::string& id,
                                  const std::string& display_name) {
    if (!wxFile::Exists(file_path)) {
        return false;
    }
    
    wxBitmapBundle bundle = loadSVGFromFile(file_path);
    if (!bundle.IsOk()) {
        return false;
    }
    
    IconInfo info;
    info.id = id;
    info.display_name = display_name;
    info.category = "custom";
    info.file_path = file_path;
    info.is_builtin = false;
    info.bitmap = bundle;
    info.svg_data = nullptr;  // Loaded from file, not embedded
    
    registerIcon(info);
    return true;
}

void IconManager::unloadCustomIcon(const std::string& id) {
    auto it = icons_.find(id);
    if (it != icons_.end() && !it->second.is_builtin) {
        icons_.erase(it);
    }
}

bool IconManager::reloadIcon(const std::string& id) {
    IconInfo* info = getIcon(id);
    if (!info) return false;
    
    if (info->svg_data) {
        info->bitmap = loadSVGFromData(info->svg_data);
        return info->bitmap.IsOk();
    } else if (!info->file_path.empty()) {
        info->bitmap = loadSVGFromFile(info->file_path);
        return info->bitmap.IsOk();
    }
    return false;
}

bool IconManager::updateIconBitmap(const std::string& id, const wxBitmapBundle& bitmap) {
    auto it = icons_.find(id);
    if (it != icons_.end()) {
        it->second.bitmap = bitmap;
        return true;
    }
    return false;
}

void IconManager::saveCustomIconsToConfig() {
    // TODO: Implement using core::AppConfig
}

void IconManager::loadCustomIconsFromConfig() {
    // TODO: Implement using core::AppConfig
}

const char* IconManager::getSVGData(const std::string& id) {
    IconInfo* info = getIcon(id);
    return info ? info->svg_data : nullptr;
}

bool IconManager::hasIcon(const std::string& id) const {
    return icons_.find(id) != icons_.end();
}

wxBitmapBundle IconManager::loadSVGFromData(const char* svg_data) {
    if (!svg_data) return wxBitmapBundle();
    
    // wxBitmapBundle::FromSVG takes a const char* (null-terminated)
    return wxBitmapBundle::FromSVG(svg_data, wxSize(16, 16));
}

wxBitmapBundle IconManager::loadSVGFromFile(const std::string& file_path) {
    if (!wxFile::Exists(file_path)) {
        return wxBitmapBundle();
    }
    
    wxFile file(file_path);
    if (!file.IsOpened()) {
        return wxBitmapBundle();
    }
    
    wxFileOffset length = file.Length();
    if (length <= 0) {
        return wxBitmapBundle();
    }
    
    // Read file into a null-terminated string
    std::string svg_data(length, '\0');
    file.Read(&svg_data[0], length);
    
    return wxBitmapBundle::FromSVG(svg_data.c_str(), wxSize(16, 16));
}

wxString IconIdToArtId(const std::string& icon_id) {
    // Use wxART_MAKE_ART_ID with a C string
    wxString id = wxString::FromUTF8(icon_id.c_str());
    wxString art_id = "ART_" + id;
    return art_id;
}

std::string ArtIdToIconId(const wxString& art_id) {
    // Remove ART_ prefix if present
    wxString str = art_id;
    if (str.StartsWith("wxART_")) {
        str = str.Mid(6);
    } else if (str.StartsWith("ART_")) {
        str = str.Mid(4);
    }
    return std::string(str.ToUTF8());
}

}  // namespace scratchrobin::res
