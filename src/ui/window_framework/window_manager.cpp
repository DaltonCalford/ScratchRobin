/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/window_manager.h"
#include "ui/window_framework/dockable_window.h"
#include "ui/window_framework/dynamic_menu_bar.h"
#include "ui/window_framework/docking_overlay.h"

namespace scratchrobin::ui {

// Global window manager instance
static WindowManager* g_window_manager = nullptr;

WindowManager* getWindowManager() {
    return g_window_manager;
}

void setWindowManager(WindowManager* mgr) {
    g_window_manager = mgr;
}

WindowManager::WindowManager(wxFrame* main_frame)
    : main_frame_(main_frame) {
    setWindowManager(this);
}

WindowManager::~WindowManager() {
    if (aui_manager_) {
        aui_manager_->UnInit();
        delete aui_manager_;
    }
    setWindowManager(nullptr);
}

void WindowManager::initialize() {
    // Create AUI manager
    aui_manager_ = new wxAuiManager(main_frame_);
    
    // Create default menu bar
    createDefaultMenuBar();
    
    // Create drop overlay
    drop_overlay_ = new DockingOverlay(main_frame_);
    
    // Create dynamic menu bar
    menu_bar_ = new DynamicMenuBar(main_frame_);
    menu_bar_->initialize();
}

void WindowManager::createDefaultMenuBar() {
    // Create standard menus for when no window is active
    default_menu_bar_ = new wxMenuBar();
    
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit the application");
    default_menu_bar_->Append(fileMenu, "&File");
    
    wxMenu* editMenu = new wxMenu();
    default_menu_bar_->Append(editMenu, "&Edit");
    
    wxMenu* viewMenu = new wxMenu();
    default_menu_bar_->Append(viewMenu, "&View");
    
    wxMenu* windowMenu = new wxMenu();
    default_menu_bar_->Append(windowMenu, "&Window");
    
    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, "&About", "About this application");
    default_menu_bar_->Append(helpMenu, "&Help");
    
    main_frame_->SetMenuBar(default_menu_bar_);
}

DockableWindow* WindowManager::createWindow(const WindowCreateParams& params) {
    const WindowTypeDefinition* type_def = WindowTypeRegistry::get().getType(params.type_id);
    if (!type_def) {
        return nullptr;
    }
    
    std::string instance_id = params.instance_id;
    if (instance_id.empty()) {
        instance_id = generateInstanceId(params.type_id);
    }
    
    // Create the window (actual creation depends on type)
    DockableWindow* window = nullptr;
    wxWindow* parent = params.parent ? params.parent : main_frame_;
    
    // TODO: Factory pattern for creating specific window types
    // For now, create generic dockable window
    window = new DockableWindow(parent, params.type_id, instance_id);
    
    if (!window) {
        return nullptr;
    }
    
    // Set title
    if (!params.title.empty()) {
        window->setTitle(params.title);
    } else {
        window->setTitle(type_def->display_name);
    }
    
    // Create AUI pane info
    wxAuiPaneInfo pane_info = createPaneInfo(params, type_def);
    
    // Add to AUI manager
    aui_manager_->AddPane(window, pane_info);
    aui_manager_->Update();
    
    // Show if requested
    if (params.activate) {
        window->Show();
        activateWindow(instance_id);
    }
    
    return window;
}

DockableWindow* WindowManager::createWindow(const std::string& type_id, const std::string& title) {
    WindowCreateParams params;
    params.type_id = type_id;
    params.title = title;
    return createWindow(params);
}

wxAuiPaneInfo WindowManager::createPaneInfo(const WindowCreateParams& params,
                                             const WindowTypeDefinition* type_def) {
    wxAuiPaneInfo info;
    
    info.Name(wxString::FromUTF8(params.instance_id.c_str()));
    info.Caption(wxString::FromUTF8(params.title.c_str()));
    
    // Set docking capabilities
    if (hasCapability(type_def->dock_capabilities, DockCapability::kLeft)) {
        info.LeftDockable(true);
    }
    if (hasCapability(type_def->dock_capabilities, DockCapability::kRight)) {
        info.RightDockable(true);
    }
    if (hasCapability(type_def->dock_capabilities, DockCapability::kTop)) {
        info.TopDockable(true);
    }
    if (hasCapability(type_def->dock_capabilities, DockCapability::kBottom)) {
        info.BottomDockable(true);
    }
    if (hasCapability(type_def->dock_capabilities, DockCapability::kFloat)) {
        info.Floatable(true);
    }
    
    // Set default dock position
    if (params.start_floating) {
        info.Float();
        if (!params.float_rect.IsEmpty()) {
            info.FloatingPosition(params.float_rect.GetPosition());
            info.FloatingSize(params.float_rect.GetSize());
        }
    } else {
        switch (type_def->default_dock) {
            case wxLEFT: info.Left(); break;
            case wxRIGHT: info.Right(); break;
            case wxTOP: info.Top(); break;
            case wxBOTTOM: info.Bottom(); break;
            case wxCENTER: info.Center(); break;
            default: break;
        }
    }
    
    // Layer and position
    switch (type_def->category) {
        case WindowCategory::kIconBar:
            info.ToolbarPane();
            break;
        case WindowCategory::kNavigator:
        case WindowCategory::kInspector:
            info.Layer(1);
            break;
        case WindowCategory::kEditor:
            info.Layer(0);
            info.CenterPane();
            break;
        case WindowCategory::kOutput:
            info.Layer(1);
            break;
        default:
            break;
    }
    
    // Size
    info.MinSize(type_def->min_size);
    info.MaxSize(type_def->max_size);
    info.BestSize(type_def->default_size);
    
    // Close button
    if (type_def->can_close) {
        info.CloseButton(true);
    } else {
        info.CloseButton(false);
    }
    
    // Pin button for auto-hide
    if (hasCapability(type_def->dock_capabilities, DockCapability::kAutoHide)) {
        info.PinButton(true);
    }
    
    return info;
}

std::string WindowManager::generateInstanceId(const std::string& type_id) {
    static int counter = 0;
    return type_id + "_" + std::to_string(++counter);
}

void WindowManager::registerWindow(DockableWindow* window) {
    if (window) {
        windows_[window->getInstanceId()] = window;
    }
}

void WindowManager::unregisterWindow(DockableWindow* window) {
    if (window) {
        windows_.erase(window->getInstanceId());
        if (active_window_ == window) {
            active_window_ = nullptr;
            updateMenuBar();
        }
    }
}

DockableWindow* WindowManager::findWindow(const std::string& instance_id) {
    auto it = windows_.find(instance_id);
    if (it != windows_.end()) {
        return it->second;
    }
    return nullptr;
}

DockableWindow* WindowManager::findWindowByType(const std::string& type_id) {
    for (auto& [id, window] : windows_) {
        if (window->getTypeId() == type_id) {
            return window;
        }
    }
    return nullptr;
}

std::vector<DockableWindow*> WindowManager::getWindowsByType(const std::string& type_id) {
    std::vector<DockableWindow*> result;
    for (auto& [id, window] : windows_) {
        if (window->getTypeId() == type_id) {
            result.push_back(window);
        }
    }
    return result;
}

std::vector<DockableWindow*> WindowManager::getAllWindows() {
    std::vector<DockableWindow*> result;
    for (auto& [id, window] : windows_) {
        result.push_back(window);
    }
    return result;
}

DockableWindow* WindowManager::getActiveWindow() {
    return active_window_;
}

void WindowManager::activateWindow(const std::string& instance_id) {
    DockableWindow* window = findWindow(instance_id);
    if (window) {
        onWindowActivated(window);
        
        wxAuiPaneInfo& pane = aui_manager_->GetPane(window);
        if (pane.IsOk()) {
            pane.Show();
            aui_manager_->Update();
        }
    }
}

void WindowManager::showWindow(const std::string& instance_id) {
    DockableWindow* window = findWindow(instance_id);
    if (window) {
        window->Show();
        wxAuiPaneInfo& pane = aui_manager_->GetPane(window);
        if (pane.IsOk()) {
            pane.Show();
            aui_manager_->Update();
        }
    }
}

void WindowManager::hideWindow(const std::string& instance_id) {
    DockableWindow* window = findWindow(instance_id);
    if (window) {
        wxAuiPaneInfo& pane = aui_manager_->GetPane(window);
        if (pane.IsOk()) {
            pane.Hide();
            aui_manager_->Update();
        }
    }
}

void WindowManager::closeWindow(const std::string& instance_id) {
    DockableWindow* window = findWindow(instance_id);
    if (window) {
        wxAuiPaneInfo& pane = aui_manager_->GetPane(window);
        if (pane.IsOk()) {
            aui_manager_->DetachPane(window);
            aui_manager_->Update();
        }
        window->Destroy();
    }
}

void WindowManager::floatWindow(const std::string& instance_id) {
    DockableWindow* window = findWindow(instance_id);
    if (window) {
        wxAuiPaneInfo& pane = aui_manager_->GetPane(window);
        if (pane.IsOk()) {
            pane.Float();
            aui_manager_->Update();
        }
    }
}

void WindowManager::dockWindow(const std::string& instance_id, wxDirection direction) {
    DockableWindow* window = findWindow(instance_id);
    if (window) {
        wxAuiPaneInfo& pane = aui_manager_->GetPane(window);
        if (pane.IsOk()) {
            switch (direction) {
                case wxLEFT: pane.Left(); break;
                case wxRIGHT: pane.Right(); break;
                case wxTOP: pane.Top(); break;
                case wxBOTTOM: pane.Bottom(); break;
                case wxCENTER: pane.Center(); break;
                default: break;
            }
            aui_manager_->Update();
        }
    }
}

void WindowManager::onWindowActivated(DockableWindow* window) {
    if (active_window_ != window) {
        if (active_window_) {
            active_window_->onDeactivated();
        }
        active_window_ = window;
        if (window) {
            window->onActivated();
        }
        updateMenuBar();
    }
}

void WindowManager::onWindowClosed(DockableWindow* window) {
    if (active_window_ == window) {
        active_window_ = nullptr;
        updateMenuBar();
    }
}

void WindowManager::updateMenuBar() {
    if (menu_bar_) {
        if (active_window_) {
            menu_bar_->setActiveWindow(active_window_);
        } else {
            menu_bar_->clearActiveWindow();
        }
    }
}

// Drag/drop operations
void WindowManager::startDrag(DockableWindow* window) {
    drag_source_ = window;
    // Show drop zones on potential targets
    showDropZones();
}

void WindowManager::updateDrag(const wxPoint& screen_pos) {
    // Update drop zone highlighting based on mouse position
    int zone = hitTestDropZone(screen_pos);
    if (drop_overlay_) {
        drop_overlay_->setHoverZone(zone);
    }
}

void WindowManager::endDrag(bool accept) {
    if (accept && drag_source_) {
        // Perform the dock operation
        int zone = drop_overlay_->getHoverZone();
        // Find target window under mouse
        // dockWindowToTarget(drag_source_, target, static_cast<wxDirection>(zone));
    }
    hideDropZones();
    drag_source_ = nullptr;
}

int WindowManager::hitTestDropZone(const wxPoint& screen_pos) {
    // Convert screen pos to client and hit test
    return -1;  // Center/tab
}

void WindowManager::showDropZones() {
    if (drop_overlay_) {
        // Show overlay on main frame
        wxRect rect = main_frame_->GetClientRect();
        drop_overlay_->show(rect, DockCapability::kLeft | DockCapability::kRight | 
                            DockCapability::kTop | DockCapability::kBottom);
    }
}

void WindowManager::hideDropZones() {
    if (drop_overlay_) {
        drop_overlay_->hide();
    }
}

// Layout persistence
void WindowManager::saveLayout(const std::string& name) {
    if (aui_manager_) {
        wxString perspective = aui_manager_->SavePerspective();
        saved_layouts_[name] = perspective;
    }
}

bool WindowManager::loadLayout(const std::string& name) {
    auto it = saved_layouts_.find(name);
    if (it != saved_layouts_.end() && aui_manager_) {
        aui_manager_->LoadPerspective(it->second);
        return true;
    }
    return false;
}

std::vector<std::string> WindowManager::getSavedLayouts() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : saved_layouts_) {
        result.push_back(name);
    }
    return result;
}

void WindowManager::resetLayout() {
    if (aui_manager_) {
        // Reset to default layout
        aui_manager_->Update();
    }
}

}  // namespace scratchrobin::ui
