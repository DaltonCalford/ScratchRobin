/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_LAYOUT_MANAGER_H
#define SCRATCHROBIN_LAYOUT_MANAGER_H

#include <memory>
#include <map>
#include <vector>
#include <functional>
#include <wx/aui/aui.h>
#include "dockable_window.h"
#include "layout_preset.h"

namespace scratchrobin {

// Forward declarations
class MainFrame;

// Event for layout changes
class LayoutChangeEvent {
public:
    enum Type {
        WindowRegistered,
        WindowUnregistered,
        WindowDocked,
        WindowFloated,
        WindowClosed,
        LayoutLoaded,
        LayoutSaved
    };
    
    Type type;
    std::string window_id;
    std::string layout_name;
};

// Layout change callback
typedef std::function<void(const LayoutChangeEvent&)> LayoutChangeCallback;

// Menu merger helper
class MenuMerger {
public:
    // Merge form menu into main menu bar
    void MergeMenuBar(wxMenuBar* main_menu, wxMenuBar* form_menu);
    void UnmergeMenuBar(wxMenuBar* main_menu, wxMenuBar* form_menu);
    
    // Merge form toolbar
    void MergeToolBar(wxToolBar* main_toolbar, wxToolBar* form_toolbar);
    void UnmergeToolBar(wxToolBar* main_toolbar, wxToolBar* form_toolbar);
    
private:
    std::map<int, wxMenu*> merged_menus_;
    std::vector<wxToolBarToolBase*> merged_tools_;
};

// Layout manager - central docking and layout system
class LayoutManager {
public:
    LayoutManager(MainFrame* main_frame);
    ~LayoutManager();
    
    // Initialization
    void Initialize();
    void Shutdown();
    
    // Window registration
    void RegisterWindow(IDockableWindow* window);
    void UnregisterWindow(const std::string& window_id);
    IDockableWindow* GetWindow(const std::string& window_id) const;
    std::vector<IDockableWindow*> GetAllWindows() const;
    std::vector<IDockableWindow*> GetVisibleWindows() const;
    
    // Window operations
    void DockWindow(const std::string& window_id, DockDirection direction, int proportion = 25);
    void FloatWindow(const std::string& window_id, const wxPoint& pos = wxDefaultPosition, 
                     const wxSize& size = wxDefaultSize);
    void ShowWindow(const std::string& window_id, bool show = true);
    void HideWindow(const std::string& window_id);
    void CloseWindow(const std::string& window_id);
    bool IsWindowVisible(const std::string& window_id) const;
    bool IsWindowDocked(const std::string& window_id) const;
    
    // Active window
    void SetActiveWindow(const std::string& window_id);
    IDockableWindow* GetActiveWindow() const;
    std::string GetActiveWindowId() const { return active_window_id_; }
    
    // Preset management
    void LoadPreset(const std::string& name);
    void LoadPreset(const LayoutPreset& preset);
    void SaveCurrentAsPreset(const std::string& name);
    void SaveCurrentAsPreset(const std::string& name, const std::string& description);
    void DeletePreset(const std::string& name);
    std::vector<LayoutPreset> GetPresets() const;
    LayoutPreset GetPreset(const std::string& name) const;
    bool HasPreset(const std::string& name) const;
    void SetDefaultPreset(const std::string& name);
    std::string GetDefaultPreset() const;
    
    // Built-in presets
    void ApplyDefaultLayout();
    void ApplySingleMonitorLayout();
    void ApplyDualMonitorLayout();
    void ApplyWideScreenLayout();
    void ApplyCompactLayout();
    
    // State persistence
    void SaveState();
    void RestoreState();
    
    // Layout change notifications
    void AddObserver(LayoutChangeCallback callback);
    void RemoveObserver(LayoutChangeCallback callback);
    
    // AUI manager access
    wxAuiManager* GetAuiManager() { return &aui_manager_; }
    
    // Auto-save
    void SetAutoSave(bool enabled) { auto_save_ = enabled; }
    bool IsAutoSaveEnabled() const { return auto_save_; }
    
private:
    void NotifyObservers(const LayoutChangeEvent& event);
    std::string GetPresetsDirectory() const;
    void SavePresetToFile(const LayoutPreset& preset);
    LayoutPreset LoadPresetFromFile(const std::string& name);
    void ApplyWindowState(const LayoutWindowState& state);
    
    MainFrame* main_frame_ = nullptr;
    wxAuiManager aui_manager_;
    std::map<std::string, IDockableWindow*> windows_;
    std::map<std::string, wxAuiPaneInfo> pane_infos_;
    std::string active_window_id_;
    std::vector<LayoutChangeCallback> observers_;
    bool auto_save_ = true;
    std::string current_preset_name_;
    
    MenuMerger menu_merger_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_LAYOUT_MANAGER_H
