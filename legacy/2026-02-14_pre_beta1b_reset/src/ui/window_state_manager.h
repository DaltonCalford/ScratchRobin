/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_WINDOW_STATE_MANAGER_H
#define SCRATCHROBIN_WINDOW_STATE_MANAGER_H

#include "auto_size_policy.h"

#include <wx/gdicmn.h>
#include <wx/display.h>

#include <string>
#include <map>
#include <memory>

namespace scratchrobin {

// Forward declarations
class MainFrame;

/**
 * WindowStateManager - Manages saving and restoring window state.
 *
 * This class handles:
 * - Main form position, size, and state (maximized/fullscreen)
 * - Panel visibility and docking state
 * - Toolbar positions
 * - Multi-monitor awareness
 * - JSON-based persistence
 */
class WindowStateManager {
public:
    /**
     * Main form window state
     */
    struct MainFormState {
        bool is_maximized = false;
        bool is_fullscreen = false;
        wxRect normal_rect{wxPoint(100, 100), wxSize(1024, 768)};
        AutoSizePolicy::Mode auto_size_mode = AutoSizePolicy::Mode::ADAPTIVE;
        int display_index = 0;  // Which monitor the window was on

        /**
         * Check if the state is valid (has positive dimensions)
         * @return true if valid
         */
        bool IsValid() const {
            return normal_rect.width > 0 && normal_rect.height > 0;
        }
    };

    /**
     * Panel state (navigator, document manager, etc.)
     */
    struct PanelState {
        bool is_visible = true;
        bool is_docked = true;
        wxRect floating_rect;
        int dock_proportion = 25;  // Percentage of parent width/height
        int display_index = 0;  // For floating windows

        PanelState() = default;
        explicit PanelState(bool visible) : is_visible(visible) {}
    };

    /**
     * Toolbar state
     */
    struct ToolbarState {
        bool is_floating = false;
        wxPoint position;
        int display_index = 0;
    };

    /**
     * Constructor
     * @param main_frame The main window to manage (can be null initially)
     */
    explicit WindowStateManager(MainFrame* main_frame = nullptr);

    /**
     * Destructor - saves state if needed
     */
    ~WindowStateManager();

    /**
     * Set the main frame (can be called after construction)
     * @param frame The main frame to manage
     */
    void SetMainFrame(MainFrame* frame) { main_frame_ = frame; }

    /**
     * Save current window state to file
     * @return true if successful
     */
    bool SaveState();

    /**
     * Restore window state from file
     * @return true if successful
     */
    bool RestoreState();

    /**
     * Reset to default layout
     */
    void ResetToDefaults();

    /**
     * Called on application exit - saves state
     */
    void OnExit();

    /**
     * Called on application startup - restores state
     */
    void OnInit();

    /**
     * Get the main form state
     * @return Reference to main form state
     */
    const MainFormState& GetMainFormState() const { return main_state_; }

    /**
     * Get mutable main form state
     * @return Reference to main form state
     */
    MainFormState& GetMainFormState() { return main_state_; }

    /**
     * Get panel state by name
     * @param name Panel identifier (e.g., "navigator", "document_manager")
     * @return Reference to panel state
     */
    PanelState& GetPanelState(const std::string& name);

    /**
     * Get panel state by name (const)
     * @param name Panel identifier
     * @return Const reference to panel state
     */
    const PanelState& GetPanelState(const std::string& name) const;

    /**
     * Get toolbar state by name
     * @param name Toolbar identifier (e.g., "main", "sql_editor")
     * @return Reference to toolbar state
     */
    ToolbarState& GetToolbarState(const std::string& name);

    /**
     * Get toolbar state by name (const)
     * @param name Toolbar identifier
     * @return Const reference to toolbar state
     */
    const ToolbarState& GetToolbarState(const std::string& name) const;

    /**
     * Set layout preset name
     * @param preset Preset name (e.g., "default", "minimal", "maximized")
     */
    void SetLayoutPreset(const std::string& preset) { layout_preset_ = preset; }

    /**
     * Get current layout preset
     * @return Preset name
     */
    std::string GetLayoutPreset() const { return layout_preset_; }

    /**
     * Check if state file exists
     * @return true if state file exists
     */
    bool HasSavedState() const;

    /**
     * Delete saved state file
     * @return true if successful or file didn't exist
     */
    bool DeleteSavedState();

    /**
     * Get the display index for a point (monitor awareness)
     * @param point The point to check
     * @return Display index, or 0 if not found
     */
    static int GetDisplayIndexForPoint(const wxPoint& point);

    /**
     * Check if a display index is valid
     * @param index The display index
     * @return true if valid
     */
    static bool IsValidDisplay(int index);

    /**
     * Get safe position on valid display
     * If the requested position is on an invalid/disconnected display,
     * returns position on primary display
     * @param rect The requested window rectangle
     * @param display_index The display index (may be updated)
     * @return Safe position
     */
    static wxPoint GetSafePosition(const wxRect& rect, int& display_index);

private:
    MainFrame* main_frame_ = nullptr;
    MainFormState main_state_;
    std::map<std::string, PanelState> panel_states_;
    std::map<std::string, ToolbarState> toolbar_states_;
    std::string layout_preset_ = "default";

    // Version for JSON format compatibility
    static constexpr int kStateVersion = 1;

    /**
     * Get the state file path
     * @return Full path to state file
     */
    std::string GetStateFilePath() const;

    /**
     * Get the config directory path
     * @return Full path to config directory
     */
    std::string GetConfigDir() const;

    /**
     * Serialize state to JSON string
     * @return JSON representation of state
     */
    std::string SerializeToJson() const;

    /**
     * Deserialize state from JSON string
     * @param json The JSON string to parse
     * @return true if successful
     */
    bool DeserializeFromJson(const std::string& json);

    /**
     * Ensure config directory exists
     * @return true if directory exists or was created
     */
    bool EnsureConfigDir() const;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_WINDOW_STATE_MANAGER_H
