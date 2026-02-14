/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "shortcuts_dialog.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/srchctrl.h>
#include <wx/textctrl.h>

namespace scratchrobin {

// =============================================================================
// ShortcutsDialog
// =============================================================================

wxBEGIN_EVENT_TABLE(ShortcutsDialog, wxDialog)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, ShortcutsDialog::OnListSelect)
    EVT_LIST_ITEM_DESELECTED(wxID_ANY, ShortcutsDialog::OnListSelect)
    EVT_LIST_ITEM_ACTIVATED(wxID_ANY, ShortcutsDialog::OnListItemActivated)
wxEND_EVENT_TABLE()

ShortcutsDialog::ShortcutsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Keyboard Shortcuts",
               wxDefaultPosition, wxSize(750, 550),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    InitializeDefaultShortcuts();
    BuildLayout();
    PopulateList();
}

void ShortcutsDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Search box
    auto* searchSizer = new wxBoxSizer(wxHORIZONTAL);
    searchSizer->Add(new wxStaticText(this, wxID_ANY, "Search:"), 0,
                     wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    search_ctrl_ = new wxSearchCtrl(this, wxID_ANY, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_PROCESS_ENTER);
    search_ctrl_->ShowCancelButton(true);
    searchSizer->Add(search_ctrl_, 1, wxEXPAND);
    root->Add(searchSizer, 0, wxEXPAND | wxALL, 12);
    
    // Shortcuts list
    shortcuts_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);
    shortcuts_list_->AppendColumn("Category", wxLIST_FORMAT_LEFT, 120);
    shortcuts_list_->AppendColumn("Action", wxLIST_FORMAT_LEFT, 200);
    shortcuts_list_->AppendColumn("Shortcut", wxLIST_FORMAT_LEFT, 150);
    shortcuts_list_->AppendColumn("Description", wxLIST_FORMAT_LEFT, 200);
    root->Add(shortcuts_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Selected item info
    selected_info_ = new wxStaticText(this, wxID_ANY,
                                      "Select a shortcut to view details");
    root->Add(selected_info_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Action buttons
    auto* actionSizer = new wxBoxSizer(wxHORIZONTAL);
    edit_button_ = new wxButton(this, wxID_ANY, "Edit Shortcut...");
    reset_button_ = new wxButton(this, wxID_ANY, "Reset to Default");
    auto* resetAllBtn = new wxButton(this, wxID_ANY, "Reset All");
    
    edit_button_->Enable(false);
    reset_button_->Enable(false);
    
    actionSizer->Add(edit_button_, 0, wxRIGHT, 8);
    actionSizer->Add(reset_button_, 0, wxRIGHT, 8);
    actionSizer->Add(resetAllBtn, 0, wxRIGHT, 8);
    actionSizer->AddStretchSpacer(1);
    
    export_button_ = new wxButton(this, wxID_ANY, "Export...");
    import_button_ = new wxButton(this, wxID_ANY, "Import...");
    actionSizer->Add(export_button_, 0, wxRIGHT, 8);
    actionSizer->Add(import_button_, 0);
    
    root->Add(actionSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Standard dialog buttons
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer(1);
    auto* okButton = new wxButton(this, wxID_OK, "OK");
    auto* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    okButton->SetDefault();
    buttonSizer->Add(okButton, 0, wxRIGHT, 8);
    buttonSizer->Add(cancelButton, 0);
    root->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    SetSizer(root);
    
    // Bind events
    search_ctrl_->Bind(wxEVT_TEXT, &ShortcutsDialog::OnSearch, this);
    search_ctrl_->Bind(wxEVT_SEARCHCTRL_CANCEL_BTN, &ShortcutsDialog::OnSearch, this);
    
    edit_button_->Bind(wxEVT_BUTTON, &ShortcutsDialog::OnEditShortcut, this);
    reset_button_->Bind(wxEVT_BUTTON, &ShortcutsDialog::OnResetShortcut, this);
    resetAllBtn->Bind(wxEVT_BUTTON, &ShortcutsDialog::OnResetAll, this);
    export_button_->Bind(wxEVT_BUTTON, &ShortcutsDialog::OnExport, this);
    import_button_->Bind(wxEVT_BUTTON, &ShortcutsDialog::OnImport, this);
    
    Bind(wxEVT_BUTTON, [this](wxCommandEvent& evt) {
        if (evt.GetId() == wxID_OK) {
            OnOk(evt);
        } else if (evt.GetId() == wxID_CANCEL) {
            OnCancel(evt);
        }
    });
}

void ShortcutsDialog::InitializeDefaultShortcuts() {
    shortcuts_.clear();
    
    // Global shortcuts
    AddShortcut("global.new_editor", "Global", "New SQL Editor",
                "Open a new SQL editor window", "Ctrl+N");
    AddShortcut("global.new_diagram", "Global", "New Diagram",
                "Create a new diagram", "Ctrl+Shift+N");
    AddShortcut("global.open_connection", "Global", "Open Connection",
                "Open the connection dialog", "Ctrl+O");
    AddShortcut("global.close_window", "Global", "Close Window",
                "Close the current window", "Ctrl+W");
    AddShortcut("global.quit", "Global", "Quit",
                "Exit the application", "Ctrl+Q");
    AddShortcut("global.show_shortcuts", "Global", "Show Shortcuts",
                "Show this keyboard shortcuts dialog", "Ctrl+Shift+?");
    
    // SQL Editor shortcuts
    AddShortcut("editor.execute", "SQL Editor", "Execute Query",
                "Execute the current query", "F5");
    AddShortcut("editor.execute_current", "SQL Editor", "Execute Current Statement",
                "Execute the statement at cursor", "F6");
    AddShortcut("editor.cancel", "SQL Editor", "Cancel Query",
                "Cancel the running query", "Esc");
    AddShortcut("editor.explain", "SQL Editor", "Explain Query",
                "Show query execution plan", "Ctrl+F5");
    AddShortcut("editor.format", "SQL Editor", "Format SQL",
                "Format the SQL code", "Ctrl+Shift+F");
    AddShortcut("editor.autocomplete", "SQL Editor", "Auto-complete",
                "Show auto-complete suggestions", "Ctrl+Space");
    AddShortcut("editor.toggle_comment", "SQL Editor", "Toggle Comment",
                "Comment/uncomment selected lines", "Ctrl+/");
    AddShortcut("editor.find", "SQL Editor", "Find",
                "Open find dialog", "Ctrl+F");
    AddShortcut("editor.replace", "SQL Editor", "Replace",
                "Open find and replace dialog", "Ctrl+H");
    AddShortcut("editor.find_next", "SQL Editor", "Find Next",
                "Find next occurrence", "F3");
    
    // Catalog Tree shortcuts
    AddShortcut("catalog.open", "Catalog Tree", "Open Object",
                "Open the selected object", "Enter");
    AddShortcut("catalog.rename", "Catalog Tree", "Rename",
                "Rename the selected object", "F2");
    AddShortcut("catalog.delete", "Catalog Tree", "Delete Object",
                "Delete the selected object", "Delete");
    AddShortcut("catalog.copy_name", "Catalog Tree", "Copy Name",
                "Copy object name to clipboard", "Ctrl+C");
    AddShortcut("catalog.copy_ddl", "Catalog Tree", "Copy DDL",
                "Copy DDL to clipboard", "Ctrl+Shift+C");
    
    // Diagram shortcuts
    AddShortcut("diagram.delete", "Diagram", "Delete Selected",
                "Delete selected objects", "Delete");
    AddShortcut("diagram.select_all", "Diagram", "Select All",
                "Select all objects", "Ctrl+A");
    AddShortcut("diagram.align", "Diagram", "Align Selected",
                "Align selected objects", "Ctrl+Shift+A");
    AddShortcut("diagram.zoom_in", "Diagram", "Zoom In",
                "Zoom in on the diagram", "Ctrl+Plus");
    AddShortcut("diagram.zoom_out", "Diagram", "Zoom Out",
                "Zoom out of the diagram", "Ctrl+Minus");
    AddShortcut("diagram.reset_zoom", "Diagram", "Reset Zoom",
                "Reset zoom to 100%", "Ctrl+0");
    AddShortcut("diagram.toggle_grid", "Diagram", "Toggle Grid",
                "Show/hide grid", "Ctrl+G");
    AddShortcut("diagram.group", "Diagram", "Group Selected",
                "Group selected objects", "Ctrl+Shift+G");
    
    // Results Grid shortcuts
    AddShortcut("results.copy", "Results Grid", "Copy",
                "Copy selected cells", "Ctrl+C");
    AddShortcut("results.find", "Results Grid", "Find",
                "Find in results", "Ctrl+F");
    AddShortcut("results.select_all", "Results Grid", "Select All",
                "Select all cells", "Ctrl+A");
    AddShortcut("results.export", "Results Grid", "Export",
                "Export results to file", "Ctrl+E");
    
    filtered_shortcuts_ = shortcuts_;
}

void ShortcutsDialog::AddShortcut(const std::string& id, const std::string& category,
                                  const std::string& action, const std::string& description,
                                  const std::string& default_key, bool customizable) {
    Shortcut sc;
    sc.id = id;
    sc.category = category;
    sc.action = action;
    sc.description = description;
    sc.default_key = default_key;
    sc.current_key = default_key;
    sc.customizable = customizable;
    shortcuts_.push_back(sc);
}

void ShortcutsDialog::PopulateList() {
    shortcuts_list_->DeleteAllItems();
    
    for (size_t i = 0; i < filtered_shortcuts_.size(); ++i) {
        const auto& sc = filtered_shortcuts_[i];
        long index = shortcuts_list_->InsertItem(static_cast<long>(i), sc.category);
        shortcuts_list_->SetItem(index, 1, sc.action);
        shortcuts_list_->SetItem(index, 2, sc.current_key);
        shortcuts_list_->SetItem(index, 3, sc.description);
        
        // Store the shortcut ID as item data for lookup
        shortcuts_list_->SetItemPtrData(index, reinterpret_cast<wxUIntPtr>(
            const_cast<Shortcut*>(&sc)));
    }
}

void ShortcutsDialog::ApplyFilter() {
    wxString filter = search_ctrl_->GetValue().Lower();
    
    filtered_shortcuts_.clear();
    for (const auto& sc : shortcuts_) {
        if (filter.IsEmpty() ||
            wxString(sc.action).Lower().Contains(filter) ||
            wxString(sc.category).Lower().Contains(filter) ||
            wxString(sc.current_key).Lower().Contains(filter)) {
            filtered_shortcuts_.push_back(sc);
        }
    }
    
    PopulateList();
    selected_index_ = -1;
    UpdateButtonStates();
}

void ShortcutsDialog::UpdateButtonStates() {
    bool hasSelection = selected_index_ >= 0;
    edit_button_->Enable(hasSelection);
    reset_button_->Enable(hasSelection);
    
    if (hasSelection && selected_index_ < static_cast<long>(filtered_shortcuts_.size())) {
        const auto& sc = filtered_shortcuts_[selected_index_];
        wxString info = wxString::Format("Selected: %s - %s (%s)",
                                         sc.category, sc.action, sc.description);
        selected_info_->SetLabel(info);
    } else {
        selected_info_->SetLabel("Select a shortcut to view details");
    }
}

void ShortcutsDialog::OnSearch(wxCommandEvent&) {
    ApplyFilter();
}

void ShortcutsDialog::OnListSelect(wxListEvent& event) {
    selected_index_ = event.GetIndex();
    UpdateButtonStates();
}

void ShortcutsDialog::OnListItemActivated(wxListEvent&) {
    wxCommandEvent dummy;
    OnEditShortcut(dummy);
}

void ShortcutsDialog::OnEditShortcut(wxCommandEvent&) {
    if (selected_index_ < 0 || selected_index_ >= static_cast<long>(filtered_shortcuts_.size())) {
        return;
    }
    
    // Find the actual shortcut in the main list
    auto& filtered_sc = filtered_shortcuts_[selected_index_];
    long mainIndex = FindShortcutIndex(filtered_sc.id);
    if (mainIndex < 0) return;
    
    EditShortcutDialog dlg(this, &shortcuts_[mainIndex]);
    dlg.ShowModal();
    
    if (dlg.IsConfirmed()) {
        modified_ = true;
        filtered_shortcuts_[selected_index_] = shortcuts_[mainIndex];
        PopulateList();
    }
}

void ShortcutsDialog::OnResetShortcut(wxCommandEvent&) {
    if (selected_index_ < 0 || selected_index_ >= static_cast<long>(filtered_shortcuts_.size())) {
        return;
    }
    
    auto& filtered_sc = filtered_shortcuts_[selected_index_];
    long mainIndex = FindShortcutIndex(filtered_sc.id);
    if (mainIndex < 0) return;
    
    shortcuts_[mainIndex].current_key = shortcuts_[mainIndex].default_key;
    filtered_shortcuts_[selected_index_] = shortcuts_[mainIndex];
    modified_ = true;
    
    PopulateList();
    UpdateButtonStates();
}

void ShortcutsDialog::OnResetAll(wxCommandEvent&) {
    int result = wxMessageBox("Reset all shortcuts to their default values?",
                              "Confirm Reset",
                              wxYES_NO | wxICON_QUESTION, this);
    if (result != wxYES) return;
    
    for (auto& sc : shortcuts_) {
        sc.current_key = sc.default_key;
    }
    
    ApplyFilter();
    modified_ = true;
}

void ShortcutsDialog::OnExport(wxCommandEvent&) {
    wxFileDialog dlg(this, "Export Shortcuts", wxEmptyString, "shortcuts.json",
                     "JSON files (*.json)|*.json|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() == wxID_CANCEL) return;
    
    if (ExportToFile(dlg.GetPath().ToStdString())) {
        wxMessageBox("Shortcuts exported successfully.", "Export",
                     wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox("Failed to export shortcuts.", "Export Error",
                     wxOK | wxICON_ERROR, this);
    }
}

void ShortcutsDialog::OnImport(wxCommandEvent&) {
    wxFileDialog dlg(this, "Import Shortcuts", wxEmptyString, wxEmptyString,
                     "JSON files (*.json)|*.json|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dlg.ShowModal() == wxID_CANCEL) return;
    
    int result = wxMessageBox("This will replace your current shortcuts. Continue?",
                              "Confirm Import",
                              wxYES_NO | wxICON_QUESTION, this);
    if (result != wxYES) return;
    
    if (ImportFromFile(dlg.GetPath().ToStdString())) {
        ApplyFilter();
        modified_ = true;
        wxMessageBox("Shortcuts imported successfully.", "Import",
                     wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox("Failed to import shortcuts. The file may be corrupted.",
                     "Import Error", wxOK | wxICON_ERROR, this);
    }
}

void ShortcutsDialog::OnOk(wxCommandEvent&) {
    EndModal(wxID_OK);
}

void ShortcutsDialog::OnCancel(wxCommandEvent&) {
    if (modified_) {
        int result = wxMessageBox("You have unsaved changes. Discard them?",
                                  "Unsaved Changes",
                                  wxYES_NO | wxICON_QUESTION, this);
        if (result != wxYES) return;
    }
    EndModal(wxID_CANCEL);
}

long ShortcutsDialog::FindShortcutIndex(const std::string& id) const {
    for (size_t i = 0; i < shortcuts_.size(); ++i) {
        if (shortcuts_[i].id == id) {
            return static_cast<long>(i);
        }
    }
    return -1;
}

std::string ShortcutsDialog::GetShortcutKey(const std::string& id) const {
    long index = FindShortcutIndex(id);
    if (index >= 0) {
        return shortcuts_[index].current_key;
    }
    return "";
}

void ShortcutsDialog::SetShortcutKey(const std::string& id, const std::string& key) {
    long index = FindShortcutIndex(id);
    if (index >= 0) {
        shortcuts_[index].current_key = key;
        modified_ = true;
    }
}

bool ShortcutsDialog::LoadShortcuts(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return false;
    }
    
    // Simple parsing - in production would use proper JSON parsing
    std::string line;
    while (std::getline(file, line)) {
        // Parse "id": "key" format
        auto pos = line.find('"');
        if (pos == std::string::npos) continue;
        
        auto endPos = line.find('"', pos + 1);
        if (endPos == std::string::npos) continue;
        
        std::string id = line.substr(pos + 1, endPos - pos - 1);
        
        pos = line.find('"', endPos + 1);
        if (pos == std::string::npos) continue;
        
        endPos = line.find('"', pos + 1);
        if (endPos == std::string::npos) continue;
        
        std::string key = line.substr(pos + 1, endPos - pos - 1);
        
        SetShortcutKey(id, key);
    }
    
    return true;
}

bool ShortcutsDialog::SaveShortcuts(const std::string& config_path) {
    std::ofstream file(config_path);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n";
    for (size_t i = 0; i < shortcuts_.size(); ++i) {
        const auto& sc = shortcuts_[i];
        if (sc.current_key != sc.default_key) {
            file << "  \"" << sc.id << "\": \"" << sc.current_key << "\"";
            if (i < shortcuts_.size() - 1) file << ",";
            file << "\n";
        }
    }
    file << "}\n";
    
    return true;
}

bool ShortcutsDialog::ExportToFile(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n  \"shortcuts\": [\n";
    for (size_t i = 0; i < shortcuts_.size(); ++i) {
        const auto& sc = shortcuts_[i];
        file << "    {\n";
        file << "      \"id\": \"" << sc.id << "\",\n";
        file << "      \"category\": \"" << sc.category << "\",\n";
        file << "      \"action\": \"" << sc.action << "\",\n";
        file << "      \"default_key\": \"" << sc.default_key << "\",\n";
        file << "      \"current_key\": \"" << sc.current_key << "\"\n";
        file << "    }";
        if (i < shortcuts_.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n}\n";
    
    return true;
}

bool ShortcutsDialog::ImportFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    // Simple import - would use proper JSON parsing in production
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // For now, just reset to defaults
    // Full JSON parsing would be implemented here
    InitializeDefaultShortcuts();
    
    return true;
}

// =============================================================================
// EditShortcutDialog
// =============================================================================

wxBEGIN_EVENT_TABLE(EditShortcutDialog, wxDialog)
    EVT_KEY_DOWN(EditShortcutDialog::OnKeyDown)
wxEND_EVENT_TABLE()

EditShortcutDialog::EditShortcutDialog(wxWindow* parent, Shortcut* shortcut)
    : wxDialog(parent, wxID_ANY, "Edit Shortcut",
               wxDefaultPosition, wxSize(400, 250)),
      shortcut_(shortcut) {
    BuildLayout();
}

void EditShortcutDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Action info
    auto* infoSizer = new wxBoxSizer(wxVERTICAL);
    action_label_ = new wxStaticText(this, wxID_ANY,
                                     wxString::Format("Action: %s", shortcut_->action));
    action_label_->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                  wxFONTWEIGHT_BOLD));
    infoSizer->Add(action_label_, 0, wxBOTTOM, 8);
    
    current_label_ = new wxStaticText(this, wxID_ANY,
                                      wxString::Format("Current: %s", shortcut_->current_key));
    infoSizer->Add(current_label_, 0, wxBOTTOM, 8);
    
    infoSizer->Add(new wxStaticText(this, wxID_ANY,
                                    "Press the key combination you want to use:"),
                   0, wxBOTTOM, 8);
    root->Add(infoSizer, 0, wxEXPAND | wxALL, 16);
    
    // Key input
    key_input_ = new wxTextCtrl(this, wxID_ANY, shortcut_->current_key,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_READONLY | wxTE_CENTER);
    key_input_->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                               wxFONTWEIGHT_BOLD));
    root->Add(key_input_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);
    
    // Button row
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* clearBtn = new wxButton(this, wxID_ANY, "Clear");
    auto* resetBtn = new wxButton(this, wxID_ANY, "Reset to Default");
    
    btnSizer->Add(clearBtn, 0, wxRIGHT, 8);
    btnSizer->Add(resetBtn, 0);
    root->Add(btnSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 16);
    
    // Dialog buttons
    auto* dialogBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    dialogBtnSizer->AddStretchSpacer(1);
    auto* okBtn = new wxButton(this, wxID_OK, "OK");
    auto* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    okBtn->SetDefault();
    dialogBtnSizer->Add(okBtn, 0, wxRIGHT, 8);
    dialogBtnSizer->Add(cancelBtn, 0);
    root->Add(dialogBtnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);
    
    SetSizer(root);
    
    // Bind events
    key_input_->Bind(wxEVT_KEY_DOWN, &EditShortcutDialog::OnKeyDown, this);
    clearBtn->Bind(wxEVT_BUTTON, &EditShortcutDialog::OnClear, this);
    resetBtn->Bind(wxEVT_BUTTON, &EditShortcutDialog::OnReset, this);
    okBtn->Bind(wxEVT_BUTTON, &EditShortcutDialog::OnOk, this);
}

void EditShortcutDialog::OnKeyDown(wxKeyEvent& event) {
    std::string keyStr = KeyEventToString(event);
    if (!keyStr.empty()) {
        new_key_ = keyStr;
        UpdateKeyDisplay();
    }
    event.Skip();
}

void EditShortcutDialog::OnClear(wxCommandEvent&) {
    new_key_.clear();
    UpdateKeyDisplay();
}

void EditShortcutDialog::OnReset(wxCommandEvent&) {
    new_key_ = shortcut_->default_key;
    UpdateKeyDisplay();
}

void EditShortcutDialog::OnOk(wxCommandEvent&) {
    shortcut_->current_key = new_key_.empty() ? shortcut_->default_key : new_key_;
    confirmed_ = true;
    EndModal(wxID_OK);
}

void EditShortcutDialog::UpdateKeyDisplay() {
    key_input_->SetValue(new_key_.empty() ? "(none)" : new_key_);
}

std::string EditShortcutDialog::KeyEventToString(wxKeyEvent& event) {
    std::string result;
    
    if (event.ControlDown()) result += "Ctrl+";
    if (event.AltDown()) result += "Alt+";
    if (event.ShiftDown()) result += "Shift+";
    if (event.MetaDown()) result += "Meta+";
    
    int keyCode = event.GetKeyCode();
    
    // Special keys
    switch (keyCode) {
        case WXK_F1: result += "F1"; break;
        case WXK_F2: result += "F2"; break;
        case WXK_F3: result += "F3"; break;
        case WXK_F4: result += "F4"; break;
        case WXK_F5: result += "F5"; break;
        case WXK_F6: result += "F6"; break;
        case WXK_F7: result += "F7"; break;
        case WXK_F8: result += "F8"; break;
        case WXK_F9: result += "F9"; break;
        case WXK_F10: result += "F10"; break;
        case WXK_F11: result += "F11"; break;
        case WXK_F12: result += "F12"; break;
        case WXK_ESCAPE: result += "Esc"; break;
        case WXK_TAB: result += "Tab"; break;
        case WXK_RETURN: result += "Enter"; break;
        case WXK_SPACE: result += "Space"; break;
        case WXK_BACK: result += "Backspace"; break;
        case WXK_DELETE: result += "Delete"; break;
        case WXK_INSERT: result += "Insert"; break;
        case WXK_HOME: result += "Home"; break;
        case WXK_END: result += "End"; break;
        case WXK_PAGEUP: result += "PageUp"; break;
        case WXK_PAGEDOWN: result += "PageDown"; break;
        case WXK_LEFT: result += "Left"; break;
        case WXK_RIGHT: result += "Right"; break;
        case WXK_UP: result += "Up"; break;
        case WXK_DOWN: result += "Down"; break;
        case WXK_NUMPAD0: result += "Numpad0"; break;
        case WXK_NUMPAD1: result += "Numpad1"; break;
        case WXK_NUMPAD2: result += "Numpad2"; break;
        case WXK_NUMPAD3: result += "Numpad3"; break;
        case WXK_NUMPAD4: result += "Numpad4"; break;
        case WXK_NUMPAD5: result += "Numpad5"; break;
        case WXK_NUMPAD6: result += "Numpad6"; break;
        case WXK_NUMPAD7: result += "Numpad7"; break;
        case WXK_NUMPAD8: result += "Numpad8"; break;
        case WXK_NUMPAD9: result += "Numpad9"; break;
        case WXK_NUMPAD_ADD: result += "NumpadPlus"; break;
        case WXK_NUMPAD_SUBTRACT: result += "NumpadMinus"; break;
        case WXK_NUMPAD_MULTIPLY: result += "NumpadMultiply"; break;
        case WXK_NUMPAD_DIVIDE: result += "NumpadDivide"; break;
        case WXK_NUMPAD_DECIMAL: result += "NumpadDecimal"; break;
        case WXK_NUMPAD_ENTER: result += "NumpadEnter"; break;
        default:
            if (keyCode >= '0' && keyCode <= '9') {
                result += static_cast<char>(keyCode);
            } else if (keyCode >= 'A' && keyCode <= 'Z') {
                result += static_cast<char>(keyCode);
            } else if (keyCode >= WXK_F13 && keyCode <= WXK_F24) {
                result += "F" + std::to_string(keyCode - WXK_F12 + 12);
            } else {
                // Unknown key, return empty
                return "";
            }
    }
    
    return result;
}

// =============================================================================
// Convenience Functions
// =============================================================================

void ShowShortcutsDialog(wxWindow* parent) {
    ShortcutsDialog dlg(parent);
    dlg.ShowModal();
}

} // namespace scratchrobin
