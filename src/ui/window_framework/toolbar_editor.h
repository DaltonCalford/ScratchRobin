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
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/splitter.h>
#include <wx/scrolwin.h>
#include <wx/srchctrl.h>
#include <wx/dnd.h>
#include <wx/imaglist.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/statbmp.h>
#include "ui/window_framework/action_registry.h"

namespace scratchrobin::ui {

// Forward declarations
class ActionListPanel;
class IconListPanel;
class ToolbarListPanel;
class ToolbarItemEditDialog;
class ActionEditDialog;

// Main toolbar editor dialog with three-list layout
// Left: Actions | Middle: Icons | Right: Toolbars
class ToolbarEditor : public wxDialog {
public:
    ToolbarEditor(wxWindow* parent);
    ~ToolbarEditor();
    
    // Show editor modally
    int ShowModal() override;
    
    // Access to drag data
    ToolbarDragData& getDragData() { return drag_data_; }
    
    // Update methods
    void updateForActionSelection(const std::string& action_id);
    void updateForIconSelection(const std::string& icon_id);
    
    // Drop handlers
    void onDropOnIconPanel(const std::string& action_id);
    void onDropOnToolbar(const std::string& toolbar_id, const std::string& insert_after_item_id);
    
    // Refresh
    void refreshAllPanels();
    void refreshToolbarPanel();

private:
    // Three main panels
    ActionListPanel* action_panel_{nullptr};
    IconListPanel* icon_panel_{nullptr};
    ToolbarListPanel* toolbar_panel_{nullptr};
    
    // Current drag-drop state
    ToolbarDragData drag_data_;
    
    // UI creation
    void createUI();
    wxPanel* createActionPanel(wxWindow* parent);
    wxPanel* createIconPanel(wxWindow* parent);
    wxPanel* createToolbarPanel(wxWindow* parent);
    void createButtonBar(wxSizer* parent_sizer);
    
    // Button handlers
    void onNewAction(wxCommandEvent& event);
    void onNewIcon(wxCommandEvent& event);
    void onNewToolbar(wxCommandEvent& event);
    void onDeleteToolbar(wxCommandEvent& event);
    void onRenameToolbar(wxCommandEvent& event);
    void onResetToolbars(wxCommandEvent& event);
    void onImportToolbars(wxCommandEvent& event);
    void onExportToolbars(wxCommandEvent& event);
    void onHelp(wxCommandEvent& event);
    
    friend class ActionListPanel;
    friend class IconListPanel;
    friend class ToolbarListPanel;
    
    DECLARE_EVENT_TABLE()
};

// Left panel - Actions list (categorized, searchable, scrollable)
class ActionListPanel : public wxPanel {
public:
    ActionListPanel(wxWindow* parent, ToolbarEditor* editor);
    
    void refreshList();
    std::string getSelectedAction() const;
    void setSearchText(const wxString& text);
    
private:
    ToolbarEditor* editor_{nullptr};
    wxTreeCtrl* tree_ctrl_{nullptr};
    wxSearchCtrl* search_ctrl_{nullptr};
    wxImageList* image_list_{nullptr};
    
    void createUI();
    void populateActions();
    void filterActions(const wxString& filter);
    
    // Event handlers
    void onSearch(wxCommandEvent& event);
    void onItemSelected(wxTreeEvent& event);
    void onItemActivated(wxTreeEvent& event);
    void onBeginDrag(wxTreeEvent& event);
    void onContextMenu(wxTreeEvent& event);
    void onNewAction(wxCommandEvent& event);
    void onEditAction(wxCommandEvent& event);
    void onDeleteAction(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Middle panel - Icons list (searchable grid, scrollable)
class IconListPanel : public wxScrolledWindow {
public:
    IconListPanel(wxWindow* parent, ToolbarEditor* editor);
    
    void refreshList();
    void setSearchText(const wxString& text);
    void setFilterForAction(const std::string& action_id);
    std::string getSelectedIcon() const { return selected_icon_; }
    
private:
    ToolbarEditor* editor_{nullptr};
    wxSearchCtrl* search_ctrl_{nullptr};
    wxPanel* icons_container_{nullptr};
    wxSizer* icons_sizer_{nullptr};
    std::string selected_icon_;
    std::string filter_action_;
    
    void createUI();
    void populateIcons();
    void filterIcons(const wxString& filter);
    void clearIconSelection();
    
    // Event handlers
    void onSearch(wxCommandEvent& event);
    void onIconClick(wxMouseEvent& event);
    void onIconBeginDrag(wxMouseEvent& event);
    void onNewIcon(wxCommandEvent& event);
    void onContextMenu(wxMouseEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Right panel - Toolbars with their items (hierarchical, expandable)
class ToolbarListPanel : public wxPanel {
public:
    ToolbarListPanel(wxWindow* parent, ToolbarEditor* editor);
    
    void refreshList();
    std::string getSelectedToolbar() const;
    std::string getSelectedItemId() const;
    bool isToolbarExpanded(const std::string& toolbar_id) const;
    void setToolbarExpanded(const std::string& toolbar_id, bool expanded);
    
    // Drop handling
    bool canAcceptDrop(const wxPoint& pos) const;
    void handleDrop(const wxPoint& pos, const ToolbarDragData& data);
    
private:
    ToolbarEditor* editor_{nullptr};
    wxSearchCtrl* search_ctrl_{nullptr};
    wxTreeCtrl* tree_ctrl_{nullptr};
    wxImageList* image_list_{nullptr};
    
    // Map tree item IDs to toolbar/item info
    struct TreeItemInfo {
        std::string toolbar_id;
        std::string item_id;      // Empty for toolbar itself
        bool is_toolbar{false};
    };
    std::map<int, TreeItemInfo> item_map_;
    std::map<std::string, bool> expanded_state_;
    
    void createUI();
    void populateToolbars();
    void filterToolbars(const wxString& filter);
    void addToolbarToTree(const ToolbarDefinition& toolbar, wxTreeItemId parent);
    int getItemImageIndex(const ToolbarItem& item);
    
    // Event handlers
    void onSearch(wxCommandEvent& event);
    void onItemSelected(wxTreeEvent& event);
    void onItemActivated(wxTreeEvent& event);
    void onBeginDrag(wxTreeEvent& event);
    void onEndDrag(wxTreeEvent& event);
    void onContextMenu(wxTreeEvent& event);
    void onItemExpanded(wxTreeEvent& event);
    void onItemCollapsed(wxTreeEvent& event);
    
    // Toolbar context menu handlers
    void onToolbarRename(wxCommandEvent& event);
    void onToolbarDelete(wxCommandEvent& event);
    void onToolbarExpandCollapse(wxCommandEvent& event);
    void onToolbarAddSeparator(wxCommandEvent& event);
    void onToolbarAddSpacer(wxCommandEvent& event);
    void onToolbarProperties(wxCommandEvent& event);
    
    // Item context menu handlers
    void onItemMoveUp(wxCommandEvent& event);
    void onItemMoveDown(wxCommandEvent& event);
    void onItemDelete(wxCommandEvent& event);
    void onItemEdit(wxCommandEvent& event);
    void onItemChangeAction(wxCommandEvent& event);
    void onItemChangeIcon(wxCommandEvent& event);
    
    TreeItemInfo* getItemInfo(wxTreeItemId id);
    wxTreeItemId findToolbarItem(const std::string& toolbar_id);
    wxTreeItemId findItemNode(const std::string& toolbar_id, const std::string& item_id);
    
    DECLARE_EVENT_TABLE()
};

// Dialog for editing a toolbar item (change action, icon, label, tooltip, help)
class ToolbarItemEditDialog : public wxDialog {
public:
    ToolbarItemEditDialog(wxWindow* parent, ToolbarItem& item, Action* action);
    
    bool ShowEditDialog();  // Returns true if OK pressed
    
private:
    ToolbarItem& item_;
    Action* action_;
    
    // UI controls
    wxStaticBitmap* icon_display_{nullptr};
    wxTextCtrl* label_ctrl_{nullptr};
    wxTextCtrl* tooltip_ctrl_{nullptr};
    wxTextCtrl* help_topic_ctrl_{nullptr};
    wxCheckBox* show_label_cb_{nullptr};
    
    // Current selections
    std::string current_icon_id_;
    std::string current_action_id_;
    
    void createUI();
    void loadData();
    void saveData();
    void updateIconDisplay();
    
    // Event handlers
    void onChangeIcon(wxCommandEvent& event);
    void onChangeAction(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Dialog for editing/creating a custom action
class ActionEditDialog : public wxDialog {
public:
    ActionEditDialog(wxWindow* parent, Action* action = nullptr);
    
    bool ShowEditDialog();
    Action getAction() const { return action_; }
    
private:
    Action action_;
    bool is_new_{false};
    
    // UI controls
    wxTextCtrl* id_ctrl_{nullptr};
    wxTextCtrl* name_ctrl_{nullptr};
    wxTextCtrl* desc_ctrl_{nullptr};
    wxTextCtrl* tooltip_ctrl_{nullptr};
    wxChoice* category_ctrl_{nullptr};
    wxTextCtrl* icon_ctrl_{nullptr};
    wxTextCtrl* accel_ctrl_{nullptr};
    wxTextCtrl* help_topic_ctrl_{nullptr};
    wxCheckBox* req_conn_cb_{nullptr};
    wxCheckBox* req_tx_cb_{nullptr};
    
    void createUI();
    void loadData();
    bool saveData();
    bool validateInput();
    void onOK(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Dialog for selecting an icon
class IconSelectDialog : public wxDialog {
public:
    IconSelectDialog(wxWindow* parent, const std::string& current_icon);
    
    std::string getSelectedIcon() const { return selected_icon_; }
    
private:
    std::string selected_icon_;
    wxSearchCtrl* search_ctrl_{nullptr};
    wxScrolledWindow* icons_panel_{nullptr};
    
    void createUI();
    void populateIcons(const wxString& filter = wxEmptyString);
    void onSearch(wxCommandEvent& event);
    void onIconSelected(wxMouseEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Dialog for selecting an action
class ActionSelectDialog : public wxDialog {
public:
    ActionSelectDialog(wxWindow* parent, const std::string& current_action);
    
    std::string getSelectedAction() const { return selected_action_; }
    
private:
    std::string selected_action_;
    wxSearchCtrl* search_ctrl_{nullptr};
    wxTreeCtrl* tree_ctrl_{nullptr};
    
    void createUI();
    void populateActions(const wxString& filter = wxEmptyString);
    void onSearch(wxCommandEvent& event);
    void onItemSelected(wxTreeEvent& event);
    void onItemActivated(wxTreeEvent& event);
    
    DECLARE_EVENT_TABLE()
};

}  // namespace scratchrobin::ui
