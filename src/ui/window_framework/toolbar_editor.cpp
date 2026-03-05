/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/toolbar_editor.h"
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
#include <wx/wrapsizer.h>
#include <wx/bmpbuttn.h>
#include <wx/textdlg.h>
#include <wx/stattext.h>
#include <wx/utils.h>
#include <wx/statline.h>
#include <wx/choicdlg.h>
#include <wx/fileconf.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <chrono>
#include <algorithm>

namespace scratchrobin::ui {

// Event IDs
enum {
    ID_NEW_ACTION = 1000,
    ID_NEW_ICON,
    ID_NEW_TOOLBAR,
    ID_DELETE_TOOLBAR,
    ID_RENAME_TOOLBAR,
    ID_RESET_TOOLBARS,
    ID_IMPORT_TOOLBARS,
    ID_EXPORT_TOOLBARS,
    ID_HELP,
    ID_SEARCH_ACTIONS,
    ID_SEARCH_ICONS,
    ID_SEARCH_TOOLBARS,
    ID_ACTION_TREE,
    ID_ICON_PANEL,
    ID_TOOLBAR_TREE,
    ID_TB_RENAME,
    ID_TB_DELETE,
    ID_TB_EXPAND_COLLAPSE,
    ID_TB_ADD_SEPARATOR,
    ID_TB_ADD_SPACER,
    ID_TB_PROPERTIES,
    ID_ITEM_MOVE_UP,
    ID_ITEM_MOVE_DOWN,
    ID_ITEM_DELETE,
    ID_ITEM_EDIT,
    ID_ITEM_CHANGE_ACTION,
    ID_ITEM_CHANGE_ICON,
    ID_ACTION_NEW,
    ID_ACTION_EDIT,
    ID_ACTION_DELETE,
    ID_ICON_NEW
};

// Custom tree data for storing string data
class ActionTreeData : public wxTreeItemData {
public:
    explicit ActionTreeData(const wxString& data) : data_(data) {}
    wxString getData() const { return data_; }
private:
    wxString data_;
};

// ============================================================================
// ToolbarEditor Implementation
// ============================================================================

BEGIN_EVENT_TABLE(ToolbarEditor, wxDialog)
    EVT_BUTTON(ID_NEW_ACTION, ToolbarEditor::onNewAction)
    EVT_BUTTON(ID_NEW_ICON, ToolbarEditor::onNewIcon)
    EVT_BUTTON(ID_NEW_TOOLBAR, ToolbarEditor::onNewToolbar)
    EVT_BUTTON(ID_DELETE_TOOLBAR, ToolbarEditor::onDeleteToolbar)
    EVT_BUTTON(ID_RENAME_TOOLBAR, ToolbarEditor::onRenameToolbar)
    EVT_BUTTON(ID_RESET_TOOLBARS, ToolbarEditor::onResetToolbars)
    EVT_BUTTON(ID_IMPORT_TOOLBARS, ToolbarEditor::onImportToolbars)
    EVT_BUTTON(ID_EXPORT_TOOLBARS, ToolbarEditor::onExportToolbars)
    EVT_BUTTON(ID_HELP, ToolbarEditor::onHelp)
END_EVENT_TABLE()

ToolbarEditor::ToolbarEditor(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Customize Toolbars", 
               wxDefaultPosition, wxSize(1200, 800),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    
    // Initialize action registry
    ActionRegistry::get()->initialize();
    
    createUI();
    refreshAllPanels();
}

ToolbarEditor::~ToolbarEditor() = default;

int ToolbarEditor::ShowModal() {
    SetMinSize(wxSize(900, 700));
    return wxDialog::ShowModal();
}

void ToolbarEditor::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Create horizontal layout with three panels
    wxBoxSizer* panels_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left panel - Actions
    wxPanel* action_box = createActionPanel(this);
    panels_sizer->Add(action_box, 1, wxEXPAND | wxALL, 5);
    
    // Middle panel - Icons
    wxPanel* icon_box = createIconPanel(this);
    panels_sizer->Add(icon_box, 1, wxEXPAND | wxALL, 5);
    
    // Right panel - Toolbars
    wxPanel* toolbar_box = createToolbarPanel(this);
    panels_sizer->Add(toolbar_box, 1, wxEXPAND | wxALL, 5);
    
    main_sizer->Add(panels_sizer, 1, wxEXPAND);
    
    // Button bar
    createButtonBar(main_sizer);
    
    SetSizer(main_sizer);
}

wxPanel* ToolbarEditor::createActionPanel(wxWindow* parent) {
    wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, parent, "Actions");
    wxStaticBox* box_win = box->GetStaticBox();
    
    // Description
    wxStaticText* desc = new wxStaticText(box_win, wxID_ANY,
        "Search and select an action,\nthen choose an icon and toolbar.");
    desc->SetForegroundColour(wxColour(100, 100, 100));
    box->Add(desc, 0, wxALL, 5);
    
    // New action button
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(new wxButton(box_win, ID_NEW_ACTION, "New Action..."), 0, wxRIGHT, 5);
    box->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Actions panel
    action_panel_ = new ActionListPanel(box_win, this);
    box->Add(action_panel_, 1, wxEXPAND | wxALL, 5);
    
    wxPanel* container = new wxPanel(parent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(box, 1, wxEXPAND);
    container->SetSizer(sizer);
    
    return container;
}

wxPanel* ToolbarEditor::createIconPanel(wxWindow* parent) {
    wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, parent, "Icons");
    wxStaticBox* box_win = box->GetStaticBox();
    
    // Description
    wxStaticText* desc = new wxStaticText(box_win, wxID_ANY,
        "Select or search for an icon,\nthen drag to a toolbar.");
    desc->SetForegroundColour(wxColour(100, 100, 100));
    box->Add(desc, 0, wxALL, 5);
    
    // New icon button
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(new wxButton(box_win, ID_NEW_ICON, "New Icon..."), 0, wxRIGHT, 5);
    box->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Icons panel
    icon_panel_ = new IconListPanel(box_win, this);
    box->Add(icon_panel_, 1, wxEXPAND | wxALL, 5);
    
    wxPanel* container = new wxPanel(parent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(box, 1, wxEXPAND);
    container->SetSizer(sizer);
    
    return container;
}

wxPanel* ToolbarEditor::createToolbarPanel(wxWindow* parent) {
    wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, parent, "Toolbars");
    wxStaticBox* box_win = box->GetStaticBox();
    
    // Description
    wxStaticText* desc = new wxStaticText(box_win, wxID_ANY,
        "Right-click items to edit.\nDouble-click toolbar to expand/collapse.");
    desc->SetForegroundColour(wxColour(100, 100, 100));
    box->Add(desc, 0, wxALL, 5);
    
    // New toolbar button
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(new wxButton(box_win, ID_NEW_TOOLBAR, "New Toolbar..."), 0);
    box->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Toolbar panel
    toolbar_panel_ = new ToolbarListPanel(box_win, this);
    box->Add(toolbar_panel_, 1, wxEXPAND | wxALL, 5);
    
    wxPanel* container = new wxPanel(parent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(box, 1, wxEXPAND);
    container->SetSizer(sizer);
    
    return container;
}

void ToolbarEditor::createButtonBar(wxSizer* parent_sizer) {
    wxStaticLine* line = new wxStaticLine(this);
    parent_sizer->Add(line, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Management buttons
    btn_sizer->Add(new wxButton(this, ID_RESET_TOOLBARS, "Reset to Defaults"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, ID_IMPORT_TOOLBARS, "Import..."), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, ID_EXPORT_TOOLBARS, "Export..."), 0, wxRIGHT, 5);
    
    btn_sizer->AddStretchSpacer(1);
    
    // Help and OK/Cancel
    btn_sizer->Add(new wxButton(this, ID_HELP, "Help"), 0, wxRIGHT, 20);
    btn_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    
    parent_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
}

void ToolbarEditor::updateForActionSelection(const std::string& action_id) {
    icon_panel_->setFilterForAction(action_id);
}

void ToolbarEditor::updateForIconSelection(const std::string& icon_id) {
    // Update icon panel selection
}

void ToolbarEditor::onDropOnIconPanel(const std::string& action_id) {
    updateForActionSelection(action_id);
}

void ToolbarEditor::onDropOnToolbar(const std::string& toolbar_id, const std::string& insert_after_item_id) {
    ActionRegistry* reg = ActionRegistry::get();
    ToolbarDefinition* toolbar = reg->findToolbar(toolbar_id);
    if (!toolbar) return;
    
    int insert_pos = -1;
    if (!insert_after_item_id.empty()) {
        // Find position after the specified item
        for (size_t i = 0; i < toolbar->items.size(); i++) {
            if (toolbar->items[i].item_id == insert_after_item_id) {
                insert_pos = (int)i + 1;
                break;
            }
        }
    }
    
    switch (drag_data_.type) {
        case ToolbarDragData::DragType::kAction: {
            // Add action with default icon
            Action* action = reg->findAction(drag_data_.action_id);
            if (action) {
                ToolbarItem item;
                item.item_id = "item_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                item.action_id = drag_data_.action_id;
                item.icon_id = action->default_icon_name;
                reg->addItemToToolbar(toolbar_id, item, insert_pos);
            }
            break;
        }
        case ToolbarDragData::DragType::kIcon: {
            // Add action with selected icon (must have action)
            if (!drag_data_.action_id.empty()) {
                ToolbarItem item;
                item.item_id = "item_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                item.action_id = drag_data_.action_id;
                item.icon_id = drag_data_.icon_id;
                reg->addItemToToolbar(toolbar_id, item, insert_pos);
            }
            break;
        }
        case ToolbarDragData::DragType::kToolbarItem: {
            // Reorder within/across toolbars
            if (drag_data_.toolbar_id == toolbar_id) {
                // Move within same toolbar
                reg->moveItemInToolbar(toolbar_id, drag_data_.item_id, insert_pos >= 0 ? insert_pos : (int)toolbar->items.size());
            } else {
                // Move to different toolbar
                ToolbarDefinition* src_toolbar = reg->findToolbar(drag_data_.toolbar_id);
                if (src_toolbar) {
                    // Find the item
                    for (const auto& item : src_toolbar->items) {
                        if (item.item_id == drag_data_.item_id) {
                            reg->addItemToToolbar(toolbar_id, item, insert_pos);
                            reg->removeItemFromToolbar(drag_data_.toolbar_id, drag_data_.item_id);
                            break;
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    
    toolbar_panel_->refreshList();
    drag_data_.reset();
}

void ToolbarEditor::refreshAllPanels() {
    action_panel_->refreshList();
    icon_panel_->refreshList();
    toolbar_panel_->refreshList();
}

void ToolbarEditor::refreshToolbarPanel() {
    toolbar_panel_->refreshList();
}

void ToolbarEditor::onNewAction(wxCommandEvent& event) {
    ActionEditDialog dlg(this);
    if (dlg.ShowEditDialog()) {
        Action action = dlg.getAction();
        ActionRegistry::get()->registerAction(action);
        action_panel_->refreshList();
    }
}

void ToolbarEditor::onNewIcon(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Select SVG Icon", "", "", 
                     "SVG files (*.svg)|*.svg|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
        wxString path = dlg.GetPath();
        wxString name = dlg.GetFilename();
        name = name.BeforeLast('.');  // Remove extension
        
        wxTextEntryDialog name_dlg(this, "Enter display name for icon:", "Icon Name", name);
        if (name_dlg.ShowModal() == wxID_OK) {
            std::string icon_id = "custom." + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            res::IconManager::get()->loadCustomIcon(
                std::string(path.ToUTF8()),
                icon_id,
                std::string(name_dlg.GetValue().ToUTF8())
            );
            icon_panel_->refreshList();
        }
    }
}

void ToolbarEditor::onNewToolbar(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this, "Enter name for new toolbar:", "New Toolbar");
    if (dlg.ShowModal() == wxID_OK) {
        wxString name = dlg.GetValue();
        if (!name.IsEmpty()) {
            ToolbarDefinition toolbar;
            auto now = std::chrono::system_clock::now();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            toolbar.toolbar_id = "custom." + std::to_string(millis);
            toolbar.display_name = std::string(name.ToUTF8());
            toolbar.description = "";
            toolbar.is_default = false;
            toolbar.is_visible = true;
            toolbar.preferred_dock = wxTOP;
            toolbar.icon_size = 24;
            toolbar.show_text_labels = false;
            toolbar.is_expanded = true;
            
            ActionRegistry::get()->registerToolbar(toolbar);
            toolbar_panel_->refreshList();
        }
    }
}

void ToolbarEditor::onDeleteToolbar(wxCommandEvent& event) {
    std::string id = toolbar_panel_->getSelectedToolbar();
    if (id.empty()) {
        wxMessageBox("Please select a toolbar to delete.", "Delete Toolbar", wxOK | wxICON_INFORMATION);
        return;
    }
    
    int result = wxMessageBox("Delete selected toolbar?", "Confirm Delete", 
                               wxYES_NO | wxICON_QUESTION);
    if (result == wxYES) {
        ActionRegistry::get()->unregisterToolbar(id);
        toolbar_panel_->refreshList();
    }
}

void ToolbarEditor::onRenameToolbar(wxCommandEvent& event) {
    std::string id = toolbar_panel_->getSelectedToolbar();
    if (id.empty()) {
        wxMessageBox("Please select a toolbar to rename.", "Rename Toolbar", wxOK | wxICON_INFORMATION);
        return;
    }
    
    ToolbarDefinition* toolbar = ActionRegistry::get()->findToolbar(id);
    if (toolbar) {
        wxTextEntryDialog dlg(this, "Enter new name:", "Rename Toolbar",
                             wxString::FromUTF8(toolbar->display_name.c_str()));
        if (dlg.ShowModal() == wxID_OK) {
            wxString name = dlg.GetValue();
            if (!name.IsEmpty()) {
                toolbar->display_name = std::string(name.ToUTF8());
                toolbar_panel_->refreshList();
            }
        }
    }
}

void ToolbarEditor::onResetToolbars(wxCommandEvent& event) {
    int result = wxMessageBox("Reset all toolbars to default?\nThis will delete all custom toolbars and modifications.",
                               "Reset Toolbars", wxYES_NO | wxICON_WARNING);
    if (result == wxYES) {
        ActionRegistry::get()->resetToDefaults();
        refreshAllPanels();
    }
}

void ToolbarEditor::onImportToolbars(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Import Toolbars", "", "", 
                     "Toolbar files (*.tbar)|*.tbar|JSON files (*.json)|*.json|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
        if (ActionRegistry::get()->importToolbarsFromFile(std::string(dlg.GetPath().ToUTF8()))) {
            wxMessageBox("Toolbars imported successfully.", "Import", wxOK | wxICON_INFORMATION);
            refreshAllPanels();
        } else {
            wxMessageBox("Failed to import toolbars.", "Import Error", wxOK | wxICON_ERROR);
        }
    }
}

void ToolbarEditor::onExportToolbars(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Export Toolbars", "", "toolbars.tbar",
                     "Toolbar files (*.tbar)|*.tbar|JSON files (*.json)|*.json|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) {
        if (ActionRegistry::get()->exportToolbarsToFile(std::string(dlg.GetPath().ToUTF8()))) {
            wxMessageBox("Toolbars exported successfully.", "Export", wxOK | wxICON_INFORMATION);
        } else {
            wxMessageBox("Failed to export toolbars.", "Export Error", wxOK | wxICON_ERROR);
        }
    }
}

void ToolbarEditor::onHelp(wxCommandEvent& event) {
    wxMessageBox(
        "Customize Toolbars Help:\n\n"
        "1. Search and select an Action from the left panel\n"
        "2. Select or drag an Icon from the middle panel\n"
        "3. Drag the Icon to a Toolbar on the right\n\n"
        "Or drag an Action directly to a Toolbar to use its default icon.\n\n"
        "Toolbar Management:\n"
        "- Double-click a toolbar name to expand/collapse\n"
        "- Right-click a toolbar name to rename or delete\n"
        "- Right-click a toolbar item to edit, move, or delete\n\n"
        "Creating Custom Elements:\n"
        "- Use 'New Action' to create custom actions\n"
        "- Use 'New Icon' to import custom SVG icons\n"
        "- Use 'New Toolbar' to create custom toolbars",
        "Toolbar Editor Help", wxOK | wxICON_INFORMATION);
}


// ============================================================================
// ActionListPanel Implementation
// ============================================================================

BEGIN_EVENT_TABLE(ActionListPanel, wxPanel)
    EVT_SEARCHCTRL_SEARCH_BTN(ID_SEARCH_ACTIONS, ActionListPanel::onSearch)
    EVT_TEXT(ID_SEARCH_ACTIONS, ActionListPanel::onSearch)
    EVT_TREE_SEL_CHANGED(ID_ACTION_TREE, ActionListPanel::onItemSelected)
    EVT_TREE_ITEM_ACTIVATED(ID_ACTION_TREE, ActionListPanel::onItemActivated)
    EVT_TREE_BEGIN_DRAG(ID_ACTION_TREE, ActionListPanel::onBeginDrag)
    EVT_TREE_ITEM_MENU(ID_ACTION_TREE, ActionListPanel::onContextMenu)
END_EVENT_TABLE()

ActionListPanel::ActionListPanel(wxWindow* parent, ToolbarEditor* editor)
    : wxPanel(parent, wxID_ANY)
    , editor_(editor) {
    createUI();
}

void ActionListPanel::createUI() {
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

void ActionListPanel::refreshList() {
    filterActions(search_ctrl_->GetValue());
}

void ActionListPanel::populateActions() {
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
            tree_ctrl_->SetItemData(item, new ActionTreeData(
                wxString::FromUTF8(action->action_id.c_str())));
        }
        
        tree_ctrl_->Expand(cat_item);
    }
}

void ActionListPanel::filterActions(const wxString& filter) {
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
        tree_ctrl_->SetItemData(item, new ActionTreeData(
            wxString::FromUTF8(action->action_id.c_str())));
    }
}

std::string ActionListPanel::getSelectedAction() const {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (sel.IsOk()) {
        ActionTreeData* data = dynamic_cast<ActionTreeData*>(tree_ctrl_->GetItemData(sel));
        if (data) {
            return std::string(data->getData().ToUTF8());
        }
    }
    return "";
}

void ActionListPanel::setSearchText(const wxString& text) {
    search_ctrl_->SetValue(text);
    filterActions(text);
}

void ActionListPanel::onSearch(wxCommandEvent& event) {
    filterActions(search_ctrl_->GetValue());
}

void ActionListPanel::onItemSelected(wxTreeEvent& event) {
    std::string action_id = getSelectedAction();
    if (!action_id.empty() && editor_) {
        editor_->updateForActionSelection(action_id);
    }
    event.Skip();
}

void ActionListPanel::onItemActivated(wxTreeEvent& event) {
    // Double-click - could show edit dialog
    onEditAction(event);
}

void ActionListPanel::onBeginDrag(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    ActionTreeData* data = dynamic_cast<ActionTreeData*>(tree_ctrl_->GetItemData(item));
    if (data && editor_) {
        std::string action_id = std::string(data->getData().ToUTF8());
        editor_->drag_data_.type = ToolbarDragData::DragType::kAction;
        editor_->drag_data_.action_id = action_id;
        
        wxTextDataObject drag_data("action:" + data->getData());
        wxDropSource drag_source(this);
        drag_source.SetData(drag_data);
        drag_source.DoDragDrop(wxDrag_CopyOnly);
    }
    event.Skip();
}

void ActionListPanel::onContextMenu(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk()) return;
    
    tree_ctrl_->SelectItem(item);
    
    wxMenu menu;
    menu.Append(ID_ACTION_NEW, "New Action...");
    menu.AppendSeparator();
    menu.Append(ID_ACTION_EDIT, "Edit...");
    menu.Append(ID_ACTION_DELETE, "Delete");
    
    PopupMenu(&menu);
}

void ActionListPanel::onNewAction(wxCommandEvent& event) {
    if (editor_) {
        editor_->onNewAction(event);
    }
}

void ActionListPanel::onEditAction(wxCommandEvent& event) {
    std::string action_id = getSelectedAction();
    if (action_id.empty()) return;
    
    Action* action = ActionRegistry::get()->findAction(action_id);
    if (!action) return;
    
    ActionEditDialog dlg(this, action);
    if (dlg.ShowEditDialog()) {
        refreshList();
    }
}

void ActionListPanel::onDeleteAction(wxCommandEvent& event) {
    std::string action_id = getSelectedAction();
    if (action_id.empty()) return;
    
    Action* action = ActionRegistry::get()->findAction(action_id);
    if (action && action->is_builtin) {
        wxMessageBox("Cannot delete built-in actions.", "Delete Action", wxOK | wxICON_INFORMATION);
        return;
    }
    
    int result = wxMessageBox("Delete this action?", "Confirm Delete", wxYES_NO | wxICON_QUESTION);
    if (result == wxYES) {
        ActionRegistry::get()->unregisterAction(action_id);
        refreshList();
    }
}


// ============================================================================
// IconListPanel Implementation
// ============================================================================

BEGIN_EVENT_TABLE(IconListPanel, wxScrolledWindow)
    EVT_SEARCHCTRL_SEARCH_BTN(ID_SEARCH_ICONS, IconListPanel::onSearch)
    EVT_TEXT(ID_SEARCH_ICONS, IconListPanel::onSearch)
END_EVENT_TABLE()

IconListPanel::IconListPanel(wxWindow* parent, ToolbarEditor* editor)
    : wxScrolledWindow(parent, wxID_ANY)
    , editor_(editor) {
    createUI();
}

void IconListPanel::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Search control
    search_ctrl_ = new wxSearchCtrl(this, ID_SEARCH_ICONS, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    search_ctrl_->SetDescriptiveText("Search icons...");
    search_ctrl_->ShowSearchButton(true);
    search_ctrl_->ShowCancelButton(true);
    main_sizer->Add(search_ctrl_, 0, wxEXPAND | wxBOTTOM, 5);
    
    // Container for icons
    icons_container_ = new wxPanel(this, ID_ICON_PANEL);
    icons_sizer_ = new wxWrapSizer(wxHORIZONTAL);
    icons_container_->SetSizer(icons_sizer_);
    
    main_sizer->Add(icons_container_, 1, wxEXPAND | wxALL, 5);
    SetSizer(main_sizer);
    
    SetScrollRate(10, 10);
    EnableScrolling(true, true);
}

void IconListPanel::refreshList() {
    filterIcons(search_ctrl_->GetValue());
}

void IconListPanel::populateIcons() {
    // Clear existing
    icons_sizer_->Clear(true);
    
    res::IconManager* mgr = res::IconManager::get();
    auto icons = mgr->getAllIcons();
    
    for (res::IconInfo* icon : icons) {
        if (!icon->bitmap.IsOk()) continue;
        
        wxBitmapButton* btn = new wxBitmapButton(icons_container_, wxID_ANY,
                                                  icon->bitmap.GetBitmap(wxSize(24, 24)),
                                                  wxDefaultPosition, wxSize(32, 32),
                                                  wxBU_AUTODRAW | wxBORDER_NONE);
        
        btn->SetToolTip(wxString::FromUTF8(icon->display_name.c_str()));
        
        btn->Bind(wxEVT_BUTTON, [this, icon_id = icon->id](wxCommandEvent&) {
            selected_icon_ = icon_id;
            if (editor_) {
                editor_->updateForIconSelection(icon_id);
            }
        });
        
        btn->Bind(wxEVT_LEFT_DOWN, [this, icon_id = icon->id](wxMouseEvent& evt) {
            selected_icon_ = icon_id;
            if (editor_) {
                editor_->drag_data_.type = ToolbarDragData::DragType::kIcon;
                editor_->drag_data_.icon_id = icon_id;
                if (!filter_action_.empty()) {
                    editor_->drag_data_.action_id = filter_action_;
                }
            }
            
            wxTextDataObject drag_data("icon:" + wxString::FromUTF8(icon_id.c_str()));
            wxDropSource drag_source(this);
            drag_source.SetData(drag_data);
            drag_source.DoDragDrop(wxDrag_CopyOnly);
        });
        
        icons_sizer_->Add(btn, 0, wxALL, 2);
    }
    
    icons_container_->Layout();
    FitInside();
}

void IconListPanel::filterIcons(const wxString& filter) {
    // Clear existing
    icons_sizer_->Clear(true);
    
    res::IconManager* mgr = res::IconManager::get();
    std::vector<res::IconInfo*> icons;
    
    if (filter.IsEmpty()) {
        icons = mgr->getAllIcons();
    } else {
        icons = mgr->searchIcons(std::string(filter.ToUTF8()));
    }
    
    for (res::IconInfo* icon : icons) {
        if (!icon->bitmap.IsOk()) continue;
        
        wxBitmapButton* btn = new wxBitmapButton(icons_container_, wxID_ANY,
                                                  icon->bitmap.GetBitmap(wxSize(24, 24)),
                                                  wxDefaultPosition, wxSize(32, 32),
                                                  wxBU_AUTODRAW | wxBORDER_NONE);
        
        btn->SetToolTip(wxString::FromUTF8((icon->display_name + " (" + icon->category + ")").c_str()));
        
        btn->Bind(wxEVT_BUTTON, [this, icon_id = icon->id](wxCommandEvent&) {
            selected_icon_ = icon_id;
            if (editor_) {
                editor_->updateForIconSelection(icon_id);
            }
        });
        
        btn->Bind(wxEVT_LEFT_DOWN, [this, icon_id = icon->id](wxMouseEvent& evt) {
            selected_icon_ = icon_id;
            if (editor_) {
                editor_->drag_data_.type = ToolbarDragData::DragType::kIcon;
                editor_->drag_data_.icon_id = icon_id;
                if (!filter_action_.empty()) {
                    editor_->drag_data_.action_id = filter_action_;
                }
            }
            
            wxTextDataObject drag_data("icon:" + wxString::FromUTF8(icon_id.c_str()));
            wxDropSource drag_source(this);
            drag_source.SetData(drag_data);
            drag_source.DoDragDrop(wxDrag_CopyOnly);
        });
        
        icons_sizer_->Add(btn, 0, wxALL, 2);
    }
    
    icons_container_->Layout();
    FitInside();
    SetScrollRate(10, 10);
}

void IconListPanel::setSearchText(const wxString& text) {
    search_ctrl_->SetValue(text);
    filterIcons(text);
}

void IconListPanel::setFilterForAction(const std::string& action_id) {
    filter_action_ = action_id;
    
    Action* action = ActionRegistry::get()->findAction(action_id);
    if (action) {
        // Highlight default icon or filter relevant icons
        // For now, just refresh
        refreshList();
    }
}

void IconListPanel::clearIconSelection() {
    selected_icon_.clear();
}

void IconListPanel::onSearch(wxCommandEvent& event) {
    filterIcons(search_ctrl_->GetValue());
}

void IconListPanel::onIconClick(wxMouseEvent& event) {
    event.Skip();
}

void IconListPanel::onIconBeginDrag(wxMouseEvent& event) {
    if (!selected_icon_.empty() && editor_) {
        editor_->drag_data_.type = ToolbarDragData::DragType::kIcon;
        editor_->drag_data_.icon_id = selected_icon_;
        if (!filter_action_.empty()) {
            editor_->drag_data_.action_id = filter_action_;
        }
    }
}

void IconListPanel::onNewIcon(wxCommandEvent& event) {
    if (editor_) {
        editor_->onNewIcon(event);
    }
}

void IconListPanel::onContextMenu(wxMouseEvent& event) {
    // Context menu for icon panel
}


// ============================================================================
// ToolbarListPanel Implementation
// ============================================================================

BEGIN_EVENT_TABLE(ToolbarListPanel, wxPanel)
    EVT_SEARCHCTRL_SEARCH_BTN(ID_SEARCH_TOOLBARS, ToolbarListPanel::onSearch)
    EVT_TEXT(ID_SEARCH_TOOLBARS, ToolbarListPanel::onSearch)
    EVT_TREE_SEL_CHANGED(ID_TOOLBAR_TREE, ToolbarListPanel::onItemSelected)
    EVT_TREE_ITEM_ACTIVATED(ID_TOOLBAR_TREE, ToolbarListPanel::onItemActivated)
    EVT_TREE_BEGIN_DRAG(ID_TOOLBAR_TREE, ToolbarListPanel::onBeginDrag)
    EVT_TREE_END_DRAG(ID_TOOLBAR_TREE, ToolbarListPanel::onEndDrag)
    EVT_TREE_ITEM_MENU(ID_TOOLBAR_TREE, ToolbarListPanel::onContextMenu)
    EVT_TREE_ITEM_EXPANDED(ID_TOOLBAR_TREE, ToolbarListPanel::onItemExpanded)
    EVT_TREE_ITEM_COLLAPSED(ID_TOOLBAR_TREE, ToolbarListPanel::onItemCollapsed)
    EVT_MENU(ID_TB_RENAME, ToolbarListPanel::onToolbarRename)
    EVT_MENU(ID_TB_DELETE, ToolbarListPanel::onToolbarDelete)
    EVT_MENU(ID_TB_EXPAND_COLLAPSE, ToolbarListPanel::onToolbarExpandCollapse)
    EVT_MENU(ID_TB_ADD_SEPARATOR, ToolbarListPanel::onToolbarAddSeparator)
    EVT_MENU(ID_TB_ADD_SPACER, ToolbarListPanel::onToolbarAddSpacer)
    EVT_MENU(ID_TB_PROPERTIES, ToolbarListPanel::onToolbarProperties)
    EVT_MENU(ID_ITEM_MOVE_UP, ToolbarListPanel::onItemMoveUp)
    EVT_MENU(ID_ITEM_MOVE_DOWN, ToolbarListPanel::onItemMoveDown)
    EVT_MENU(ID_ITEM_DELETE, ToolbarListPanel::onItemDelete)
    EVT_MENU(ID_ITEM_EDIT, ToolbarListPanel::onItemEdit)
    EVT_MENU(ID_ITEM_CHANGE_ACTION, ToolbarListPanel::onItemChangeAction)
    EVT_MENU(ID_ITEM_CHANGE_ICON, ToolbarListPanel::onItemChangeIcon)
END_EVENT_TABLE()

ToolbarListPanel::ToolbarListPanel(wxWindow* parent, ToolbarEditor* editor)
    : wxPanel(parent, wxID_ANY)
    , editor_(editor) {
    createUI();
}

void ToolbarListPanel::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Search control
    search_ctrl_ = new wxSearchCtrl(this, ID_SEARCH_TOOLBARS, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    search_ctrl_->SetDescriptiveText("Search toolbars...");
    search_ctrl_->ShowSearchButton(true);
    search_ctrl_->ShowCancelButton(true);
    main_sizer->Add(search_ctrl_, 0, wxEXPAND | wxBOTTOM, 5);
    
    // Tree control
    tree_ctrl_ = new wxTreeCtrl(this, ID_TOOLBAR_TREE, wxDefaultPosition, wxDefaultSize,
                                wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT |
                                wxTR_FULL_ROW_HIGHLIGHT | wxTR_SINGLE);
    main_sizer->Add(tree_ctrl_, 1, wxEXPAND);
    
    SetSizer(main_sizer);
}

void ToolbarListPanel::refreshList() {
    filterToolbars(search_ctrl_->GetValue());
}

void ToolbarListPanel::populateToolbars() {
    item_map_.clear();
    tree_ctrl_->DeleteAllItems();
    wxTreeItemId root = tree_ctrl_->AddRoot("Toolbars");
    
    ActionRegistry* reg = ActionRegistry::get();
    auto toolbars = reg->getAllToolbars();
    
    for (ToolbarDefinition* toolbar : toolbars) {
        addToolbarToTree(*toolbar, root);
    }
    
    tree_ctrl_->ExpandAll();
}

void ToolbarListPanel::addToolbarToTree(const ToolbarDefinition& toolbar, wxTreeItemId parent) {
    wxString label = wxString::FromUTF8(toolbar.display_name.c_str());
    wxTreeItemId toolbar_item = tree_ctrl_->AppendItem(parent, label);
    
    // Set expanded state
    bool expanded = toolbar.is_expanded;
    auto it = expanded_state_.find(toolbar.toolbar_id);
    if (it != expanded_state_.end()) {
        expanded = it->second;
    }
    
    // Map this item
    TreeItemInfo info;
    info.toolbar_id = toolbar.toolbar_id;
    info.is_toolbar = true;
    item_map_[(int)(intptr_t)toolbar_item.m_pItem] = info;
    
    // Add items
    for (const auto& item : toolbar.items) {
        Action* action = ActionRegistry::get()->findAction(item.action_id);
        wxString item_label;
        
        if (item.is_separator) {
            item_label = "----- Separator -----";
        } else if (item.is_spacer) {
            item_label = "~~~~~ Spacer ~~~~~";
        } else {
            if (!item.custom_label.empty()) {
                item_label = wxString::FromUTF8(item.custom_label.c_str());
            } else if (action) {
                item_label = wxString::FromUTF8(action->display_name.c_str());
            } else {
                item_label = wxString::FromUTF8(("Unknown: " + item.action_id).c_str());
            }
        }
        
        wxTreeItemId item_node = tree_ctrl_->AppendItem(toolbar_item, item_label);
        
        // Map this item
        TreeItemInfo item_info;
        item_info.toolbar_id = toolbar.toolbar_id;
        item_info.item_id = item.item_id;
        item_info.is_toolbar = false;
        item_map_[(int)(intptr_t)item_node.m_pItem] = item_info;
    }
    
    if (expanded) {
        tree_ctrl_->Expand(toolbar_item);
    }
}

void ToolbarListPanel::filterToolbars(const wxString& filter) {
    if (filter.IsEmpty()) {
        populateToolbars();
        return;
    }
    
    item_map_.clear();
    tree_ctrl_->DeleteAllItems();
    wxTreeItemId root = tree_ctrl_->AddRoot("Toolbars");
    
    ActionRegistry* reg = ActionRegistry::get();
    auto toolbars = reg->searchToolbars(std::string(filter.ToUTF8()));
    
    for (ToolbarDefinition* toolbar : toolbars) {
        addToolbarToTree(*toolbar, root);
    }
}

std::string ToolbarListPanel::getSelectedToolbar() const {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (sel.IsOk()) {
        auto it = item_map_.find((int)(intptr_t)sel.m_pItem);
        if (it != item_map_.end()) {
            return it->second.toolbar_id;
        }
    }
    return "";
}

std::string ToolbarListPanel::getSelectedItemId() const {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (sel.IsOk()) {
        auto it = item_map_.find((int)(intptr_t)sel.m_pItem);
        if (it != item_map_.end() && !it->second.is_toolbar) {
            return it->second.item_id;
        }
    }
    return "";
}

bool ToolbarListPanel::isToolbarExpanded(const std::string& toolbar_id) const {
    auto it = expanded_state_.find(toolbar_id);
    if (it != expanded_state_.end()) {
        return it->second;
    }
    ToolbarDefinition* toolbar = ActionRegistry::get()->findToolbar(toolbar_id);
    return toolbar ? toolbar->is_expanded : true;
}

void ToolbarListPanel::setToolbarExpanded(const std::string& toolbar_id, bool expanded) {
    expanded_state_[toolbar_id] = expanded;
    ToolbarDefinition* toolbar = ActionRegistry::get()->findToolbar(toolbar_id);
    if (toolbar) {
        toolbar->is_expanded = expanded;
    }
}

void ToolbarListPanel::onSearch(wxCommandEvent& event) {
    filterToolbars(search_ctrl_->GetValue());
}

void ToolbarListPanel::onItemSelected(wxTreeEvent& event) {
    event.Skip();
}

void ToolbarListPanel::onItemActivated(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    auto it = item_map_.find((int)(intptr_t)item.m_pItem);
    if (it != item_map_.end()) {
        if (it->second.is_toolbar) {
            // Toggle expand/collapse on double-click
            if (tree_ctrl_->IsExpanded(item)) {
                tree_ctrl_->Collapse(item);
            } else {
                tree_ctrl_->Expand(item);
            }
        } else {
            // Edit item on double-click
            wxCommandEvent dummy;
            onItemEdit(dummy);
        }
    }
}

void ToolbarListPanel::onBeginDrag(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    auto it = item_map_.find((int)(intptr_t)item.m_pItem);
    if (it != item_map_.end() && !it->second.is_toolbar && editor_) {
        editor_->drag_data_.type = ToolbarDragData::DragType::kToolbarItem;
        editor_->drag_data_.toolbar_id = it->second.toolbar_id;
        editor_->drag_data_.item_id = it->second.item_id;
        event.Allow();
    }
}

void ToolbarListPanel::onEndDrag(wxTreeEvent& event) {
    if (!editor_) return;
    
    wxTreeItemId src = event.GetItem();
    wxPoint pos = event.GetPoint();
    
    int flags;
    wxTreeItemId dst = tree_ctrl_->HitTest(pos, flags);
    
    if (dst.IsOk()) {
        auto dst_it = item_map_.find((int)(intptr_t)dst.m_pItem);
        if (dst_it != item_map_.end()) {
            std::string insert_after;
            if (!dst_it->second.is_toolbar) {
                insert_after = dst_it->second.item_id;
            }
            editor_->onDropOnToolbar(dst_it->second.toolbar_id, insert_after);
        }
    }
}

void ToolbarListPanel::onContextMenu(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk()) return;
    
    tree_ctrl_->SelectItem(item);
    
    auto it = item_map_.find((int)(intptr_t)item.m_pItem);
    if (it == item_map_.end()) return;
    
    wxMenu menu;
    
    if (it->second.is_toolbar) {
        // Toolbar context menu
        bool expanded = tree_ctrl_->IsExpanded(item);
        menu.Append(ID_TB_EXPAND_COLLAPSE, expanded ? "Collapse" : "Expand");
        menu.AppendSeparator();
        menu.Append(ID_TB_RENAME, "Rename...");
        menu.Append(ID_TB_DELETE, "Delete");
        menu.AppendSeparator();
        menu.Append(ID_TB_ADD_SEPARATOR, "Add Separator");
        menu.Append(ID_TB_ADD_SPACER, "Add Spacer");
        menu.AppendSeparator();
        menu.Append(ID_TB_PROPERTIES, "Properties...");
    } else {
        // Item context menu
        menu.Append(ID_ITEM_MOVE_UP, "Move Up");
        menu.Append(ID_ITEM_MOVE_DOWN, "Move Down");
        menu.AppendSeparator();
        menu.Append(ID_ITEM_EDIT, "Edit...");
        menu.Append(ID_ITEM_CHANGE_ACTION, "Change Action...");
        menu.Append(ID_ITEM_CHANGE_ICON, "Change Icon...");
        menu.AppendSeparator();
        menu.Append(ID_ITEM_DELETE, "Delete");
    }
    
    PopupMenu(&menu);
}

void ToolbarListPanel::onItemExpanded(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    auto it = item_map_.find((int)(intptr_t)item.m_pItem);
    if (it != item_map_.end() && it->second.is_toolbar) {
        setToolbarExpanded(it->second.toolbar_id, true);
    }
}

void ToolbarListPanel::onItemCollapsed(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    auto it = item_map_.find((int)(intptr_t)item.m_pItem);
    if (it != item_map_.end() && it->second.is_toolbar) {
        setToolbarExpanded(it->second.toolbar_id, false);
    }
}

void ToolbarListPanel::onToolbarRename(wxCommandEvent& event) {
    if (editor_) {
        editor_->onRenameToolbar(event);
    }
}

void ToolbarListPanel::onToolbarDelete(wxCommandEvent& event) {
    if (editor_) {
        editor_->onDeleteToolbar(event);
    }
}

void ToolbarListPanel::onToolbarExpandCollapse(wxCommandEvent& event) {
    wxTreeItemId sel = tree_ctrl_->GetSelection();
    if (!sel.IsOk()) return;
    
    if (tree_ctrl_->IsExpanded(sel)) {
        tree_ctrl_->Collapse(sel);
    } else {
        tree_ctrl_->Expand(sel);
    }
}

void ToolbarListPanel::onToolbarAddSeparator(wxCommandEvent& event) {
    std::string toolbar_id = getSelectedToolbar();
    if (toolbar_id.empty()) return;
    
    ToolbarItem sep;
    sep.item_id = "sep_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    sep.is_separator = true;
    
    ActionRegistry::get()->addItemToToolbar(toolbar_id, sep);
    refreshList();
}

void ToolbarListPanel::onToolbarAddSpacer(wxCommandEvent& event) {
    std::string toolbar_id = getSelectedToolbar();
    if (toolbar_id.empty()) return;
    
    ToolbarItem spacer;
    spacer.item_id = "spacer_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    spacer.is_spacer = true;
    
    ActionRegistry::get()->addItemToToolbar(toolbar_id, spacer);
    refreshList();
}

void ToolbarListPanel::onToolbarProperties(wxCommandEvent& event) {
    // TODO: Show toolbar properties dialog
}

void ToolbarListPanel::onItemMoveUp(wxCommandEvent& event) {
    std::string toolbar_id = getSelectedToolbar();
    std::string item_id = getSelectedItemId();
    if (toolbar_id.empty() || item_id.empty()) return;
    
    ToolbarDefinition* toolbar = ActionRegistry::get()->findToolbar(toolbar_id);
    if (!toolbar) return;
    
    for (size_t i = 0; i < toolbar->items.size(); i++) {
        if (toolbar->items[i].item_id == item_id) {
            if (i > 0) {
                std::swap(toolbar->items[i], toolbar->items[i-1]);
                refreshList();
            }
            break;
        }
    }
}

void ToolbarListPanel::onItemMoveDown(wxCommandEvent& event) {
    std::string toolbar_id = getSelectedToolbar();
    std::string item_id = getSelectedItemId();
    if (toolbar_id.empty() || item_id.empty()) return;
    
    ToolbarDefinition* toolbar = ActionRegistry::get()->findToolbar(toolbar_id);
    if (!toolbar) return;
    
    for (size_t i = 0; i < toolbar->items.size(); i++) {
        if (toolbar->items[i].item_id == item_id) {
            if (i < toolbar->items.size() - 1) {
                std::swap(toolbar->items[i], toolbar->items[i+1]);
                refreshList();
            }
            break;
        }
    }
}

void ToolbarListPanel::onItemDelete(wxCommandEvent& event) {
    std::string toolbar_id = getSelectedToolbar();
    std::string item_id = getSelectedItemId();
    if (toolbar_id.empty() || item_id.empty()) return;
    
    ActionRegistry::get()->removeItemFromToolbar(toolbar_id, item_id);
    refreshList();
}

void ToolbarListPanel::onItemEdit(wxCommandEvent& event) {
    std::string toolbar_id = getSelectedToolbar();
    std::string item_id = getSelectedItemId();
    if (toolbar_id.empty() || item_id.empty()) return;
    
    ToolbarDefinition* toolbar = ActionRegistry::get()->findToolbar(toolbar_id);
    if (!toolbar) return;
    
    ToolbarItem* item = nullptr;
    for (auto& it : toolbar->items) {
        if (it.item_id == item_id) {
            item = &it;
            break;
        }
    }
    if (!item) return;
    
    Action* action = ActionRegistry::get()->findAction(item->action_id);
    
    ToolbarItemEditDialog dlg(this, *item, action);
    if (dlg.ShowEditDialog()) {
        refreshList();
    }
}

void ToolbarListPanel::onItemChangeAction(wxCommandEvent& event) {
    std::string toolbar_id = getSelectedToolbar();
    std::string item_id = getSelectedItemId();
    if (toolbar_id.empty() || item_id.empty()) return;
    
    ToolbarItem* item = ActionRegistry::get()->findToolbarItem(toolbar_id, item_id);
    if (!item) return;
    
    ActionSelectDialog dlg(this, item->action_id);
    if (dlg.ShowModal() == wxID_OK) {
        item->action_id = dlg.getSelectedAction();
        // Update icon to new action's default
        Action* action = ActionRegistry::get()->findAction(item->action_id);
        if (action) {
            item->icon_id = action->default_icon_name;
        }
        refreshList();
    }
}

void ToolbarListPanel::onItemChangeIcon(wxCommandEvent& event) {
    std::string toolbar_id = getSelectedToolbar();
    std::string item_id = getSelectedItemId();
    if (toolbar_id.empty() || item_id.empty()) return;
    
    ToolbarItem* item = ActionRegistry::get()->findToolbarItem(toolbar_id, item_id);
    if (!item) return;
    
    IconSelectDialog dlg(this, item->icon_id);
    if (dlg.ShowModal() == wxID_OK) {
        item->icon_id = dlg.getSelectedIcon();
        refreshList();
    }
}

ToolbarListPanel::TreeItemInfo* ToolbarListPanel::getItemInfo(wxTreeItemId id) {
    auto it = item_map_.find((int)(intptr_t)id.m_pItem);
    if (it != item_map_.end()) {
        return &it->second;
    }
    return nullptr;
}

wxTreeItemId ToolbarListPanel::findToolbarItem(const std::string& toolbar_id) {
    wxTreeItemIdValue cookie;
    wxTreeItemId child = tree_ctrl_->GetFirstChild(tree_ctrl_->GetRootItem(), cookie);
    while (child.IsOk()) {
        auto it = item_map_.find((int)(intptr_t)child.m_pItem);
        if (it != item_map_.end() && it->second.is_toolbar && it->second.toolbar_id == toolbar_id) {
            return child;
        }
        child = tree_ctrl_->GetNextChild(tree_ctrl_->GetRootItem(), cookie);
    }
    return wxTreeItemId();
}

wxTreeItemId ToolbarListPanel::findItemNode(const std::string& toolbar_id, const std::string& item_id) {
    wxTreeItemId tb = findToolbarItem(toolbar_id);
    if (!tb.IsOk()) return wxTreeItemId();
    
    wxTreeItemIdValue cookie;
    wxTreeItemId child = tree_ctrl_->GetFirstChild(tb, cookie);
    while (child.IsOk()) {
        auto it = item_map_.find((int)(intptr_t)child.m_pItem);
        if (it != item_map_.end() && !it->second.is_toolbar && it->second.item_id == item_id) {
            return child;
        }
        child = tree_ctrl_->GetNextChild(tb, cookie);
    }
    return wxTreeItemId();
}


// ============================================================================
// ToolbarItemEditDialog Implementation
// ============================================================================

BEGIN_EVENT_TABLE(ToolbarItemEditDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, ToolbarItemEditDialog::onChangeIcon)
END_EVENT_TABLE()

ToolbarItemEditDialog::ToolbarItemEditDialog(wxWindow* parent, ToolbarItem& item, Action* action)
    : wxDialog(parent, wxID_ANY, "Edit Toolbar Item", wxDefaultPosition, wxSize(450, 400))
    , item_(item)
    , action_(action) {
    current_icon_id_ = item.icon_id;
    current_action_id_ = item.action_id;
    createUI();
    loadData();
}

void ToolbarItemEditDialog::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Icon and action display
    wxStaticBoxSizer* icon_box = new wxStaticBoxSizer(wxHORIZONTAL, this, "Icon");
    icon_display_ = new wxStaticBitmap(this, wxID_ANY, wxBitmapBundle().GetBitmap(wxSize(32, 32)));
    icon_box->Add(icon_display_, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    icon_box->Add(new wxButton(this, wxID_ANY, "Change Icon..."), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    main_sizer->Add(icon_box, 0, wxEXPAND | wxALL, 5);
    
    // Action info
    wxStaticBoxSizer* action_box = new wxStaticBoxSizer(wxVERTICAL, this, "Action");
    if (action_) {
        action_box->Add(new wxStaticText(this, wxID_ANY, 
            wxString::FromUTF8(("Name: " + action_->display_name).c_str())));
        action_box->Add(new wxStaticText(this, wxID_ANY, 
            wxString::FromUTF8(("ID: " + action_->action_id).c_str())));
    } else {
        action_box->Add(new wxStaticText(this, wxID_ANY, "Unknown action"));
    }
    wxButton* change_action_btn = new wxButton(this, wxID_ANY, "Change Action...");
    change_action_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        ActionSelectDialog dlg(this, current_action_id_);
        if (dlg.ShowModal() == wxID_OK) {
            current_action_id_ = dlg.getSelectedAction();
            action_ = ActionRegistry::get()->findAction(current_action_id_);
        }
    });
    action_box->Add(change_action_btn, 0, wxTOP, 5);
    main_sizer->Add(action_box, 0, wxEXPAND | wxALL, 5);
    
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
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Help Topic:"), 0, wxALIGN_CENTER_VERTICAL);
    help_topic_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(help_topic_ctrl_, 1, wxEXPAND);
    
    props_box->Add(grid, 1, wxEXPAND | wxALL, 5);
    
    show_label_cb_ = new wxCheckBox(this, wxID_ANY, "Show text label on toolbar");
    props_box->Add(show_label_cb_, 0, wxALL, 5);
    
    main_sizer->Add(props_box, 1, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer(1);
    btn_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
}

void ToolbarItemEditDialog::loadData() {
    label_ctrl_->SetValue(wxString::FromUTF8(item_.custom_label.c_str()));
    tooltip_ctrl_->SetValue(wxString::FromUTF8(item_.custom_tooltip.c_str()));
    help_topic_ctrl_->SetValue(wxString::FromUTF8(item_.help_topic.c_str()));
    show_label_cb_->SetValue(item_.show_label);
    updateIconDisplay();
}

void ToolbarItemEditDialog::saveData() {
    item_.custom_label = std::string(label_ctrl_->GetValue().ToUTF8());
    item_.custom_tooltip = std::string(tooltip_ctrl_->GetValue().ToUTF8());
    item_.help_topic = std::string(help_topic_ctrl_->GetValue().ToUTF8());
    item_.show_label = show_label_cb_->GetValue();
    item_.icon_id = current_icon_id_;
    item_.action_id = current_action_id_;
}

void ToolbarItemEditDialog::updateIconDisplay() {
    wxBitmapBundle bundle = res::IconManager::get()->getBitmapBundle(current_icon_id_, wxSize(32, 32));
    if (bundle.IsOk()) {
        icon_display_->SetBitmap(bundle.GetBitmap(wxSize(32, 32)));
    }
}

void ToolbarItemEditDialog::onChangeIcon(wxCommandEvent& event) {
    IconSelectDialog dlg(this, current_icon_id_);
    if (dlg.ShowModal() == wxID_OK) {
        current_icon_id_ = dlg.getSelectedIcon();
        updateIconDisplay();
    }
}



bool ToolbarItemEditDialog::ShowEditDialog() {
    return ShowModal() == wxID_OK;
}

// ============================================================================
// ActionEditDialog Implementation
// ============================================================================

BEGIN_EVENT_TABLE(ActionEditDialog, wxDialog)
    EVT_BUTTON(wxID_OK, ActionEditDialog::onOK)
END_EVENT_TABLE()

ActionEditDialog::ActionEditDialog(wxWindow* parent, Action* action)
    : wxDialog(parent, wxID_ANY, action ? "Edit Action" : "New Action", 
               wxDefaultPosition, wxSize(500, 500)) {
    is_new_ = (action == nullptr);
    if (action) {
        action_ = *action;
    }
    createUI();
    if (!is_new_) {
        loadData();
    }
}

void ActionEditDialog::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticBoxSizer* form_box = new wxStaticBoxSizer(wxVERTICAL, this, "Action Properties");
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 5, 5);
    grid->AddGrowableCol(1);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "ID:"), 0, wxALIGN_CENTER_VERTICAL);
    id_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    id_ctrl_->Enable(is_new_);
    grid->Add(id_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Display Name:"), 0, wxALIGN_CENTER_VERTICAL);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(name_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Description:"), 0, wxALIGN_CENTER_VERTICAL);
    desc_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, 
                                 wxDefaultSize, wxTE_MULTILINE);
    grid->Add(desc_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Tooltip:"), 0, wxALIGN_CENTER_VERTICAL);
    tooltip_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(tooltip_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Category:"), 0, wxALIGN_CENTER_VERTICAL);
    category_ctrl_ = new wxChoice(this, wxID_ANY);
    category_ctrl_->Append("File");
    category_ctrl_->Append("Edit");
    category_ctrl_->Append("View");
    category_ctrl_->Append("Database");
    category_ctrl_->Append("Transaction");
    category_ctrl_->Append("Query");
    category_ctrl_->Append("Tools");
    category_ctrl_->Append("Window");
    category_ctrl_->Append("Help");
    category_ctrl_->Append("Custom");
    category_ctrl_->SetSelection(8);  // Custom default
    grid->Add(category_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Default Icon:"), 0, wxALIGN_CENTER_VERTICAL);
    icon_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(icon_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Accelerator:"), 0, wxALIGN_CENTER_VERTICAL);
    accel_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(accel_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Help Topic:"), 0, wxALIGN_CENTER_VERTICAL);
    help_topic_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(help_topic_ctrl_, 1, wxEXPAND);
    
    form_box->Add(grid, 1, wxEXPAND | wxALL, 5);
    
    req_conn_cb_ = new wxCheckBox(this, wxID_ANY, "Requires database connection");
    form_box->Add(req_conn_cb_, 0, wxALL, 5);
    
    req_tx_cb_ = new wxCheckBox(this, wxID_ANY, "Requires active transaction");
    form_box->Add(req_tx_cb_, 0, wxALL, 5);
    
    main_sizer->Add(form_box, 1, wxEXPAND | wxALL, 10);
    
    // Buttons
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer(1);
    btn_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
}

void ActionEditDialog::loadData() {
    id_ctrl_->SetValue(wxString::FromUTF8(action_.action_id.c_str()));
    name_ctrl_->SetValue(wxString::FromUTF8(action_.display_name.c_str()));
    desc_ctrl_->SetValue(wxString::FromUTF8(action_.description.c_str()));
    tooltip_ctrl_->SetValue(wxString::FromUTF8(action_.tooltip.c_str()));
    icon_ctrl_->SetValue(wxString::FromUTF8(action_.default_icon_name.c_str()));
    accel_ctrl_->SetValue(wxString::FromUTF8(action_.accelerator.c_str()));
    help_topic_ctrl_->SetValue(wxString::FromUTF8(action_.help_topic.c_str()));
    req_conn_cb_->SetValue(action_.requires_connection);
    req_tx_cb_->SetValue(action_.requires_transaction);
    
    wxString cat = wxString::FromUTF8(actionCategoryToString(action_.category).c_str());
    int sel = category_ctrl_->FindString(cat);
    if (sel != wxNOT_FOUND) {
        category_ctrl_->SetSelection(sel);
    }
}

bool ActionEditDialog::saveData() {
    if (!validateInput()) {
        return false;
    }
    
    action_.action_id = std::string(id_ctrl_->GetValue().ToUTF8());
    action_.display_name = std::string(name_ctrl_->GetValue().ToUTF8());
    action_.description = std::string(desc_ctrl_->GetValue().ToUTF8());
    action_.tooltip = std::string(tooltip_ctrl_->GetValue().ToUTF8());
    action_.default_icon_name = std::string(icon_ctrl_->GetValue().ToUTF8());
    action_.accelerator = std::string(accel_ctrl_->GetValue().ToUTF8());
    action_.help_topic = std::string(help_topic_ctrl_->GetValue().ToUTF8());
    action_.requires_connection = req_conn_cb_->GetValue();
    action_.requires_transaction = req_tx_cb_->GetValue();
    action_.is_builtin = false;
    
    wxString cat = category_ctrl_->GetStringSelection();
    action_.category = actionCategoryFromString(std::string(cat.ToUTF8()));
    
    return true;
}

bool ActionEditDialog::validateInput() {
    if (id_ctrl_->GetValue().IsEmpty()) {
        wxMessageBox("Action ID is required.", "Validation Error", wxOK | wxICON_ERROR);
        return false;
    }
    if (name_ctrl_->GetValue().IsEmpty()) {
        wxMessageBox("Display name is required.", "Validation Error", wxOK | wxICON_ERROR);
        return false;
    }
    return true;
}

void ActionEditDialog::onOK(wxCommandEvent& event) {
    if (saveData()) {
        EndModal(wxID_OK);
    }
}

bool ActionEditDialog::ShowEditDialog() {
    return ShowModal() == wxID_OK;
}


// ============================================================================
// IconSelectDialog Implementation
// ============================================================================

BEGIN_EVENT_TABLE(IconSelectDialog, wxDialog)
    EVT_SEARCHCTRL_SEARCH_BTN(wxID_ANY, IconSelectDialog::onSearch)
    EVT_TEXT(wxID_ANY, IconSelectDialog::onSearch)
END_EVENT_TABLE()

IconSelectDialog::IconSelectDialog(wxWindow* parent, const std::string& current_icon)
    : wxDialog(parent, wxID_ANY, "Select Icon", wxDefaultPosition, wxSize(400, 500)) {
    selected_icon_ = current_icon;
    createUI();
    populateIcons();
}

void IconSelectDialog::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Search
    search_ctrl_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    search_ctrl_->SetDescriptiveText("Search icons...");
    search_ctrl_->ShowSearchButton(true);
    search_ctrl_->ShowCancelButton(true);
    main_sizer->Add(search_ctrl_, 0, wxEXPAND | wxALL, 5);
    
    // Icons panel
    icons_panel_ = new wxScrolledWindow(this, wxID_ANY);
    main_sizer->Add(icons_panel_, 1, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer(1);
    btn_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
}

void IconSelectDialog::populateIcons(const wxString& filter) {
    icons_panel_->DestroyChildren();
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxWrapSizer* wrap = new wxWrapSizer(wxHORIZONTAL);
    
    res::IconManager* mgr = res::IconManager::get();
    auto icons = filter.IsEmpty() ? mgr->getAllIcons() : 
                  mgr->searchIcons(std::string(filter.ToUTF8()));
    
    for (res::IconInfo* icon : icons) {
        if (!icon->bitmap.IsOk()) continue;
        
        wxBitmapButton* btn = new wxBitmapButton(icons_panel_, wxID_ANY,
                                                  icon->bitmap.GetBitmap(wxSize(24, 24)),
                                                  wxDefaultPosition, wxSize(36, 36),
                                                  wxBU_AUTODRAW | wxBORDER_NONE);
        btn->SetToolTip(wxString::FromUTF8(icon->display_name.c_str()));
        
        if (icon->id == selected_icon_) {
            btn->SetBackgroundColour(wxColour(200, 220, 255));
        }
        
        btn->Bind(wxEVT_BUTTON, [this, icon_id = icon->id](wxCommandEvent&) {
            selected_icon_ = icon_id;
            populateIcons(search_ctrl_->GetValue());  // Refresh to show selection
        });
        
        wrap->Add(btn, 0, wxALL, 3);
    }
    
    sizer->Add(wrap, 1, wxEXPAND);
    icons_panel_->SetSizer(sizer);
    icons_panel_->FitInside();
    icons_panel_->SetScrollRate(10, 10);
}

void IconSelectDialog::onSearch(wxCommandEvent& event) {
    populateIcons(search_ctrl_->GetValue());
}

// ============================================================================
// ActionSelectDialog Implementation
// ============================================================================

BEGIN_EVENT_TABLE(ActionSelectDialog, wxDialog)
    EVT_SEARCHCTRL_SEARCH_BTN(wxID_ANY, ActionSelectDialog::onSearch)
    EVT_TEXT(wxID_ANY, ActionSelectDialog::onSearch)
    EVT_TREE_SEL_CHANGED(wxID_ANY, ActionSelectDialog::onItemSelected)
    EVT_TREE_ITEM_ACTIVATED(wxID_ANY, ActionSelectDialog::onItemActivated)
END_EVENT_TABLE()

ActionSelectDialog::ActionSelectDialog(wxWindow* parent, const std::string& current_action)
    : wxDialog(parent, wxID_ANY, "Select Action", wxDefaultPosition, wxSize(450, 500)) {
    selected_action_ = current_action;
    createUI();
    populateActions();
}

void ActionSelectDialog::createUI() {
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Search
    search_ctrl_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    search_ctrl_->SetDescriptiveText("Search actions...");
    search_ctrl_->ShowSearchButton(true);
    search_ctrl_->ShowCancelButton(true);
    main_sizer->Add(search_ctrl_, 0, wxEXPAND | wxALL, 5);
    
    // Tree
    tree_ctrl_ = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT |
                                wxTR_FULL_ROW_HIGHLIGHT | wxTR_SINGLE);
    main_sizer->Add(tree_ctrl_, 1, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer(1);
    btn_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
}

void ActionSelectDialog::populateActions(const wxString& filter) {
    tree_ctrl_->DeleteAllItems();
    wxTreeItemId root = tree_ctrl_->AddRoot("Actions");
    
    ActionRegistry* reg = ActionRegistry::get();
    
    if (filter.IsEmpty()) {
        auto categories = reg->getCategoriesWithActions();
        for (auto cat : categories) {
            wxString cat_name = wxString::FromUTF8(actionCategoryToString(cat).c_str());
            wxTreeItemId cat_item = tree_ctrl_->AppendItem(root, cat_name);
            
            auto actions = reg->getActionsByCategory(cat);
            for (Action* action : actions) {
                wxString label = wxString::FromUTF8(action->display_name.c_str());
                wxTreeItemId item = tree_ctrl_->AppendItem(cat_item, label);
                tree_ctrl_->SetItemData(item, new ActionTreeData(
                    wxString::FromUTF8(action->action_id.c_str())));
                
                if (action->action_id == selected_action_) {
                    tree_ctrl_->SelectItem(item);
                }
            }
            tree_ctrl_->Expand(cat_item);
        }
    } else {
        auto actions = reg->searchActions(std::string(filter.ToUTF8()));
        for (Action* action : actions) {
            wxString label = wxString::FromUTF8(action->display_name.c_str());
            wxTreeItemId item = tree_ctrl_->AppendItem(root, label);
            tree_ctrl_->SetItemData(item, new ActionTreeData(
                wxString::FromUTF8(action->action_id.c_str())));
            
            if (action->action_id == selected_action_) {
                tree_ctrl_->SelectItem(item);
            }
        }
    }
}

void ActionSelectDialog::onSearch(wxCommandEvent& event) {
    populateActions(search_ctrl_->GetValue());
}

void ActionSelectDialog::onItemSelected(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    ActionTreeData* data = dynamic_cast<ActionTreeData*>(tree_ctrl_->GetItemData(item));
    if (data) {
        selected_action_ = std::string(data->getData().ToUTF8());
    }
}

void ActionSelectDialog::onItemActivated(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    ActionTreeData* data = dynamic_cast<ActionTreeData*>(tree_ctrl_->GetItemData(item));
    if (data) {
        selected_action_ = std::string(data->getData().ToUTF8());
        EndModal(wxID_OK);
    }
}


}  // namespace scratchrobin::ui
