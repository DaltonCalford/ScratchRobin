/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/window_types.h"

namespace scratchrobin::ui {

// WindowTypeRegistry implementation
WindowTypeRegistry& WindowTypeRegistry::get() {
    static WindowTypeRegistry instance;
    return instance;
}

WindowTypeRegistry::WindowTypeRegistry() {
    registerBuiltinTypes();
}

void WindowTypeRegistry::registerBuiltinTypes() {
    // Main container
    WindowTypeDefinition mainContainer;
    mainContainer.type_id = WindowTypes::kMainContainer;
    mainContainer.display_name = "Main Window";
    mainContainer.category = WindowCategory::kContainer;
    mainContainer.dock_capabilities = DockCapability::kNone;
    mainContainer.size_behavior = SizeBehavior::kStretchBoth;
    mainContainer.default_size = {1024, 768};
    mainContainer.min_size = {800, 600};
    mainContainer.max_size = {wxDefaultCoord, wxDefaultCoord};
    mainContainer.allow_multiple_instances = false;
    mainContainer.singleton_per_container = true;
    mainContainer.persist_state = true;
    mainContainer.show_in_window_menu = false;
    mainContainer.can_close = false;
    mainContainer.activate_on_show = true;
    mainContainer.icon_index = -1;
    mainContainer.default_dock = -1;
    registerType(mainContainer);
    
    // Project Navigator
    WindowTypeDefinition navigator;
    navigator.type_id = WindowTypes::kProjectNavigator;
    navigator.display_name = "Project Navigator";
    navigator.category = WindowCategory::kNavigator;
    navigator.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                  DockCapability::kFloat | DockCapability::kAutoHide;
    navigator.size_behavior = SizeBehavior::kStretchVertical;
    navigator.default_size = {250, 600};
    navigator.min_size = {150, 200};
    navigator.max_size = {400, wxDefaultCoord};
    navigator.allow_multiple_instances = false;
    navigator.singleton_per_container = true;
    navigator.persist_state = true;
    navigator.show_in_window_menu = true;
    navigator.can_close = true;
    navigator.activate_on_show = true;
    navigator.icon_index = -1;
    navigator.default_dock = wxLEFT;
    registerType(navigator);
    
    // Icon Bar
    WindowTypeDefinition iconBar;
    iconBar.type_id = WindowTypes::kIconBar;
    iconBar.display_name = "Toolbar";
    iconBar.category = WindowCategory::kIconBar;
    iconBar.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                DockCapability::kTop | DockCapability::kBottom | 
                                DockCapability::kFloat;
    iconBar.size_behavior = SizeBehavior::kAutoSize;
    iconBar.default_size = {600, 32};
    iconBar.min_size = {100, 28};
    iconBar.max_size = {wxDefaultCoord, 64};
    iconBar.allow_multiple_instances = true;
    iconBar.singleton_per_container = false;
    iconBar.persist_state = true;
    iconBar.show_in_window_menu = false;
    iconBar.can_close = true;
    iconBar.activate_on_show = false;
    iconBar.icon_index = -1;
    iconBar.default_dock = wxTOP;
    registerType(iconBar);
    
    // SQL Editor
    WindowTypeDefinition sqlEditor;
    sqlEditor.type_id = WindowTypes::kSQLEditor;
    sqlEditor.display_name = "SQL Editor";
    sqlEditor.category = WindowCategory::kEditor;
    sqlEditor.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                  DockCapability::kTop | DockCapability::kBottom | 
                                  DockCapability::kFloat | DockCapability::kTab;
    sqlEditor.size_behavior = SizeBehavior::kStretchBoth;
    sqlEditor.default_size = {600, 400};
    sqlEditor.min_size = {300, 200};
    sqlEditor.max_size = {wxDefaultCoord, wxDefaultCoord};
    sqlEditor.allow_multiple_instances = true;
    sqlEditor.singleton_per_container = false;
    sqlEditor.persist_state = true;
    sqlEditor.show_in_window_menu = true;
    sqlEditor.can_close = true;
    sqlEditor.activate_on_show = true;
    sqlEditor.icon_index = -1;
    sqlEditor.default_dock = -1;
    registerType(sqlEditor);
    
    // Object Inspector
    WindowTypeDefinition inspector;
    inspector.type_id = WindowTypes::kObjectInspector;
    inspector.display_name = "Object Inspector";
    inspector.category = WindowCategory::kInspector;
    inspector.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                  DockCapability::kFloat | DockCapability::kAutoHide;
    inspector.size_behavior = SizeBehavior::kStretchVertical;
    inspector.default_size = {250, 600};
    inspector.min_size = {150, 200};
    inspector.max_size = {400, wxDefaultCoord};
    inspector.allow_multiple_instances = false;
    inspector.singleton_per_container = true;
    inspector.persist_state = true;
    inspector.show_in_window_menu = true;
    inspector.can_close = true;
    inspector.activate_on_show = true;
    inspector.icon_index = -1;
    inspector.default_dock = wxRIGHT;
    registerType(inspector);
    
    // Output Panel
    WindowTypeDefinition output;
    output.type_id = WindowTypes::kOutputPanel;
    output.display_name = "Output";
    output.category = WindowCategory::kOutput;
    output.dock_capabilities = DockCapability::kTop | DockCapability::kBottom | 
                               DockCapability::kFloat | DockCapability::kAutoHide;
    output.size_behavior = SizeBehavior::kStretchHorizontal;
    output.default_size = {800, 150};
    output.min_size = {200, 80};
    output.max_size = {wxDefaultCoord, 400};
    output.allow_multiple_instances = false;
    output.singleton_per_container = true;
    output.persist_state = true;
    output.show_in_window_menu = true;
    output.can_close = true;
    output.activate_on_show = false;
    output.icon_index = -1;
    output.default_dock = wxBOTTOM;
    registerType(output);
    
    // Message Log
    WindowTypeDefinition messageLog;
    messageLog.type_id = WindowTypes::kMessageLog;
    messageLog.display_name = "Message Log";
    messageLog.category = WindowCategory::kOutput;
    messageLog.dock_capabilities = DockCapability::kBottom | DockCapability::kFloat | 
                                   DockCapability::kTab;
    messageLog.size_behavior = SizeBehavior::kStretchHorizontal;
    messageLog.default_size = {800, 150};
    messageLog.min_size = {200, 80};
    messageLog.max_size = {wxDefaultCoord, 400};
    messageLog.allow_multiple_instances = false;
    messageLog.singleton_per_container = true;
    messageLog.persist_state = true;
    messageLog.show_in_window_menu = true;
    messageLog.can_close = true;
    messageLog.activate_on_show = false;
    messageLog.icon_index = -1;
    messageLog.default_dock = wxBOTTOM;
    registerType(messageLog);
    
    // Properties Panel
    WindowTypeDefinition properties;
    properties.type_id = WindowTypes::kPropertiesPanel;
    properties.display_name = "Properties";
    properties.category = WindowCategory::kInspector;
    properties.dock_capabilities = DockCapability::kRight | DockCapability::kLeft | 
                                   DockCapability::kFloat | DockCapability::kAutoHide;
    properties.size_behavior = SizeBehavior::kStretchVertical;
    properties.default_size = {250, 600};
    properties.min_size = {150, 200};
    properties.max_size = {400, wxDefaultCoord};
    properties.allow_multiple_instances = false;
    properties.singleton_per_container = true;
    properties.persist_state = true;
    properties.show_in_window_menu = true;
    properties.can_close = true;
    properties.activate_on_show = true;
    properties.icon_index = -1;
    properties.default_dock = wxRIGHT;
    registerType(properties);
}

void WindowTypeRegistry::registerType(const WindowTypeDefinition& def) {
    types_[def.type_id] = def;
}

const WindowTypeDefinition* WindowTypeRegistry::getType(const std::string& type_id) const {
    auto it = types_.find(type_id);
    if (it != types_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const WindowTypeDefinition*> WindowTypeRegistry::getTypesByCategory(WindowCategory cat) const {
    std::vector<const WindowTypeDefinition*> result;
    for (const auto& [id, def] : types_) {
        if (def.category == cat) {
            result.push_back(&def);
        }
    }
    return result;
}

std::vector<const WindowTypeDefinition*> WindowTypeRegistry::getAllTypes() const {
    std::vector<const WindowTypeDefinition*> result;
    for (const auto& [id, def] : types_) {
        result.push_back(&def);
    }
    return result;
}

}  // namespace scratchrobin::ui
