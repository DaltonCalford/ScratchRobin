// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include <string>
#include <vector>
#include <wx/wx.h>

namespace scratchrobin {
namespace ui {
namespace toolbar {

/**
 * @brief Type of toolbar item
 */
enum class ToolbarItemType {
    Icon,           // Standard icon button
    Separator,      // Visual separator
    Dropdown,       // Dropdown button with menu
    Text,           // Text-only label
    Combo,          // Combo box (dropdown with input)
    Toggle,         // Toggle button (on/off)
    Spacer,         // Flexible spacer
    FixedSpacer     // Fixed-width spacer
};

/**
 * @brief Toolbar item data structure
 */
struct ToolbarItem {
    // Identification
    std::string id;                      // Unique item ID
    std::string action_id;               // Associated action ID (for icons/toggles)
    ToolbarItemType type = ToolbarItemType::Icon;
    
    // Display properties
    std::string icon_name;               // Icon resource name
    std::string label;                   // Text label (if any)
    std::string tooltip;                 // Hover tooltip
    
    // State
    bool is_enabled = true;
    bool is_visible = true;
    bool is_toggled = false;             // For toggle buttons
    
    // For combo items
    std::vector<std::string> combo_items;
    int combo_selected = -1;
    
    // For spacers
    int spacer_width = 8;                // For fixed spacers (pixels)
    
    // Helper methods
    bool IsInteractive() const {
        return type == ToolbarItemType::Icon || 
               type == ToolbarItemType::Dropdown ||
               type == ToolbarItemType::Combo ||
               type == ToolbarItemType::Toggle;
    }
    
    bool IsSpacer() const {
        return type == ToolbarItemType::Spacer || 
               type == ToolbarItemType::FixedSpacer;
    }
};

/**
 * @brief Available toolbar item template for the palette
 */
struct ToolbarItemTemplate {
    std::string id;
    std::string action_id;
    std::string icon_name;
    std::string label;
    std::string tooltip;
    ToolbarItemType type = ToolbarItemType::Icon;
    std::string category;                // For grouping in palette (e.g., "File", "Edit", "SQL")
    bool is_custom = false;              // True for custom/separator items
};

} // namespace toolbar
} // namespace ui
} // namespace scratchrobin
