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

#include <wx/dialog.h>
#include <wx/treectrl.h>
#include <wx/srchctrl.h>
#include <wx/splitter.h>
#include <wx/listbox.h>
#include <wx/checklst.h>
#include <wx/checkbox.h>
#include "ui/window_framework/menu_registry.h"

namespace scratchrobin::ui {

// Forward declarations
class ActionListPanel2;
class MenuStructurePanel;
class WindowTypePanel;
class MenuItemEditDialog;
class SubmenuEditDialog;

// Main menu editor dialog
// Left: Actions | Middle: Menu Structure | Right: Window Types
class MenuEditor : public wxDialog {
public:
    MenuEditor(wxWindow* parent);
    ~MenuEditor();
    
    int ShowModal() override;
    
    // Access drag data
    MenuDragData& getDragData() { return drag_data_; }
    
    // Update methods
    void onActionSelected(const std::string& action_id);
    void onMenuItemSelected(const std::string& menu_id, const std::string& item_id);
    void onWindowTypeSelected(const std::string& window_type_id);
    
    // Current selections
    std::string getCurrentMenuId() const { return current_menu_id_; }
    std::string getCurrentWindowType() const { return current_window_type_id_; }
    
    // Refresh
    void refreshAllPanels();
    void refreshMenuPanel();
    void refreshWindowTypePanel();

private:
    // Three main panels
    ActionListPanel2* action_panel_{nullptr};
    MenuStructurePanel* menu_panel_{nullptr};
    WindowTypePanel* window_type_panel_{nullptr};
    
    // Current state
    MenuDragData drag_data_;
    std::string current_menu_id_;
    std::string current_window_type_id_;
    
    // UI creation
    void createUI();
    wxPanel* createActionPanel(wxWindow* parent);
    wxPanel* createMenuPanel(wxWindow* parent);
    wxPanel* createWindowTypePanel(wxWindow* parent);
    void createButtonBar(wxSizer* parent_sizer);
    
    // Button handlers
    void onNewMenu(wxCommandEvent& event);
    void onDeleteMenu(wxCommandEvent& event);
    void onRenameMenu(wxCommandEvent& event);
    void onNewWindowType(wxCommandEvent& event);
    void onResetMenus(wxCommandEvent& event);
    void onImportMenus(wxCommandEvent& event);
    void onExportMenus(wxCommandEvent& event);
    void onHelp(wxCommandEvent& event);
    
    friend class ActionListPanel2;
    friend class MenuStructurePanel;
    friend class WindowTypePanel;
    
    DECLARE_EVENT_TABLE()
};

// Left panel - Actions list (same as toolbar editor)
class ActionListPanel2 : public wxPanel {
public:
    ActionListPanel2(wxWindow* parent, MenuEditor* editor);
    
    void refreshList();
    std::string getSelectedAction() const;
    void setSearchText(const wxString& text);
    
private:
    MenuEditor* editor_{nullptr};
    wxTreeCtrl* tree_ctrl_{nullptr};
    wxSearchCtrl* search_ctrl_{nullptr};
    
    void createUI();
    void populateActions();
    void filterActions(const wxString& filter);
    
    // Event handlers
    void onSearch(wxCommandEvent& event);
    void onItemSelected(wxTreeEvent& event);
    void onBeginDrag(wxTreeEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Middle panel - Menu structure tree
class MenuStructurePanel : public wxPanel {
public:
    MenuStructurePanel(wxWindow* parent, MenuEditor* editor);
    
    void refreshList();
    void setCurrentMenu(const std::string& menu_id);
    std::string getSelectedItemId() const;
    
    // Drop handling
    void onDropAction(const std::string& action_id, const wxTreeItemId& target);
    
private:
    MenuEditor* editor_{nullptr};
    wxSearchCtrl* search_ctrl_{nullptr};
    wxTreeCtrl* tree_ctrl_{nullptr};
    
    // Map tree items to menu info
    struct TreeItemInfo {
        std::string menu_id;
        std::string item_id;
        bool is_menu{false};
        bool is_submenu{false};
    };
    std::map<int, TreeItemInfo> item_map_;
    std::string current_menu_id_;
    
    void createUI();
    void populateMenu(const std::string& menu_id);
    void addItemToTree(const MenuItem& item, wxTreeItemId parent, const std::string& menu_id);
    void clearTree();
    
    // Event handlers
    void onSearch(wxCommandEvent& event);
    void onItemSelected(wxTreeEvent& event);
    void onBeginDrag(wxTreeEvent& event);
    void onEndDrag(wxTreeEvent& event);
    void onContextMenu(wxTreeEvent& event);
    void onItemActivated(wxTreeEvent& event);
    void onKeyDown(wxTreeEvent& event);
    
    // Context menu handlers
    void onNewSubmenu(wxCommandEvent& event);
    void onAddSeparator(wxCommandEvent& event);
    void onEditItem(wxCommandEvent& event);
    void onDeleteItem(wxCommandEvent& event);
    void onMoveUp(wxCommandEvent& event);
    void onMoveDown(wxCommandEvent& event);
    void onChangeAction(wxCommandEvent& event);
    void onToggleVisibility(wxCommandEvent& event);
    
    TreeItemInfo* getItemInfo(wxTreeItemId id);
    
    DECLARE_EVENT_TABLE()
};

// Right panel - Window types and their menu assignments
class WindowTypePanel : public wxPanel {
public:
    WindowTypePanel(wxWindow* parent, MenuEditor* editor);
    
    void refreshList();
    std::string getSelectedWindowType() const;
    void selectWindowType(const std::string& window_type_id);
    
private:
    MenuEditor* editor_{nullptr};
    wxListBox* window_type_list_{nullptr};
    wxCheckListBox* menu_checklist_{nullptr};
    wxCheckBox* merge_cb_{nullptr};
    
    std::string selected_window_type_;
    
    void createUI();
    void populateWindowTypes();
    void populateMenuChecklist(const std::string& window_type_id);
    void updateMenuAssignments();
    
    // Event handlers
    void onWindowTypeSelected(wxCommandEvent& event);
    void onMenuToggled(wxCommandEvent& event);
    void onMergeChanged(wxCommandEvent& event);
    void onContextMenu(wxContextMenuEvent& event);
    
    // Context menu handlers
    void onNewWindowType(wxCommandEvent& event);
    void onDeleteWindowType(wxCommandEvent& event);
    void onRenameWindowType(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Dialog for editing a menu item
class MenuItemEditDialog : public wxDialog {
public:
    MenuItemEditDialog(wxWindow* parent, MenuItem& item, Action* action);
    
    bool ShowEditDialog();
    
private:
    MenuItem& item_;
    Action* action_;
    
    // UI controls
    wxTextCtrl* label_ctrl_{nullptr};
    wxTextCtrl* tooltip_ctrl_{nullptr};
    wxTextCtrl* accel_ctrl_{nullptr};
    wxTextCtrl* help_topic_ctrl_{nullptr};
    wxCheckBox* visible_cb_{nullptr};
    wxCheckBox* checkable_cb_{nullptr};
    wxCheckBox* checked_cb_{nullptr};
    
    // Current selections
    std::string current_icon_id_;
    std::string current_action_id_;
    
    void createUI();
    void loadData();
    void saveData();
    
    void onChangeAction(wxCommandEvent& event);
    void onChangeIcon(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Dialog for creating/editing a submenu
class SubmenuEditDialog : public wxDialog {
public:
    SubmenuEditDialog(wxWindow* parent, MenuItem& item);
    
    bool ShowEditDialog();
    
private:
    MenuItem& item_;
    
    wxTextCtrl* label_ctrl_{nullptr};
    wxTextCtrl* item_id_ctrl_{nullptr};
    
    void createUI();
    void loadData();
    void saveData();
    bool validateInput();
    void onOK(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

}  // namespace scratchrobin::ui
