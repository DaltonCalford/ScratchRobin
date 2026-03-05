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

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <wx/bitmap.h>
#include <wx/bmpbndl.h>

// Include the action catalog for ActionCategory enum
#include "ui/window_framework/action_catalog.h"

// Forward declaration for JSON - we'll use nlohmann/json or implement simple JSON
namespace nlohmann {
    class json;
}

namespace scratchrobin::ui {

// Forward declaration
class ActionRegistry;

// Definition of an executable action
struct Action {
    std::string action_id;           // Unique identifier (e.g., "db.new_table")
    std::string display_name;        // Human-readable name
    std::string description;         // Detailed description
    std::string tooltip;             // Tooltip/hint text
    std::string help_topic;          // Link to help file/topic
    ActionCategory category;         // Category for grouping
    std::string default_icon_name;   // Default icon identifier
    std::string accelerator;         // Keyboard shortcut
    std::function<void()> callback;  // Function to execute
    bool requires_connection{false}; // Needs DB connection
    bool requires_transaction{false};// Needs active transaction
    bool is_builtin{true};           // Built-in vs custom
    

};

// An icon that can be assigned to actions
struct ActionIcon {
    std::string icon_id;             // Unique identifier
    std::string display_name;        // Human-readable name
    wxBitmapBundle bitmap;           // The icon image
    std::vector<std::string> tags;   // Search tags
    bool is_builtin{true};           // Built-in vs custom
    std::string file_path;           // For custom icons
    

};

// Item in a toolbar - combines action + icon with full customization
struct ToolbarItem {
    std::string item_id;             // Unique item ID within toolbar
    std::string action_id;           // Linked action
    std::string icon_id;             // Can override action's default
    std::string custom_label;        // Optional override label
    std::string custom_tooltip;      // Optional override tooltip
    std::string help_topic;          // Optional override help link
    bool show_label{false};          // Show text alongside icon
    int sort_order{0};               // Position in toolbar
    bool is_separator{false};        // Is this a separator?
    bool is_spacer{false};           // Is this a flexible spacer?
    

};

// Definition of a toolbar
struct ToolbarDefinition {
    std::string toolbar_id;
    std::string display_name;
    std::string description;
    std::vector<ToolbarItem> items;
    bool is_default{false};          // Built-in vs custom
    bool is_visible{true};           // Show/hide toolbar
    wxDirection preferred_dock{wxTOP}; // Preferred docking side
    int icon_size{24};               // 16, 24, or 32
    bool show_text_labels{false};    // Global text label setting
    bool is_expanded{true};          // Expanded/collapsed in editor
    

};

// Central registry for all available actions, icons, and toolbars
class ActionRegistry {
public:
    static ActionRegistry* get();
    
    // Initialize - register builtins and load user customizations
    void initialize();
    
    // Register built-in actions
    void registerBuiltinActions();
    void registerBuiltinIcons();
    void registerStandardToolbars();
    
    // Action management
    void registerAction(const Action& action);
    void unregisterAction(const std::string& action_id);
    Action* findAction(const std::string& action_id);
    std::vector<Action*> getActionsByCategory(ActionCategory cat);
    std::vector<Action*> getAllActions();
    std::vector<ActionCategory> getCategoriesWithActions();
    std::vector<Action*> searchActions(const std::string& query);
    
    // Custom action creation
    std::string generateActionId(const std::string& base_name);
    bool isActionIdAvailable(const std::string& action_id) const;
    
    // Icon management (delegates to IconManager)
    wxBitmapBundle getIconBitmap(const std::string& icon_id, const wxSize& size = wxSize(16, 16));
    std::vector<std::string> getAvailableIcons();
    std::vector<std::string> searchIcons(const std::string& query);
    
    // Toolbar management
    void registerToolbar(const ToolbarDefinition& toolbar);
    void unregisterToolbar(const std::string& toolbar_id);
    ToolbarDefinition* findToolbar(const std::string& toolbar_id);
    std::vector<ToolbarDefinition*> getAllToolbars();
    std::vector<ToolbarDefinition*> searchToolbars(const std::string& query);
    std::string generateToolbarId(const std::string& base_name);
    
    // Toolbar item management
    void addItemToToolbar(const std::string& toolbar_id, const ToolbarItem& item, int position = -1);
    void removeItemFromToolbar(const std::string& toolbar_id, const std::string& item_id);
    void moveItemInToolbar(const std::string& toolbar_id, const std::string& item_id, int new_position);
    ToolbarItem* findToolbarItem(const std::string& toolbar_id, const std::string& item_id);
    
    // Persistence - Import/Export
    bool exportToolbarsToFile(const std::string& file_path);
    bool importToolbarsFromFile(const std::string& file_path);

    
    // Persistence - User config
    void loadFromConfig();
    void saveToConfig();
    
    // Execute action
    void executeAction(const std::string& action_id);
    bool canExecute(const std::string& action_id);
    
    // Reset to defaults
    void resetToDefaults();

private:
    ActionRegistry() = default;
    
    std::map<std::string, Action> actions_;
    std::map<std::string, ToolbarDefinition> toolbars_;
    
    // Default toolbars for reset
    std::map<std::string, ToolbarDefinition> default_toolbars_;
    
    void registerFileActions();
    void registerEditActions();
    void registerViewActions();
    void registerDatabaseActions();
    void registerTransactionActions();
    void registerQueryActions();
    void registerToolsActions();
    void registerWindowActions();
    void registerHelpActions();
    
    // Standard toolbar definitions
    void createStandardToolbar();
    void createDatabaseToolbar();
    void createTransactionToolbar();
    void createQueryToolbar();
    void createNavigationToolbar();
};

// Drag-drop data for toolbar editor
class ToolbarDragData {
public:
    enum class DragType {
        kNone,
        kAction,        // Dragging from action list
        kIcon,          // Dragging from icon list  
        kToolbarItem,   // Dragging within/across toolbars
    };
    
    DragType type{DragType::kNone};
    std::string action_id;
    std::string icon_id;
    std::string toolbar_id;
    std::string item_id;
    int item_index{-1};  // For reordering within toolbar
    
    bool isValid() const { return type != DragType::kNone; }
    void reset() { type = DragType::kNone; action_id.clear(); icon_id.clear(); 
                   toolbar_id.clear(); item_id.clear(); item_index = -1; }
};

}  // namespace scratchrobin::ui
