// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "language_dialog.h"

#include "../i18n/localization_manager.h"
#include "../core/config.h"
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statline.h>

namespace scratchrobin {

// LanguageDialog implementation

wxBEGIN_EVENT_TABLE(LanguageDialog, wxDialog)
    EVT_CHOICE(wxID_ANY, LanguageDialog::OnLanguageSelected)
wxEND_EVENT_TABLE()

LanguageDialog::LanguageDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Select Language"), wxDefaultPosition, 
               wxSize(500, 400)),
      selected_language_(i18n::LocalizationManager::Instance().GetCurrentLanguage()) {
    CreateControls();
    PopulateLanguages();
}

void LanguageDialog::CreateControls() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Header
    main_sizer->Add(new wxStaticText(this, wxID_ANY, 
        _("Choose your preferred language:")), 
        0, wxALL, 12);
    
    // Language selection
    auto* choice_sizer = new wxBoxSizer(wxHORIZONTAL);
    choice_sizer->Add(new wxStaticText(this, wxID_ANY, _("Language:")), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    language_choice_ = new wxChoice(this, wxID_ANY);
    choice_sizer->Add(language_choice_, 1);
    main_sizer->Add(choice_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Preview section
    main_sizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    main_sizer->Add(new wxStaticText(this, wxID_ANY, _("Preview:")), 
                    0, wxLEFT | wxRIGHT | wxTOP, 12);
    
    preview_label_ = new wxStaticText(this, wxID_ANY, _("Sample text in selected language"),
                                      wxDefaultPosition, wxDefaultSize,
                                      wxST_NO_AUTORESIZE | wxBORDER_SUNKEN);
    preview_label_->SetMinSize(wxSize(-1, 60));
    main_sizer->Add(preview_label_, 0, wxEXPAND | wxALL, 12);
    
    // Translation coverage
    main_sizer->Add(new wxStaticText(this, wxID_ANY, _("Translation Coverage:")),
                    0, wxLEFT | wxRIGHT | wxTOP, 12);
    
    coverage_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT | wxLC_SINGLE_SEL);
    coverage_list_->AppendColumn(_("Language"), wxLIST_FORMAT_LEFT, 150);
    coverage_list_->AppendColumn(_("Coverage"), wxLIST_FORMAT_LEFT, 100);
    coverage_list_->AppendColumn(_("Status"), wxLIST_FORMAT_LEFT, 100);
    main_sizer->Add(coverage_list_, 1, wxEXPAND | wxALL, 12);
    
    // Buttons
    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->AddStretchSpacer(1);
    btn_sizer->Add(new wxButton(this, wxID_OK, _("Apply")), 0, wxRIGHT, 8);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, _("Cancel")));
    main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(main_sizer);
}

void LanguageDialog::PopulateLanguages() {
    // Show all languages including beta
    languages_ = i18n::GetAllLanguages();
    
    // Sort by native name
    std::sort(languages_.begin(), languages_.end(),
              [](const auto& a, const auto& b) { return a.name < b.name; });
    
    // Populate choice
    int selected_index = 0;
    for (size_t i = 0; i < languages_.size(); ++i) {
        wxString label = wxString::FromUTF8(languages_[i].flag_emoji + " " + 
                                             languages_[i].name);
        if (languages_[i].is_beta) {
            label += " (Beta)";
        }
        language_choice_->Append(label);
        
        if (languages_[i].code == selected_language_) {
            selected_index = static_cast<int>(i);
        }
    }
    
    language_choice_->SetSelection(selected_index);
    
    // Populate coverage list
    auto& manager = i18n::LocalizationManager::Instance();
    for (size_t i = 0; i < languages_.size(); ++i) {
        long index = coverage_list_->InsertItem(static_cast<long>(i),
                                                 wxString::FromUTF8(languages_[i].name));
        
        float coverage = manager.GetTranslationCoverage(languages_[i].code);
        wxString coverage_str = wxString::Format("%.0f%%", coverage * 100);
        coverage_list_->SetItem(index, 1, coverage_str);
        
        wxString status = languages_[i].is_beta ? _("Beta") : _("Release");
        coverage_list_->SetItem(index, 2, status);
    }
    
    UpdatePreview();
}

void LanguageDialog::OnLanguageSelected(wxCommandEvent& event) {
    int sel = language_choice_->GetSelection();
    if (sel >= 0 && sel < static_cast<int>(languages_.size())) {
        selected_language_ = languages_[sel].code;
        UpdatePreview();
    }
}

void LanguageDialog::OnPreviewLanguage(wxCommandEvent& event) {
    // Preview the selected language temporarily
}

void LanguageDialog::UpdatePreview() {
    int sel = language_choice_->GetSelection();
    if (sel < 0 || sel >= static_cast<int>(languages_.size())) return;
    
    const auto& info = languages_[sel];
    
    // Build preview text
    wxString preview;
    preview += wxString::FromUTF8(info.flag_emoji) + " " + 
               wxString::FromUTF8(info.name) + "\n";
    preview += _("Locale: ") + wxString(info.locale_code) + "\n";
    preview += _("Date format: ") + wxString(info.date_format);
    
    preview_label_->SetLabel(preview);
}

bool LanguageDialog::ShowModalAndGetLanguage(i18n::Language* selected_language) {
    if (ShowModal() == wxID_OK && selected_language) {
        *selected_language = selected_language_;
        return true;
    }
    return false;
}

// LanguageMenuHelper implementation

wxMenu* LanguageMenuHelper::CreateLanguageMenu(int base_id) {
    auto* menu = new wxMenu();
    
    auto languages = i18n::GetSupportedLanguages();
    
    // Sort by English name
    std::sort(languages.begin(), languages.end(),
              [](const auto& a, const auto& b) { return a.english_name < b.english_name; });
    
    auto current = i18n::LocalizationManager::Instance().GetCurrentLanguage();
    
    for (size_t i = 0; i < languages.size(); ++i) {
        wxString label = wxString::FromUTF8(languages[i].flag_emoji + " " + 
                                             languages[i].name);
        int id = base_id + static_cast<int>(languages[i].code);
        menu->AppendRadioItem(id, label);
        
        if (languages[i].code == current) {
            menu->Check(id, true);
        }
    }
    
    // Add separator and "More languages..." option
    menu->AppendSeparator();
    menu->Append(base_id + static_cast<int>(i18n::Language::Count), 
                 _("More Languages..."));
    
    return menu;
}

bool LanguageMenuHelper::HandleLanguageSelection(int selected_id, int base_id) {
    int lang_code = selected_id - base_id;
    
    if (lang_code == static_cast<int>(i18n::Language::Count)) {
        // "More Languages..." selected - show dialog
        LanguageDialog dialog(nullptr);
        i18n::Language selected;
        if (dialog.ShowModalAndGetLanguage(&selected)) {
            i18n::LocalizationManager::Instance().SetLanguage(selected);
            return true;
        }
        return false;
    }
    
    if (lang_code >= 0 && lang_code < static_cast<int>(i18n::Language::Count)) {
        auto lang = static_cast<i18n::Language>(lang_code);
        i18n::LocalizationManager::Instance().SetLanguage(lang);
        return true;
    }
    
    return false;
}

i18n::Language LanguageMenuHelper::GetLanguageFromMenuId(int menu_id, int base_id) {
    int lang_code = menu_id - base_id;
    if (lang_code >= 0 && lang_code < static_cast<int>(i18n::Language::Count)) {
        return static_cast<i18n::Language>(lang_code);
    }
    return i18n::Language::Default;
}

int LanguageMenuHelper::GetMenuIdFromLanguage(i18n::Language lang, int base_id) {
    return base_id + static_cast<int>(lang);
}

void LanguageMenuHelper::UpdateMenuCheckmarks(wxMenu* menu, i18n::Language current, 
                                               int base_id) {
    if (!menu) return;
    
    for (size_t i = 0; i < static_cast<size_t>(i18n::Language::Count); ++i) {
        int id = base_id + static_cast<int>(i);
        if (menu->FindItem(id)) {
            menu->Check(id, (static_cast<i18n::Language>(i) == current));
        }
    }
}

// TranslationStatusPanel implementation

wxBEGIN_EVENT_TABLE(TranslationStatusPanel, wxPanel)
wxEND_EVENT_TABLE()

TranslationStatusPanel::TranslationStatusPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    CreateControls();
    RefreshStatus();
}

void TranslationStatusPanel::CreateControls() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(this, wxID_ANY, _("Translation Status")),
               0, wxALL, 12);
    
    // Status text
    auto* status_text = new wxStaticText(this, wxID_ANY,
        _("Current language coverage and contribution options."));
    sizer->Add(status_text, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Buttons
    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* export_btn = new wxButton(this, wxID_ANY, _("Export Missing"));
    auto* import_btn = new wxButton(this, wxID_ANY, _("Import"));
    auto* contribute_btn = new wxButton(this, wxID_ANY, _("Contribute"));
    
    btn_sizer->Add(export_btn, 0, wxRIGHT, 8);
    btn_sizer->Add(import_btn, 0, wxRIGHT, 8);
    btn_sizer->Add(contribute_btn);
    
    sizer->Add(btn_sizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    SetSizer(sizer);
}

void TranslationStatusPanel::RefreshStatus() {
    // Refresh translation coverage display
}

void TranslationStatusPanel::OnExportMissing(wxCommandEvent& event) {
    // Export untranslated keys
}

void TranslationStatusPanel::OnImportTranslations(wxCommandEvent& event) {
    // Import translation file
}

void TranslationStatusPanel::OnContribute(wxCommandEvent& event) {
    wxMessageBox(_("Visit our translation portal to contribute translations."),
                 _("Contribute"), wxOK | wxICON_INFORMATION, this);
}

} // namespace scratchrobin
