// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "toolbar_item.h"
#include <vector>
#include <string>

namespace scratchrobin {
namespace ui {
namespace toolbar {

/**
 * @brief Toolbar dock position
 */
enum class ToolbarPosition {
    Top,        // Top of window (default)
    Bottom,     // Bottom of window
    Left,       // Left side (vertical)
    Right,      // Right side (vertical)
    Floating    // Floating toolbar window
};

/**
 * @brief Toolbar visibility scope
 */
enum class ToolbarScope {
    Global,         // Always visible
    DocumentType,   // Visible for specific document types
    ConnectionType, // Visible for specific database types
    Contextual      // Dynamically shown based on context
};

/**
 * @brief Complete toolbar definition
 */
struct ToolbarDefinition {
    // Identification
    std::string id;                      // Unique toolbar ID (e.g., "main.standard")
    std::string name;                    // Display name
    std::string description;             // Description for UI
    
    // Position and layout
    ToolbarPosition position = ToolbarPosition::Top;
    int row = 0;                         // Row index for multiple toolbars
    bool is_visible = true;
    bool is_locked = false;              // Prevent modification
    
    // Scope
    ToolbarScope scope = ToolbarScope::Global;
    std::vector<std::string> scope_contexts; // Document types or connection types
    
    // Items
    std::vector<ToolbarItem> items;
    
    // Floating state (when position == Floating)
    int float_x = -1;
    int float_y = -1;
    int float_width = 300;
    int float_height = 40;
    
    // Helper methods
    bool IsCustom() const {
        return !is_locked && id.find("custom.") == 0;
    }
    
    size_t GetVisibleItemCount() const {
        size_t count = 0;
        for (const auto& item : items) {
            if (item.is_visible && item.type != ToolbarItemType::Spacer) {
                count++;
            }
        }
        return count;
    }
    
    bool CanModify() const {
        return !is_locked;
    }
};

/**
 * @brief Toolbar preset/collection
 */
struct ToolbarPreset {
    std::string id;
    std::string name;
    std::string description;
    bool is_builtin = false;
    std::vector<std::string> toolbar_ids;
};

} // namespace toolbar
} // namespace ui
} // namespace scratchrobin
