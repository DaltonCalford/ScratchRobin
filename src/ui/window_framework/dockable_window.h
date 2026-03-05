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

#include <string>
#include <vector>
#include <functional>
#include <wx/panel.h>
#include <wx/aui/aui.h>
#include "ui/window_framework/window_types.h"

namespace scratchrobin::ui {

// Forward declarations
class WindowManager;

// Base class for all dockable windows
class DockableWindow : public wxPanel {
public:
    DockableWindow(wxWindow* parent, const std::string& type_id, 
                   const std::string& instance_id);
    virtual ~DockableWindow();
    
    // Type information
    const std::string& getTypeId() const { return type_id_; }
    const std::string& getInstanceId() const { return instance_id_; }
    const WindowTypeDefinition* getTypeDef() const { return type_def_; }
    WindowCategory getCategory() const;
    
    // Window state
    const WindowState& getState() const { return state_; }
    void setState(const WindowState& state);
    bool isFloating() const { return state_.is_floating; }
    bool isDocked() const { return !state_.is_floating; }
    bool isActive() const { return state_.is_active; }
    
    // Title
    virtual void setTitle(const std::string& title);
    const std::string& getTitle() const { return title_; }
    
    // Menu contributions - override to add menu items
    virtual std::vector<MenuContribution> getMenuContributions();
    
    // Activation
    virtual void onActivated();   // Called when window becomes active
    virtual void onDeactivated(); // Called when window loses active status
    
    // Docking operations
    void floatWindow();
    void dockWindow(wxDirection direction);
    void dockTo(DockableWindow* target, wxDirection direction);
    void tabWith(DockableWindow* target);
    
    // Drag support
    void startDrag();
    bool isDragging() const { return is_dragging_; }
    
    // Size behavior
    SizeBehavior getSizeBehavior() const;
    virtual wxSize getDefaultSize() const;
    virtual wxSize getMinSize() const;
    virtual wxSize getMaxSize() const;
    
    // Capability checks
    bool canDock(wxDirection direction) const;
    bool canFloat() const;
    bool canClose() const;
    
    // Window manager access
    WindowManager* getWindowManager();
    
    // AUI pane info
    wxAuiPaneInfo* getPaneInfo();
    void updatePaneInfo();
    
    // Save/restore state
    virtual void saveState();
    virtual void restoreState();

protected:
    // Override in derived classes
    virtual void onWindowCreated() {}  // Called after construction
    virtual void onWindowClosing() {}  // Called before destruction
    virtual bool onWindowClosingQuery() { return true; }  // Return false to prevent close
    
    // Event handlers
    void onActivate(wxActivateEvent& event);
    void onClose(wxCloseEvent& event);
    void onMouseLeftDown(wxMouseEvent& event);
    void onMouseMove(wxMouseEvent& event);
    void onMouseLeftUp(wxMouseEvent& event);
    void onKeyDown(wxKeyEvent& event);
    
    // Helper to get drag start area (caption bar area)
    virtual bool isInDragArea(const wxPoint& pos) const;
    
    // Setup caption/title bar for dragging
    virtual void createCaptionBar();
    
private:
    std::string type_id_;
    std::string instance_id_;
    std::string title_;
    const WindowTypeDefinition* type_def_{nullptr};
    WindowState state_;
    
    bool is_dragging_{false};
    wxPoint drag_start_pos_;
    wxPoint drag_start_mouse_pos_;
    
    // Caption bar for non-AUI drag
    wxWindow* caption_bar_{nullptr};
    
    DECLARE_EVENT_TABLE()
};

// Helper macros for creating dockable window classes
#define DECLARE_DOCKABLE_WINDOW() \
    public: \
        static const char* getStaticTypeId() { return type_id_; } \
        static const WindowTypeDefinition& getStaticTypeDef(); \
    private: \
        static const char* type_id_; \
        static const WindowTypeDefinition type_def_;

#define IMPLEMENT_DOCKABLE_WINDOW(ClassName, TypeId) \
    const char* ClassName::type_id_ = TypeId; \
    const WindowTypeDefinition ClassName::type_def_ = []() { \
        WindowTypeDefinition def; \
        def.type_id = TypeId; \
        return def; \
    }();

}  // namespace scratchrobin::ui
