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

#include <map>
#include <vector>
#include <functional>
#include <wx/menu.h>
#include <wx/frame.h>
#include "ui/window_framework/window_types.h"

namespace scratchrobin::ui {

// Forward declarations
class DockableWindow;
class WindowManager;

// Dynamic menu bar - updates based on active window
// Only the main window has a menu bar - other windows contribute to it
class DynamicMenuBar {
public:
    DynamicMenuBar(wxFrame* main_frame);
    ~DynamicMenuBar();
    
    // Initialize with default menus
    void initialize();
    
    // Set active window - updates menu bar with window's contributions
    void setActiveWindow(DockableWindow* window);
    void clearActiveWindow();
    
    // Menu contributions
    void addContributions(const std::vector<MenuContribution>& contributions);
    void removeContributions(const std::vector<MenuContribution>& contributions);
    void clearContributions();
    
    // Standard menu access
    wxMenu* getFileMenu() { return file_menu_; }
    wxMenu* getEditMenu() { return edit_menu_; }
    wxMenu* getViewMenu() { return view_menu_; }
    wxMenu* getWindowMenu() { return window_menu_; }
    wxMenu* getHelpMenu() { return help_menu_; }
    
    // Get or create a menu by name
    wxMenu* getOrCreateMenu(const std::string& name, int position = -1);
    
    // Remove a menu
    void removeMenu(const std::string& name);
    
    // Update the physical menu bar
    void updateMenuBar();
    
    // Enable/disable menu items by ID
    void enableItem(int id, bool enable);
    void checkItem(int id, bool check);
    
    // Set menu item label
    void setItemLabel(int id, const wxString& label);

private:
    wxFrame* main_frame_{nullptr};
    wxMenuBar* menu_bar_{nullptr};
    
    // Standard menus
    wxMenu* file_menu_{nullptr};
    wxMenu* edit_menu_{nullptr};
    wxMenu* view_menu_{nullptr};
    wxMenu* window_menu_{nullptr};
    wxMenu* help_menu_{nullptr};
    
    // Dynamic menus from windows
    std::map<std::string, wxMenu*> dynamic_menus_;
    std::vector<MenuContribution> current_contributions_;
    std::map<int, std::function<void()>> callbacks_;
    
    // Menu item ID counter
    int next_menu_id_{wxID_HIGHEST + 1000};
    
    // Build the window menu
    void buildWindowMenu();
    
    // Event handler for menu items
    void onMenuItemSelected(wxCommandEvent& event);
    
    // Helper to find menu by name
    wxMenu* findMenu(const std::string& name);
    int findMenuIndex(const std::string& name);
};

}  // namespace scratchrobin::ui
