/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_LAYOUT_PRESET_H
#define SCRATCHROBIN_LAYOUT_PRESET_H

#include <string>
#include <map>
#include <vector>
#include <wx/gdicmn.h>
#include "dockable_window.h"

namespace scratchrobin {

// State of a single window in a layout
struct LayoutWindowState {
    std::string window_id;
    std::string window_type;
    bool is_visible = true;
    bool is_docked = true;
    DockDirection dock_direction = DockDirection::Left;
    wxRect floating_rect;
    int dock_proportion = 25;  // Percentage of parent
    int dock_row = 0;
    int dock_layer = 0;
    bool is_maximized = false;
    
    // Serialization
    std::string ToJson() const;
    static LayoutWindowState FromJson(const std::string& json);
};

// Monitor configuration
struct MonitorConfig {
    int index = 0;
    wxRect geometry;
    bool is_primary = true;
    std::string name;
    
    std::string ToJson() const;
    static MonitorConfig FromJson(const std::string& json);
};

// Layout preset - complete workspace configuration
class LayoutPreset {
public:
    LayoutPreset() = default;
    explicit LayoutPreset(const std::string& name);
    
    // Properties
    std::string GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    
    bool IsDefault() const { return is_default_; }
    void SetDefault(bool is_default) { is_default_ = is_default; }
    
    std::string GetDescription() const { return description_; }
    void SetDescription(const std::string& desc) { description_ = desc; }
    
    // Window states
    void SetWindowState(const std::string& window_id, const LayoutWindowState& state);
    LayoutWindowState GetWindowState(const std::string& window_id) const;
    bool HasWindowState(const std::string& window_id) const;
    void RemoveWindowState(const std::string& window_id);
    std::vector<std::string> GetWindowIds() const;
    
    // Main form state
    void SetMainFormRect(const wxRect& rect) { main_form_rect_ = rect; }
    wxRect GetMainFormRect() const { return main_form_rect_; }
    
    void SetMainFormMaximized(bool maximized) { main_form_maximized_ = maximized; }
    bool IsMainFormMaximized() const { return main_form_maximized_; }
    
    // Monitor configuration
    void SetMonitorConfig(const MonitorConfig& config) { monitor_config_ = config; }
    MonitorConfig GetMonitorConfig() const { return monitor_config_; }
    
    // Serialization
    std::string ToJson() const;
    static LayoutPreset FromJson(const std::string& json);
    bool SaveToFile(const std::string& path) const;
    static LayoutPreset LoadFromFile(const std::string& path);
    
    // Factory methods for built-in presets
    static LayoutPreset CreateDefault();
    static LayoutPreset CreateSingleMonitor();
    static LayoutPreset CreateDualMonitor();
    static LayoutPreset CreateWideScreen();
    static LayoutPreset CreateCompact();
    
private:
    std::string name_;
    std::string description_;
    bool is_default_ = false;
    int version_ = 1;
    
    std::map<std::string, LayoutWindowState> window_states_;
    wxRect main_form_rect_ = wxRect(100, 100, 1280, 900);
    bool main_form_maximized_ = false;
    MonitorConfig monitor_config_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_LAYOUT_PRESET_H
