// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "toolbar_manager.h"
#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/spinctrl.h>
#include <memory>

namespace scratchrobin {
namespace ui {
namespace toolbar {

// Forward declarations
class ToolbarPreviewPanel;
class ItemPalettePanel;
class ToolbarPropertiesPanel;

/**
 * @brief Dialog for customizing toolbars
 * 
 * Provides a visual interface for:
 * - Creating/deleting/renaming custom toolbars
 * - Adding/removing/reordering toolbar items
 * - Configuring toolbar properties
 * - Drag-and-drop item arrangement
 */
class ToolbarEditorForm : public wxDialog {
public:
    ToolbarEditorForm(wxWindow* parent);
    ~ToolbarEditorForm();

    // Show the editor, optionally focusing on a specific toolbar
    bool ShowEditor(const std::string& initial_toolbar_id = "");

private:
    // UI Creation
    void CreateControls();
    void CreateToolbarListPanel(wxBoxSizer* parent_sizer);
    void CreatePreviewPanel(wxBoxSizer* parent_sizer);
    void CreatePalettePanel(wxBoxSizer* parent_sizer);
    void CreatePropertiesPanel(wxBoxSizer* parent_sizer);
    void CreateButtonPanel(wxBoxSizer* parent_sizer);

    // Data loading
    void LoadToolbarList();
    void LoadSelectedToolbar();
    void UpdatePreview();
    void UpdateProperties();
    void UpdateButtonStates();

    // Event handlers
    void OnToolbarSelected(wxCommandEvent& event);
    void OnNewToolbar(wxCommandEvent& event);
    void OnDeleteToolbar(wxCommandEvent& event);
    void OnRenameToolbar(wxCommandEvent& event);
    void OnResetToolbars(wxCommandEvent& event);
    void OnItemSelected(wxCommandEvent& event);
    void OnItemMoved(wxCommandEvent& event);
    void OnItemDeleted(wxCommandEvent& event);
    void OnPaletteItemSelected(wxTreeEvent& event);
    void OnAddItem(wxCommandEvent& event);
    void OnPropertyChanged(wxCommandEvent& event);
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);

    // Drag and drop
    void OnDragStart(wxListEvent& event);
    void OnDrop(wxListEvent& event);

    // Helper methods
    ToolbarItem* GetSelectedItem();
    void RefreshPreview();
    void SaveCurrentChanges();
    bool ValidateChanges();
    void PopulatePalette();

    // UI Controls
    wxComboBox* toolbar_combo_ = nullptr;
    wxButton* new_toolbar_btn_ = nullptr;
    wxButton* delete_toolbar_btn_ = nullptr;
    wxButton* rename_toolbar_btn_ = nullptr;
    wxButton* reset_toolbars_btn_ = nullptr;
    
    ToolbarPreviewPanel* preview_panel_ = nullptr;
    ItemPalettePanel* palette_panel_ = nullptr;
    ToolbarPropertiesPanel* properties_panel_ = nullptr;
    
    wxButton* add_item_btn_ = nullptr;
    wxButton* remove_item_btn_ = nullptr;
    wxButton* move_up_btn_ = nullptr;
    wxButton* move_down_btn_ = nullptr;
    
    wxButton* ok_btn_ = nullptr;
    wxButton* cancel_btn_ = nullptr;
    wxButton* apply_btn_ = nullptr;
    wxButton* help_btn_ = nullptr;

    // State
    std::string current_toolbar_id_;
    int selected_item_index_ = -1;
    bool has_unsaved_changes_ = false;
    bool is_loading_ = false;

    // Event bindings
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Panel showing a visual preview of the toolbar
 */
class ToolbarPreviewPanel : public wxPanel {
public:
    ToolbarPreviewPanel(wxWindow* parent);
    
    void SetToolbar(ToolbarDefinition* toolbar);
    void SetSelectedIndex(int index);
    int GetSelectedIndex() const { return selected_index_; }
    
    // Events
    wxEventTypeTag<wxCommandEvent> GetSelectionEvent() const { return selection_event_; }
    wxEventTypeTag<wxCommandEvent> GetMoveEvent() const { return move_event_; }
    wxEventTypeTag<wxCommandEvent> GetDeleteEvent() const { return delete_event_; }

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnSize(wxSizeEvent& event);
    
    void DrawItem(wxDC& dc, const ToolbarItem& item, int x, int& width);
    int CalculateItemWidth(const ToolbarItem& item) const;
    int HitTest(int x) const;
    void StartDrag(int index);
    void EndDrag(int drop_index);
    
    ToolbarDefinition* toolbar_ = nullptr;
    int selected_index_ = -1;
    int hover_index_ = -1;
    int drag_start_index_ = -1;
    int drag_current_x_ = -1;
    bool is_dragging_ = false;
    
    static constexpr int ITEM_HEIGHT = 32;
    static constexpr int ITEM_MARGIN = 4;
    static constexpr int SEPARATOR_WIDTH = 8;
    static constexpr int SPACER_MIN_WIDTH = 16;
    
    wxEventTypeTag<wxCommandEvent> selection_event_;
    wxEventTypeTag<wxCommandEvent> move_event_;
    wxEventTypeTag<wxCommandEvent> delete_event_;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Panel showing available items to add to toolbars
 */
class ItemPalettePanel : public wxPanel {
public:
    ItemPalettePanel(wxWindow* parent);
    
    void RefreshItems();
    ToolbarItemTemplate* GetSelectedTemplate();
    
private:
    void OnCategorySelected(wxCommandEvent& event);
    void OnItemActivated(wxListEvent& event);
    void PopulateCategories();
    void PopulateItems(const std::string& category);
    
    wxComboBox* category_combo_ = nullptr;
    wxListCtrl* items_list_ = nullptr;
    std::vector<ToolbarItemTemplate> current_items_;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Panel for editing properties of selected toolbar/item
 */
class ToolbarPropertiesPanel : public wxPanel {
public:
    ToolbarPropertiesPanel(wxWindow* parent);
    
    void ShowToolbarProperties(ToolbarDefinition* toolbar);
    void ShowItemProperties(ToolbarItem* item);
    void Clear();
    
    // Get current values
    std::string GetItemLabel() const;
    std::string GetItemTooltip() const;
    int GetSpacerWidth() const;
    
private:
    void OnPropertyChange(wxCommandEvent& event);
    void OnSpacerWidthChange(wxSpinEvent& event);
    void CreateToolbarControls();
    void CreateItemControls();
    void ShowToolbarMode();
    void ShowItemMode();
    
    // Toolbar controls
    wxTextCtrl* toolbar_name_ctrl_ = nullptr;
    wxTextCtrl* toolbar_desc_ctrl_ = nullptr;
    wxChoice* toolbar_position_ctrl_ = nullptr;
    wxCheckBox* toolbar_visible_ctrl_ = nullptr;
    
    // Item controls
    wxTextCtrl* item_id_ctrl_ = nullptr;
    wxTextCtrl* item_label_ctrl_ = nullptr;
    wxTextCtrl* item_tooltip_ctrl_ = nullptr;
    wxTextCtrl* item_icon_ctrl_ = nullptr;
    wxSpinCtrl* spacer_width_ctrl_ = nullptr;
    
    wxStaticBoxSizer* toolbar_box_ = nullptr;
    wxStaticBoxSizer* item_box_ = nullptr;
    
    enum Mode { None, Toolbar, Item };
    Mode current_mode_ = None;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace toolbar
} // namespace ui
} // namespace scratchrobin
