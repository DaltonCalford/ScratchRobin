/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/dockable_window.h"
#include "ui/window_framework/window_manager.h"
#include <wx/aui/aui.h>

namespace scratchrobin::ui {

BEGIN_EVENT_TABLE(DockableWindow, wxPanel)
    EVT_ACTIVATE(DockableWindow::onActivate)
END_EVENT_TABLE()

DockableWindow::DockableWindow(wxWindow* parent, const std::string& type_id,
                               const std::string& instance_id)
    : wxPanel(parent, wxID_ANY)
    , type_id_(type_id)
    , instance_id_(instance_id)
    , title_(type_id) {
    
    type_def_ = WindowTypeRegistry::get().getType(type_id);
    
    // Register with window manager
    WindowManager* mgr = getWindowManager();
    if (mgr) {
        mgr->registerWindow(this);
    }
    
    onWindowCreated();
}

DockableWindow::~DockableWindow() {
    onWindowClosing();
    
    // Unregister from window manager
    WindowManager* mgr = getWindowManager();
    if (mgr) {
        mgr->unregisterWindow(this);
    }
}

WindowCategory DockableWindow::getCategory() const {
    if (type_def_) {
        return type_def_->category;
    }
    return WindowCategory::kDialog;
}

void DockableWindow::setTitle(const std::string& title) {
    title_ = title;
    updatePaneInfo();
}

std::vector<MenuContribution> DockableWindow::getMenuContributions() {
    return {};
}

void DockableWindow::onActivated() {
    state_.is_active = true;
}

void DockableWindow::onDeactivated() {
    state_.is_active = false;
}

void DockableWindow::floatWindow() {
    WindowManager* mgr = getWindowManager();
    if (mgr) {
        mgr->floatWindow(instance_id_);
    }
}

void DockableWindow::dockWindow(wxDirection direction) {
    WindowManager* mgr = getWindowManager();
    if (mgr) {
        mgr->dockWindow(instance_id_, direction);
    }
}

void DockableWindow::dockTo(DockableWindow* target, wxDirection direction) {
    // Implementation in WindowManager
}

void DockableWindow::tabWith(DockableWindow* target) {
    // Implementation in WindowManager
}

void DockableWindow::startDrag() {
    WindowManager* mgr = getWindowManager();
    if (mgr) {
        mgr->startDrag(this);
    }
}

SizeBehavior DockableWindow::getSizeBehavior() const {
    if (type_def_) {
        return type_def_->size_behavior;
    }
    return SizeBehavior::kFixed;
}

wxSize DockableWindow::getDefaultSize() const {
    if (type_def_) {
        return type_def_->default_size;
    }
    return wxSize(400, 300);
}

wxSize DockableWindow::getMinSize() const {
    if (type_def_) {
        return type_def_->min_size;
    }
    return wxSize(100, 50);
}

wxSize DockableWindow::getMaxSize() const {
    if (type_def_) {
        return type_def_->max_size;
    }
    return wxSize(wxDefaultCoord, wxDefaultCoord);
}

bool DockableWindow::canDock(wxDirection direction) const {
    if (!type_def_) return false;
    
    switch (direction) {
        case wxLEFT: return hasCapability(type_def_->dock_capabilities, DockCapability::kLeft);
        case wxRIGHT: return hasCapability(type_def_->dock_capabilities, DockCapability::kRight);
        case wxTOP: return hasCapability(type_def_->dock_capabilities, DockCapability::kTop);
        case wxBOTTOM: return hasCapability(type_def_->dock_capabilities, DockCapability::kBottom);
        case wxCENTER: return hasCapability(type_def_->dock_capabilities, DockCapability::kTab);
        default: return false;
    }
}

bool DockableWindow::canFloat() const {
    if (type_def_) {
        return hasCapability(type_def_->dock_capabilities, DockCapability::kFloat);
    }
    return false;
}

bool DockableWindow::canClose() const {
    if (type_def_) {
        return type_def_->can_close;
    }
    return true;
}

WindowManager* DockableWindow::getWindowManager() {
    // Call the global getWindowManager() function
    // Use explicit namespace qualification to avoid calling this same method
    return scratchrobin::ui::getWindowManager();
}

wxAuiPaneInfo* DockableWindow::getPaneInfo() {
    WindowManager* mgr = getWindowManager();
    if (mgr && mgr->getAuiManager()) {
        return &mgr->getAuiManager()->GetPane(this);
    }
    return nullptr;
}

void DockableWindow::updatePaneInfo() {
    WindowManager* mgr = getWindowManager();
    if (mgr && mgr->getAuiManager()) {
        wxAuiPaneInfo& pane = mgr->getAuiManager()->GetPane(this);
        pane.Caption(wxString::FromUTF8(title_.c_str()));
        mgr->getAuiManager()->Update();
    }
}

void DockableWindow::saveState() {
    // Persist window state to config
}

void DockableWindow::restoreState() {
    // Restore window state from config
}

void DockableWindow::onActivate(wxActivateEvent& event) {
    if (event.GetActive()) {
        onActivated();
        WindowManager* mgr = getWindowManager();
        if (mgr) {
            mgr->onWindowActivated(this);
        }
    } else {
        onDeactivated();
    }
    event.Skip();
}

void DockableWindow::onClose(wxCloseEvent& event) {
    if (!onWindowClosingQuery()) {
        event.Veto();
        return;
    }
    event.Skip();
}

bool DockableWindow::isInDragArea(const wxPoint& pos) const {
    // Check if position is in caption/drag area
    return false;
}

void DockableWindow::createCaptionBar() {
    // Create custom caption bar for non-AUI drag
}

}  // namespace scratchrobin::ui
