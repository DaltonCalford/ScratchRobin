/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_LAYOUT_DOCKABLE_WINDOW_H
#define SCRATCHROBIN_LAYOUT_DOCKABLE_WINDOW_H

#include <wx/window.h>
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/aui/aui.h>
#include <wx/panel.h>
#include <string>
#include <memory>

namespace scratchrobin {

// Forward declarations
class LayoutManager;

// Window dock directions
enum class DockDirection {
    Left,
    Right,
    Top,
    Bottom,
    Center,
    Floating
};

// Window state flags (renamed to avoid conflict with session_state.h)
enum class DockableWindowState {
    Visible = 1 << 0,
    Docked = 1 << 1,
    Active = 1 << 2,
    Modified = 1 << 3
};

// Base interface for all dockable windows
class IDockableWindow {
public:
    virtual ~IDockableWindow() = default;
    
    // Identity
    virtual std::string GetWindowId() const = 0;
    virtual std::string GetWindowTitle() const = 0;
    virtual std::string GetWindowType() const = 0;
    
    // Docking capabilities
    virtual bool CanDock() const { return true; }
    virtual bool CanFloat() const { return true; }
    virtual bool CanClose() const { return true; }
    
    // Menu/Toolbar integration
    virtual wxMenuBar* GetMenuBar() { return nullptr; }
    virtual wxToolBar* GetToolBar() { return nullptr; }
    virtual wxMenu* GetContextMenu() { return nullptr; }
    
    // Lifecycle
    virtual void OnActivate() {}
    virtual void OnDeactivate() {}
    virtual bool OnCloseRequest() { return true; }
    
    // State
    virtual bool IsModified() const { return false; }
    virtual bool Save() { return true; }
    virtual bool SaveAs() { return true; }
    
    // Window access
    virtual wxWindow* GetWindow() = 0;
    virtual const wxWindow* GetWindow() const = 0;
};

// Interface for navigator panels
class INavigatorWindow : public IDockableWindow {
public:
    virtual ~INavigatorWindow() = default;
    
    // Navigator-specific
    virtual void RefreshContent() = 0;
    virtual void SetFilter(const std::string& filter) = 0;
    virtual std::string GetSelectedPath() const = 0;
    virtual void SelectPath(const std::string& path) = 0;
};

// Interface for document/editor windows
class IDocumentWindow : public IDockableWindow {
public:
    virtual ~IDocumentWindow() = default;
    
    // Document identity
    virtual std::string GetDocumentType() const = 0;
    virtual std::string GetDocumentPath() const { return ""; }
    
    // Document operations
    virtual bool Load(const std::string& path) = 0;
    virtual bool Reload() = 0;
    
    // Tab interface
    void SetTabWindow(wxWindow* tab) { tab_window_ = tab; }
    wxWindow* GetTabWindow() const { return tab_window_; }
    
    // Frame access for document manager
    virtual wxFrame* GetFrame() { return nullptr; }
    virtual wxWindow* GetContent() { return GetWindow(); }
    
private:
    wxWindow* tab_window_ = nullptr;
};

// Base implementation for dockable forms
class DockableFormBase : public wxPanel, public IDockableWindow {
public:
    DockableFormBase(wxWindow* parent, const std::string& id, const std::string& title);
    
    // IDockableWindow implementation
    std::string GetWindowId() const override { return window_id_; }
    std::string GetWindowTitle() const override { return window_title_; }
    wxWindow* GetWindow() override { return this; }
    const wxWindow* GetWindow() const override { return this; }
    
    // Activation
    void Activate();
    void Deactivate();
    bool IsActive() const { return is_active_; }
    
protected:
    virtual void OnActivated() {}
    virtual void OnDeactivated() {}
    
    std::string window_id_;
    std::string window_title_;
    bool is_active_ = false;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_LAYOUT_DOCKABLE_WINDOW_H
