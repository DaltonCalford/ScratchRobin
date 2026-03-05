/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/menu_editor.h"
#include "ui/window_framework/menu_registry.h"
#include "ui/window_framework/action_registry.h"
#include "res/icon_manager.h"
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/dnd.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/textdlg.h>
#include <chrono>
#include <algorithm>

namespace scratchrobin::ui {

// Event IDs
enum {
    ID_NEW_MENU = 2000,
    ID_DELETE_MENU,
    ID_RENAME_MENU,
    ID_NEW_WINDOW_TYPE,
    ID_RESET_MENUS,
    ID_IMPORT_MENUS,
    ID_EXPORT_MENUS,
    ID_HELP,
    ID_SEARCH_ACTIONS,
    ID_SEARCH_MENU,
    ID_ACTION_TREE,
    ID_MENU_TREE,
    ID_WINDOW_TYPE_LIST,
    ID_MENU_CHECKLIST,
    ID_MERGE_CB,
    ID_NEW_SUBMENU,
    ID_ADD_SEPARATOR,
    ID_EDIT_ITEM,
    ID_DELETE_ITEM,
    ID_MOVE_UP,
    ID_MOVE_DOWN,
    ID_CHANGE_ACTION,
    ID_TOGGLE_VISIBILITY
};

// Custom tree data for storing string data
class ActionTreeData2 : public wxTreeItemData {
public:
    explicit ActionTreeData2(const wxString& data) : data_(data) {}
    wxString getData() const { return data_; }
private:
    wxString data_;
};

// ============================================================================
// MenuEditor Implementation
// ============================================================================

BEGIN_EVENT_TABLE(MenuEditor, wxDialog)
    EVT_BUTTON(ID_NEW_MENU, MenuEditor::onNewMenu)
    EVT_BUTTON(ID_DELETE_MENU, MenuEditor::onDeleteMenu)
    EVT_BUTTON(ID_RENAME_MENU, MenuEditor::onRenameMenu)
    EVT_BUTTON(ID_NEW_WINDOW_TYPE, MenuEditor::onNewWindowType)
    EVT_BUTTON(ID_RESET_MENUS, MenuEditor::onResetMenus)
    EVT_BUTTON(ID_IMPORT_MENUS, MenuEditor::onImportMenus)
    EVT_BUTTON(ID_EXPORT_MENUS, MenuEditor::onExportMenus)
    EVT_BUTTON(ID_HELP, MenuEditor::onHelp)
END_EVENT_TABLE()

MenuEditor::MenuEditor(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Customize Menus", 
               wxDefaultPosition, wxSize(1200, 800),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    
    // Initialize menu registry
    MenuRegistry::get()->initialize();
    
    createUI();
    refreshAllPanels();
}

MenuEditor::~MenuEditor() = default;

int MenuEditor::ShowModal() {
    SetMinSize(wxSize(900, 700));
    return wxDialog::ShowModal();
}

void MenuEditor::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Create horizontal layout with three panels
    wxBoxSizer* panels_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left panel - Actions
    wxPanel* action_box = createActionPanel(this);
    panels_sizer->Add(action_box, 1, wxEXPAND | wxALL, 5);
    
    // Middle panel - Menu Structure
    wxPanel* menu_box = createMenuPanel(this);
    panels_sizer->Add(menu_box, 1, wxEXPAND | wxALL, 5);
    
    // Right panel - Window Types
    wxPanel* window_type_box = createWindowTypePanel(this);
    panels_sizer->Add(window_type_box, 1, wxEXPAND | wxALL, 5);
    
    main_sizer->Add(panels_sizer, 1, wxEXPAND);
    
    // Button bar
    createButtonBar(main_sizer);
    
    SetSizer(main_sizer);
}

wxPanel* MenuEditor::createActionPanel(wxWindow* parent) {
    wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, parent, "Actions");
    wxStaticBox* box_win = box->GetStaticBox();
    
    // Description
    wxStaticText* desc = new wxStaticText(box_win, wxID_ANY,
        "Drag actions to the menu structure\nto add them to menus.");
    desc->SetForegroundColour(wxColour(100, 100, 100));
    box->Add(desc, 0, wxALL, 5);
    
    // Actions panel
    action_panel_ = new ActionListPanel2(box_win, this);
    box->Add(action_panel_, 1, wxEXPAND | wxALL, 5);
    
    wxPanel* container = new wxPanel(parent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(box, 1, wxEXPAND);
    container->SetSizer(sizer);
    
    return container;
}

wxPanel* MenuEditor::createMenuPanel(wxWindow* parent) {
    wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, parent, "Menu Structure");
    wxStaticBox* box_win = box->GetStaticBox();
    
    // Description
    wxStaticText* desc = new wxStaticText(box_win, wxID_ANY,
        "Right-click items to edit.\nDrag to reorder.\nDrop actions to add.");
    desc->SetForegroundColour(wxColour(100, 100, 100));
    box->Add(desc, 0, wxALL, 5);
    
    // Menu buttons
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(new wxButton(box_win, ID_NEW_MENU, "New Menu..."), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(box_win, ID_DELETE_MENU, "Delete"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(box_win, ID_RENAME_MENU, "Rename..."), 0);
    box->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Menu structure panel
    menu_panel_ = new MenuStructurePanel(box_win, this);
    box->Add(menu_panel_, 1, wxEXPAND | wxALL, 5);
    
    wxPanel* container = new wxPanel(parent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(box, 1, wxEXPAND);
    container->SetSizer(sizer);
    
    return container;
}

wxPanel* MenuEditor::createWindowTypePanel(wxWindow* parent) {
    wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, parent, "Window Types");
    wxStaticBox* box_win = box->GetStaticBox();
    
    // Description
    wxStaticText* desc = new wxStaticText(box_win, wxID_ANY,
        "Select a window type to see\nwhich menus it contributes.\nCheck menus to assign them.");
    desc->SetForegroundColour(wxColour(100, 100, 100));
    box->Add(desc, 0, wxALL, 5);
    
    // New window type button
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(new wxButton(box_win, ID_NEW_WINDOW_TYPE, "New Window Type..."), 0);
    box->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Window type panel
    window_type_panel_ = new WindowTypePanel(box_win, this);
    box->Add(window_type_panel_, 1, wxEXPAND | wxALL, 5);
    
    wxPanel* container = new wxPanel(parent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(box, 1, wxEXPAND);
    container->SetSizer(sizer);
    
    return container;
}

void MenuEditor::createButtonBar(wxSizer* parent_sizer) {
    wxStaticLine* line = new wxStaticLine(this);
    parent_sizer->Add(line, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Management buttons
    btn_sizer->Add(new wxButton(this, ID_RESET_MENUS, "Reset to Defaults"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, ID_IMPORT_MENUS, "Import..."), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, ID_EXPORT_MENUS, "Export..."), 0, wxRIGHT, 5);
    
    btn_sizer->AddStretchSpacer(1);
    
    // Help and OK/Cancel
    btn_sizer->Add(new wxButton(this, ID_HELP, "Help"), 0, wxRIGHT, 20);
    btn_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    
    parent_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
}

void MenuEditor::onActionSelected(const std::string& action_id) {
    // Action selected in left panel
}

void MenuEditor::onMenuItemSelected(const std::string& menu_id, const std::string& item_id) {
    current_menu_id_ = menu_id;
}

void MenuEditor::onWindowTypeSelected(const std::string& window_type_id) {
    current_window_type_id_ = window_type_id;
}

void MenuEditor::refreshAllPanels() {
    action_panel_->refreshList();
    menu_panel_->refreshList();
    window_type_panel_->refreshList();
}

void MenuEditor::refreshMenuPanel() {
    menu_panel_->refreshList();
}

void MenuEditor::refreshWindowTypePanel() {
    window_type_panel_->refreshList();
}

void MenuEditor::onNewMenu(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this, "Enter name for new menu:\n(Use & for accelerator, e.g., &File)", "New Menu");
    if (dlg.ShowModal() == wxID_OK) {
        wxString name = dlg.GetValue();
        if (!name.IsEmpty()) {
            MenuDefinition menu;
            menu.menu_id = MenuRegistry::get()->generateMenuId(std::string(name.ToUTF8()));
            menu.display_name = std::string(name.ToUTF8());
            menu.is_builtin = false;
            menu.sort_order = 100;  // Custom menus at end
            menu.is_visible = true;
            
            MenuRegistry::get()->registerMenu(menu);
            refreshMenuPanel();
        }
    }
}

void MenuEditor::onDeleteMenu(wxCommandEvent& event) {
    std::string menu_id = menu_panel_->getSelectedItemId();
    if (menu_id.empty()) {
        wxMessageBox("Please select a menu to delete.", "Delete Menu", wxOK | wxICON_INFORMATION);
        return;
    }
    
    MenuDefinition* menu = MenuRegistry::get()->findMenu(menu_id);
    if (menu && menu->is_builtin) {
        wxMessageBox("Cannot delete built-in menus.", "Delete Menu", wxOK | wxICON_INFORMATION);
        return;
    }
    
    int result = wxMessageBox("Delete selected menu?", "Confirm Delete", 
                               wxYES_NO | wxICON_QUESTION);
    if (result == wxYES) {
        MenuRegistry::get()->unregisterMenu(menu_id);
        refreshMenuPanel();
        refreshWindowTypePanel();
    }
}

void MenuEditor::onRenameMenu(wxCommandEvent& event) {
    std::string menu_id = menu_panel_->getSelectedItemId();
    if (menu_id.empty()) {
        wxMessageBox("Please select a menu to rename.", "Rename Menu", wxOK | wxICON_INFORMATION);
        return;
    }
    
    MenuDefinition* menu = MenuRegistry::get()->findMenu(menu_id);
    if (!menu) return;
    
    wxTextEntryDialog dlg(this, "Enter new name:\n(Use & for accelerator, e.g., &File)", 
                         "Rename Menu", wxString::FromUTF8(menu->display_name.c_str()));
    if (dlg.ShowModal() == wxID_OK) {
        wxString name = dlg.GetValue();
        if (!name.IsEmpty()) {
            menu->display_name = std::string(name.ToUTF8());
            refreshMenuPanel();
        }
    }
}

void MenuEditor::onNewWindowType(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this, "Enter ID for new window type:\n(e.g., sql_editor, report_viewer)", 
                         "New Window Type");
    if (dlg.ShowModal() == wxID_OK) {
        wxString id = dlg.GetValue();
        if (!id.IsEmpty()) {
            // Get display name
            wxTextEntryDialog name_dlg(this, "Enter display name:", "Window Type Name", id);
            if (name_dlg.ShowModal() == wxID_OK) {
                WindowTypeMenuSet menu_set;
                menu_set.window_type_id = std::string(id.ToUTF8());
                menu_set.display_name = std::string(name_dlg.GetValue().ToUTF8());
                menu_set.merge_with_main = true;
                
                MenuRegistry::get()->registerWindowTypeMenuSet(menu_set);
                refreshWindowTypePanel();
            }
        }
    }
}

void MenuEditor::onResetMenus(wxCommandEvent& event) {
    int result = wxMessageBox("Reset all menus to default?\nThis will delete all custom menus and modifications.",
                               "Reset Menus", wxYES_NO | wxICON_WARNING);
    if (result == wxYES) {
        MenuRegistry::get()->resetToDefaults();
        refreshAllPanels();
    }
}

void MenuEditor::onImportMenus(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Import Menus", "", "", 
                     "Menu files (*.mnu)|*.mnu|JSON files (*.json)|*.json|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
        if (MenuRegistry::get()->importMenusFromFile(std::string(dlg.GetPath().ToUTF8()))) {
            wxMessageBox("Menus imported successfully.", "Import", wxOK | wxICON_INFORMATION);
            refreshAllPanels();
        } else {
            wxMessageBox("Failed to import menus.", "Import Error", wxOK | wxICON_ERROR);
        }
    }
}

void MenuEditor::onExportMenus(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Export Menus", "", "menus.mnu",
                     "Menu files (*.mnu)|*.mnu|JSON files (*.json)|*.json|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) {
        if (MenuRegistry::get()->exportMenusToFile(std::string(dlg.GetPath().ToUTF8()))) {
            wxMessageBox("Menus exported successfully.", "Export", wxOK | wxICON_INFORMATION);
        } else {
            wxMessageBox("Failed to export menus.", "Export Error", wxOK | wxICON_ERROR);
        }
    }
}

void MenuEditor::onHelp(wxCommandEvent& event) {
    wxMessageBox(
        "Customize Menus Help:\n\n"
        "1. Select an Action from the left panel\n"
        "2. Drag it to a menu in the middle panel\n"
        "3. Right-click menu items to edit, move, or delete\n\n"
        "Window Types:\n"
        "- Select a window type on the right\n"
        "- Check menus to assign them to that window type\n"
        "- Menus are shown when windows of that type are active\n\n"
        "Main Window:\n"
        "- Always contributes its menus\n"
        "- Other window types merge with or replace main menus\n\n"
        "Creating Menus:\n"
        "- Use 'New Menu' to create custom menus\n"
        "- Use 'New Window Type' to create new window types",
        "Menu Editor Help", wxOK | wxICON_INFORMATION);
}

// ============================================================================
// ActionListPanel2 Implementation
// ============================================================================

BEGIN_EVENT_TABLE(ActionListPanel2, wxPanel)
    EVT_SEARCHCTRL_SEARCH_BTN(ID_SEARCH_ACTIONS, ActionListPanel2::onSearch)
    EVT_TEXT(ID_SEARCH_ACTIONS, ActionListPanel2::onSearch)
    EVT_TREE_SEL_CHANGED(ID_ACTION_TREE, ActionListPanel2::onItemSelected)
    EVT_TREE_BEGIN_DRAG(ID_ACTION_TREE, ActionListPanel2::onBeginDrag)
END_EVENT_TABLE()

ActionListPanel2::ActionListPanel2(wxWindow* parent, MenuEditor* editor)
    : wxPanel(parent, wxID_ANY)
    , editor_(editor) {
    createUI();
}

void ActionListPanel2::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Search control
    search_ctrl_ = new wxSearchCtrl(this, ID_SEARCH_ACTIONS, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    search_ctrl_->SetDescriptiveText("Search actions...");
    search_ctrl_->ShowSearchButton(true);
    search_ctrl_->ShowCancelButton(true);
    main_sizer->Add(search_ctrl_, 0, wxEXPAND | wxBOTTOM, 5);
    
    // Tree control
    tree_ctrl_ = new wxTreeCtrl(this, ID_ACTION_TREE, wxDefaultPosition, wxDefaultSize,
                                wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT |
                                wxTR_FULL_ROW_HIGHLIGHT | wxTR_SINGLE);
    main_sizer->Add(tree_ctrl_, 1, wxEXPAND);
    
    SetSizer(main_sizer);
}

void ActionListPanel2::refreshList() {
    filterActions(search_ctrl_->GetValue());
}

void ActionListPanel2::populateActions() {
    tree_ctrl_->DeleteAllItems();
    wxTreeItemId root = tree_ctrl_->AddRoot("Actions");
    
    ActionRegistry* reg = ActionRegistry::get();
    auto categories = reg->getCategoriesWithActions();
    
    for (auto cat : categories) {
        wxString cat_name = wxString::FromUTF8(actionCategoryToString(cat).c_str());
        wxTreeItemId cat_item = tree_ctrl_->AppendItem(root, cat_name);
        
        auto actions = reg->getActionsByCategory(cat);
        for (Action* action : actions) {
            wxString label = wxString::FromUTF8(action->display_name.c_str());
            wxTreeItemId item = tree_ctrl_->AppendItem(cat_item, label);
            tree_ctrl_->SetItemData(item, new ActionTreeData2(
                wxString::FromUTF8(action->action_id.c_str())));
        }
        
        tree_ctrl_->Expand(cat_item);
    }
}

void ActionListPanel2::filterActions(const wxString& filter) {
    if (filter.IsEmpty()) {
        populateActions();
        return;
    }
    
    tree_ctrl_->DeleteAllItems();
    wxTreeItemId root = tree_ctrl_->AddRoot("Actions");
    
    ActionRegistry* reg = ActionRegistry::get();
    auto actions = reg->searchActions(std::string(filter.ToUTF8()));
    
    for (Action* action : actions) {
        wxString label = wxString::FromUTF8(action->display_name.c_str());
        wxString category = wxString::FromUTF8(actionCategoryToString(action->category).c_str());
        label = category + ": " + label;
        wxTreeItemId item = tree_ctrl_->AppendItem(root, label);
        tree_ctrl_->SetItemData(item, new ActionTreeData2(
            wxString::FromUTF8(action->action_id.c_str())));
    }
}

std::string ActionListPanel2::getSelectedAction() const {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (sel.IsOk()) {
        ActionTreeData2* data = dynamic_cast<ActionTreeData2*>(tree_ctrl_->GetItemData(sel));
        if (data) {
            return std::string(data->getData().ToUTF8());
        }
    }
    return "";
}

void ActionListPanel2::setSearchText(const wxString& text) {
    search_ctrl_->SetValue(text);
    filterActions(text);
}

void ActionListPanel2::onSearch(wxCommandEvent& event) {
    filterActions(search_ctrl_->GetValue());
}

void ActionListPanel2::onItemSelected(wxTreeEvent& event) {
    std::string action_id = getSelectedAction();
    if (!action_id.empty() && editor_) {
        editor_->onActionSelected(action_id);
    }
    event.Skip();
}

void ActionListPanel2::onBeginDrag(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    ActionTreeData2* data = dynamic_cast<ActionTreeData2*>(tree_ctrl_->GetItemData(item));
    if (data && editor_) {
        std::string action_id = std::string(data->getData().ToUTF8());
        editor_->drag_data_.type = MenuDragData::DragType::kAction;
        editor_->drag_data_.action_id = action_id;
        
        wxTextDataObject drag_data("action:" + data->getData());
        wxDropSource drag_source(this);
        drag_source.SetData(drag_data);
        drag_source.DoDragDrop(wxDrag_CopyOnly);
    }
    event.Skip();
}


// ============================================================================
// MenuStructurePanel Implementation
// ============================================================================

BEGIN_EVENT_TABLE(MenuStructurePanel, wxPanel)
    EVT_SEARCHCTRL_SEARCH_BTN(ID_SEARCH_MENU, MenuStructurePanel::onSearch)
    EVT_TEXT(ID_SEARCH_MENU, MenuStructurePanel::onSearch)
    EVT_TREE_SEL_CHANGED(ID_MENU_TREE, MenuStructurePanel::onItemSelected)
    EVT_TREE_BEGIN_DRAG(ID_MENU_TREE, MenuStructurePanel::onBeginDrag)
    EVT_TREE_END_DRAG(ID_MENU_TREE, MenuStructurePanel::onEndDrag)
    EVT_TREE_ITEM_MENU(ID_MENU_TREE, MenuStructurePanel::onContextMenu)
    EVT_TREE_ITEM_ACTIVATED(ID_MENU_TREE, MenuStructurePanel::onItemActivated)
    EVT_MENU(ID_NEW_SUBMENU, MenuStructurePanel::onNewSubmenu)
    EVT_MENU(ID_ADD_SEPARATOR, MenuStructurePanel::onAddSeparator)
    EVT_MENU(ID_EDIT_ITEM, MenuStructurePanel::onEditItem)
    EVT_MENU(ID_DELETE_ITEM, MenuStructurePanel::onDeleteItem)
    EVT_MENU(ID_MOVE_UP, MenuStructurePanel::onMoveUp)
    EVT_MENU(ID_MOVE_DOWN, MenuStructurePanel::onMoveDown)
    EVT_MENU(ID_CHANGE_ACTION, MenuStructurePanel::onChangeAction)
END_EVENT_TABLE()

MenuStructurePanel::MenuStructurePanel(wxWindow* parent, MenuEditor* editor)
    : wxPanel(parent, wxID_ANY)
    , editor_(editor) {
    createUI();
}

void MenuStructurePanel::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Search control
    search_ctrl_ = new wxSearchCtrl(this, ID_SEARCH_MENU, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    search_ctrl_->SetDescriptiveText("Search menus...");
    search_ctrl_->ShowSearchButton(true);
    search_ctrl_->ShowCancelButton(true);
    main_sizer->Add(search_ctrl_, 0, wxEXPAND | wxBOTTOM, 5);
    
    // Tree control
    tree_ctrl_ = new wxTreeCtrl(this, ID_MENU_TREE, wxDefaultPosition, wxDefaultSize,
                                wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT |
                                wxTR_FULL_ROW_HIGHLIGHT | wxTR_SINGLE |
                                wxTR_EDIT_LABELS);
    main_sizer->Add(tree_ctrl_, 1, wxEXPAND);
    
    SetSizer(main_sizer);
}

void MenuStructurePanel::refreshList() {
    if (current_menu_id_.empty()) {
        // Show all menus
        clearTree();
        item_map_.clear();
        
        wxTreeItemId root = tree_ctrl_->AddRoot("Menus");
        
        MenuRegistry* reg = MenuRegistry::get();
        auto menus = reg->getAllMenus();
        
        for (MenuDefinition* menu : menus) {
            wxString label = wxString::FromUTF8(menu->display_name.c_str());
            wxTreeItemId menu_item = tree_ctrl_->AppendItem(root, label);
            
            TreeItemInfo info;
            info.menu_id = menu->menu_id;
            info.item_id = "";
            info.is_menu = true;
            info.is_submenu = false;
            item_map_[(int)(intptr_t)menu_item.m_pItem] = info;
            
            // Add menu items
            for (const auto& item : menu->items) {
                addItemToTree(item, menu_item, menu->menu_id);
            }
            
            tree_ctrl_->Expand(menu_item);
        }
    } else {
        // Show specific menu
        setCurrentMenu(current_menu_id_);
    }
}

void MenuStructurePanel::setCurrentMenu(const std::string& menu_id) {
    current_menu_id_ = menu_id;
    populateMenu(menu_id);
}

void MenuStructurePanel::populateMenu(const std::string& menu_id) {
    clearTree();
    item_map_.clear();
    
    MenuRegistry* reg = MenuRegistry::get();
    MenuDefinition* menu = reg->findMenu(menu_id);
    if (!menu) return;
    
    wxTreeItemId root = tree_ctrl_->AddRoot(wxString::FromUTF8(menu->display_name.c_str()));
    
    TreeItemInfo root_info;
    root_info.menu_id = menu_id;
    root_info.item_id = "";
    root_info.is_menu = true;
    item_map_[(int)(intptr_t)root.m_pItem] = root_info;
    
    for (const auto& item : menu->items) {
        addItemToTree(item, root, menu_id);
    }
    
    tree_ctrl_->Expand(root);
}

void MenuStructurePanel::addItemToTree(const MenuItem& item, wxTreeItemId parent, 
                                        const std::string& menu_id) {
    wxString label;
    if (item.isSeparator()) {
        label = "----- Separator -----";
    } else if (!item.custom_label.empty()) {
        label = wxString::FromUTF8(item.custom_label.c_str());
    } else {
        Action* action = ActionRegistry::get()->findAction(item.action_id);
        label = action ? wxString::FromUTF8(action->display_name.c_str()) 
                       : wxString::FromUTF8(("Unknown: " + item.action_id).c_str());
    }
    
    // Add accelerator if present
    if (!item.custom_accel.empty()) {
        label += wxString::Format("\t%s", wxString::FromUTF8(item.custom_accel.c_str()));
    }
    
    wxTreeItemId item_node = tree_ctrl_->AppendItem(parent, label);
    
    TreeItemInfo info;
    info.menu_id = menu_id;
    info.item_id = item.item_id;
    info.is_menu = false;
    info.is_submenu = item.isSubmenu();
    item_map_[(int)(intptr_t)item_node.m_pItem] = info;
    
    // Add children if submenu
    if (item.isSubmenu() && !item.children.empty()) {
        for (const auto& child : item.children) {
            addItemToTree(child, item_node, menu_id);
        }
        tree_ctrl_->Expand(item_node);
    }
}

void MenuStructurePanel::clearTree() {
    tree_ctrl_->DeleteAllItems();
    item_map_.clear();
}

std::string MenuStructurePanel::getSelectedItemId() const {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (sel.IsOk()) {
        auto it = item_map_.find((int)(intptr_t)sel.m_pItem);
        if (it != item_map_.end()) {
            if (it->second.is_menu) {
                return it->second.menu_id;
            } else {
                return it->second.item_id;
            }
        }
    }
    return "";
}

void MenuStructurePanel::onSearch(wxCommandEvent& event) {
    // Filter menus based on search text
    wxString filter = search_ctrl_->GetValue();
    if (filter.IsEmpty()) {
        refreshList();
        return;
    }
    
    // Simple implementation: show all menus containing the search text
    clearTree();
    item_map_.clear();
    
    wxTreeItemId root = tree_ctrl_->AddRoot("Search Results");
    
    MenuRegistry* reg = MenuRegistry::get();
    auto menus = reg->searchMenus(std::string(filter.ToUTF8()));
    
    for (MenuDefinition* menu : menus) {
        wxString label = wxString::FromUTF8(menu->display_name.c_str());
        wxTreeItemId menu_item = tree_ctrl_->AppendItem(root, label);
        
        TreeItemInfo info;
        info.menu_id = menu->menu_id;
        info.item_id = "";
        info.is_menu = true;
        item_map_[(int)(intptr_t)menu_item.m_pItem] = info;
    }
}

void MenuStructurePanel::onItemSelected(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    auto it = item_map_.find((int)(intptr_t)item.m_pItem);
    if (it != item_map_.end() && editor_) {
        editor_->onMenuItemSelected(it->second.menu_id, it->second.item_id);
    }
}

void MenuStructurePanel::onBeginDrag(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    auto it = item_map_.find((int)(intptr_t)item.m_pItem);
    if (it != item_map_.end() && editor_ && !it->second.is_menu) {
        editor_->drag_data_.type = MenuDragData::DragType::kMenuItem;
        editor_->drag_data_.menu_id = it->second.menu_id;
        editor_->drag_data_.item_id = it->second.item_id;
        event.Allow();
    }
}

void MenuStructurePanel::onEndDrag(wxTreeEvent& event) {
    if (!editor_) return;
    
    wxTreeItemId src = event.GetItem();
    wxPoint pos = event.GetPoint();
    
    int flags;
    wxTreeItemId dst = tree_ctrl_->HitTest(pos, flags);
    
    if (dst.IsOk() && editor_->drag_data_.type == MenuDragData::DragType::kAction) {
        // Dropping an action from left panel
        onDropAction(editor_->drag_data_.action_id, dst);
    }
    
    editor_->drag_data_.reset();
}

void MenuStructurePanel::onDropAction(const std::string& action_id, const wxTreeItemId& target) {
    auto it = item_map_.find((int)(intptr_t)target.m_pItem);
    if (it == item_map_.end()) return;
    
    MenuRegistry* reg = MenuRegistry::get();
    MenuDefinition* menu = reg->findMenu(it->second.menu_id);
    if (!menu) return;
    
    // Create new menu item for this action
    MenuItem item;
    item.item_id = menu->menu_id + "_" + action_id + "_" + 
                   std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    item.type = MenuItemType::kAction;
    item.action_id = action_id;
    
    Action* action = ActionRegistry::get()->findAction(action_id);
    if (action) {
        item.custom_label = action->display_name;
        item.custom_accel = action->accelerator;
        item.icon_id = action->default_icon_name;
    }
    
    // Add after the target item, or append if target is the menu root
    if (it->second.is_menu) {
        menu->addItem(item);
    } else {
        // Find position of target item
        for (size_t i = 0; i < menu->items.size(); i++) {
            if (menu->items[i].item_id == it->second.item_id) {
                menu->addItem(item, (int)i + 1);
                break;
            }
        }
    }
    
    refreshList();
}

void MenuStructurePanel::onContextMenu(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk()) return;
    
    tree_ctrl_->SelectItem(item);
    
    auto it = item_map_.find((int)(intptr_t)item.m_pItem);
    if (it == item_map_.end()) return;
    
    wxMenu menu;
    
    if (it->second.is_menu) {
        // Menu context menu
        menu.Append(ID_NEW_SUBMENU, "New Submenu...");
        menu.Append(ID_ADD_SEPARATOR, "Add Separator");
        menu.AppendSeparator();
        menu.Append(ID_DELETE_ITEM, "Delete Menu");
    } else {
        // Item context menu
        menu.Append(ID_EDIT_ITEM, "Edit...");
        menu.Append(ID_CHANGE_ACTION, "Change Action...");
        menu.AppendSeparator();
        menu.Append(ID_MOVE_UP, "Move Up");
        menu.Append(ID_MOVE_DOWN, "Move Down");
        menu.AppendSeparator();
        menu.Append(ID_NEW_SUBMENU, "New Submenu...");
        menu.Append(ID_ADD_SEPARATOR, "Add Separator");
        menu.AppendSeparator();
        menu.Append(ID_DELETE_ITEM, "Delete");
    }
    
    PopupMenu(&menu);
}

void MenuStructurePanel::onItemActivated(wxTreeEvent& event) {
    wxCommandEvent dummy;
    onEditItem(dummy);
}

void MenuStructurePanel::onNewSubmenu(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this, "Enter name for submenu:\n(Use & for accelerator)", "New Submenu");
    if (dlg.ShowModal() == wxID_OK) {
        wxString name = dlg.GetValue();
        if (name.IsEmpty()) return;
        
        // Get current menu
        std::string menu_id = current_menu_id_;
        if (menu_id.empty()) {
            // Try to get from selection
            wxTreeItemId sel = tree_ctrl_->GetSelection();
            if (sel.IsOk()) {
                auto it = item_map_.find((int)(intptr_t)sel.m_pItem);
                if (it != item_map_.end()) {
                    menu_id = it->second.menu_id;
                }
            }
        }
        
        if (menu_id.empty()) return;
        
        MenuRegistry* reg = MenuRegistry::get();
        MenuDefinition* menu = reg->findMenu(menu_id);
        if (!menu) return;
        
        MenuItem item;
        item.item_id = menu_id + "_submenu_" + 
                       std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        item.type = MenuItemType::kSubmenu;
        item.custom_label = std::string(name.ToUTF8());
        
        menu->addItem(item);
        refreshList();
    }
}

void MenuStructurePanel::onAddSeparator(wxCommandEvent& event) {
    std::string menu_id = current_menu_id_;
    if (menu_id.empty()) {
        wxTreeItemId sel = tree_ctrl_->GetSelection();
        if (sel.IsOk()) {
            auto it = item_map_.find((int)(intptr_t)sel.m_pItem);
            if (it != item_map_.end()) {
                menu_id = it->second.menu_id;
            }
        }
    }
    
    if (menu_id.empty()) return;
    
    MenuRegistry* reg = MenuRegistry::get();
    MenuDefinition* menu = reg->findMenu(menu_id);
    if (!menu) return;
    
    MenuItem sep;
    sep.item_id = menu_id + "_sep_" + std::to_string(menu->items.size());
    sep.type = MenuItemType::kSeparator;
    
    menu->addItem(sep);
    refreshList();
}

void MenuStructurePanel::onEditItem(wxCommandEvent& event) {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (!sel.IsOk()) return;
    
    auto it = item_map_.find((int)(intptr_t)sel.m_pItem);
    if (it == item_map_.end() || it->second.is_menu) return;
    
    MenuRegistry* reg = MenuRegistry::get();
    MenuDefinition* menu = reg->findMenu(it->second.menu_id);
    if (!menu) return;
    
    MenuItem* item = menu->findItem(it->second.item_id);
    if (!item) return;
    
    Action* action = ActionRegistry::get()->findAction(item->action_id);
    
    if (item->isSubmenu()) {
        SubmenuEditDialog dlg(this, *item);
        if (dlg.ShowEditDialog()) {
            refreshList();
        }
    } else {
        MenuItemEditDialog dlg(this, *item, action);
        if (dlg.ShowEditDialog()) {
            refreshList();
        }
    }
}

void MenuStructurePanel::onDeleteItem(wxCommandEvent& event) {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (!sel.IsOk()) return;
    
    auto it = item_map_.find((int)(intptr_t)sel.m_pItem);
    if (it == item_map_.end()) return;
    
    if (it->second.is_menu) {
        // Delete entire menu
        int result = wxMessageBox("Delete this menu?", "Confirm Delete", wxYES_NO | wxICON_QUESTION);
        if (result == wxYES) {
            MenuRegistry::get()->unregisterMenu(it->second.menu_id);
            refreshList();
        }
    } else {
        // Delete menu item
        MenuDefinition* menu = MenuRegistry::get()->findMenu(it->second.menu_id);
        if (menu) {
            menu->removeItem(it->second.item_id);
            refreshList();
        }
    }
}

void MenuStructurePanel::onMoveUp(wxCommandEvent& event) {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (!sel.IsOk()) return;
    
    auto it = item_map_.find((int)(intptr_t)sel.m_pItem);
    if (it == item_map_.end() || it->second.is_menu) return;
    
    MenuDefinition* menu = MenuRegistry::get()->findMenu(it->second.menu_id);
    if (menu && menu->moveItemUp(it->second.item_id)) {
        refreshList();
    }
}

void MenuStructurePanel::onMoveDown(wxCommandEvent& event) {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (!sel.IsOk()) return;
    
    auto it = item_map_.find((int)(intptr_t)sel.m_pItem);
    if (it == item_map_.end() || it->second.is_menu) return;
    
    MenuDefinition* menu = MenuRegistry::get()->findMenu(it->second.menu_id);
    if (menu && menu->moveItemDown(it->second.item_id)) {
        refreshList();
    }
}

void MenuStructurePanel::onChangeAction(wxCommandEvent& event) {
    // TODO: Show action selection dialog
}

MenuStructurePanel::TreeItemInfo* MenuStructurePanel::getItemInfo(wxTreeItemId id) {
    auto it = item_map_.find((int)(intptr_t)id.m_pItem);
    if (it != item_map_.end()) {
        return &it->second;
    }
    return nullptr;
}


// ============================================================================
// WindowTypePanel Implementation
// ============================================================================

BEGIN_EVENT_TABLE(WindowTypePanel, wxPanel)
    EVT_LISTBOX(ID_WINDOW_TYPE_LIST, WindowTypePanel::onWindowTypeSelected)
    EVT_CHECKLISTBOX(ID_MENU_CHECKLIST, WindowTypePanel::onMenuToggled)
    EVT_CHECKBOX(ID_MERGE_CB, WindowTypePanel::onMergeChanged)
END_EVENT_TABLE()

WindowTypePanel::WindowTypePanel(wxWindow* parent, MenuEditor* editor)
    : wxPanel(parent, wxID_ANY)
    , editor_(editor) {
    createUI();
}

void WindowTypePanel::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Window type list
    wxStaticText* label1 = new wxStaticText(this, wxID_ANY, "Window Types:");
    main_sizer->Add(label1, 0, wxBOTTOM, 5);
    
    window_type_list_ = new wxListBox(this, ID_WINDOW_TYPE_LIST);
    main_sizer->Add(window_type_list_, 1, wxEXPAND | wxBOTTOM, 10);
    
    // Merge checkbox
    merge_cb_ = new wxCheckBox(this, ID_MERGE_CB, "Merge with Main Window menus");
    main_sizer->Add(merge_cb_, 0, wxBOTTOM, 10);
    
    // Menu checklist
    wxStaticText* label2 = new wxStaticText(this, wxID_ANY, "Menus for this window type:");
    main_sizer->Add(label2, 0, wxBOTTOM, 5);
    
    menu_checklist_ = new wxCheckListBox(this, ID_MENU_CHECKLIST);
    main_sizer->Add(menu_checklist_, 2, wxEXPAND);
    
    SetSizer(main_sizer);
}

void WindowTypePanel::refreshList() {
    populateWindowTypes();
}

void WindowTypePanel::populateWindowTypes() {
    window_type_list_->Clear();
    
    MenuRegistry* reg = MenuRegistry::get();
    auto menu_sets = reg->getAllWindowTypeMenuSets();
    
    for (WindowTypeMenuSet* menu_set : menu_sets) {
        wxString label = wxString::FromUTF8(menu_set->display_name.c_str());
        int idx = window_type_list_->Append(label);
        window_type_list_->SetClientData(idx, (void*)menu_set);
    }
    
    if (window_type_list_->GetCount() > 0) {
        window_type_list_->SetSelection(0);
        wxCommandEvent dummy;
        onWindowTypeSelected(dummy);
    }
}

void WindowTypePanel::populateMenuChecklist(const std::string& window_type_id) {
    menu_checklist_->Clear();
    
    MenuRegistry* reg = MenuRegistry::get();
    WindowTypeMenuSet* menu_set = reg->findWindowTypeMenuSet(window_type_id);
    
    if (!menu_set) return;
    
    // Update merge checkbox
    merge_cb_->SetValue(menu_set->merge_with_main);
    
    // Get all menus
    auto all_menus = reg->getAllMenus();
    
    for (MenuDefinition* menu : all_menus) {
        wxString label = wxString::FromUTF8(menu->display_name.c_str());
        int idx = menu_checklist_->Append(label);
        menu_checklist_->Check(idx, menu_set->hasMenu(menu->menu_id));
        menu_checklist_->SetClientData(idx, (void*)menu);
    }
}

std::string WindowTypePanel::getSelectedWindowType() const {
    int sel = window_type_list_->GetSelection();
    if (sel != wxNOT_FOUND) {
        WindowTypeMenuSet* menu_set = (WindowTypeMenuSet*)window_type_list_->GetClientData(sel);
        if (menu_set) {
            return menu_set->window_type_id;
        }
    }
    return "";
}

void WindowTypePanel::selectWindowType(const std::string& window_type_id) {
    for (unsigned int i = 0; i < window_type_list_->GetCount(); i++) {
        WindowTypeMenuSet* menu_set = (WindowTypeMenuSet*)window_type_list_->GetClientData(i);
        if (menu_set && menu_set->window_type_id == window_type_id) {
            window_type_list_->SetSelection(i);
            populateMenuChecklist(window_type_id);
            break;
        }
    }
}

void WindowTypePanel::onWindowTypeSelected(wxCommandEvent& event) {
    std::string window_type = getSelectedWindowType();
    if (!window_type.empty()) {
        selected_window_type_ = window_type;
        populateMenuChecklist(window_type);
        if (editor_) {
            editor_->onWindowTypeSelected(window_type);
        }
    }
}

void WindowTypePanel::onMenuToggled(wxCommandEvent& event) {
    int idx = event.GetInt();
    bool checked = menu_checklist_->IsChecked(idx);
    
    MenuDefinition* menu = (MenuDefinition*)menu_checklist_->GetClientData(idx);
    if (!menu) return;
    
    WindowTypeMenuSet* menu_set = MenuRegistry::get()->findWindowTypeMenuSet(selected_window_type_);
    if (!menu_set) return;
    
    if (checked) {
        menu_set->addMenu(menu->menu_id);
    } else {
        menu_set->removeMenu(menu->menu_id);
    }
}

void WindowTypePanel::onMergeChanged(wxCommandEvent& event) {
    WindowTypeMenuSet* menu_set = MenuRegistry::get()->findWindowTypeMenuSet(selected_window_type_);
    if (menu_set) {
        menu_set->merge_with_main = merge_cb_->GetValue();
    }
}

void WindowTypePanel::updateMenuAssignments() {
    // Refresh the checklist based on current assignments
    populateMenuChecklist(selected_window_type_);
}


// ============================================================================
// MenuItemEditDialog Implementation
// ============================================================================

BEGIN_EVENT_TABLE(MenuItemEditDialog, wxDialog)
    EVT_BUTTON(wxID_OK, MenuItemEditDialog::onOK)
END_EVENT_TABLE()

MenuItemEditDialog::MenuItemEditDialog(wxWindow* parent, MenuItem& item, Action* action)
    : wxDialog(parent, wxID_ANY, "Edit Menu Item", wxDefaultPosition, wxSize(450, 400))
    , item_(item)
    , action_(action) {
    current_icon_id_ = item.icon_id;
    current_action_id_ = item.action_id;
    createUI();
    loadData();
}

void MenuItemEditDialog::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Properties
    wxStaticBoxSizer* props_box = new wxStaticBoxSizer(wxVERTICAL, this, "Properties");
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 5, 5);
    grid->AddGrowableCol(1);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Label:"), 0, wxALIGN_CENTER_VERTICAL);
    label_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(label_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Tooltip:"), 0, wxALIGN_CENTER_VERTICAL);
    tooltip_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(tooltip_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Accelerator:"), 0, wxALIGN_CENTER_VERTICAL);
    accel_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(accel_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Help Topic:"), 0, wxALIGN_CENTER_VERTICAL);
    help_topic_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(help_topic_ctrl_, 1, wxEXPAND);
    
    props_box->Add(grid, 1, wxEXPAND | wxALL, 5);
    
    visible_cb_ = new wxCheckBox(this, wxID_ANY, "Visible");
    props_box->Add(visible_cb_, 0, wxALL, 5);
    
    checkable_cb_ = new wxCheckBox(this, wxID_ANY, "Checkable");
    props_box->Add(checkable_cb_, 0, wxALL, 5);
    
    checked_cb_ = new wxCheckBox(this, wxID_ANY, "Checked by default");
    props_box->Add(checked_cb_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    main_sizer->Add(props_box, 1, wxEXPAND | wxALL, 10);
    
    // Buttons
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer(1);
    btn_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
}

void MenuItemEditDialog::loadData() {
    label_ctrl_->SetValue(wxString::FromUTF8(item_.custom_label.c_str()));
    tooltip_ctrl_->SetValue(wxString::FromUTF8(item_.custom_tooltip.c_str()));
    accel_ctrl_->SetValue(wxString::FromUTF8(item_.custom_accel.c_str()));
    help_topic_ctrl_->SetValue(wxString::FromUTF8(item_.help_topic.c_str()));
    visible_cb_->SetValue(item_.is_visible);
    checkable_cb_->SetValue(item_.type == MenuItemType::kCheckBox || item_.type == MenuItemType::kRadio);
    checked_cb_->SetValue(item_.is_checked);
    checked_cb_->Enable(checkable_cb_->GetValue());
}

void MenuItemEditDialog::saveData() {
    item_.custom_label = std::string(label_ctrl_->GetValue().ToUTF8());
    item_.custom_tooltip = std::string(tooltip_ctrl_->GetValue().ToUTF8());
    item_.custom_accel = std::string(accel_ctrl_->GetValue().ToUTF8());
    item_.help_topic = std::string(help_topic_ctrl_->GetValue().ToUTF8());
    item_.is_visible = visible_cb_->GetValue();
    item_.is_checked = checked_cb_->GetValue();
    
    if (checkable_cb_->GetValue()) {
        // Keep as checkbox or radio, default to checkbox
        if (item_.type != MenuItemType::kCheckBox && item_.type != MenuItemType::kRadio) {
            item_.type = MenuItemType::kCheckBox;
        }
    } else {
        // Make it a regular action item
        item_.type = MenuItemType::kAction;
    }
}

void MenuItemEditDialog::onChangeAction(wxCommandEvent& event) {
    // TODO: Show action selection dialog
}

void MenuItemEditDialog::onChangeIcon(wxCommandEvent& event) {
    // TODO: Show icon selection dialog
}

void MenuItemEditDialog::onOK(wxCommandEvent& event) {
    saveData();
    EndModal(wxID_OK);
}

bool MenuItemEditDialog::ShowEditDialog() {
    return ShowModal() == wxID_OK;
}

// ============================================================================
// SubmenuEditDialog Implementation
// ============================================================================

BEGIN_EVENT_TABLE(SubmenuEditDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SubmenuEditDialog::onOK)
END_EVENT_TABLE()

SubmenuEditDialog::SubmenuEditDialog(wxWindow* parent, MenuItem& item)
    : wxDialog(parent, wxID_ANY, "Edit Submenu", wxDefaultPosition, wxSize(400, 200))
    , item_(item) {
    createUI();
    loadData();
}

void SubmenuEditDialog::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticBoxSizer* form_box = new wxStaticBoxSizer(wxVERTICAL, this, "Submenu Properties");
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 5, 5);
    grid->AddGrowableCol(1);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "ID:"), 0, wxALIGN_CENTER_VERTICAL);
    item_id_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    item_id_ctrl_->Enable(false);
    grid->Add(item_id_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Label:"), 0, wxALIGN_CENTER_VERTICAL);
    label_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(label_ctrl_, 1, wxEXPAND);
    
    form_box->Add(grid, 1, wxEXPAND | wxALL, 5);
    main_sizer->Add(form_box, 1, wxEXPAND | wxALL, 10);
    
    // Buttons
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer(1);
    btn_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
}

void SubmenuEditDialog::loadData() {
    item_id_ctrl_->SetValue(wxString::FromUTF8(item_.item_id.c_str()));
    label_ctrl_->SetValue(wxString::FromUTF8(item_.custom_label.c_str()));
}

void SubmenuEditDialog::saveData() {
    item_.custom_label = std::string(label_ctrl_->GetValue().ToUTF8());
}

bool SubmenuEditDialog::validateInput() {
    if (label_ctrl_->GetValue().IsEmpty()) {
        wxMessageBox("Label is required.", "Validation Error", wxOK | wxICON_ERROR);
        return false;
    }
    return true;
}

void SubmenuEditDialog::onOK(wxCommandEvent& event) {
    if (validateInput()) {
        saveData();
        EndModal(wxID_OK);
    }
}

bool SubmenuEditDialog::ShowEditDialog() {
    return ShowModal() == wxID_OK;
}


}  // namespace scratchrobin::ui
