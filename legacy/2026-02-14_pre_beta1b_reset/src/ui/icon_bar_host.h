/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ICON_BAR_HOST_H
#define SCRATCHROBIN_ICON_BAR_HOST_H

#include <wx/string.h>
#include <wx/gdicmn.h>
#include <wx/event.h>
#include <vector>
#include <map>
#include <memory>

// Forward declarations (global wxWidgets types)
class wxWindow;
class wxMenu;
class wxMenuBar;

namespace scratchrobin {

// Forward declarations
class DraggableToolBar;
class FloatingToolBarFrame;
class FloatingFrame;
class DockableForm;
class MainFrame;

// ============================================================================
// ToolBarInfo - Information about a toolbar
// ============================================================================
struct ToolBarInfo {
    wxString name;
    wxString label;
    bool visible = true;
    bool can_float = true;
    std::vector<wxString> associated_forms;  // Forms that use this toolbar
};

// ============================================================================
// IconBarHost - Manages icon bars in the main form
// ============================================================================
class IconBarHost {
public:
    IconBarHost();
    ~IconBarHost();

    // Initialization
    void SetMainFrame(MainFrame* frame) { main_frame_ = frame; }
    MainFrame* GetMainFrame() const { return main_frame_; }

    // Toolbar registration
    void RegisterToolBar(DraggableToolBar* toolbar, const ToolBarInfo& info);
    void UnregisterToolBar(const wxString& name);
    DraggableToolBar* GetToolBar(const wxString& name) const;
    std::vector<DraggableToolBar*> GetAllToolBars() const;
    bool HasToolBar(const wxString& name) const;

    // Toolbar visibility (docked only)
    void ShowToolBar(const wxString& name);
    void HideToolBar(const wxString& name);
    void ToggleToolBar(const wxString& name);
    bool IsToolBarVisible(const wxString& name) const;

    // Context-sensitive toolbar management
    void ShowFormToolBars(const std::vector<wxString>& bar_names);
    void HideAllFormToolBars();
    void ShowOnlyToolBars(const std::vector<wxString>& bar_names);

    // Floating/Docking operations
    void FloatToolBar(const wxString& name, const wxPoint& pos = wxDefaultPosition);
    void DockToolBar(const wxString& name);
    void DockAllToolBars();
    void FloatAllToolBars();
    bool IsToolBarFloating(const wxString& name) const;

    // Toolbar callbacks (called by DraggableToolBar)
    void OnToolBarFloated(DraggableToolBar* toolbar);
    void OnToolBarDocked(DraggableToolBar* toolbar);

    // Floating form management
    void FloatForm(DockableForm* form, const wxPoint& pos = wxDefaultPosition);
    void DockForm(DockableForm* form);
    void DockAllForms();
    bool IsFormFloating(DockableForm* form) const;
    FloatingFrame* GetFloatingFrame(DockableForm* form) const;

    // Menu integration
    void BuildViewMenu(wxMenu* menu);
    void UpdateToolBarMenuItems(wxMenu* menu);
    void OnToolBarMenuItem(wxCommandEvent& event);

    // Layout presets
    enum class LayoutPreset {
        Default,      // Standard layout with all toolbars docked
        Minimal,      // Minimal toolbars
        Developer,    // Developer-focused layout
        DataAnalyst,  // Data analysis layout
        DBA,          // Database administrator layout
        Custom
    };
    
    void ApplyLayoutPreset(LayoutPreset preset);
    void SaveCurrentLayout(const wxString& name);
    void LoadLayout(const wxString& name);
    std::vector<wxString> GetSavedLayoutNames() const;

    // State persistence
    void SaveState();
    void RestoreState();
    void ResetToDefaults();

    // Form activation handling
    void OnFormActivated(DockableForm* form);
    void OnFormDeactivated(DockableForm* form);

protected:
    // Helper methods
    void CreateDefaultToolBars();
    void LayoutDockedToolBars();
    void UpdateMenuCheckmarks();
    void SaveLayoutToConfig(const wxString& name);
    bool LoadLayoutFromConfig(const wxString& name);

private:
    MainFrame* main_frame_ = nullptr;
    
    // Registered toolbars
    std::map<wxString, DraggableToolBar*> toolbars_;
    std::map<wxString, ToolBarInfo> toolbar_info_;
    std::vector<wxString> toolbar_order_;
    
    // Floating frames for forms
    std::map<DockableForm*, FloatingFrame*> floating_forms_;
    
    // Currently visible form-specific toolbars
    std::vector<wxString> active_form_toolbars_;
    
    // Menu ID to toolbar name mapping
    std::map<int, wxString> menu_id_to_toolbar_;
    
    // Layout storage
    std::map<wxString, std::string> saved_layouts_;
};

// ============================================================================
// Menu IDs for toolbar management
// ============================================================================
constexpr int ID_TOOLBAR_BASE = wxID_HIGHEST + 1000;
constexpr int ID_TOOLBAR_MAX = wxID_HIGHEST + 1100;
constexpr int ID_MENU_DOCK_ALL_WINDOWS = wxID_HIGHEST + 1101;
constexpr int ID_MENU_FLOAT_ALL_WINDOWS = wxID_HIGHEST + 1102;
// Note: ID_MENU_RESET_LAYOUT is defined in menu_ids.h (wxID_HIGHEST + 807)
constexpr int ID_MENU_LAYOUT_PRESETS_BASE = wxID_HIGHEST + 1110;
constexpr int ID_MENU_LAYOUT_PRESETS_MAX = wxID_HIGHEST + 1120;

} // namespace scratchrobin

#endif // SCRATCHROBIN_ICON_BAR_HOST_H
