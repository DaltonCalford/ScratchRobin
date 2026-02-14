/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_SHORTCUTS_DIALOG_H
#define SCRATCHROBIN_SHORTCUTS_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxButton;
class wxListCtrl;
class wxListEvent;
class wxSearchCtrl;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

// Shortcut data structure
struct Shortcut {
    std::string id;
    std::string category;
    std::string action;
    std::string description;
    std::string default_key;
    std::string current_key;
    bool customizable = true;
};

// Keyboard Shortcuts Manager Dialog
class ShortcutsDialog : public wxDialog {
public:
    ShortcutsDialog(wxWindow* parent);

    // Load shortcuts from config file
    bool LoadShortcuts(const std::string& config_path);
    
    // Save shortcuts to config file
    bool SaveShortcuts(const std::string& config_path);
    
    // Get all shortcuts
    const std::vector<Shortcut>& GetShortcuts() const { return shortcuts_; }
    
    // Get shortcut by ID
    std::string GetShortcutKey(const std::string& id) const;
    
    // Set shortcut key
    void SetShortcutKey(const std::string& id, const std::string& key);

private:
    void BuildLayout();
    void InitializeDefaultShortcuts();
    void PopulateList();
    void ApplyFilter();
    void UpdateButtonStates();
    
    // Event handlers
    void OnSearch(wxCommandEvent& event);
    void OnListSelect(wxListEvent& event);
    void OnListItemActivated(wxListEvent& event);
    void OnEditShortcut(wxCommandEvent& event);
    void OnResetShortcut(wxCommandEvent& event);
    void OnResetAll(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnImport(wxCommandEvent& event);
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    
    // Helper methods
    void AddShortcut(const std::string& id, const std::string& category,
                     const std::string& action, const std::string& description,
                     const std::string& default_key, bool customizable = true);
    long FindShortcutIndex(const std::string& id) const;
    std::vector<Shortcut> GetFilteredShortcuts() const;
    bool ExportToFile(const std::string& path);
    bool ImportFromFile(const std::string& path);
    
    // UI elements
    wxSearchCtrl* search_ctrl_ = nullptr;
    wxListCtrl* shortcuts_list_ = nullptr;
    wxStaticText* selected_info_ = nullptr;
    wxButton* edit_button_ = nullptr;
    wxButton* reset_button_ = nullptr;
    wxButton* export_button_ = nullptr;
    wxButton* import_button_ = nullptr;
    
    // Data
    std::vector<Shortcut> shortcuts_;
    std::vector<Shortcut> filtered_shortcuts_;
    long selected_index_ = -1;
    bool modified_ = false;
    
    wxDECLARE_EVENT_TABLE();
};

// Edit Shortcut Dialog
class EditShortcutDialog : public wxDialog {
public:
    EditShortcutDialog(wxWindow* parent, Shortcut* shortcut);
    
    bool IsConfirmed() const { return confirmed_; }

private:
    void BuildLayout();
    void OnKeyDown(wxKeyEvent& event);
    void OnClear(wxCommandEvent& event);
    void OnReset(wxCommandEvent& event);
    void OnOk(wxCommandEvent& event);
    void UpdateKeyDisplay();
    std::string KeyEventToString(wxKeyEvent& event);
    
    Shortcut* shortcut_ = nullptr;
    std::string new_key_;
    bool confirmed_ = false;
    
    // UI elements
    wxStaticText* action_label_ = nullptr;
    wxStaticText* current_label_ = nullptr;
    wxTextCtrl* key_input_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

// Convenience function to show shortcuts dialog
void ShowShortcutsDialog(wxWindow* parent);

} // namespace scratchrobin

#endif // SCRATCHROBIN_SHORTCUTS_DIALOG_H
