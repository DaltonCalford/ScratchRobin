/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/settings_dialog.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/slider.h>
#include <wx/radiobut.h>
#include <wx/checkbox.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/msgdlg.h>

namespace scratchrobin::ui {

BEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_RADIOBUTTON(static_cast<int>(ControlId::kThemeLight), SettingsDialog::onThemeChanged)
    EVT_RADIOBUTTON(static_cast<int>(ControlId::kThemeDark), SettingsDialog::onThemeChanged)
    EVT_RADIOBUTTON(static_cast<int>(ControlId::kThemeHighContrast), SettingsDialog::onThemeChanged)
    EVT_RADIOBUTTON(static_cast<int>(ControlId::kThemeNight), SettingsDialog::onThemeChanged)
    EVT_SLIDER(static_cast<int>(ControlId::kMasterScaleSlider), SettingsDialog::onMasterScaleChanged)
    EVT_SLIDER(static_cast<int>(ControlId::kMenuFontSlider), SettingsDialog::onIndividualScaleChanged)
    EVT_SLIDER(static_cast<int>(ControlId::kTreeTextSlider), SettingsDialog::onIndividualScaleChanged)
    EVT_SLIDER(static_cast<int>(ControlId::kTreeIconSlider), SettingsDialog::onIndividualScaleChanged)
    EVT_SLIDER(static_cast<int>(ControlId::kDialogFontSlider), SettingsDialog::onIndividualScaleChanged)
    EVT_SLIDER(static_cast<int>(ControlId::kEditorFontSlider), SettingsDialog::onIndividualScaleChanged)
    EVT_SLIDER(static_cast<int>(ControlId::kStatusBarSlider), SettingsDialog::onIndividualScaleChanged)
    EVT_CHECKBOX(static_cast<int>(ControlId::kLinkScales), SettingsDialog::onLinkScalesChanged)
    EVT_BUTTON(static_cast<int>(ControlId::kResetDefaults), SettingsDialog::onResetDefaults)
    EVT_BUTTON(static_cast<int>(ControlId::kApply), SettingsDialog::onApply)
    EVT_BUTTON(static_cast<int>(ControlId::kPreview), SettingsDialog::onPreview)
    EVT_BUTTON(wxID_OK, SettingsDialog::onOK)
    EVT_BUTTON(wxID_CANCEL, SettingsDialog::onCancel)
END_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Settings"), wxDefaultPosition,
               wxSize(500, 450), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    
    // Load current settings from global settings
    core::Settings& settings = core::Settings::get();
    current_theme_ = settings.getTheme();
    current_scales_ = settings.getScaleSettings();
    
    // Save original settings for potential restore
    original_theme_ = current_theme_;
    original_scales_ = current_scales_;
    original_scales_linked_ = scales_linked_;
    
    // Create main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook with tabs
    notebook_ = new wxNotebook(this, static_cast<int>(ControlId::kNotebook));
    notebook_->AddPage(createAppearancePage(notebook_), wxT("Appearance"));
    notebook_->AddPage(createScalingPage(notebook_), wxT("Interface Scaling"));
    main_sizer->Add(notebook_, 1, wxEXPAND | wxALL, 10);
    
    // Button sizer
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* reset_btn = new wxButton(this, static_cast<int>(ControlId::kResetDefaults),
                                       wxT("Reset to Defaults"));
    button_sizer->Add(reset_btn, 0, wxALL, 5);
    
    button_sizer->AddStretchSpacer();
    
    wxButton* preview_btn = new wxButton(this, static_cast<int>(ControlId::kPreview),
                                         wxT("Refresh Preview"));
    button_sizer->Add(preview_btn, 0, wxALL, 5);
    
    wxButton* ok_btn = new wxButton(this, wxID_OK, wxT("OK"));
    ok_btn->SetDefault();
    button_sizer->Add(ok_btn, 0, wxALL, 5);
    
    wxButton* cancel_btn = new wxButton(this, wxID_CANCEL, wxT("Cancel"));
    button_sizer->Add(cancel_btn, 0, wxALL, 5);
    
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    SetSizer(main_sizer);
    CentreOnParent();
    
    // Apply initial live preview
    doLivePreview();
}

SettingsDialog::~SettingsDialog() {
    // If dialog is destroyed without accepting changes (e.g., close button),
    // restore original settings
    if (changes_applied_) {
        restoreOriginalSettings();
    }
}

wxPanel* SettingsDialog::createAppearancePage(wxWindow* parent) {
    wxPanel* panel = new wxPanel(parent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Theme selection group
    wxStaticBoxSizer* theme_box = new wxStaticBoxSizer(wxVERTICAL, panel, wxT("Theme"));
    
    theme_light_ = new wxRadioButton(panel, static_cast<int>(ControlId::kThemeLight),
                                     wxT("Light Mode"), wxDefaultPosition, wxDefaultSize,
                                     wxRB_GROUP);
    theme_dark_ = new wxRadioButton(panel, static_cast<int>(ControlId::kThemeDark),
                                    wxT("Dark Mode"));
    theme_high_contrast_ = new wxRadioButton(panel, static_cast<int>(ControlId::kThemeHighContrast),
                                             wxT("High Contrast"));
    theme_night_ = new wxRadioButton(panel, static_cast<int>(ControlId::kThemeNight),
                                     wxT("Night Mode (Blue-reduced)"));
    
    // Set current selection
    switch (current_theme_) {
        case core::Theme::kLight: theme_light_->SetValue(true); break;
        case core::Theme::kDark: theme_dark_->SetValue(true); break;
        case core::Theme::kHighContrast: theme_high_contrast_->SetValue(true); break;
        case core::Theme::kNight: theme_night_->SetValue(true); break;
    }
    
    theme_box->Add(theme_light_, 0, wxALL, 5);
    theme_box->Add(theme_dark_, 0, wxALL, 5);
    theme_box->Add(theme_high_contrast_, 0, wxALL, 5);
    theme_box->Add(theme_night_, 0, wxALL, 5);
    
    sizer->Add(theme_box, 0, wxEXPAND | wxALL, 10);
    
    // Theme description
    wxStaticText* desc = new wxStaticText(panel, wxID_ANY,
        wxT("Changes are previewed live in the main window behind this dialog.\n"
            "Click OK to confirm and apply to this dialog as well.\n"
            "High Contrast mode is designed for accessibility.\n"
            "Night Mode reduces blue light for evening use."));
    desc->Wrap(400);
    sizer->Add(desc, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    
    panel->SetSizer(sizer);
    return panel;
}

wxPanel* SettingsDialog::createScalingPage(wxWindow* parent) {
    wxPanel* panel = new wxPanel(parent);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Master scale section
    wxStaticBoxSizer* master_box = new wxStaticBoxSizer(wxVERTICAL, panel,
                                                        wxT("Master Scale"));
    
    wxBoxSizer* master_row = new wxBoxSizer(wxHORIZONTAL);
    master_scale_slider_ = new wxSlider(panel, static_cast<int>(ControlId::kMasterScaleSlider),
                                        current_scales_.master_scale,
                                        50, 300, wxDefaultPosition, wxSize(250, -1));
    master_scale_text_ = new wxStaticText(panel, static_cast<int>(ControlId::kMasterScaleText),
                                          wxString::Format(wxT("%d%%"), current_scales_.master_scale));
    master_scale_text_->SetMinSize(wxSize(40, -1));
    
    master_row->Add(master_scale_slider_, 1, wxEXPAND | wxRIGHT, 10);
    master_row->Add(master_scale_text_, 0, wxALIGN_CENTER_VERTICAL);
    
    master_box->Add(master_row, 0, wxEXPAND | wxALL, 10);
    
    wxStaticText* master_desc = new wxStaticText(panel, wxID_ANY,
        wxT("Adjusts the overall scale of all interface elements."));
    master_box->Add(master_desc, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    main_sizer->Add(master_box, 0, wxEXPAND | wxALL, 10);
    
    // Link scales checkbox
    link_scales_check_ = new wxCheckBox(panel, static_cast<int>(ControlId::kLinkScales),
                                        wxT("Link individual scales to master"));
    link_scales_check_->SetValue(scales_linked_);
    main_sizer->Add(link_scales_check_, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);
    
    // Individual scales section
    wxStaticBoxSizer* individual_box = new wxStaticBoxSizer(wxVERTICAL, panel,
                                                            wxT("Individual Element Scales"));
    
    auto create_scale_row = [&](wxPanel* p, const wxString& label, int value,
                                 ControlId slider_id, ControlId text_id,
                                 wxSlider** slider_out, wxStaticText** text_out) {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText* lbl = new wxStaticText(p, wxID_ANY, label);
        lbl->SetMinSize(wxSize(120, -1));
        
        *slider_out = new wxSlider(p, static_cast<int>(slider_id), value, 50, 300,
                                   wxDefaultPosition, wxSize(200, -1));
        *text_out = new wxStaticText(p, static_cast<int>(text_id),
                                     wxString::Format(wxT("%d%%"), value));
        (*text_out)->SetMinSize(wxSize(40, -1));
        
        row->Add(lbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
        row->Add(*slider_out, 1, wxEXPAND | wxRIGHT, 10);
        row->Add(*text_out, 0, wxALIGN_CENTER_VERTICAL);
        
        return row;
    };
    
    individual_box->Add(create_scale_row(panel, wxT("Menu Font:"),
                                         current_scales_.menu_font_scale,
                                         ControlId::kMenuFontSlider,
                                         ControlId::kMenuFontText,
                                         &menu_font_slider_, &menu_font_text_), 
                        0, wxEXPAND | wxALL, 5);
    
    individual_box->Add(create_scale_row(panel, wxT("Tree Text:"),
                                         current_scales_.tree_text_scale,
                                         ControlId::kTreeTextSlider,
                                         ControlId::kTreeTextText,
                                         &tree_text_slider_, &tree_text_text_),
                        0, wxEXPAND | wxALL, 5);
    
    individual_box->Add(create_scale_row(panel, wxT("Tree Icons:"),
                                         current_scales_.tree_icon_scale,
                                         ControlId::kTreeIconSlider,
                                         ControlId::kTreeIconText,
                                         &tree_icon_slider_, &tree_icon_text_),
                        0, wxEXPAND | wxALL, 5);
    
    individual_box->Add(create_scale_row(panel, wxT("Dialog Font:"),
                                         current_scales_.dialog_font_scale,
                                         ControlId::kDialogFontSlider,
                                         ControlId::kDialogFontText,
                                         &dialog_font_slider_, &dialog_font_text_),
                        0, wxEXPAND | wxALL, 5);
    
    individual_box->Add(create_scale_row(panel, wxT("Editor Font:"),
                                         current_scales_.editor_font_scale,
                                         ControlId::kEditorFontSlider,
                                         ControlId::kEditorFontText,
                                         &editor_font_slider_, &editor_font_text_),
                        0, wxEXPAND | wxALL, 5);
    
    individual_box->Add(create_scale_row(panel, wxT("Status Bar:"),
                                         current_scales_.status_bar_scale,
                                         ControlId::kStatusBarSlider,
                                         ControlId::kStatusBarText,
                                         &status_bar_slider_, &status_bar_text_),
                        0, wxEXPAND | wxALL, 5);
    
    main_sizer->Add(individual_box, 0, wxEXPAND | wxALL, 10);
    
    // Update enabled state of individual controls
    if (scales_linked_) {
        menu_font_slider_->Enable(false);
        tree_text_slider_->Enable(false);
        tree_icon_slider_->Enable(false);
        dialog_font_slider_->Enable(false);
        editor_font_slider_->Enable(false);
        status_bar_slider_->Enable(false);
    }
    
    panel->SetSizer(main_sizer);
    return panel;
}

void SettingsDialog::doLivePreview() {
    // Apply current settings to the main application (parent window)
    // The dialog itself keeps its original appearance until confirmed
    applyCurrentSettingsToApp();
    changes_applied_ = true;
}

void SettingsDialog::onThemeChanged(wxCommandEvent& event) {
    if (theme_light_->GetValue()) {
        current_theme_ = core::Theme::kLight;
    } else if (theme_dark_->GetValue()) {
        current_theme_ = core::Theme::kDark;
    } else if (theme_high_contrast_->GetValue()) {
        current_theme_ = core::Theme::kHighContrast;
    } else if (theme_night_->GetValue()) {
        current_theme_ = core::Theme::kNight;
    }
    doLivePreview();
}

void SettingsDialog::onMasterScaleChanged(wxCommandEvent& event) {
    int value = master_scale_slider_->GetValue();
    current_scales_.master_scale = value;
    updateMasterScaleDisplay();
    
    if (scales_linked_) {
        syncLinkedScales(value);
    }
    doLivePreview();
}

void SettingsDialog::onLinkScalesChanged(wxCommandEvent& event) {
    scales_linked_ = link_scales_check_->IsChecked();
    
    menu_font_slider_->Enable(!scales_linked_);
    tree_text_slider_->Enable(!scales_linked_);
    tree_icon_slider_->Enable(!scales_linked_);
    dialog_font_slider_->Enable(!scales_linked_);
    editor_font_slider_->Enable(!scales_linked_);
    status_bar_slider_->Enable(!scales_linked_);
    
    if (scales_linked_) {
        syncLinkedScales(master_scale_slider_->GetValue());
        doLivePreview();
    }
}

void SettingsDialog::onIndividualScaleChanged(wxCommandEvent& event) {
    if (scales_linked_) {
        // Unlink if user manually adjusts individual scale
        scales_linked_ = false;
        link_scales_check_->SetValue(false);
        menu_font_slider_->Enable(true);
        tree_text_slider_->Enable(true);
        tree_icon_slider_->Enable(true);
        dialog_font_slider_->Enable(true);
        editor_font_slider_->Enable(true);
        status_bar_slider_->Enable(true);
    }
    
    current_scales_.menu_font_scale = menu_font_slider_->GetValue();
    current_scales_.tree_text_scale = tree_text_slider_->GetValue();
    current_scales_.tree_icon_scale = tree_icon_slider_->GetValue();
    current_scales_.dialog_font_scale = dialog_font_slider_->GetValue();
    current_scales_.editor_font_scale = editor_font_slider_->GetValue();
    current_scales_.status_bar_scale = status_bar_slider_->GetValue();
    
    updateIndividualScaleDisplays();
    doLivePreview();
}

void SettingsDialog::updateMasterScaleDisplay() {
    master_scale_text_->SetLabel(wxString::Format(wxT("%d%%"), 
                                  master_scale_slider_->GetValue()));
}

void SettingsDialog::updateIndividualScaleDisplays() {
    menu_font_text_->SetLabel(wxString::Format(wxT("%d%%"), menu_font_slider_->GetValue()));
    tree_text_text_->SetLabel(wxString::Format(wxT("%d%%"), tree_text_slider_->GetValue()));
    tree_icon_text_->SetLabel(wxString::Format(wxT("%d%%"), tree_icon_slider_->GetValue()));
    dialog_font_text_->SetLabel(wxString::Format(wxT("%d%%"), dialog_font_slider_->GetValue()));
    editor_font_text_->SetLabel(wxString::Format(wxT("%d%%"), editor_font_slider_->GetValue()));
    status_bar_text_->SetLabel(wxString::Format(wxT("%d%%"), status_bar_slider_->GetValue()));
}

void SettingsDialog::syncLinkedScales(int master_value) {
    menu_font_slider_->SetValue(master_value);
    tree_text_slider_->SetValue(master_value);
    tree_icon_slider_->SetValue(master_value);
    dialog_font_slider_->SetValue(master_value);
    editor_font_slider_->SetValue(master_value);
    status_bar_slider_->SetValue(master_value);
    
    current_scales_.menu_font_scale = master_value;
    current_scales_.tree_text_scale = master_value;
    current_scales_.tree_icon_scale = master_value;
    current_scales_.dialog_font_scale = master_value;
    current_scales_.editor_font_scale = master_value;
    current_scales_.status_bar_scale = master_value;
    
    updateIndividualScaleDisplays();
}

void SettingsDialog::onResetDefaults(wxCommandEvent& event) {
    // Reset to defaults
    current_theme_ = core::Theme::kLight;
    current_scales_ = core::ScaleSettings{};
    scales_linked_ = true;
    
    // Update UI
    theme_light_->SetValue(true);
    theme_dark_->SetValue(false);
    theme_high_contrast_->SetValue(false);
    theme_night_->SetValue(false);
    
    master_scale_slider_->SetValue(100);
    link_scales_check_->SetValue(true);
    
    syncLinkedScales(100);
    updateMasterScaleDisplay();
    
    menu_font_slider_->Enable(false);
    tree_text_slider_->Enable(false);
    tree_icon_slider_->Enable(false);
    dialog_font_slider_->Enable(false);
    editor_font_slider_->Enable(false);
    status_bar_slider_->Enable(false);
    
    doLivePreview();
}

void SettingsDialog::onApply(wxCommandEvent& event) {
    // Save settings and apply immediately without confirmation
    applyCurrentSettingsToApp();
    core::Settings::get().save();
    changes_applied_ = false;  // Don't restore on close
}

void SettingsDialog::onPreview(wxCommandEvent& event) {
    // Refresh the live preview
    doLivePreview();
}

void SettingsDialog::onOK(wxCommandEvent& event) {
    showConfirmationAndAccept();
}

void SettingsDialog::onCancel(wxCommandEvent& event) {
    // Restore original settings
    restoreOriginalSettings();
    changes_applied_ = false;
    EndModal(wxID_CANCEL);
}

void SettingsDialog::applyCurrentSettingsToApp() {
    core::Settings& settings = core::Settings::get();
    settings.setTheme(current_theme_);
    settings.setScaleSettings(current_scales_);
    // Note: We don't call settings.save() here - that happens on OK/Apply
}

void SettingsDialog::applyCurrentSettingsToDialog() {
    // Apply the current theme and scaling to this dialog itself
    core::Settings& settings = core::Settings::get();
    const core::ThemeColors colors = settings.getThemeColors();
    
    // Apply theme colors
    SetBackgroundColour(colors.background);
    SetForegroundColour(colors.foreground);
    
    // Apply to all child controls
    wxWindowList& children = GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        child->SetBackgroundColour(colors.background);
        child->SetForegroundColour(colors.foreground);
        
        // Apply font scaling
        wxFont font = child->GetFont();
        int base_size = 9;
        int scaled_size = (base_size * current_scales_.getEffectiveScale(current_scales_.dialog_font_scale)) / 100;
        font.SetPointSize(scaled_size);
        child->SetFont(font);
    }
    
    Layout();
    Refresh();
}

void SettingsDialog::restoreOriginalSettings() {
    core::Settings& settings = core::Settings::get();
    settings.setTheme(original_theme_);
    settings.setScaleSettings(original_scales_);
}

void SettingsDialog::showConfirmationAndAccept() {
    // Show confirmation dialog
    wxMessageDialog confirmDlg(this,
        wxT("Accept these settings changes?\n\n"
            "The new theme and scaling will be applied to all windows."),
        wxT("Confirm Settings"),
        wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
    
    if (confirmDlg.ShowModal() == wxID_YES) {
        // Save settings
        core::Settings& settings = core::Settings::get();
        settings.setTheme(current_theme_);
        settings.setScaleSettings(current_scales_);
        settings.save();
        
        // Apply to this dialog itself
        applyCurrentSettingsToDialog();
        
        changes_applied_ = false;  // Don't restore on close
        EndModal(wxID_OK);
    }
    // If No, stay in dialog and continue previewing
}

bool SettingsDialog::ShowWithLivePreview() {
    // Show modal - live preview happens automatically during changes
    int result = ShowModal();
    return result == wxID_OK;
}

}  // namespace scratchrobin::ui
