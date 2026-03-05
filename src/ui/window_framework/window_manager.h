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

#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/aui/aui.h>

#include "ui/window_framework/window_types.h"

namespace scratchrobin::ui {

// Forward declarations
class DockableWindow;
class DynamicMenuBar;
class DockingOverlay;

// Window manager - handles all dockable windows
class WindowManager {
public:
    WindowManager(wxFrame* main_frame);
    ~WindowManager();
    
    // Initialize the manager
    void initialize();
    
    // Window creation
    DockableWindow* createWindow(const WindowCreateParams& params);
    DockableWindow* createWindow(const std::string& type_id, const std::string& title = "");
    
    // Window lookup
    DockableWindow* findWindow(const std::string& instance_id);
    DockableWindow* findWindowByType(const std::string& type_id);
    std::vector<DockableWindow*> getWindowsByType(const std::string& type_id);
    std::vector<DockableWindow*> getAllWindows();
    DockableWindow* getActiveWindow();
    
    // Window operations
    void showWindow(const std::string& instance_id);
    void hideWindow(const std::string& instance_id);
    void closeWindow(const std::string& instance_id);
    void activateWindow(const std::string& instance_id);
    void floatWindow(const std::string& instance_id);
    void dockWindow(const std::string& instance_id, wxDirection direction);
    
    // Menu management
    void updateMenuBar();  // Call when active window changes
    DynamicMenuBar* getMenuBar() { return menu_bar_; }
    
    // Drag/drop docking
    void startDrag(DockableWindow* window);
    void updateDrag(const wxPoint& screen_pos);
    void endDrag(bool accept);
    bool isDragging() const { return drag_source_ != nullptr; }
    DockableWindow* getDragSource() { return drag_source_; }
    
    // Drop zone detection
    int hitTestDropZone(const wxPoint& screen_pos);
    void showDropZones();
    void hideDropZones();
    
    // Layout management
    void saveLayout(const std::string& name);
    bool loadLayout(const std::string& name);
    std::vector<std::string> getSavedLayouts() const;
    void resetLayout();
    
    // Access to AUI manager
    wxAuiManager* getAuiManager() { return aui_manager_; }
    
    // Main frame access
    wxFrame* getMainFrame() { return main_frame_; }
    
    // Window registration (called by DockableWindow constructor)
    void registerWindow(DockableWindow* window);
    void unregisterWindow(DockableWindow* window);
    
    // Event handlers
    void onWindowActivated(DockableWindow* window);
    void onWindowClosed(DockableWindow* window);

private:
    wxFrame* main_frame_{nullptr};
    wxAuiManager* aui_manager_{nullptr};
    DynamicMenuBar* menu_bar_{nullptr};
    DockingOverlay* drop_overlay_{nullptr};
    
    // Window registry
    std::map<std::string, DockableWindow*> windows_;
    DockableWindow* active_window_{nullptr};
    DockableWindow* drag_source_{nullptr};
    
    // Menu bar placeholder (when no active window)
    wxMenuBar* default_menu_bar_{nullptr};
    
    // Layout persistence
    std::map<std::string, wxString> saved_layouts_;
    
    // Helper methods
    wxAuiPaneInfo createPaneInfo(const WindowCreateParams& params, 
                                  const WindowTypeDefinition* type_def);
    std::string generateInstanceId(const std::string& type_id);
    void createDefaultMenuBar();
};

// Global accessor
WindowManager* getWindowManager();
void setWindowManager(WindowManager* mgr);

}  // namespace scratchrobin::ui
