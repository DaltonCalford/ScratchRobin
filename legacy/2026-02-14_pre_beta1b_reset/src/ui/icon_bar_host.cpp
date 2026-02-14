/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "icon_bar_host.h"

#include "draggable_toolbar.h"
#include "floating_frame.h"
#include "dockable_form.h"
#include "main_frame.h"
#include "icon_bar.h"
#include "menu_ids.h"

#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/config.h>
#include <wx/frame.h>

namespace scratchrobin {

// ============================================================================
// IconBarHost Implementation
// ============================================================================
IconBarHost::IconBarHost() = default;

IconBarHost::~IconBarHost() {
    // Clean up floating forms
    for (auto& [form, frame] : floating_forms_) {
        if (frame) {
            frame->Destroy();
        }
    }
    floating_forms_.clear();
}

void IconBarHost::RegisterToolBar(DraggableToolBar* toolbar, const ToolBarInfo& info) {
    if (!toolbar) {
        return;
    }
    
    wxString name = info.name.IsEmpty() ? toolbar->GetBarName() : info.name;
    
    // Store the toolbar
    toolbars_[name] = toolbar;
    toolbar_info_[name] = info;
    
    // Add to order if not already present
    if (std::find(toolbar_order_.begin(), toolbar_order_.end(), name) == toolbar_order_.end()) {
        toolbar_order_.push_back(name);
    }
    
    // Set up the toolbar
    toolbar->SetIconBarHost(this);
    toolbar->SetBarName(name);
    
    // Set initial visibility
    if (!info.visible) {
        toolbar->Hide();
    }
}

void IconBarHost::UnregisterToolBar(const wxString& name) {
    auto it = toolbars_.find(name);
    if (it != toolbars_.end()) {
        // If floating, dock first
        if (it->second && it->second->IsFloating()) {
            DockToolBar(name);
        }
        
        toolbars_.erase(it);
        toolbar_info_.erase(name);
        
        // Remove from order
        auto order_it = std::find(toolbar_order_.begin(), toolbar_order_.end(), name);
        if (order_it != toolbar_order_.end()) {
            toolbar_order_.erase(order_it);
        }
    }
}

DraggableToolBar* IconBarHost::GetToolBar(const wxString& name) const {
    auto it = toolbars_.find(name);
    if (it != toolbars_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<DraggableToolBar*> IconBarHost::GetAllToolBars() const {
    std::vector<DraggableToolBar*> result;
    for (const auto& [name, toolbar] : toolbars_) {
        if (toolbar) {
            result.push_back(toolbar);
        }
    }
    return result;
}

bool IconBarHost::HasToolBar(const wxString& name) const {
    return toolbars_.find(name) != toolbars_.end();
}

void IconBarHost::ShowToolBar(const wxString& name) {
    auto* toolbar = GetToolBar(name);
    if (toolbar) {
        toolbar->Show();
        auto& info = toolbar_info_[name];
        info.visible = true;
    }
}

void IconBarHost::HideToolBar(const wxString& name) {
    auto* toolbar = GetToolBar(name);
    if (toolbar) {
        toolbar->Hide();
        auto& info = toolbar_info_[name];
        info.visible = false;
    }
}

void IconBarHost::ToggleToolBar(const wxString& name) {
    auto* toolbar = GetToolBar(name);
    if (toolbar) {
        if (toolbar->IsShown()) {
            HideToolBar(name);
        } else {
            ShowToolBar(name);
        }
    }
}

bool IconBarHost::IsToolBarVisible(const wxString& name) const {
    auto* toolbar = GetToolBar(name);
    if (toolbar) {
        return toolbar->IsShown();
    }
    return false;
}

void IconBarHost::ShowFormToolBars(const std::vector<wxString>& bar_names) {
    // Hide current form-specific toolbars
    for (const auto& name : active_form_toolbars_) {
        HideToolBar(name);
    }
    active_form_toolbars_.clear();
    
    // Show requested toolbars
    for (const auto& name : bar_names) {
        ShowToolBar(name);
        active_form_toolbars_.push_back(name);
    }
    
    LayoutDockedToolBars();
}

void IconBarHost::HideAllFormToolBars() {
    for (const auto& name : active_form_toolbars_) {
        HideToolBar(name);
    }
    active_form_toolbars_.clear();
    
    LayoutDockedToolBars();
}

void IconBarHost::ShowOnlyToolBars(const std::vector<wxString>& bar_names) {
    // Hide all toolbars first
    for (const auto& [name, toolbar] : toolbars_) {
        if (toolbar) {
            toolbar->Hide();
        }
    }
    
    // Show only requested toolbars
    for (const auto& name : bar_names) {
        ShowToolBar(name);
    }
    
    LayoutDockedToolBars();
}

void IconBarHost::FloatToolBar(const wxString& name, const wxPoint& pos) {
    auto* toolbar = GetToolBar(name);
    if (toolbar && !toolbar->IsFloating()) {
        toolbar->Float(pos);
    }
}

void IconBarHost::DockToolBar(const wxString& name) {
    auto* toolbar = GetToolBar(name);
    if (toolbar && toolbar->IsFloating()) {
        toolbar->Dock();
    }
}

void IconBarHost::DockAllToolBars() {
    for (const auto& [name, toolbar] : toolbars_) {
        if (toolbar && toolbar->IsFloating()) {
            toolbar->Dock();
        }
    }
}

void IconBarHost::FloatAllToolBars() {
    int offset = 0;
    for (const auto& [name, toolbar] : toolbars_) {
        if (toolbar && !toolbar->IsFloating()) {
            wxPoint pos = wxDefaultPosition;
            if (main_frame_) {
                pos = main_frame_->GetPosition() + wxPoint(50 + offset, 50 + offset);
            }
            toolbar->Float(pos);
            offset += 30;
        }
    }
}

bool IconBarHost::IsToolBarFloating(const wxString& name) const {
    auto* toolbar = GetToolBar(name);
    return toolbar && toolbar->IsFloating();
}

void IconBarHost::OnToolBarFloated(DraggableToolBar* toolbar) {
    if (!toolbar) {
        return;
    }
    
    // Update layout for remaining docked toolbars
    LayoutDockedToolBars();
    
    // Update menu checkmarks
    UpdateMenuCheckmarks();
}

void IconBarHost::OnToolBarDocked(DraggableToolBar* toolbar) {
    if (!toolbar) {
        return;
    }
    
    // Update layout
    LayoutDockedToolBars();
    
    // Update menu checkmarks
    UpdateMenuCheckmarks();
}

void IconBarHost::FloatForm(DockableForm* form, const wxPoint& pos) {
    if (!form || floating_forms_.find(form) != floating_forms_.end()) {
        return;
    }
    
    // Create floating frame
    auto* frame = new FloatingFrame(main_frame_, form->GetDocumentTitleWx());
    frame->SetContent(form);
    
    wxPoint floatPos = pos;
    if (floatPos == wxDefaultPosition && main_frame_) {
        floatPos = main_frame_->GetPosition() + wxPoint(100, 100);
    }
    frame->SetPosition(floatPos);
    frame->Show(true);
    
    floating_forms_[form] = frame;
}

void IconBarHost::DockForm(DockableForm* form) {
    if (!form) {
        return;
    }
    
    auto it = floating_forms_.find(form);
    if (it != floating_forms_.end()) {
        if (it->second) {
            it->second->ClearContent();
            it->second->Destroy();
        }
        floating_forms_.erase(it);
    }
}

void IconBarHost::DockAllForms() {
    // Copy the map since DockForm modifies it
    auto forms_copy = floating_forms_;
    for (auto& [form, frame] : forms_copy) {
        DockForm(form);
    }
}

bool IconBarHost::IsFormFloating(DockableForm* form) const {
    return floating_forms_.find(form) != floating_forms_.end();
}

FloatingFrame* IconBarHost::GetFloatingFrame(DockableForm* form) const {
    auto it = floating_forms_.find(form);
    if (it != floating_forms_.end()) {
        return it->second;
    }
    return nullptr;
}

void IconBarHost::BuildViewMenu(wxMenu* menu) {
    if (!menu) {
        return;
    }
    
    // Toolbars submenu
    auto* toolbarMenu = new wxMenu();
    
    int menuId = ID_TOOLBAR_BASE;
    for (const auto& name : toolbar_order_) {
        if (menuId >= ID_TOOLBAR_MAX) {
            break;
        }
        
        auto it = toolbar_info_.find(name);
        if (it != toolbar_info_.end()) {
            wxString label = it->second.label.IsEmpty() ? name : it->second.label;
            auto* item = toolbarMenu->AppendCheckItem(menuId, label);
            item->Check(IsToolBarVisible(name));
            menu_id_to_toolbar_[menuId] = name;
            menuId++;
        }
    }
    
    menu->AppendSubMenu(toolbarMenu, "&Toolbars");
    menu->AppendSeparator();
    
    // Layout operations
    menu->Append(ID_MENU_DOCK_ALL_WINDOWS, "&Dock All Windows");
    menu->Append(ID_MENU_FLOAT_ALL_WINDOWS, "&Float All Windows");
    menu->AppendSeparator();
    menu->Append(ID_MENU_RESET_LAYOUT, "&Reset Layout");
    
    // Layout presets submenu
    auto* presetMenu = new wxMenu();
    presetMenu->Append(ID_MENU_LAYOUT_PRESETS_BASE + 0, "&Default");
    presetMenu->Append(ID_MENU_LAYOUT_PRESETS_BASE + 1, "&Minimal");
    presetMenu->Append(ID_MENU_LAYOUT_PRESETS_BASE + 2, "&Developer");
    presetMenu->Append(ID_MENU_LAYOUT_PRESETS_BASE + 3, "&Data Analyst");
    presetMenu->Append(ID_MENU_LAYOUT_PRESETS_BASE + 4, "DB& Administrator");
    menu->AppendSubMenu(presetMenu, "Layout &Presets");
}

void IconBarHost::UpdateToolBarMenuItems(wxMenu* menu) {
    if (!menu) {
        return;
    }
    
    // Find the Toolbars submenu
    wxMenuItem* toolbarItem = nullptr;
    for (size_t i = 0; i < menu->GetMenuItemCount(); ++i) {
        wxMenuItem* item = menu->FindItemByPosition(i);
        if (item && item->GetSubMenu()) {
            wxString label = item->GetItemLabelText();
            if (label == "Toolbars") {
                toolbarItem = item;
                break;
            }
        }
    }
    
    if (toolbarItem && toolbarItem->GetSubMenu()) {
        wxMenu* toolbarMenu = toolbarItem->GetSubMenu();
        
        // Update checkmarks
        for (auto& [menuId, toolbarName] : menu_id_to_toolbar_) {
            wxMenuItem* item = toolbarMenu->FindItem(menuId);
            if (item) {
                item->Check(IsToolBarVisible(toolbarName));
            }
        }
    }
}

void IconBarHost::OnToolBarMenuItem(wxCommandEvent& event) {
    int id = event.GetId();
    
    auto it = menu_id_to_toolbar_.find(id);
    if (it != menu_id_to_toolbar_.end()) {
        ToggleToolBar(it->second);
    } else if (id == ID_MENU_DOCK_ALL_WINDOWS) {
        DockAllToolBars();
        DockAllForms();
    } else if (id == ID_MENU_FLOAT_ALL_WINDOWS) {
        FloatAllToolBars();
    } else if (id == ID_MENU_RESET_LAYOUT) {
        ResetToDefaults();
    } else if (id >= ID_MENU_LAYOUT_PRESETS_BASE && id < ID_MENU_LAYOUT_PRESETS_MAX) {
        int presetIndex = id - ID_MENU_LAYOUT_PRESETS_BASE;
        switch (presetIndex) {
            case 0: ApplyLayoutPreset(LayoutPreset::Default); break;
            case 1: ApplyLayoutPreset(LayoutPreset::Minimal); break;
            case 2: ApplyLayoutPreset(LayoutPreset::Developer); break;
            case 3: ApplyLayoutPreset(LayoutPreset::DataAnalyst); break;
            case 4: ApplyLayoutPreset(LayoutPreset::DBA); break;
        }
    }
}

void IconBarHost::ApplyLayoutPreset(LayoutPreset preset) {
    DockAllToolBars();
    
    switch (preset) {
        case LayoutPreset::Default:
            // Show main toolbar only
            ShowOnlyToolBars({"Main"});
            break;
            
        case LayoutPreset::Minimal:
            // Hide all toolbars
            for (const auto& [name, toolbar] : toolbars_) {
                HideToolBar(name);
            }
            break;
            
        case LayoutPreset::Developer:
            // Show developer-focused toolbars
            ShowOnlyToolBars({"Main", "SqlEditor", "Diagram"});
            break;
            
        case LayoutPreset::DataAnalyst:
            // Show data analysis toolbars
            ShowOnlyToolBars({"Main", "SqlEditor", "Reporting"});
            break;
            
        case LayoutPreset::DBA:
            // Show DBA-focused toolbars
            ShowOnlyToolBars({"Main", "Monitoring", "UsersRoles"});
            break;
            
        case LayoutPreset::Custom:
            // Load custom layout
            break;
    }
    
    LayoutDockedToolBars();
}

void IconBarHost::SaveCurrentLayout(const wxString& name) {
    SaveLayoutToConfig(name);
}

void IconBarHost::LoadLayout(const wxString& name) {
    LoadLayoutFromConfig(name);
}

std::vector<wxString> IconBarHost::GetSavedLayoutNames() const {
    std::vector<wxString> names;
    for (const auto& [name, config] : saved_layouts_) {
        names.push_back(name);
    }
    return names;
}

void IconBarHost::SaveState() {
    // Save toolbar states
    for (const auto& [name, toolbar] : toolbars_) {
        if (toolbar) {
            DraggableToolBar::State state = toolbar->GetState();
            // Serialize state to config
            // This would use the app's config system
        }
    }
}

void IconBarHost::RestoreState() {
    // Restore toolbar states from config
    for (const auto& [name, toolbar] : toolbars_) {
        if (toolbar) {
            DraggableToolBar::State state;
            // Deserialize state from config
            toolbar->RestoreState(state);
        }
    }
    
    LayoutDockedToolBars();
}

void IconBarHost::ResetToDefaults() {
    ApplyLayoutPreset(LayoutPreset::Default);
}

void IconBarHost::OnFormActivated(DockableForm* form) {
    if (!form) {
        return;
    }
    
    // Get the document type
    wxString docType = form->GetDocumentType();
    
    // Map document types to toolbar names
    std::vector<wxString> toolbars;
    if (docType == "sql") {
        toolbars = {"Main", "SqlEditor"};
    } else if (docType == "diagram") {
        toolbars = {"Main", "Diagram"};
    } else if (docType == "monitoring") {
        toolbars = {"Main", "Monitoring"};
    } else if (docType == "users") {
        toolbars = {"Main", "UsersRoles"};
    } else {
        toolbars = {"Main"};
    }
    
    ShowFormToolBars(toolbars);
}

void IconBarHost::OnFormDeactivated(DockableForm* form) {
    // Hide form-specific toolbars when form is deactivated
    HideAllFormToolBars();
}

void IconBarHost::LayoutDockedToolBars() {
    // This is handled by the wxFrame's toolbar management
    // when toolbars are reparented to the frame
    if (main_frame_) {
        main_frame_->Layout();
    }
}

void IconBarHost::UpdateMenuCheckmarks() {
    // This would be called when the View menu is opened
    // to update the toolbar visibility checkmarks
}

void IconBarHost::SaveLayoutToConfig(const wxString& name) {
    // Serialize current layout to JSON/config
    // This is a placeholder for the actual implementation
    saved_layouts_[name.ToStdString()] = "layout_config";
}

bool IconBarHost::LoadLayoutFromConfig(const wxString& name) {
    auto it = saved_layouts_.find(name.ToStdString());
    if (it != saved_layouts_.end()) {
        // Deserialize layout from JSON/config
        // This is a placeholder for the actual implementation
        return true;
    }
    return false;
}

} // namespace scratchrobin
