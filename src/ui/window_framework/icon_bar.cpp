/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/icon_bar.h"
#include "ui/window_framework/window_types.h"

namespace scratchrobin::ui {

BEGIN_EVENT_TABLE(IconBar, DockableWindow)
END_EVENT_TABLE()

IconBar::IconBar(wxWindow* parent, const std::string& instance_id)
    : DockableWindow(parent, WindowTypes::kIconBar, instance_id) {
}

void IconBar::onWindowCreated() {
    // Create toolbar
    long style = wxTB_FLAT | wxTB_HORIZONTAL;
    toolbar_ = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, style);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar_, 1, wxEXPAND);
    SetSizer(sizer);
}

void IconBar::addIcon(const IconItem& item) {
    items_.push_back(item);
    
    if (item.is_separator) {
        toolbar_->AddSeparator();
    } else {
        if (item.bitmap.IsOk()) {
            toolbar_->AddTool(item.id, item.label, item.bitmap, item.tooltip, 
                             item.is_toggle ? wxITEM_CHECK : wxITEM_NORMAL);
        }
        toolbar_->Bind(wxEVT_TOOL, &IconBar::onToolClicked, this, item.id);
    }
    toolbar_->Realize();
}

void IconBar::addSeparator() {
    IconItem sep;
    sep.is_separator = true;
    addIcon(sep);
}

void IconBar::addStretch() {
    toolbar_->AddStretchableSpace();
}

void IconBar::setIconEnabled(int id, bool enabled) {
    toolbar_->EnableTool(id, enabled);
}

void IconBar::setIconChecked(int id, bool checked) {
    toolbar_->ToggleTool(id, checked);
}

void IconBar::setIconBitmap(int id, const wxBitmap& bitmap) {
    // Requires toolbar recreation or wxToolBarToolBase access
}

void IconBar::setIconTooltip(int id, const wxString& tooltip) {
    toolbar_->SetToolShortHelp(id, tooltip);
}

void IconBar::removeIcon(int id) {
    toolbar_->DeleteTool(id);
    items_.erase(std::remove_if(items_.begin(), items_.end(),
                                [id](const IconItem& item) { return item.id == id; }),
                 items_.end());
}

void IconBar::clearIcons() {
    toolbar_->ClearTools();
    items_.clear();
}

void IconBar::setOrientation(wxOrientation orient) {
    orientation_ = orient;
    long style = toolbar_->GetWindowStyle();
    
    // Remove orientation flags
    style &= ~(wxTB_HORIZONTAL | wxTB_VERTICAL);
    
    if (orient == wxHORIZONTAL) {
        style |= wxTB_HORIZONTAL;
    } else {
        style |= wxTB_VERTICAL;
    }
    
    toolbar_->SetWindowStyle(style);
    toolbar_->Realize();
}

std::vector<MenuContribution> IconBar::getMenuContributions() {
    // Icon bars typically don't contribute menus
    return {};
}

wxSize IconBar::getDefaultSize() const {
    if (orientation_ == wxHORIZONTAL) {
        return wxSize(600, 32);
    } else {
        return wxSize(32, 400);
    }
}

void IconBar::onToolClicked(wxCommandEvent& event) {
    int id = event.GetId();
    for (const auto& item : items_) {
        if (item.id == id && item.callback) {
            item.callback();
            break;
        }
    }
}

// Factory functions
IconBar* createMainToolbar(wxWindow* parent) {
    IconBar* bar = new IconBar(parent, "main_toolbar");
    
    // Add standard icons
    IconBar::IconItem newItem;
    newItem.id = wxID_NEW;
    newItem.label = "New";
    newItem.tooltip = "Create new file";
    bar->addIcon(newItem);
    
    IconBar::IconItem openItem;
    openItem.id = wxID_OPEN;
    openItem.label = "Open";
    openItem.tooltip = "Open file";
    bar->addIcon(openItem);
    
    bar->addSeparator();
    
    IconBar::IconItem saveItem;
    saveItem.id = wxID_SAVE;
    saveItem.label = "Save";
    saveItem.tooltip = "Save file";
    bar->addIcon(saveItem);
    
    return bar;
}

IconBar* createViewToolbar(wxWindow* parent) {
    IconBar* bar = new IconBar(parent, "view_toolbar");
    return bar;
}

IconBar* createQueryToolbar(wxWindow* parent) {
    IconBar* bar = new IconBar(parent, "query_toolbar");
    return bar;
}

}  // namespace scratchrobin::ui
