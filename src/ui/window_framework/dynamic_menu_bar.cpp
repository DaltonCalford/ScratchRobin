/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/dynamic_menu_bar.h"
#include "ui/window_framework/dockable_window.h"
#include "ui/window_framework/window_manager.h"

namespace scratchrobin::ui {

// No event table needed - we use Bind() for dynamic handlers

DynamicMenuBar::DynamicMenuBar(wxFrame* main_frame)
    : main_frame_(main_frame) {
}

DynamicMenuBar::~DynamicMenuBar() {
    // Cleanup menus
}

void DynamicMenuBar::initialize() {
    // Create the physical menu bar
    menu_bar_ = new wxMenuBar();
    
    // Create standard menus
    file_menu_ = new wxMenu();
    edit_menu_ = new wxMenu();
    view_menu_ = new wxMenu();
    window_menu_ = new wxMenu();
    help_menu_ = new wxMenu();
    
    // Add to menu bar
    menu_bar_->Append(file_menu_, "&File");
    menu_bar_->Append(edit_menu_, "&Edit");
    menu_bar_->Append(view_menu_, "&View");
    menu_bar_->Append(window_menu_, "&Window");
    menu_bar_->Append(help_menu_, "&Help");
    
    // Build initial window menu
    buildWindowMenu();
    
    // Set on main frame
    main_frame_->SetMenuBar(menu_bar_);
}

void DynamicMenuBar::setActiveWindow(DockableWindow* window) {
    clearContributions();
    
    if (window) {
        // Get contributions from window
        std::vector<MenuContribution> contributions = window->getMenuContributions();
        addContributions(contributions);
    }
    
    updateMenuBar();
}

void DynamicMenuBar::clearActiveWindow() {
    clearContributions();
    updateMenuBar();
}

void DynamicMenuBar::addContributions(const std::vector<MenuContribution>& contributions) {
    for (const auto& contrib : contributions) {
        current_contributions_.push_back(contrib);
    }
}

void DynamicMenuBar::removeContributions(const std::vector<MenuContribution>& contributions) {
    // Remove matching contributions
    for (const auto& to_remove : contributions) {
        auto it = std::remove_if(current_contributions_.begin(), 
                                  current_contributions_.end(),
                                  [&to_remove](const MenuContribution& c) {
                                      return c.item_id == to_remove.item_id;
                                  });
        current_contributions_.erase(it, current_contributions_.end());
    }
}

void DynamicMenuBar::clearContributions() {
    current_contributions_.clear();
    callbacks_.clear();
}

void DynamicMenuBar::updateMenuBar() {
    // Clear dynamic menus - recreate File menu from scratch
    for (const auto& [name, menu] : dynamic_menus_) {
        delete menu;
    }
    dynamic_menus_.clear();
    
    // Remove old menu bar and create new one to avoid issues
    menu_bar_->Remove(0);  // Remove File menu
    delete file_menu_;
    
    // Rebuild File menu
    file_menu_ = new wxMenu();
    file_menu_->Append(wxID_NEW, "&New...\tCtrl+N");
    file_menu_->Append(wxID_OPEN, "&Open...\tCtrl+O");
    file_menu_->AppendSeparator();
    file_menu_->Append(wxID_SAVE, "&Save\tCtrl+S");
    file_menu_->Append(wxID_SAVEAS, "Save &As...\tCtrl+Shift+S");
    file_menu_->AppendSeparator();
    file_menu_->Append(wxID_EXIT, "E&xit\tAlt+F4");
    menu_bar_->Insert(0, file_menu_, "&File");
    
    // Add window contributions to appropriate menus
    for (const auto& contrib : current_contributions_) {
        wxMenu* menu = getOrCreateMenu(contrib.menu_name);
        if (menu) {
            int id = contrib.item_id ? contrib.item_id : next_menu_id_++;
            callbacks_[id] = contrib.callback;
            
            if (contrib.is_separator) {
                menu->AppendSeparator();
            } else {
                wxString label = wxString::FromUTF8(contrib.item_label.c_str());
                if (!contrib.accelerator.empty()) {
                    label += "\t" + wxString::FromUTF8(contrib.accelerator.c_str());
                }
                
                if (contrib.is_checkable) {
                    menu->AppendCheckItem(id, label, 
                        wxString::FromUTF8(contrib.item_help.c_str()));
                    menu->Check(id, contrib.is_checked);
                } else {
                    menu->Append(id, label, 
                        wxString::FromUTF8(contrib.item_help.c_str()));
                }
                
                menu->Bind(wxEVT_MENU, &DynamicMenuBar::onMenuItemSelected, this, id);
            }
        }
    }
    
    // Rebuild Window menu
    buildWindowMenu();
    
    // Refresh menu bar
    menu_bar_->Refresh();
}

wxMenu* DynamicMenuBar::getOrCreateMenu(const std::string& name, int position) {
    // Check if menu already exists
    wxMenu* existing = findMenu(name);
    if (existing) {
        return existing;
    }
    
    // Create new menu
    wxMenu* menu = new wxMenu();
    dynamic_menus_[name] = menu;
    
    wxString menuLabel = "&" + wxString::FromUTF8(name.c_str());
    if (position >= 0) {
        menu_bar_->Insert(position, menu, menuLabel);
    } else {
        // Insert before Window and Help
        int insertPos = menu_bar_->FindMenu("Window");
        if (insertPos == wxNOT_FOUND) {
            insertPos = menu_bar_->GetMenuCount() - 1;
        }
        menu_bar_->Insert(insertPos, menu, menuLabel);
    }
    
    return menu;
}

void DynamicMenuBar::removeMenu(const std::string& name) {
    int index = findMenuIndex(name);
    if (index != wxNOT_FOUND) {
        menu_bar_->Remove(index);
        dynamic_menus_.erase(name);
    }
}

wxMenu* DynamicMenuBar::findMenu(const std::string& name) {
    // Check standard menus
    if (name == "File") return file_menu_;
    if (name == "Edit") return edit_menu_;
    if (name == "View") return view_menu_;
    if (name == "Window") return window_menu_;
    if (name == "Help") return help_menu_;
    
    // Check dynamic menus
    auto it = dynamic_menus_.find(name);
    if (it != dynamic_menus_.end()) {
        return it->second;
    }
    
    return nullptr;
}

int DynamicMenuBar::findMenuIndex(const std::string& name) {
    wxString menuLabel = "&" + wxString::FromUTF8(name.c_str());
    for (size_t i = 0; i < menu_bar_->GetMenuCount(); ++i) {
        if (menu_bar_->GetMenuLabelText(i) == menuLabel) {
            return i;
        }
    }
    return wxNOT_FOUND;
}

void DynamicMenuBar::buildWindowMenu() {
    // Clear window menu - remove all items
    while (window_menu_->GetMenuItemCount() > 0) {
        window_menu_->Destroy(window_menu_->FindItemByPosition(0));
    }
    
    WindowManager* mgr = getWindowManager();
    
    if (mgr) {
        // Add window list
        auto windows = mgr->getAllWindows();
        for (auto* window : windows) {
            const WindowTypeDefinition* type_def = window->getTypeDef();
            if (type_def && type_def->show_in_window_menu) {
                wxString label = wxString::FromUTF8(window->getTitle().c_str());
                // TODO: Add checkmark for active window
                // TODO: Add click handler to activate window
                window_menu_->Append(wxID_ANY, label);
            }
        }
        
        if (!windows.empty()) {
            window_menu_->AppendSeparator();
        }
    }
    
    window_menu_->Append(wxID_ANY, "&Reset Layout");
    window_menu_->AppendSeparator();
    window_menu_->Append(wxID_ANY, "&New Window");
}

void DynamicMenuBar::enableItem(int id, bool enable) {
    menu_bar_->Enable(id, enable);
}

void DynamicMenuBar::checkItem(int id, bool check) {
    menu_bar_->Check(id, check);
}

void DynamicMenuBar::setItemLabel(int id, const wxString& label) {
    menu_bar_->SetLabel(id, label);
}

void DynamicMenuBar::onMenuItemSelected(wxCommandEvent& event) {
    int id = event.GetId();
    auto it = callbacks_.find(id);
    if (it != callbacks_.end() && it->second) {
        it->second();
    }
    event.Skip();
}

}  // namespace scratchrobin::ui
