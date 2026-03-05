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
#include <wx/window.h>
#include <wx/menu.h>

namespace scratchrobin::ui {

// Forward declarations
class DockableWindow;
class WindowManager;

// Window type categories - defines behavior and capabilities
enum class WindowCategory {
    kContainer,      // Can contain other windows (Main window)
    kNavigator,      // Tree/project navigator - dockable left/right
    kIconBar,        // Toolbar with icons - dockable top/bottom/left/right
    kEditor,         // SQL Editor, text editors - dockable anywhere, can be floating
    kInspector,      // Properties panel - dockable right/left
    kOutput,         // Output/log panels - dockable bottom
    kDialog,         // Modal/non-modal dialogs - floating only
    kPopup,          // Temporary popups - floating, auto-close
};

// Docking capabilities for a window type
enum class DockCapability {
    kNone = 0,
    kLeft = 1 << 0,
    kRight = 1 << 1,
    kTop = 1 << 2,
    kBottom = 1 << 3,
    kFloat = 1 << 4,       // Can be undocked/floating
    kTab = 1 << 5,         // Can tab with other windows
    kAutoHide = 1 << 6,    // Can auto-hide to edge
};

inline DockCapability operator|(DockCapability a, DockCapability b) {
    return static_cast<DockCapability>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool hasCapability(DockCapability caps, DockCapability cap) {
    return (static_cast<int>(caps) & static_cast<int>(cap)) != 0;
}

// Window size behavior
enum class SizeBehavior {
    kFixed,              // Fixed size (dialog style)
    kAutoSize,           // Size to content
    kStretchHorizontal,  // Stretch horizontally
    kStretchVertical,    // Stretch vertically
    kStretchBoth,        // Fill available space
    kSplitProportional,  // Share space proportionally with siblings
};

// Window state
struct WindowState {
    bool is_visible{true};
    bool is_floating{false};
    bool is_minimized{false};
    bool is_auto_hidden{false};
    bool is_active{false};
    wxRect float_rect;      // Position when floating
    int dock_size{-1};      // Size when docked (-1 = default)
};

// Menu contribution from a window
struct MenuContribution {
    std::string menu_name;           // e.g., "File", "Edit", "View"
    std::string item_label;          // Menu item text
    std::string item_help;           // Status bar help text
    int item_id{0};                  // wx ID
    int position{-1};                // Position in menu (-1 = append)
    std::function<void()> callback;  // Click handler
    bool is_checkable{false};
    bool is_checked{false};
    bool is_separator{false};
    std::string accelerator;         // e.g., "Ctrl+N"
};

// Window type definition - describes behavior of a window class
struct WindowTypeDefinition {
    std::string type_id;                    // Unique identifier
    std::string display_name;               // Human-readable name
    std::string description;                // Detailed description
    WindowCategory category;                // General category
    DockCapability dock_capabilities;       // Where it can dock
    SizeBehavior size_behavior;             // How it sizes
    
    // Default sizes
    wxSize default_size{400, 300};
    wxSize min_size{100, 50};
    wxSize max_size{wxDefaultCoord, wxDefaultCoord};
    
    // Behavior flags
    bool allow_multiple_instances{true};    // Can have multiple of this type
    bool singleton_per_container{false};    // Only one per parent container
    bool persist_state{true};               // Save/restore position
    bool show_in_window_menu{true};         // Show in Windows menu
    bool can_close{true};                   // User can close this window
    bool activate_on_show{true};            // Auto-activate when shown
    
    // Appearance
    bool has_title_bar{true};               // Show title bar
    bool has_caption{true};                 // Show caption when docked
    bool can_float{true};                   // Can be undocked/floating
    bool can_tab{true};                     // Can tab with other windows
    
    // Menu contributions
    std::vector<std::string> menu_set_ids;  // Menu sets to activate
    bool merge_menus_with_main{true};       // Merge with main window menus
    
    // Instance behavior
    bool singleton{false};                  // Only one instance allowed (global)
    bool allow_multiple{true};              // Multiple instances allowed
    int max_instances{0};                   // Max instances (0 = unlimited)
    
    // Persistence
    bool persist_layout{true};              // Save/restore layout
    
    // Activation behavior
    bool steal_focus{false};                // Steal focus on open
    
    // Special flags
    bool is_modal{false};                   // Modal dialog
    bool is_popup{false};                   // Popup window
    bool is_tool_window{false};             // Tool window (smaller title bar)
    bool is_always_on_top{false};           // Always on top
    
    // Icon (for window menu, tabs)
    int icon_index{-1};                     // Index in image list
    
    // Default docking position for first creation: wxLEFT, wxRIGHT, wxTOP, wxBOTTOM, or -1 for center
    int default_dock{-1};
};

// Window type registry - defines all available window types
class WindowTypeRegistry {
public:
    static WindowTypeRegistry& get();
    
    // Register built-in window types
    void registerBuiltinTypes();
    
    // Register a custom window type
    void registerType(const WindowTypeDefinition& def);
    
    // Get type definition
    const WindowTypeDefinition* getType(const std::string& type_id) const;
    
    // Get all types in a category
    std::vector<const WindowTypeDefinition*> getTypesByCategory(WindowCategory cat) const;
    
    // Get all registered types
    std::vector<const WindowTypeDefinition*> getAllTypes() const;

private:
    WindowTypeRegistry();
    std::map<std::string, WindowTypeDefinition> types_;
};

// Built-in window type IDs
namespace WindowTypes {
    constexpr const char* kMainContainer = "main_container";
    constexpr const char* kProjectNavigator = "project_navigator";
    constexpr const char* kIconBar = "icon_bar";
    constexpr const char* kSQLEditor = "sql_editor";
    constexpr const char* kObjectInspector = "object_inspector";
    constexpr const char* kOutputPanel = "output_panel";
    constexpr const char* kSearchResults = "search_results";
    constexpr const char* kPropertiesPanel = "properties_panel";
    constexpr const char* kDatabaseBrowser = "database_browser";
    constexpr const char* kQueryBuilder = "query_builder";
    constexpr const char* kDataGrid = "data_grid";
    constexpr const char* kMessageLog = "message_log";
}

// Window creation parameters
struct WindowCreateParams {
    std::string type_id;
    std::string instance_id;        // Optional unique ID (auto-generated if empty)
    std::string title;
    wxWindow* parent{nullptr};      // Parent container
    DockableWindow* dock_target{nullptr};  // Dock relative to this window
    int dock_direction{wxLEFT};     // Direction to dock: wxLEFT, wxRIGHT, wxTOP, wxBOTTOM, or -1 for center/tab
    wxRect float_rect;              // Initial position if floating
    bool start_floating{false};
    bool activate{true};
    std::vector<MenuContribution> menus;  // Initial menu contributions
};

}  // namespace scratchrobin::ui
