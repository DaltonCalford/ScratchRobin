// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "toolbar_editor_form.h"
#include "../../core/simple_json.h"
#include <wx/artprov.h>
#include <wx/imaglist.h>
#include <algorithm>

namespace scratchrobin {
namespace ui {
namespace toolbar {

// Event tables
wxBEGIN_EVENT_TABLE(ToolbarEditorForm, wxDialog)
    EVT_COMBOBOX(wxID_ANY, ToolbarEditorForm::OnToolbarSelected)
    EVT_BUTTON(wxID_ANY, ToolbarEditorForm::OnNewToolbar)
    EVT_BUTTON(wxID_ANY, ToolbarEditorForm::OnDeleteToolbar)
    EVT_BUTTON(wxID_ANY, ToolbarEditorForm::OnRenameToolbar)
    EVT_BUTTON(wxID_ANY, ToolbarEditorForm::OnResetToolbars)
    EVT_BUTTON(wxID_OK, ToolbarEditorForm::OnOk)
    EVT_BUTTON(wxID_CANCEL, ToolbarEditorForm::OnCancel)
    EVT_BUTTON(wxID_APPLY, ToolbarEditorForm::OnApply)
    EVT_BUTTON(wxID_HELP, ToolbarEditorForm::OnHelp)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(ToolbarPreviewPanel, wxPanel)
    EVT_PAINT(ToolbarPreviewPanel::OnPaint)
    EVT_LEFT_DOWN(ToolbarPreviewPanel::OnMouseDown)
    EVT_MOTION(ToolbarPreviewPanel::OnMouseMove)
    EVT_LEFT_UP(ToolbarPreviewPanel::OnMouseUp)
    EVT_KEY_DOWN(ToolbarPreviewPanel::OnKeyDown)
    EVT_SIZE(ToolbarPreviewPanel::OnSize)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(ItemPalettePanel, wxPanel)
    EVT_COMBOBOX(wxID_ANY, ItemPalettePanel::OnCategorySelected)
    EVT_LIST_ITEM_ACTIVATED(wxID_ANY, ItemPalettePanel::OnItemActivated)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(ToolbarPropertiesPanel, wxPanel)
    EVT_TEXT(wxID_ANY, ToolbarPropertiesPanel::OnPropertyChange)
    EVT_CHOICE(wxID_ANY, ToolbarPropertiesPanel::OnPropertyChange)
    EVT_CHECKBOX(wxID_ANY, ToolbarPropertiesPanel::OnPropertyChange)
    EVT_SPINCTRL(wxID_ANY, ToolbarPropertiesPanel::OnSpacerWidthChange)
wxEND_EVENT_TABLE()

// ToolbarEditorForm implementation

ToolbarEditorForm::ToolbarEditorForm(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Customize Toolbars", wxDefaultPosition, 
               wxSize(900, 650), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    
    SetMinSize(wxSize(800, 550));
    CreateControls();
    CentreOnParent();
}

ToolbarEditorForm::~ToolbarEditorForm() = default;

bool ToolbarEditorForm::ShowEditor(const std::string& initial_toolbar_id) {
    LoadToolbarList();
    
    if (!initial_toolbar_id.empty()) {
        int index = toolbar_combo_->FindString(wxString(initial_toolbar_id));
        if (index != wxNOT_FOUND) {
            toolbar_combo_->SetSelection(index);
        }
    }
    
    LoadSelectedToolbar();
    has_unsaved_changes_ = false;
    
    return ShowModal() == wxID_OK;
}

void ToolbarEditorForm::CreateControls() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    auto* title = new wxStaticText(this, wxID_ANY, "Customize Toolbars");
    auto title_font = title->GetFont();
    title_font.SetPointSize(title_font.GetPointSize() + 2);
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(title_font);
    main_sizer->Add(title, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    
    auto* desc = new wxStaticText(this, wxID_ANY, 
        "Drag items to reorder. Select items to edit properties.");
    main_sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_CENTER_HORIZONTAL, 10);
    
    // Content area
    auto* content_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left column - Toolbar list and Palette
    auto* left_sizer = new wxBoxSizer(wxVERTICAL);
    CreateToolbarListPanel(left_sizer);
    left_sizer->AddSpacer(10);
    CreatePalettePanel(left_sizer);
    content_sizer->Add(left_sizer, 0, wxEXPAND | wxALL, 5);
    
    // Center column - Preview
    CreatePreviewPanel(content_sizer);
    
    // Right column - Properties
    CreatePropertiesPanel(content_sizer);
    
    main_sizer->Add(content_sizer, 1, wxEXPAND | wxALL, 5);
    
    // Button panel
    CreateButtonPanel(main_sizer);
    
    SetSizer(main_sizer);
    Layout();
}

void ToolbarEditorForm::CreateToolbarListPanel(wxBoxSizer* parent_sizer) {
    auto* box = new wxStaticBoxSizer(wxVERTICAL, this, "Toolbar");
    
    // Toolbar selection
    auto* combo_sizer = new wxBoxSizer(wxHORIZONTAL);
    combo_sizer->Add(new wxStaticText(this, wxID_ANY, "Select:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    toolbar_combo_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, 
                                     wxSize(200, -1), 0, nullptr, wxCB_READONLY);
    combo_sizer->Add(toolbar_combo_, 1, wxEXPAND);
    box->Add(combo_sizer, 0, wxEXPAND | wxALL, 5);
    
    // Toolbar buttons
    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    new_toolbar_btn_ = new wxButton(this, wxID_ANY, "New...", wxDefaultPosition, wxSize(70, -1));
    delete_toolbar_btn_ = new wxButton(this, wxID_ANY, "Delete", wxDefaultPosition, wxSize(70, -1));
    rename_toolbar_btn_ = new wxButton(this, wxID_ANY, "Rename...", wxDefaultPosition, wxSize(70, -1));
    
    btn_sizer->Add(new_toolbar_btn_, 0, wxRIGHT, 5);
    btn_sizer->Add(delete_toolbar_btn_, 0, wxRIGHT, 5);
    btn_sizer->Add(rename_toolbar_btn_, 0);
    box->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Reset button
    reset_toolbars_btn_ = new wxButton(this, wxID_ANY, "Reset to Defaults");
    box->Add(reset_toolbars_btn_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    parent_sizer->Add(box, 0, wxEXPAND);
}

void ToolbarEditorForm::CreatePreviewPanel(wxBoxSizer* parent_sizer) {
    auto* box = new wxStaticBoxSizer(wxVERTICAL, this, "Preview");
    
    preview_panel_ = new ToolbarPreviewPanel(this);
    box->Add(preview_panel_, 1, wxEXPAND | wxALL, 5);
    
    // Item manipulation buttons
    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    remove_item_btn_ = new wxButton(this, wxID_ANY, "Remove", wxDefaultPosition, wxSize(80, -1));
    move_up_btn_ = new wxButton(this, wxID_ANY, "Up", wxDefaultPosition, wxSize(60, -1));
    move_down_btn_ = new wxButton(this, wxID_ANY, "Down", wxDefaultPosition, wxSize(60, -1));
    
    btn_sizer->Add(remove_item_btn_, 0, wxRIGHT, 5);
    btn_sizer->AddStretchSpacer(1);
    btn_sizer->Add(move_up_btn_, 0, wxRIGHT, 5);
    btn_sizer->Add(move_down_btn_, 0);
    
    box->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    parent_sizer->Add(box, 1, wxEXPAND | wxALL, 5);
}

void ToolbarEditorForm::CreatePalettePanel(wxBoxSizer* parent_sizer) {
    palette_panel_ = new ItemPalettePanel(this);
    parent_sizer->Add(palette_panel_, 1, wxEXPAND);
}

void ToolbarEditorForm::CreatePropertiesPanel(wxBoxSizer* parent_sizer) {
    properties_panel_ = new ToolbarPropertiesPanel(this);
    parent_sizer->Add(properties_panel_, 0, wxEXPAND | wxALL, 5);
}

void ToolbarEditorForm::CreateButtonPanel(wxBoxSizer* parent_sizer) {
    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    help_btn_ = new wxButton(this, wxID_HELP, "Help");
    btn_sizer->Add(help_btn_, 0, wxRIGHT, 10);
    
    btn_sizer->AddStretchSpacer(1);
    
    ok_btn_ = new wxButton(this, wxID_OK, "OK");
    ok_btn_->SetDefault();
    btn_sizer->Add(ok_btn_, 0, wxRIGHT, 5);
    
    cancel_btn_ = new wxButton(this, wxID_CANCEL, "Cancel");
    btn_sizer->Add(cancel_btn_, 0, wxRIGHT, 5);
    
    apply_btn_ = new wxButton(this, wxID_APPLY, "Apply");
    apply_btn_->Enable(false);
    btn_sizer->Add(apply_btn_, 0);
    
    parent_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 10);
}

void ToolbarEditorForm::LoadToolbarList() {
    is_loading_ = true;
    toolbar_combo_->Clear();
    
    auto& manager = ToolbarManager::GetInstance();
    auto toolbars = manager.GetAllToolbars();
    
    for (auto* toolbar : toolbars) {
        wxString display = wxString(toolbar->name) + " (" + wxString(toolbar->id) + ")";
        toolbar_combo_->Append(display, new wxStringClientData(wxString(toolbar->id)));
    }
    
    if (toolbar_combo_->GetCount() > 0) {
        toolbar_combo_->SetSelection(0);
    }
    
    is_loading_ = false;
}

void ToolbarEditorForm::LoadSelectedToolbar() {
    if (is_loading_) return;
    
    int sel = toolbar_combo_->GetSelection();
    if (sel == wxNOT_FOUND) {
        current_toolbar_id_.clear();
        preview_panel_->SetToolbar(nullptr);
        properties_panel_->Clear();
        UpdateButtonStates();
        return;
    }
    
    auto* data = static_cast<wxStringClientData*>(toolbar_combo_->GetClientObject(sel));
    if (!data) return;
    
    current_toolbar_id_ = std::string(data->GetData());
    
    auto& manager = ToolbarManager::GetInstance();
    auto* toolbar = manager.GetToolbar(current_toolbar_id_);
    
    preview_panel_->SetToolbar(toolbar);
    properties_panel_->ShowToolbarProperties(toolbar);
    
    UpdateButtonStates();
}

void ToolbarEditorForm::UpdateButtonStates() {
    auto& manager = ToolbarManager::GetInstance();
    auto* toolbar = manager.GetToolbar(current_toolbar_id_);
    
    bool has_toolbar = (toolbar != nullptr);
    bool can_modify = has_toolbar && toolbar->CanModify();
    bool has_selection = (selected_item_index_ >= 0);
    
    delete_toolbar_btn_->Enable(can_modify);
    rename_toolbar_btn_->Enable(can_modify);
    remove_item_btn_->Enable(can_modify && has_selection);
    move_up_btn_->Enable(can_modify && has_selection && selected_item_index_ > 0);
    move_down_btn_->Enable(can_modify && has_selection && 
        static_cast<size_t>(selected_item_index_) < toolbar->items.size() - 1);
    
    apply_btn_->Enable(has_unsaved_changes_);
}

void ToolbarEditorForm::OnToolbarSelected(wxCommandEvent& event) {
    LoadSelectedToolbar();
}

void ToolbarEditorForm::OnNewToolbar(wxCommandEvent& event) {
    wxTextEntryDialog dialog(this, "Enter name for new toolbar:", "New Toolbar", "My Toolbar");
    if (dialog.ShowModal() == wxID_OK) {
        wxString name = dialog.GetValue();
        if (!name.IsEmpty()) {
            auto& manager = ToolbarManager::GetInstance();
            auto* toolbar = manager.CreateCustomToolbar(std::string(name));
            if (toolbar) {
                LoadToolbarList();
                // Select the new toolbar
                for (unsigned int i = 0; i < toolbar_combo_->GetCount(); i++) {
                    auto* data = static_cast<wxStringClientData*>(toolbar_combo_->GetClientObject(i));
                    if (data && data->GetData() == wxString(toolbar->id)) {
                        toolbar_combo_->SetSelection(i);
                        break;
                    }
                }
                LoadSelectedToolbar();
                has_unsaved_changes_ = true;
                UpdateButtonStates();
            }
        }
    }
}

void ToolbarEditorForm::OnDeleteToolbar(wxCommandEvent& event) {
    if (current_toolbar_id_.empty()) return;
    
    auto& manager = ToolbarManager::GetInstance();
    auto* toolbar = manager.GetToolbar(current_toolbar_id_);
    if (!toolbar || !toolbar->CanModify()) return;
    
    wxString msg = wxString::Format("Are you sure you want to delete the toolbar '%s'?", 
                                    wxString(toolbar->name));
    if (wxMessageBox(msg, "Confirm Delete", wxYES_NO | wxICON_QUESTION, this) == wxYES) {
        manager.DeleteCustomToolbar(current_toolbar_id_);
        LoadToolbarList();
        LoadSelectedToolbar();
        has_unsaved_changes_ = true;
        UpdateButtonStates();
    }
}

void ToolbarEditorForm::OnRenameToolbar(wxCommandEvent& event) {
    if (current_toolbar_id_.empty()) return;
    
    auto& manager = ToolbarManager::GetInstance();
    auto* toolbar = manager.GetToolbar(current_toolbar_id_);
    if (!toolbar || !toolbar->CanModify()) return;
    
    wxTextEntryDialog dialog(this, "Enter new name:", "Rename Toolbar", 
                             wxString(toolbar->name));
    if (dialog.ShowModal() == wxID_OK) {
        wxString name = dialog.GetValue();
        if (!name.IsEmpty()) {
            manager.RenameCustomToolbar(current_toolbar_id_, std::string(name));
            LoadToolbarList();
            LoadSelectedToolbar();
            has_unsaved_changes_ = true;
        }
    }
}

void ToolbarEditorForm::OnResetToolbars(wxCommandEvent& event) {
    if (wxMessageBox("Reset all toolbars to default configuration?\nThis will delete all custom toolbars.",
                     "Confirm Reset", wxYES_NO | wxICON_WARNING, this) == wxYES) {
        auto& manager = ToolbarManager::GetInstance();
        manager.ResetToDefaults();
        LoadToolbarList();
        LoadSelectedToolbar();
        has_unsaved_changes_ = false;
        UpdateButtonStates();
    }
}

void ToolbarEditorForm::OnItemSelected(wxCommandEvent& event) {
    selected_item_index_ = event.GetInt();
    
    auto& manager = ToolbarManager::GetInstance();
    auto* toolbar = manager.GetToolbar(current_toolbar_id_);
    
    if (toolbar && selected_item_index_ >= 0 && 
        static_cast<size_t>(selected_item_index_) < toolbar->items.size()) {
        properties_panel_->ShowItemProperties(&toolbar->items[selected_item_index_]);
    } else {
        properties_panel_->Clear();
    }
    
    UpdateButtonStates();
}

void ToolbarEditorForm::OnItemMoved(wxCommandEvent& event) {
    has_unsaved_changes_ = true;
    UpdateButtonStates();
}

void ToolbarEditorForm::OnItemDeleted(wxCommandEvent& event) {
    has_unsaved_changes_ = true;
    selected_item_index_ = -1;
    UpdateButtonStates();
}

void ToolbarEditorForm::OnPaletteItemSelected(wxTreeEvent& event) {}

void ToolbarEditorForm::OnAddItem(wxCommandEvent& event) {
    auto* tmpl = palette_panel_->GetSelectedTemplate();
    if (!tmpl || current_toolbar_id_.empty()) return;
    
    auto& manager = ToolbarManager::GetInstance();
    auto* toolbar = manager.GetToolbar(current_toolbar_id_);
    if (!toolbar || !toolbar->CanModify()) return;
    
    ToolbarItem item;
    item.id = tmpl->action_id + "_" + std::to_string(toolbar->items.size());
    item.action_id = tmpl->action_id;
    item.type = tmpl->type;
    item.icon_name = tmpl->icon_name;
    item.label = tmpl->label;
    item.tooltip = tmpl->tooltip;
    
    manager.AddItem(current_toolbar_id_, item);
    preview_panel_->Refresh();
    has_unsaved_changes_ = true;
    UpdateButtonStates();
}

void ToolbarEditorForm::OnPropertyChanged(wxCommandEvent& event) {}

void ToolbarEditorForm::OnOk(wxCommandEvent& event) {
    if (has_unsaved_changes_) {
        // Changes are applied immediately via the manager, so just end dialog
    }
    EndModal(wxID_OK);
}

void ToolbarEditorForm::OnCancel(wxCommandEvent& event) {
    if (has_unsaved_changes_) {
        int result = wxMessageBox("You have unsaved changes. Discard them?",
                                  "Unsaved Changes", wxYES_NO | wxICON_QUESTION, this);
        if (result != wxYES) {
            return;
        }
        // TODO: Restore original state from backup
    }
    EndModal(wxID_CANCEL);
}

void ToolbarEditorForm::OnApply(wxCommandEvent& event) {
    has_unsaved_changes_ = false;
    UpdateButtonStates();
}

void ToolbarEditorForm::OnHelp(wxCommandEvent& event) {
    wxLaunchDefaultBrowser("https://docs.scratchrobin.dev/toolbar-customization");
}

// ToolbarPreviewPanel implementation

ToolbarPreviewPanel::ToolbarPreviewPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
              wxBORDER_SUNKEN | wxFULL_REPAINT_ON_RESIZE),
      selection_event_(wxNewEventType()),
      move_event_(wxNewEventType()),
      delete_event_(wxNewEventType()) {
    
    SetBackgroundColour(wxColour(240, 240, 240));
    SetMinSize(wxSize(400, 80));
}

void ToolbarPreviewPanel::SetToolbar(ToolbarDefinition* toolbar) {
    toolbar_ = toolbar;
    selected_index_ = -1;
    Refresh();
}

void ToolbarPreviewPanel::SetSelectedIndex(int index) {
    selected_index_ = index;
    Refresh();
    
    wxCommandEvent event(selection_event_);
    event.SetInt(index);
    ProcessEvent(event);
}

void ToolbarPreviewPanel::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    
    if (!toolbar_) {
        dc.DrawLabel("No toolbar selected", wxNullBitmap, wxRect(GetClientSize()), 
                     wxALIGN_CENTER);
        return;
    }
    
    int x = ITEM_MARGIN;
    int y = (GetClientSize().GetHeight() - ITEM_HEIGHT) / 2;
    
    for (size_t i = 0; i < toolbar_->items.size(); i++) {
        const auto& item = toolbar_->items[i];
        int width = CalculateItemWidth(item);
        
        // Draw selection background
        if (static_cast<int>(i) == selected_index_) {
            dc.SetBrush(wxBrush(wxColour(200, 220, 255)));
            dc.SetPen(wxPen(wxColour(100, 150, 220), 2));
            dc.DrawRoundedRectangle(x - 2, y - 2, width + 4, ITEM_HEIGHT + 4, 3);
        }
        
        DrawItem(dc, item, x, width);
        x += width + ITEM_MARGIN;
    }
}

void ToolbarPreviewPanel::DrawItem(wxDC& dc, const ToolbarItem& item, int x, int& width) {
    int y = (GetClientSize().GetHeight() - ITEM_HEIGHT) / 2;
    
    switch (item.type) {
        case ToolbarItemType::Separator:
            dc.SetPen(wxPen(wxColour(180, 180, 180), 1));
            dc.DrawLine(x + 3, y + 4, x + 3, y + ITEM_HEIGHT - 4);
            break;
            
        case ToolbarItemType::Icon:
        case ToolbarItemType::Toggle: {
            // Draw button background
            wxColour bg = item.is_enabled ? wxColour(250, 250, 250) : wxColour(230, 230, 230);
            if (item.is_toggled) {
                bg = wxColour(200, 220, 255);
            }
            dc.SetBrush(wxBrush(bg));
            dc.SetPen(wxPen(wxColour(180, 180, 180), 1));
            dc.DrawRoundedRectangle(x, y, width, ITEM_HEIGHT, 3);
            
            // Draw icon placeholder (would load actual icon)
            dc.SetBrush(wxBrush(wxColour(150, 150, 150)));
            dc.DrawRectangle(x + 8, y + 8, 16, 16);
            
            // Draw label if present
            if (!item.label.empty()) {
                dc.SetTextForeground(item.is_enabled ? wxColour(0, 0, 0) : wxColour(150, 150, 150));
                dc.DrawText(wxString(item.label), x + 28, y + 8);
            }
            break;
        }
            
        case ToolbarItemType::Spacer:
            // Visual spacer indication
            dc.SetPen(wxPen(wxColour(200, 200, 200), 1, wxPENSTYLE_DOT));
            dc.DrawLine(x, y + ITEM_HEIGHT / 2, x + width, y + ITEM_HEIGHT / 2);
            break;
            
        case ToolbarItemType::FixedSpacer:
            // Just empty space
            break;
            
        default:
            // Generic representation
            dc.SetBrush(wxBrush(wxColour(240, 240, 240)));
            dc.SetPen(wxPen(wxColour(200, 200, 200), 1));
            dc.DrawRoundedRectangle(x, y, width, ITEM_HEIGHT, 3);
            dc.DrawText(wxString(item.label), x + 4, y + 8);
            break;
    }
}

int ToolbarPreviewPanel::CalculateItemWidth(const ToolbarItem& item) const {
    switch (item.type) {
        case ToolbarItemType::Separator:
            return SEPARATOR_WIDTH;
        case ToolbarItemType::Spacer:
            return SPACER_MIN_WIDTH;
        case ToolbarItemType::FixedSpacer:
            return item.spacer_width;
        case ToolbarItemType::Icon:
        case ToolbarItemType::Toggle:
            return item.label.empty() ? 32 : 80;
        default:
            return 80;
    }
}

int ToolbarPreviewPanel::HitTest(int x) const {
    if (!toolbar_) return -1;
    
    int current_x = ITEM_MARGIN;
    for (size_t i = 0; i < toolbar_->items.size(); i++) {
        int width = CalculateItemWidth(toolbar_->items[i]);
        if (x >= current_x && x < current_x + width) {
            return static_cast<int>(i);
        }
        current_x += width + ITEM_MARGIN;
    }
    return -1;
}

void ToolbarPreviewPanel::OnMouseDown(wxMouseEvent& event) {
    SetFocus();
    int index = HitTest(event.GetX());
    SetSelectedIndex(index);
    
    if (index >= 0) {
        StartDrag(index);
    }
}

void ToolbarPreviewPanel::OnMouseMove(wxMouseEvent& event) {
    if (is_dragging_) {
        drag_current_x_ = event.GetX();
        Refresh();
    } else {
        int hover = HitTest(event.GetX());
        if (hover != hover_index_) {
            hover_index_ = hover;
            SetCursor(hover >= 0 ? wxCursor(wxCURSOR_HAND) : wxCursor(wxCURSOR_ARROW));
        }
    }
}

void ToolbarPreviewPanel::OnMouseUp(wxMouseEvent& event) {
    if (is_dragging_) {
        int drop_index = HitTest(event.GetX());
        if (drop_index >= 0 && drop_index != drag_start_index_) {
            EndDrag(drop_index);
        }
        is_dragging_ = false;
        drag_start_index_ = -1;
        Refresh();
    }
}

void ToolbarPreviewPanel::OnKeyDown(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_DELETE && selected_index_ >= 0 && toolbar_) {
        // Remove item
        auto& manager = ToolbarManager::GetInstance();
        auto* item = &toolbar_->items[selected_index_];
        manager.RemoveItem(toolbar_->id, item->id);
        
        wxCommandEvent evt(delete_event_);
        ProcessEvent(evt);
        
        if (static_cast<size_t>(selected_index_) >= toolbar_->items.size()) {
            selected_index_ = static_cast<int>(toolbar_->items.size()) - 1;
        }
        Refresh();
    }
    event.Skip();
}

void ToolbarPreviewPanel::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

void ToolbarPreviewPanel::StartDrag(int index) {
    is_dragging_ = true;
    drag_start_index_ = index;
}

void ToolbarPreviewPanel::EndDrag(int drop_index) {
    if (!toolbar_ || drag_start_index_ < 0) return;
    
    auto& manager = ToolbarManager::GetInstance();
    auto* item = &toolbar_->items[drag_start_index_];
    
    if (manager.MoveItem(toolbar_->id, item->id, drop_index)) {
        wxCommandEvent evt(move_event_);
        ProcessEvent(evt);
        
        selected_index_ = drop_index;
        Refresh();
    }
}

// ItemPalettePanel implementation

ItemPalettePanel::ItemPalettePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Category selector
    sizer->Add(new wxStaticText(this, wxID_ANY, "Available Items:"), 0, wxBOTTOM, 5);
    category_combo_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, 
                                      wxDefaultSize, 0, nullptr, wxCB_READONLY);
    sizer->Add(category_combo_, 0, wxEXPAND | wxBOTTOM, 10);
    
    // Items list
    items_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
    items_list_->AppendColumn("Item", wxLIST_FORMAT_LEFT, 150);
    items_list_->AppendColumn("Type", wxLIST_FORMAT_LEFT, 80);
    sizer->Add(items_list_, 1, wxEXPAND);
    
    SetSizer(sizer);
    
    PopulateCategories();
}

void ItemPalettePanel::PopulateCategories() {
    category_combo_->Clear();
    
    auto& manager = ToolbarManager::GetInstance();
    auto categories = manager.GetItemCategories();
    
    category_combo_->Append("All Categories");
    for (const auto& cat : categories) {
        category_combo_->Append(wxString(cat));
    }
    
    if (category_combo_->GetCount() > 0) {
        category_combo_->SetSelection(0);
        PopulateItems("");
    }
}

void ItemPalettePanel::PopulateItems(const std::string& category) {
    items_list_->DeleteAllItems();
    current_items_.clear();
    
    auto& manager = ToolbarManager::GetInstance();
    auto templates = manager.GetItemTemplates();
    
    for (const auto& tmpl : templates) {
        if (category.empty() || tmpl.category == category) {
            long index = items_list_->GetItemCount();
            items_list_->InsertItem(index, wxString(tmpl.label));
            
            wxString type_str;
            switch (tmpl.type) {
                case ToolbarItemType::Icon: type_str = "Icon"; break;
                case ToolbarItemType::Separator: type_str = "Separator"; break;
                case ToolbarItemType::Dropdown: type_str = "Dropdown"; break;
                case ToolbarItemType::Spacer: type_str = "Spacer"; break;
                default: type_str = "Other"; break;
            }
            items_list_->SetItem(index, 1, type_str);
            
            current_items_.push_back(tmpl);
        }
    }
}

void ItemPalettePanel::RefreshItems() {
    int sel = category_combo_->GetSelection();
    wxString category = (sel > 0) ? category_combo_->GetString(sel) : wxString("");
    PopulateItems(std::string(category));
}

ToolbarItemTemplate* ItemPalettePanel::GetSelectedTemplate() {
    long sel = items_list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel >= 0 && static_cast<size_t>(sel) < current_items_.size()) {
        return &current_items_[sel];
    }
    return nullptr;
}

void ItemPalettePanel::OnCategorySelected(wxCommandEvent& event) {
    int sel = category_combo_->GetSelection();
    wxString category = (sel > 0) ? category_combo_->GetString(sel) : wxString("");
    PopulateItems(std::string(category));
}

void ItemPalettePanel::OnItemActivated(wxListEvent& event) {
    // Could trigger add action
}

// ToolbarPropertiesPanel implementation

ToolbarPropertiesPanel::ToolbarPropertiesPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Toolbar properties
    toolbar_box_ = new wxStaticBoxSizer(wxVERTICAL, this, "Toolbar Properties");
    CreateToolbarControls();
    main_sizer->Add(toolbar_box_, 0, wxEXPAND | wxBOTTOM, 10);
    
    // Item properties
    item_box_ = new wxStaticBoxSizer(wxVERTICAL, this, "Item Properties");
    CreateItemControls();
    main_sizer->Add(item_box_, 0, wxEXPAND);
    
    SetSizer(main_sizer);
    ShowToolbarMode();
}

void ToolbarPropertiesPanel::CreateToolbarControls() {
    auto* grid = new wxFlexGridSizer(2, 5, 5);
    grid->AddGrowableCol(1);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    toolbar_name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(toolbar_name_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Description:"), 0, wxALIGN_CENTER_VERTICAL);
    toolbar_desc_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(toolbar_desc_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Position:"), 0, wxALIGN_CENTER_VERTICAL);
    toolbar_position_ctrl_ = new wxChoice(this, wxID_ANY);
    toolbar_position_ctrl_->Append("Top");
    toolbar_position_ctrl_->Append("Bottom");
    toolbar_position_ctrl_->Append("Left");
    toolbar_position_ctrl_->Append("Right");
    toolbar_position_ctrl_->Append("Floating");
    grid->Add(toolbar_position_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, ""), 0);
    toolbar_visible_ctrl_ = new wxCheckBox(this, wxID_ANY, "Visible");
    grid->Add(toolbar_visible_ctrl_, 0);
    
    toolbar_box_->Add(grid, 1, wxEXPAND | wxALL, 5);
}

void ToolbarPropertiesPanel::CreateItemControls() {
    auto* grid = new wxFlexGridSizer(2, 5, 5);
    grid->AddGrowableCol(1);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "ID:"), 0, wxALIGN_CENTER_VERTICAL);
    item_id_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    item_id_ctrl_->SetEditable(false);
    grid->Add(item_id_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Label:"), 0, wxALIGN_CENTER_VERTICAL);
    item_label_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(item_label_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Tooltip:"), 0, wxALIGN_CENTER_VERTICAL);
    item_tooltip_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(item_tooltip_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Icon:"), 0, wxALIGN_CENTER_VERTICAL);
    item_icon_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    grid->Add(item_icon_ctrl_, 1, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, "Spacer Width:"), 0, wxALIGN_CENTER_VERTICAL);
    spacer_width_ctrl_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, 
                                         wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 8);
    grid->Add(spacer_width_ctrl_, 1, wxEXPAND);
    
    item_box_->Add(grid, 1, wxEXPAND | wxALL, 5);
}

void ToolbarPropertiesPanel::ShowToolbarProperties(ToolbarDefinition* toolbar) {
    if (!toolbar) {
        Clear();
        return;
    }
    
    current_mode_ = Toolbar;
    ShowToolbarMode();
    
    toolbar_name_ctrl_->SetValue(wxString(toolbar->name));
    toolbar_desc_ctrl_->SetValue(wxString(toolbar->description));
    toolbar_position_ctrl_->SetSelection(static_cast<int>(toolbar->position));
    toolbar_visible_ctrl_->SetValue(toolbar->is_visible);
}

void ToolbarPropertiesPanel::ShowItemProperties(ToolbarItem* item) {
    if (!item) {
        Clear();
        return;
    }
    
    current_mode_ = Item;
    ShowItemMode();
    
    item_id_ctrl_->SetValue(wxString(item->id));
    item_label_ctrl_->SetValue(wxString(item->label));
    item_tooltip_ctrl_->SetValue(wxString(item->tooltip));
    item_icon_ctrl_->SetValue(wxString(item->icon_name));
    spacer_width_ctrl_->SetValue(item->spacer_width);
    
    // Enable/disable spacer width based on item type
    spacer_width_ctrl_->Enable(item->type == ToolbarItemType::FixedSpacer);
}

void ToolbarPropertiesPanel::Clear() {
    current_mode_ = None;
    toolbar_name_ctrl_->Clear();
    toolbar_desc_ctrl_->Clear();
    toolbar_position_ctrl_->SetSelection(wxNOT_FOUND);
    toolbar_visible_ctrl_->SetValue(false);
    
    item_id_ctrl_->Clear();
    item_label_ctrl_->Clear();
    item_tooltip_ctrl_->Clear();
    item_icon_ctrl_->Clear();
    spacer_width_ctrl_->SetValue(8);
}

void ToolbarPropertiesPanel::ShowToolbarMode() {
    toolbar_box_->Show(true);
    item_box_->Show(false);
    Layout();
}

void ToolbarPropertiesPanel::ShowItemMode() {
    toolbar_box_->Show(false);
    item_box_->Show(true);
    Layout();
}

std::string ToolbarPropertiesPanel::GetItemLabel() const {
    return std::string(item_label_ctrl_->GetValue());
}

std::string ToolbarPropertiesPanel::GetItemTooltip() const {
    return std::string(item_tooltip_ctrl_->GetValue());
}

int ToolbarPropertiesPanel::GetSpacerWidth() const {
    return spacer_width_ctrl_->GetValue();
}

void ToolbarPropertiesPanel::OnPropertyChange(wxCommandEvent& event) {
    // Property changes are applied immediately via events
}

void ToolbarPropertiesPanel::OnSpacerWidthChange(wxSpinEvent& event) {
    // Property changes are applied immediately via events
}

} // namespace toolbar
} // namespace ui
} // namespace scratchrobin
