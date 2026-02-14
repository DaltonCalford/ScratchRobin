// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "../i18n/locales.h"
#include <wx/dialog.h>
#include <wx/panel.h>

class wxChoice;
class wxStaticText;
class wxListCtrl;

namespace scratchrobin {

/**
 * @brief Dialog for selecting application language
 */
class LanguageDialog : public wxDialog {
public:
    LanguageDialog(wxWindow* parent);
    
    // Show dialog and return selected language
    bool ShowModalAndGetLanguage(i18n::Language* selected_language);
    
private:
    void CreateControls();
    void PopulateLanguages();
    void OnLanguageSelected(wxCommandEvent& event);
    void OnPreviewLanguage(wxCommandEvent& event);
    void UpdatePreview();
    
    wxChoice* language_choice_ = nullptr;
    wxStaticText* preview_label_ = nullptr;
    wxListCtrl* coverage_list_ = nullptr;
    
    std::vector<i18n::LanguageInfo> languages_;
    i18n::Language selected_language_;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Language selection menu helper
 */
class LanguageMenuHelper {
public:
    // Build a language submenu
    static wxMenu* CreateLanguageMenu(int base_id);
    
    // Handle language selection from menu
    static bool HandleLanguageSelection(int selected_id, int base_id);
    
    // Get language from menu ID
    static i18n::Language GetLanguageFromMenuId(int menu_id, int base_id);
    static int GetMenuIdFromLanguage(i18n::Language lang, int base_id);
    
    // Update menu checkmarks
    static void UpdateMenuCheckmarks(wxMenu* menu, i18n::Language current, int base_id);
};

/**
 * @brief Panel showing translation status and coverage
 */
class TranslationStatusPanel : public wxPanel {
public:
    TranslationStatusPanel(wxWindow* parent);
    
    void RefreshStatus();
    
private:
    void CreateControls();
    void OnExportMissing(wxCommandEvent& event);
    void OnImportTranslations(wxCommandEvent& event);
    void OnContribute(wxCommandEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin
