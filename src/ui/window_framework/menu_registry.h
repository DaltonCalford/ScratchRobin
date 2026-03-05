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
#include <wx/bitmap.h>
#include <wx/bmpbndl.h>

namespace scratchrobin::ui {

// Forward declarations
class Action;
class ActionRegistry;

// Types of menu items
enum class MenuItemType {
    kAction,        // Links to an action
    kSeparator,     // Horizontal separator line
    kSubmenu,       // Nested submenu
    kCheckBox,      // Checkable menu item
    kRadio,         // Radio menu item (part of a group)
};

// A single item in a menu
struct MenuItem {
    std::string item_id;             // Unique ID within menu
    MenuItemType type{MenuItemType::kAction};
    
    // For action items
    std::string action_id;           // Linked action (empty for separators)
    std::string custom_label;        // Override action's display name
    std::string custom_tooltip;      // Override action's tooltip
    std::string custom_accel;        // Override accelerator
    
    // For submenus
    std::string submenu_id;          // Reference to submenu definition
    std::vector<MenuItem> children;  // Inline submenu children
    
    // For checkboxes and radio items
    bool is_checked{false};          // Initial checked state
    std::string radio_group;         // Radio button group name
    
    // Icon (optional override)
    std::string icon_id;             // Override action's icon
    
    // Position
    int sort_order{0};               // Position in menu
    bool is_visible{true};           // Show/hide item
    
    // Help
    std::string help_topic;          // Help file link
    
    // Check if this is a separator
    bool isSeparator() const { return type == MenuItemType::kSeparator; }
    bool isSubmenu() const { return type == MenuItemType::kSubmenu; }
};

// A menu definition (File, Edit, View, etc.)
struct MenuDefinition {
    std::string menu_id;             // Unique identifier (e.g., "file", "edit")
    std::string display_name;        // Menu title (e.g., "&File", "&Edit")
    std::vector<MenuItem> items;     // Menu items
    bool is_builtin{true};           // Built-in vs custom
    int sort_order{0};               // Position in menu bar
    bool is_visible{true};           // Show/hide entire menu
    
    // Find an item by ID
    MenuItem* findItem(const std::string& item_id);
    const MenuItem* findItem(const std::string& item_id) const;
    
    // Move item up/down
    bool moveItemUp(const std::string& item_id);
    bool moveItemDown(const std::string& item_id);
    
    // Remove item
    void removeItem(const std::string& item_id);
    
    // Add item at position (-1 = append)
    void addItem(const MenuItem& item, int position = -1);
};

// Menu set for a window type - collection of menus contributed by a window type
struct WindowTypeMenuSet {
    std::string window_type_id;      // e.g., "sql_editor", "settings", "main"
    std::string display_name;        // Human readable name
    std::vector<std::string> menu_ids; // Menu IDs this window type contributes
    bool merge_with_main{true};      // Merge with main window menus or replace
    
    // Check if this window type contributes a specific menu
    bool hasMenu(const std::string& menu_id) const;
    
    // Add/remove menus
    void addMenu(const std::string& menu_id);
    void removeMenu(const std::string& menu_id);
};

// Menu registry - central repository for all menu definitions
class MenuRegistry {
public:
    static MenuRegistry* get();
    
    // Initialize with built-in menus
    void initialize();
    
    // Menu management
    void registerMenu(const MenuDefinition& menu);
    void unregisterMenu(const std::string& menu_id);
    MenuDefinition* findMenu(const std::string& menu_id);
    std::vector<MenuDefinition*> getAllMenus();
    std::vector<MenuDefinition*> searchMenus(const std::string& query);
    
    // Window type menu sets
    void registerWindowTypeMenuSet(const WindowTypeMenuSet& menu_set);
    void unregisterWindowTypeMenuSet(const std::string& window_type_id);
    WindowTypeMenuSet* findWindowTypeMenuSet(const std::string& window_type_id);
    std::vector<WindowTypeMenuSet*> getAllWindowTypeMenuSets();
    
    // Get menus for a window type
    std::vector<MenuDefinition*> getMenusForWindowType(const std::string& window_type_id);
    
    // Standard menu creation helpers
    void createStandardMenus();       // File, Edit, View, Help
    void createDatabaseMenus();       // Database, Tools
    void createSQLEditorMenus();      // Query, Transaction
    void createSettingsMenus();       // Settings-specific
    
    // Built-in menu registrations
    void registerBuiltinMenus();
    void registerFileMenu();
    void registerEditMenu();
    void registerViewMenu();
    void registerDatabaseMenu();
    void registerQueryMenu();
    void registerTransactionMenu();
    void registerToolsMenu();
    void registerWindowMenu();
    void registerHelpMenu();
    
    // Persistence
    bool exportMenusToFile(const std::string& file_path);
    bool importMenusFromFile(const std::string& file_path);
    void saveToConfig();
    void loadFromConfig();
    
    // Reset to defaults
    void resetToDefaults();
    
    // Generate unique IDs
    std::string generateMenuId(const std::string& base_name);
    std::string generateItemId(const std::string& menu_id, const std::string& base_name);

private:
    MenuRegistry() = default;
    
    std::map<std::string, MenuDefinition> menus_;
    std::map<std::string, WindowTypeMenuSet> window_type_menus_;
    std::map<std::string, MenuDefinition> default_menus_;  // For reset
    
    void registerMenuItem(MenuDefinition& menu, const std::string& action_id, 
                          const std::string& custom_label = "",
                          const std::string& custom_accel = "");
    void registerSeparator(MenuDefinition& menu);
    void registerSubmenu(MenuDefinition& parent, const std::string& submenu_id,
                         const std::string& display_name,
                         const std::vector<std::pair<std::string, std::string>>& actions);
};

// Drag-drop data for menu editor
class MenuDragData {
public:
    enum class DragType {
        kNone,
        kAction,        // Dragging from action list
        kMenuItem,      // Dragging within menu
        kSubmenu,       // Dragging a submenu
    };
    
    DragType type{DragType::kNone};
    std::string action_id;           // For kAction
    std::string menu_id;             // Source menu
    std::string item_id;             // Source item
    std::string submenu_id;          // For kSubmenu
    bool is_copy{false};             // Copy vs move
    
    bool isValid() const { return type != DragType::kNone; }
    void reset() { 
        type = DragType::kNone; 
        action_id.clear(); 
        menu_id.clear(); 
        item_id.clear(); 
        submenu_id.clear();
        is_copy = false;
    }
};

}  // namespace scratchrobin::ui
