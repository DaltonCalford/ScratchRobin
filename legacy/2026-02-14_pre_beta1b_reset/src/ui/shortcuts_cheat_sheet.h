/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_SHORTCUTS_CHEAT_SHEET_H
#define SCRATCHROBIN_SHORTCUTS_CHEAT_SHEET_H

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxButton;
class wxHtmlWindow;
class wxScrolledWindow;
class wxStaticText;

namespace scratchrobin {

// Shortcut entry for cheat sheet
struct CheatSheetEntry {
    std::string shortcut;
    std::string description;
};

// Category with shortcuts for cheat sheet
struct CheatSheetCategory {
    std::string name;
    std::vector<CheatSheetEntry> entries;
};

// Keyboard Shortcuts Cheat Sheet Dialog
// A simple dialog showing common shortcuts in a formatted layout
class ShortcutsCheatSheet : public wxDialog {
public:
    ShortcutsCheatSheet(wxWindow* parent);

private:
    void BuildLayout();
    void InitializeCategories();
    void PopulateContent();
    wxString GenerateHtmlContent();
    wxString GenerateTextContent();
    
    // Event handlers
    void OnPrint(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    
    // Data
    std::vector<CheatSheetCategory> categories_;
    
    // UI elements
    wxScrolledWindow* scrolled_window_ = nullptr;
    wxStaticText* content_text_ = nullptr;
    wxHtmlWindow* html_window_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

// Convenience function to show the cheat sheet
void ShowShortcutsCheatSheet(wxWindow* parent);

} // namespace scratchrobin

#endif // SCRATCHROBIN_SHORTCUTS_CHEAT_SHEET_H
